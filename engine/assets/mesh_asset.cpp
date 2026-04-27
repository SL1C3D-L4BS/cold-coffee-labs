#include "mesh_asset.hpp"
#include "engine/render/hal/vulkan_device.hpp"
#include <cstring>
#include <vk_mem_alloc.h>
#include <volk.h>

namespace gw::assets {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
namespace {

// Upload bytes to a device-local buffer via a host-visible staging buffer.
// Uses a simple one-shot command buffer on the transfer queue.
struct StagedBuffer {
    VkBuffer      buffer    = VK_NULL_HANDLE;
    VmaAllocation alloc     = VK_NULL_HANDLE;
};

AssetResult<StagedBuffer> create_device_local_buffer(
    render::hal::VulkanDevice& device,
    VkBufferUsageFlags usage,
    const void* data,
    VkDeviceSize size)
{
    if (size == 0) return StagedBuffer{};

    VmaAllocator vma = device.vma_allocator();

    // Staging buffer (host-visible, host-coherent).
    VkBufferCreateInfo staging_info{};
    staging_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    staging_info.size  = size;
    staging_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    staging_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo staging_alloc_ci{};
    staging_alloc_ci.usage = VMA_MEMORY_USAGE_AUTO;
    staging_alloc_ci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                           | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkBuffer staging_buf = VK_NULL_HANDLE;
    VmaAllocation staging_alloc = VK_NULL_HANDLE;
    VmaAllocationInfo staging_info_out{};

    if (vmaCreateBuffer(vma, &staging_info, &staging_alloc_ci,
                        &staging_buf, &staging_alloc, &staging_info_out) != VK_SUCCESS) {
        return std::unexpected(AssetError{AssetErrorCode::GpuUploadFailed,
                                          "staging buffer creation failed"});
    }

    std::memcpy(staging_info_out.pMappedData, data, size);
    vmaFlushAllocation(vma, staging_alloc, 0, VK_WHOLE_SIZE);

    // Device-local buffer.
    VkBufferCreateInfo dev_info{};
    dev_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    dev_info.size  = size;
    dev_info.usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    dev_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo dev_alloc_ci{};
    dev_alloc_ci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    StagedBuffer result{};
    if (vmaCreateBuffer(vma, &dev_info, &dev_alloc_ci,
                        &result.buffer, &result.alloc, nullptr) != VK_SUCCESS) {
        vmaDestroyBuffer(vma, staging_buf, staging_alloc);
        return std::unexpected(AssetError{AssetErrorCode::GpuUploadFailed,
                                          "device-local buffer creation failed"});
    }

    // One-shot copy command on the graphics pool (transfer queue falls back
    // to graphics queue if no dedicated transfer family).
    VkCommandPool pool = device.graphics_command_pool();
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool        = pool;
    alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(device.native_handle(), &alloc_info, &cmd);

    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &begin);

