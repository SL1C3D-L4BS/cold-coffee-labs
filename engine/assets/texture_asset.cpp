#include "texture_asset.hpp"
#include "engine/render/hal/vulkan_device.hpp"
#include <cstring>
#include <vk_mem_alloc.h>
#include <volk.h>

// basis_universal transcoder (included as header-only via basisu CPM dep).
#include <basisu_transcoder.h>

namespace gw::assets {

namespace {

// Global transcoder initialiser — call once before any transcoding.
struct BasisuInit {
    BasisuInit() { basist::basisu_transcoder_init(); }
};

static const BasisuInit g_basisu_init;

// Map VkFormat to basist::transcoder_texture_format.
basist::transcoder_texture_format vk_to_basis_format(uint32_t vk_format) {
    // VK_FORMAT_BC7_UNORM_BLOCK = 145, VK_FORMAT_BC7_SRGB_BLOCK = 146
    if (vk_format == 145 || vk_format == 146)
        return basist::transcoder_texture_format::cTFBC7_RGBA;
    // VK_FORMAT_BC5_UNORM_BLOCK = 143
    if (vk_format == 143)
        return basist::transcoder_texture_format::cTFBC5_RG;
    // VK_FORMAT_BC6H_UFLOAT_BLOCK = 148
    if (vk_format == 148)
        return basist::transcoder_texture_format::cTFBC6H;
    // VK_FORMAT_BC1_RGB_UNORM_BLOCK = 131
    if (vk_format == 131 || vk_format == 132)
        return basist::transcoder_texture_format::cTFBC1_RGB;
    // Default to BC7
    return basist::transcoder_texture_format::cTFBC7_RGBA;
}

} // anonymous namespace

AssetOk TextureAsset::upload(render::hal::VulkanDevice& device,
                              std::span<const uint8_t> raw)
{
    if (raw.size() < sizeof(GwTexHeader)) {
        return std::unexpected(AssetError{AssetErrorCode::CorruptData,
                                          "gwtex too small for header"});
    }
    std::memcpy(&header_, raw.data(), sizeof(GwTexHeader));

    if (header_.magic != magic::kTexture) {
        return std::unexpected(AssetError{AssetErrorCode::InvalidMagic,
                                          "gwtex magic mismatch"});
    }
    if (header_.version != 1u) {
        return std::unexpected(AssetError{AssetErrorCode::UnsupportedVersion,
                                          "unsupported gwtex version"});
    }

    // KTX2 payload.
    const uint8_t* ktx2_data = raw.data() + header_.ktx2_offset;
    const uint32_t ktx2_size = header_.ktx2_size;

    if (header_.ktx2_offset + ktx2_size > raw.size()) {
        return std::unexpected(AssetError{AssetErrorCode::CorruptData,
                                          "gwtex KTX2 payload out of bounds"});
    }

    // Transcode from UASTC to the target GPU format on the CPU.
    basist::ktx2_transcoder ktx2_dec;
    if (!ktx2_dec.init(ktx2_data, ktx2_size)) {
        return std::unexpected(AssetError{AssetErrorCode::CorruptData,
                                          "basis KTX2 init failed"});
    }
    if (!ktx2_dec.start_transcoding()) {
        return std::unexpected(AssetError{AssetErrorCode::CookFailed,
                                          "basis KTX2 start_transcoding failed"});
    }

    const auto basis_fmt = vk_to_basis_format(header_.vk_format);
    const uint32_t mips  = header_.mip_count;
    const uint32_t layers= std::max<uint32_t>(1, header_.depth_layers);

    // Accumulate transcoded mip data.
    std::vector<std::vector<uint8_t>> mip_data(mips);
    uint32_t total_bytes = 0;
    for (uint32_t m = 0; m < mips; ++m) {
        uint32_t w = std::max<uint32_t>(1, header_.width  >> m);
        uint32_t h = std::max<uint32_t>(1, header_.height >> m);
        const uint32_t blocks_x = (w + 3) / 4;
        const uint32_t blocks_y = (h + 3) / 4;
        const uint32_t output_size =
            blocks_x * blocks_y * basist::basis_get_bytes_per_block_or_pixel(basis_fmt);
        mip_data[m].resize(output_size * layers);

        for (uint32_t layer = 0; layer < layers; ++layer) {
            if (!ktx2_dec.transcode_image_level(
                    m, layer, 0,
                    mip_data[m].data() + layer * output_size,
                    output_size,
                    basis_fmt)) {
                return std::unexpected(AssetError{AssetErrorCode::CookFailed,
                                                   "basis transcode failed for mip"});
            }
        }
        total_bytes += static_cast<uint32_t>(mip_data[m].size());
    }

    // ---- Create GPU image -------------------------------------------------
    VmaAllocator vma = device.vma_allocator();

    VkImageCreateInfo image_ci{};
    image_ci.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_ci.imageType     = VK_IMAGE_TYPE_2D;
    image_ci.format        = static_cast<VkFormat>(header_.vk_format);
    image_ci.extent        = {header_.width, header_.height, 1};
    image_ci.mipLevels     = mips;
    image_ci.arrayLayers   = layers;
    image_ci.samples       = VK_SAMPLE_COUNT_1_BIT;
    image_ci.tiling        = VK_IMAGE_TILING_OPTIMAL;
    image_ci.usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    image_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (header_.tex_type == static_cast<uint8_t>(TexType::TexCube)) {
        image_ci.flags       = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        image_ci.arrayLayers = 6;
    }

    VmaAllocationCreateInfo alloc_ci{};
    alloc_ci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    if (vmaCreateImage(vma, &image_ci, &alloc_ci, &image_, &alloc_, nullptr) != VK_SUCCESS) {
        return std::unexpected(AssetError{AssetErrorCode::GpuUploadFailed,
                                          "image creation failed"});
    }

    // ---- Staging buffer ---------------------------------------------------
    VkBufferCreateInfo staging_ci{};
    staging_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    staging_ci.size  = total_bytes;
    staging_ci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stg_alloc_ci{};
    stg_alloc_ci.usage = VMA_MEMORY_USAGE_AUTO;
    stg_alloc_ci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                       | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkBuffer          stg_buf   = VK_NULL_HANDLE;
    VmaAllocation     stg_alloc = VK_NULL_HANDLE;
    VmaAllocationInfo stg_info{};

    if (vmaCreateBuffer(vma, &staging_ci, &stg_alloc_ci,
                        &stg_buf, &stg_alloc, &stg_info) != VK_SUCCESS) {
        return std::unexpected(AssetError{AssetErrorCode::GpuUploadFailed,
                                          "staging buffer creation failed"});
    }

    // Copy transcoded pixels into the staging buffer.
    uint8_t* mapped = static_cast<uint8_t*>(stg_info.pMappedData);
    uint32_t offset = 0;
    for (uint32_t m = 0; m < mips; ++m) {
        std::memcpy(mapped + offset, mip_data[m].data(), mip_data[m].size());
        offset += static_cast<uint32_t>(mip_data[m].size());
    }
    vmaFlushAllocation(vma, stg_alloc, 0, VK_WHOLE_SIZE);

    // ---- One-shot command buffer to upload ---------------------------------
    VkCommandBufferAllocateInfo cb_alloc{};
    cb_alloc.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cb_alloc.commandPool        = device.graphics_command_pool();
    cb_alloc.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cb_alloc.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(device.native_handle(), &cb_alloc, &cmd);

    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &begin);

