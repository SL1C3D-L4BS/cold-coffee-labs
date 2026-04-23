#pragma once
// editor/render/imgui_texture_cache.hpp — CPU decode → GPU RGBA8 → ImGui_ImplVulkan_AddTexture.

#include <imgui.h>

#include <cstdint>
#include <filesystem>
#include <memory>

// Forward-declare Vulkan / VMA — defined in editor_app.cpp TU via volk + vk_mem_alloc.h.
struct VkDevice_T;
struct VkPhysicalDevice_T;
struct VkQueue_T;
struct VkCommandPool_T;
struct VmaAllocator_T;
using VkDevice            = VkDevice_T*;
using VkPhysicalDevice    = VkPhysicalDevice_T*;
using VkQueue             = VkQueue_T*;
using VkCommandPool       = VkCommandPool_T*;
using VmaAllocator        = VmaAllocator_T*;

namespace gw::editor::render {

class ImGuiTextureCache {
public:
    ImGuiTextureCache(VkDevice device, VkPhysicalDevice physical_device, VkQueue graphics_queue,
                      std::uint32_t graphics_queue_family, VkCommandPool command_pool,
                      VmaAllocator allocator, std::size_t max_live_textures = 192);
    ~ImGuiTextureCache();

    ImGuiTextureCache(const ImGuiTextureCache&)            = delete;
    ImGuiTextureCache& operator=(const ImGuiTextureCache&) = delete;

    /// Returns ImTextureID for ImGui::Image, or 0 if decode/upload failed.
    [[nodiscard]] ImTextureID texture_id_for_image_file(const std::filesystem::path& path);

    /// True if `path` is already resident (no decode/upload). For upload budgeting.
    [[nodiscard]] bool has_cached_texture(const std::filesystem::path& path) const;

    void flush_idle(); // optional: GPU finish (e.g. before shutdown)

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gw::editor::render