    VkBufferCopy copy{};
    copy.size = size;
    vkCmdCopyBuffer(cmd, staging_buf, result.buffer, 1, &copy);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit{};
    submit.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers    = &cmd;
    vkQueueSubmit(device.graphics_queue(), 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(device.graphics_queue());

    vkFreeCommandBuffers(device.native_handle(), pool, 1, &cmd);
    vmaDestroyBuffer(vma, staging_buf, staging_alloc);

    return result;
}

} // anonymous namespace

std::uint32_t MeshAsset::stream2_skin_packed_vertex_byte_offset() const noexcept {
    if (!skinned() || header_.joint_count == 0u) {
        return 0u;
    }
    const std::uint64_t ib_bytes =
        static_cast<std::uint64_t>(header_.joint_count) * 64u;
    return static_cast<std::uint32_t>((ib_bytes + 15ull) & ~15ull);
}

// ---------------------------------------------------------------------------
// MeshAsset::upload
// ---------------------------------------------------------------------------
AssetOk MeshAsset::upload(render::hal::VulkanDevice& device,
                           std::span<const uint8_t> raw)
{
    if (raw.size() < kGwMeshHeaderV1Bytes) {
        return std::unexpected(AssetError{AssetErrorCode::CorruptData,
                                          "gwmesh too small for header"});
    }

    uint16_t file_version = 0;
    std::memcpy(&file_version, raw.data() + 4u, sizeof(file_version));

    if (file_version == 1u) {
        if (raw.size() < kGwMeshHeaderV1Bytes) {
            return std::unexpected(AssetError{AssetErrorCode::CorruptData,
                                              "gwmesh v1 header truncated"});
        }
        std::memset(&header_, 0, sizeof(header_));
        std::memcpy(&header_, raw.data(), kGwMeshHeaderV1Bytes);
        header_.stream2_offset = 0;
        header_.stream2_size   = 0;
        header_.joint_count    = 0;
        header_.reserved0      = 0;
    } else if (file_version == 2u) {
        if (raw.size() < kGwMeshHeaderV2Bytes) {
            return std::unexpected(AssetError{AssetErrorCode::CorruptData,
                                              "gwmesh v2 header truncated"});
        }
        std::memcpy(&header_, raw.data(), sizeof(GwMeshHeader));
    } else {
        return std::unexpected(AssetError{AssetErrorCode::UnsupportedVersion,
                                            "unsupported gwmesh version"});
    }

    if (header_.magic != magic::kMesh) {
        return std::unexpected(AssetError{AssetErrorCode::InvalidMagic,
                                          "gwmesh magic mismatch"});
    }

    // Populate AABB.
    std::memcpy(aabb_.min, header_.aabb_min, 12);
    std::memcpy(aabb_.max, header_.aabb_max, 12);

    // --- Parse LOD table ---------------------------------------------------
    lod_ranges_.resize(header_.lod_count);
    if (header_.lod_table_off + header_.lod_count * 8u > raw.size()) {
        return std::unexpected(AssetError{AssetErrorCode::CorruptData,
                                          "gwmesh lod table out of bounds"});
    }
    const uint8_t* lod_ptr = raw.data() + header_.lod_table_off;
    for (uint32_t i = 0; i < header_.lod_count; ++i) {
        std::memcpy(&lod_ranges_[i], lod_ptr + i * 8u, 8u);
    }

    // --- Parse submesh table -----------------------------------------------
    submeshes_.resize(header_.submesh_count);
    const uint8_t* sub_ptr = raw.data() + header_.submesh_table_off;
    for (uint32_t i = 0; i < header_.submesh_count; ++i) {
        std::memcpy(&submeshes_[i], sub_ptr + i * sizeof(Submesh), sizeof(Submesh));
    }

    // --- Upload stream 0 (positions) ---------------------------------------
    if (header_.stream0_size > 0) {
        const void* s0 = raw.data() + header_.stream0_offset;
        auto res = create_device_local_buffer(device, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                               s0, header_.stream0_size);
        if (!res) return std::unexpected(res.error());
        stream0_buf_   = res->buffer;
        stream0_alloc_ = res->alloc;
    }

    // --- Upload stream 1 (attributes) --------------------------------------
    if (header_.stream1_size > 0) {
        const void* s1 = raw.data() + header_.stream1_offset;
        auto res = create_device_local_buffer(device, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                               s1, header_.stream1_size);
        if (!res) return std::unexpected(res.error());
        stream1_buf_   = res->buffer;
        stream1_alloc_ = res->alloc;
    }

    // --- Upload stream 2 (skin: inverse binds + packed joint weights) -------
    inverse_bind_bytes_.clear();
    if (header_.stream2_size > 0) {
        if (static_cast<std::size_t>(header_.stream2_offset) + header_.stream2_size > raw.size()) {
            return std::unexpected(AssetError{AssetErrorCode::CorruptData,
                                              "gwmesh stream2 out of bounds"});
        }
        if ((header_.flags & kMeshFlagSkinned) != 0 && header_.joint_count > 0u) {
            const std::size_t ib_bytes =
                static_cast<std::size_t>(header_.joint_count) * kInverseBindMat4Bytes;
            if (ib_bytes > header_.stream2_size) {
                return std::unexpected(AssetError{AssetErrorCode::CorruptData,
                                                  "gwmesh stream2 inverse binds truncated"});
            }
            const auto* const ib_begin =
                reinterpret_cast<const std::byte*>(raw.data() + header_.stream2_offset);
            inverse_bind_bytes_.assign(ib_begin, ib_begin + static_cast<std::ptrdiff_t>(ib_bytes));
        }
        const void* s2 = raw.data() + header_.stream2_offset;
        auto res = create_device_local_buffer(device, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                               s2, header_.stream2_size);
        if (!res) return std::unexpected(res.error());
        stream2_buf_   = res->buffer;
        stream2_alloc_ = res->alloc;
    }

    // --- Upload index buffer -----------------------------------------------
    if (header_.index_size > 0) {
        const void* idx = raw.data() + header_.index_offset;
        auto res = create_device_local_buffer(device, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                               idx, header_.index_size);
        if (!res) return std::unexpected(res.error());
        index_buf_   = res->buffer;
        index_alloc_ = res->alloc;
    }

    // Infer index type from header stream sizes: if stream0 vertex stride * vertex
    // count leaves ≤ 65535 indices it is UINT16, otherwise UINT32.
    // Simpler: check the index buffer stride in the header flags (see spec §6.2).
    // We store the info in the first submesh vertex count field.
    // Heuristic: if total vertex count fits in uint16, use UINT16.
    uint32_t max_verts = 0;
    for (const auto& sub : submeshes_) {
        max_verts = std::max(max_verts, sub.vertex_start + sub.vertex_count);
    }
    index_type_ = (max_verts <= 65535u) ? MeshIndexElementType::Uint16
                                        : MeshIndexElementType::Uint32;

    device_ = &device;

    raw_bytes_.assign(raw.begin(), raw.end());

    return asset_ok();
}

// ---------------------------------------------------------------------------
// MeshAsset::destroy_gpu
// ---------------------------------------------------------------------------
void MeshAsset::destroy_gpu() noexcept {
    if (!device_) return;
    VmaAllocator vma = device_->vma_allocator();
    if (stream0_buf_)  vmaDestroyBuffer(vma, stream0_buf_, stream0_alloc_);
    if (stream1_buf_)  vmaDestroyBuffer(vma, stream1_buf_, stream1_alloc_);
    if (stream2_buf_)  vmaDestroyBuffer(vma, stream2_buf_, stream2_alloc_);
    if (index_buf_)    vmaDestroyBuffer(vma, index_buf_,   index_alloc_);
    stream0_buf_ = stream1_buf_ = stream2_buf_ = index_buf_ = VK_NULL_HANDLE;
    stream0_alloc_ = stream1_alloc_ = stream2_alloc_ = index_alloc_ = VK_NULL_HANDLE;
    inverse_bind_bytes_.clear();
}

} // namespace gw::assets
