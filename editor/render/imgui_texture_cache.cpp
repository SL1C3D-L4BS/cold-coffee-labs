// editor/render/imgui_texture_cache.cpp

#include "editor/render/imgui_texture_cache.hpp"

#include "engine/assets/image_decode_rgba8.hpp"

#include <volk.h>
#include <vk_mem_alloc.h>

#include <imgui_impl_vulkan.h>

#include <algorithm>
#include <list>
#include <unordered_map>

namespace gw::editor::render {
namespace {

void downsample_max(std::vector<std::uint8_t>& rgba, int& w, int& h, int max_dim) {
    while (w > max_dim || h > max_dim) {
        const int nw = std::max(1, w / 2);
        const int nh = std::max(1, h / 2);
        std::vector<std::uint8_t> out(static_cast<std::size_t>(nw) * static_cast<std::size_t>(nh) * 4u);
        for (int y = 0; y < nh; ++y) {
            const int sy = std::min(h - 1, (y * h) / nh);
            for (int x = 0; x < nw; ++x) {
                const int sx = std::min(w - 1, (x * w) / nw);
                const std::size_t sidx =
                    (static_cast<std::size_t>(sy) * static_cast<std::size_t>(w) +
                     static_cast<std::size_t>(sx)) *
                    4u;
                const std::size_t didx =
                    (static_cast<std::size_t>(y) * static_cast<std::size_t>(nw) +
                     static_cast<std::size_t>(x)) *
                    4u;
                std::memcpy(out.data() + didx, rgba.data() + sidx, 4);
            }
        }
        rgba.swap(out);
        w = nw;
        h = nh;
    }
}

#define GW_VKCHECK(expr)                                                                           \
    do {                                                                                           \
        VkResult _r = (expr);                                                                      \
        if (_r != VK_SUCCESS) return false;                                                        \
    } while (0)

[[nodiscard]] bool upload_rgba8(VkDevice device, VkPhysicalDevice /*phys*/, VkQueue queue,
                                 std::uint32_t qfam, VkCommandPool pool, VmaAllocator allocator,
                                 const std::uint8_t* rgba, int w, int h, VkImage* out_image,
                                 VmaAllocation* out_alloc, VkImageView* out_view,
                                 VkSampler* out_sampler) {
    if (!device || !queue || !pool || !allocator || !rgba || w <= 0 || h <= 0) return false;

    const VkDeviceSize staging_size =
        static_cast<VkDeviceSize>(w) * static_cast<VkDeviceSize>(h) * 4u;

    VkBufferCreateInfo sbci{};
    sbci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    sbci.size        = staging_size;
    sbci.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    sbci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo saci{};
    saci.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    VkBuffer       staging = VK_NULL_HANDLE;
    VmaAllocation  salloc  = nullptr;
    if (vmaCreateBuffer(allocator, &sbci, &saci, &staging, &salloc, nullptr) != VK_SUCCESS)
        return false;

    void* mapped = nullptr;
    if (vmaMapMemory(allocator, salloc, &mapped) != VK_SUCCESS) {
        vmaDestroyBuffer(allocator, staging, salloc);
        return false;
    }
    std::memcpy(mapped, rgba, static_cast<std::size_t>(staging_size));
    vmaUnmapMemory(allocator, salloc);

    VkImageCreateInfo ici{};
    ici.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ici.imageType     = VK_IMAGE_TYPE_2D;
    ici.format        = VK_FORMAT_R8G8B8A8_SRGB;
    ici.extent        = {static_cast<std::uint32_t>(w), static_cast<std::uint32_t>(h), 1u};
    ici.mipLevels     = 1;
    ici.arrayLayers   = 1;
    ici.samples       = VK_SAMPLE_COUNT_1_BIT;
    ici.tiling        = VK_IMAGE_TILING_OPTIMAL;
    ici.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ici.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo iaci{};
    iaci.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VkImage       image = VK_NULL_HANDLE;
    VmaAllocation ialloc = nullptr;
    if (vmaCreateImage(allocator, &ici, &iaci, &image, &ialloc, nullptr) != VK_SUCCESS) {
        vmaDestroyBuffer(allocator, staging, salloc);
        return false;
    }

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VkCommandBufferAllocateInfo cbai{};
    cbai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.commandPool        = pool;
    cbai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = 1;
    GW_VKCHECK(vkAllocateCommandBuffers(device, &cbai, &cmd));

    VkCommandBufferBeginInfo bi{};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    GW_VKCHECK(vkBeginCommandBuffer(cmd, &bi));

    VkImageMemoryBarrier2 to_transfer{};
    to_transfer.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    to_transfer.srcStageMask                  = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    to_transfer.srcAccessMask                 = 0;
    to_transfer.dstStageMask                  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    to_transfer.dstAccessMask                 = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    to_transfer.oldLayout                     = VK_IMAGE_LAYOUT_UNDEFINED;
    to_transfer.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    to_transfer.image                         = image;
    to_transfer.subresourceRange.aspectMask   = VK_IMAGE_ASPECT_COLOR_BIT;
    to_transfer.subresourceRange.levelCount   = 1;
    to_transfer.subresourceRange.layerCount   = 1;

    VkDependencyInfo dep{};
    dep.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.imageMemoryBarrierCount = 1;
    dep.pImageMemoryBarriers    = &to_transfer;
    vkCmdPipelineBarrier2(cmd, &dep);

    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.width           = static_cast<std::uint32_t>(w);
    region.imageExtent.height          = static_cast<std::uint32_t>(h);
    region.imageExtent.depth           = 1;
    vkCmdCopyBufferToImage(cmd, staging, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                           &region);

    VkImageMemoryBarrier2 to_sample{};
    to_sample.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    to_sample.srcStageMask                    = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    to_sample.srcAccessMask                   = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    to_sample.dstStageMask                    = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    to_sample.dstAccessMask                   = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
    to_sample.oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    to_sample.newLayout                       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    to_sample.image                           = image;
    to_sample.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    to_sample.subresourceRange.levelCount     = 1;
    to_sample.subresourceRange.layerCount     = 1;

    dep.pImageMemoryBarriers = &to_sample;
    vkCmdPipelineBarrier2(cmd, &dep);

    GW_VKCHECK(vkEndCommandBuffer(cmd));

    VkCommandBufferSubmitInfo cbsi{};
    cbsi.sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    cbsi.commandBuffer = cmd;

    VkSubmitInfo2 si{};
    si.sType                   = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    si.commandBufferInfoCount  = 1;
    si.pCommandBufferInfos     = &cbsi;
    GW_VKCHECK(vkQueueSubmit2(queue, 1, &si, VK_NULL_HANDLE));
    GW_VKCHECK(vkQueueWaitIdle(queue));

    vkFreeCommandBuffers(device, pool, 1, &cmd);
    vmaDestroyBuffer(allocator, staging, salloc);

    VkImageViewCreateInfo ivi{};
    ivi.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ivi.image                           = image;
    ivi.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    ivi.format                          = VK_FORMAT_R8G8B8A8_SRGB;
    ivi.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    ivi.subresourceRange.levelCount     = 1;
    ivi.subresourceRange.layerCount     = 1;
    GW_VKCHECK(vkCreateImageView(device, &ivi, nullptr, out_view));

    VkSamplerCreateInfo sci{};
    sci.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sci.magFilter    = VK_FILTER_LINEAR;
    sci.minFilter    = VK_FILTER_LINEAR;
    sci.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sci.minLod       = 0.f;
    sci.maxLod       = 1.f;
    GW_VKCHECK(vkCreateSampler(device, &sci, nullptr, out_sampler));

    *out_image = image;
    *out_alloc = ialloc;
    return true;
}

#undef GW_VKCHECK

struct GpuTex {
    VkImage         image   = VK_NULL_HANDLE;
    VmaAllocation   alloc   = nullptr;
    VkImageView     view    = VK_NULL_HANDLE;
    VkSampler       sampler = VK_NULL_HANDLE;
    VkDescriptorSet tex_ds  = VK_NULL_HANDLE;
};

} // namespace

struct ImGuiTextureCache::Impl {
    VkDevice            device = VK_NULL_HANDLE;
    VkPhysicalDevice    physical_device = VK_NULL_HANDLE;
    VkQueue             queue = VK_NULL_HANDLE;
    std::uint32_t       qfam = 0;
    VkCommandPool       pool = VK_NULL_HANDLE;
    VmaAllocator        allocator = VK_NULL_HANDLE;
    std::size_t         max_live = 192;

