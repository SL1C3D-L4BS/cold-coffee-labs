#pragma once
// editor/render/editor_scene_pass.hpp — Wave 1C: minimal Vulkan unlit + depth
// for the offscreen scene target (Volk path; not engine HAL).

#include <cstdint>
#include <memory>

#include <glm/mat4x4.hpp>
#include <volk.h> // Vk* types; do not add parallel `using Vk* =` aliases (redefine with vulkan_core.h).

// VMA: include pulled from implementation TUs <vk_mem_alloc.h>; only opaque in this header.
struct VmaAllocator_T;
using VmaAllocator = VmaAllocator_T*;

namespace gw::ecs { class World; }
namespace gw::assets { class AssetDatabase; }
namespace gw::anim { class AnimationWorld; }

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

    /// One aggregate per scene render — avoids brittle 11-argument calls and keeps
    /// clang/clangd happy when `AssetDatabase` is only forward-declared here.
    struct RecordFrame {
        VkCommandBuffer            cmd{};
        VkDevice                    device{};
        VmaAllocator                allocator{};
        std::uint32_t               extent_w{};
        std::uint32_t               extent_h{};
        glm::mat4                   view{1.f};
        glm::mat4                   proj{1.f};
        gw::ecs::World*             world{};
        std::uint32_t               frame_index{};
        std::uint32_t               max_frames_in_flight{};
        gw::assets::AssetDatabase*  asset_db{};
        /// Optional; when set with a valid `EditorCookedMeshComponent::anim_instance_id`,
        /// skinned draws fill the palette via `AnimationWorld::build_skin_matrix_palette`.
        gw::anim::AnimationWorld*   anim_world{};
    };

    void record(const RecordFrame& frame) noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_{};
};

} // namespace gw::editor::render
