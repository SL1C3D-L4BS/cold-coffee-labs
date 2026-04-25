#pragma once
// editor/render/editor_scene_pass.hpp — Wave 1C: minimal Vulkan unlit + depth
// for the offscreen scene target (Volk path; not engine HAL).

#include <cstdint>
#include <memory>

#include <glm/fwd.hpp>
#include <volk.h> // Vk* types; do not add parallel `using Vk* =` aliases (redefine with vulkan_core.h).

// VMA: include pulled from implementation TUs <vk_mem_alloc.h>; only opaque in this header.
struct VmaAllocator_T;
using VmaAllocator = VmaAllocator_T*;

namespace gw::ecs { class World; }

namespace gw::editor::render {

class EditorScenePass {
public:
    EditorScenePass();
    ~EditorScenePass();

    EditorScenePass(const EditorScenePass&)            = delete;
    EditorScenePass& operator=(const EditorScenePass&) = delete;

    /// Must be called once scene colour + depth formats are known. Idempotent
    /// when formats and device are unchanged; recreates the pipeline on format
    /// change.
    /// \param color_format / depth_format: VkFormat (see `vulkan_core.h`); 0 = UNDEFINED.
    [[nodiscard]] bool init_or_recreate(
        VkDevice device, VmaAllocator alloc, std::uint32_t color_format, std::uint32_t depth_format);

    void shutdown() noexcept;

    void record(
        VkCommandBuffer cmd, VkDevice device, VmaAllocator alloc, std::uint32_t extent_w,
        std::uint32_t extent_h, const glm::mat4& view, const glm::mat4& proj,
        gw::ecs::World& world, std::uint32_t frame_index, std::uint32_t max_frames_in_flight) noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_{};
};

} // namespace gw::editor::render