    std::list<std::string> lru_keys;
    struct MapEntry {
        std::list<std::string>::iterator lru_it;
        GpuTex                           gpu{};
    };
    std::unordered_map<std::string, MapEntry> map;

    void evict_one() {
        if (lru_keys.empty()) return;
        const std::string key = std::move(lru_keys.front());
        lru_keys.pop_front();
        auto it = map.find(key);
        if (it == map.end()) return;
        GpuTex& g = it->second.gpu;
        if (g.tex_ds)
            ImGui_ImplVulkan_RemoveTexture(g.tex_ds);
        if (g.view) vkDestroyImageView(device, g.view, nullptr);
        if (g.sampler) vkDestroySampler(device, g.sampler, nullptr);
        if (g.image) vmaDestroyImage(allocator, g.image, g.alloc);
        map.erase(it);
    }

    void touch(const std::string& key, MapEntry& me) {
        lru_keys.erase(me.lru_it);
        lru_keys.push_back(key);
        me.lru_it = std::prev(lru_keys.end());
    }
};

ImGuiTextureCache::ImGuiTextureCache(VkDevice device, VkPhysicalDevice physical_device,
                                     VkQueue graphics_queue, std::uint32_t graphics_queue_family,
                                     VkCommandPool command_pool, VmaAllocator allocator,
                                     std::size_t max_live_textures)
    : impl_(std::make_unique<Impl>()) {
    impl_->device            = device;
    impl_->physical_device   = physical_device;
    impl_->queue             = graphics_queue;
    impl_->qfam               = graphics_queue_family;
    impl_->pool              = command_pool;
    impl_->allocator         = allocator;
    impl_->max_live          = std::max<std::size_t>(16, max_live_textures);
}

ImGuiTextureCache::~ImGuiTextureCache() {
    if (!impl_) return;
    while (!impl_->lru_keys.empty())
        impl_->evict_one();
}

void ImGuiTextureCache::flush_idle() {
    if (impl_ && impl_->queue)
        vkQueueWaitIdle(impl_->queue);
}

bool ImGuiTextureCache::has_cached_texture(const std::filesystem::path& path) const {
    if (!impl_) return false;
    namespace fs = std::filesystem;
    std::error_code ec;
    const fs::path abs = fs::weakly_canonical(path, ec);
    const std::string key = abs.generic_string();
    return impl_->map.find(key) != impl_->map.end();
}

ImTextureID ImGuiTextureCache::texture_id_for_image_file(const std::filesystem::path& path) {
    if (!impl_) return (ImTextureID)0;
    namespace fs = std::filesystem;
    std::error_code ec;
    const fs::path abs = fs::weakly_canonical(path, ec);
    const std::string key = abs.generic_string();

    if (auto it = impl_->map.find(key); it != impl_->map.end()) {
        impl_->touch(key, it->second);
        return reinterpret_cast<ImTextureID>(it->second.gpu.tex_ds);
    }

    auto decoded = gw::assets::decode_image_rgba8_from_file(abs);
    if (!decoded) return (ImTextureID)0;

    int w = decoded->width;
    int h = decoded->height;
    downsample_max(decoded->rgba, w, h, 256);

    while (impl_->map.size() >= impl_->max_live)
        impl_->evict_one();

    GpuTex gpu{};
    if (!upload_rgba8(impl_->device, impl_->physical_device, impl_->queue, impl_->qfam,
                       impl_->pool, impl_->allocator, decoded->rgba.data(), w, h, &gpu.image,
                       &gpu.alloc, &gpu.view, &gpu.sampler)) {
        return (ImTextureID)0;
    }

    gpu.tex_ds = ImGui_ImplVulkan_AddTexture(gpu.sampler, gpu.view,
                                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    if (!gpu.tex_ds) {
        vkDestroySampler(impl_->device, gpu.sampler, nullptr);
        vkDestroyImageView(impl_->device, gpu.view, nullptr);
        vmaDestroyImage(impl_->allocator, gpu.image, gpu.alloc);
        return (ImTextureID)0;
    }

    Impl::MapEntry me{};
    me.gpu = gpu;
    ImTextureID out_id = reinterpret_cast<ImTextureID>(me.gpu.tex_ds);
    impl_->lru_keys.push_back(key);
    me.lru_it = std::prev(impl_->lru_keys.end());
    impl_->map.emplace(key, std::move(me));
    return out_id;
}

} // namespace gw::editor::render
