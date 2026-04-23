// engine/world/gptm/gptm_mesh_builder_upload.cpp — VMA / Vulkan upload path (isolated TU for tooling).

#include "engine/world/gptm/gptm_mesh_builder.hpp"

#include "engine/render/hal/vulkan_device.hpp"

#include <cstring>

#include <vk_mem_alloc.h>

namespace gw::world::gptm {
namespace {

[[nodiscard]] bool create_host_visible_buffer(gw::render::hal::VulkanDevice& device,
                                              const void* data, VkDeviceSize size,
                                              VkBufferUsageFlags               usage,
                                              VkBuffer*                        out_buf,
                                              VmaAllocation*                   out_alloc) noexcept {
    if (size == 0 || out_buf == nullptr || out_alloc == nullptr) {
        return false;
    }

    VmaAllocator vma = device.vma_allocator();

    VkBufferCreateInfo bi{};
    bi.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bi.size        = size;
    bi.usage       = usage;
    bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo aci{};
    aci.usage = VMA_MEMORY_USAGE_AUTO;
    aci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
              | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkBuffer      buf = VK_NULL_HANDLE;
    VmaAllocation alloc{};
    VmaAllocationInfo inf{};
    if (vmaCreateBuffer(vma, &bi, &aci, &buf, &alloc, &inf) != VK_SUCCESS) {
        return false;
    }
    if (inf.pMappedData != nullptr && data != nullptr) {
        std::memcpy(inf.pMappedData, data, static_cast<std::size_t>(size));
        vmaFlushAllocation(vma, alloc, 0, VK_WHOLE_SIZE);
    }
    *out_buf   = buf;
    *out_alloc = alloc;
    return true;
}

} // namespace

bool GptmMeshBuilder::upload_tile(gw::render::hal::VulkanDevice& device, const GptmMeshCpuData& mesh,
                                  GptmTile& out_tile) noexcept {
    if (mesh.vertices == nullptr || mesh.indices == nullptr || mesh.vertex_count == 0
        || mesh.index_count == 0) {
        return false;
    }

    VkBuffer      vb{};
    VmaAllocation vba{};
    const VkDeviceSize vsize =
        static_cast<VkDeviceSize>(sizeof(GptmVertex) * static_cast<std::size_t>(mesh.vertex_count));
    if (!create_host_visible_buffer(device, mesh.vertices, vsize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                    &vb, &vba)) {
        return false;
    }

    VkBuffer      ib{};
    VmaAllocation iba{};
    const VkDeviceSize isize =
        static_cast<VkDeviceSize>(sizeof(std::uint32_t) * static_cast<std::size_t>(mesh.index_count));
    if (!create_host_visible_buffer(device, mesh.indices, isize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                    &ib, &iba)) {
        vmaDestroyBuffer(device.vma_allocator(), vb, vba);
        return false;
    }

    out_tile.vertex_buffer.buffer     = reinterpret_cast<std::uintptr_t>(vb);
    out_tile.vertex_buffer.allocation = reinterpret_cast<std::uintptr_t>(vba);
    out_tile.index_buffer.buffer      = reinterpret_cast<std::uintptr_t>(ib);
    out_tile.index_buffer.allocation  = reinterpret_cast<std::uintptr_t>(iba);
    out_tile.index_count              = mesh.index_count;
    out_tile.world_bounds             = mesh.world_bounds;
    out_tile.dominant_biome           = (mesh.vertices != nullptr) ? mesh.vertices[0].biome_id : 0u;
    return true;
}

void GptmMeshBuilder::destroy_tile_buffers(gw::render::hal::VulkanDevice& device,
                                           GptmTile&                     tile) noexcept {
    VmaAllocator vma = device.vma_allocator();
    if (tile.vertex_buffer.buffer != 0u) {
        vmaDestroyBuffer(vma, reinterpret_cast<VkBuffer>(tile.vertex_buffer.buffer),
                         reinterpret_cast<VmaAllocation>(tile.vertex_buffer.allocation));
        tile.vertex_buffer = {};
    }
    if (tile.index_buffer.buffer != 0u) {
        vmaDestroyBuffer(vma, reinterpret_cast<VkBuffer>(tile.index_buffer.buffer),
                         reinterpret_cast<VmaAllocation>(tile.index_buffer.allocation));
        tile.index_buffer = {};
    }
}

} // namespace gw::world::gptm