    // Transition UNDEFINED → TRANSFER_DST_OPTIMAL.
    VkImageMemoryBarrier barrier{};
    barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image               = image_;
    barrier.subresourceRange    = {VK_IMAGE_ASPECT_COLOR_BIT, 0, mips, 0, layers};
    barrier.srcAccessMask       = 0;
    barrier.dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Copy each mip level.
    uint32_t buf_offset = 0;
    for (uint32_t m = 0; m < mips; ++m) {
        uint32_t w = std::max<uint32_t>(1, header_.width  >> m);
        uint32_t h = std::max<uint32_t>(1, header_.height >> m);
        uint32_t mip_bytes = static_cast<uint32_t>(mip_data[m].size());

        VkBufferImageCopy region{};
        region.bufferOffset      = buf_offset;
        region.imageSubresource  = {VK_IMAGE_ASPECT_COLOR_BIT,
                                    m, 0, layers};
        region.imageExtent       = {w, h, 1};
        vkCmdCopyBufferToImage(cmd, stg_buf, image_,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1, &region);
        buf_offset += mip_bytes;
    }

    // Transition TRANSFER_DST → SHADER_READ_ONLY.
    barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit{};
    submit.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers    = &cmd;
    vkQueueSubmit(device.graphics_queue(), 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(device.graphics_queue());

    vkFreeCommandBuffers(device.native_handle(), device.graphics_command_pool(), 1, &cmd);
    vmaDestroyBuffer(vma, stg_buf, stg_alloc);

    // ---- Image view -------------------------------------------------------
    VkImageViewCreateInfo view_ci{};
    view_ci.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_ci.image    = image_;
    view_ci.viewType = (header_.tex_type == static_cast<uint8_t>(TexType::TexCube))
                       ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
    view_ci.format   = static_cast<VkFormat>(header_.vk_format);
    view_ci.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, mips, 0, layers};
    vkCreateImageView(device.native_handle(), &view_ci, nullptr, &view_);

    // ---- Default sampler --------------------------------------------------
    VkSamplerCreateInfo sampler_ci{};
    sampler_ci.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_ci.magFilter    = VK_FILTER_LINEAR;
    sampler_ci.minFilter    = VK_FILTER_LINEAR;
    sampler_ci.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_ci.minLod       = 0.0f;
    sampler_ci.maxLod       = static_cast<float>(mips);
    sampler_ci.anisotropyEnable = device.supports_anisotropy() ? VK_TRUE : VK_FALSE;
    sampler_ci.maxAnisotropy    = device.supports_anisotropy() ? 8.0f : 1.0f;
    vkCreateSampler(device.native_handle(), &sampler_ci, nullptr, &sampler_);

    device_ = &device;
    raw_bytes_.assign(raw.begin(), raw.end());
    return asset_ok();
}

void TextureAsset::destroy_gpu() noexcept {
    if (!device_) return;
    VkDevice dev = device_->native_handle();
    if (sampler_) vkDestroySampler(dev, sampler_, nullptr);
    if (view_)    vkDestroyImageView(dev, view_, nullptr);
    if (image_)   vmaDestroyImage(device_->vma_allocator(), image_, alloc_);
    sampler_ = VK_NULL_HANDLE;
    view_    = VK_NULL_HANDLE;
    image_   = VK_NULL_HANDLE;
    alloc_   = VK_NULL_HANDLE;
}

} // namespace gw::assets
