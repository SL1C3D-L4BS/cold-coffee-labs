#pragma once
// editor/panels/viewport_panel.hpp
// Offscreen render target + camera + gizmos + debug draw + entity picking.
// Spec ref: Phase 7 §6.

#include "panel.hpp"
#include "editor/undo/commands.hpp"
#include "editor/viewport/editor_camera.hpp"
#include "editor/viewport/gizmo_system.hpp"

#include <cstdint>
#include <volk.h> // VkDescriptorSet for set_scene_texture; canonical Vk* (no `using` aliases)

namespace gw::editor {

class ViewportPanel final : public IPanel {
public:
    // The panel has no Vulkan dependency at construction; `EditorApplication`
    // passes the scene image descriptor from `set_scene_texture()` (Phase 7 §6.1).
    ViewportPanel()           = default;
    ~ViewportPanel() override;

    void on_imgui_render(EditorContext& ctx) override;
    void on_event(int glfw_key, int action) override;
    [[nodiscard]] const char* name() const override { return "Viewport"; }

    // Called by EditorApplication when a resize is safe (after wait_idle).
    void apply_resize(uint32_t w, uint32_t h);

    // EditorApplication owns the Vulkan scene image; it hands the panel the
    // ImGui-compatible descriptor set (and its size) whenever the RT is
    // (re-)created. The panel only displays — it does not own GPU state.
    void set_scene_texture(VkDescriptorSet ds, uint32_t w, uint32_t h) noexcept {
        scene_ds_  = ds;
        rt_width_  = w;
        rt_height_ = h;
    }

    [[nodiscard]] const EditorCamera& camera() const noexcept { return camera_; }
    [[nodiscard]] EditorCamera& camera() noexcept { return camera_; }

    // Scroll delta injected by GLFW scroll callback.
    void inject_scroll(float delta) noexcept { scroll_accumulator_ += delta; }

    /// Blockout / level tool: 0 Place, 1 Move, 2 Rotate, 3 Scale (Phase-1 stub).
    [[nodiscard]] int blockout_tool() const noexcept { return blockout_tool_; }
    void set_blockout_tool(int t) noexcept { blockout_tool_ = t; }

private:
    void draw_toolbar(EditorContext& ctx);
    void draw_overlay(EditorContext& ctx);

    // EditorApplication owns the Vulkan image and passes us the sampled
    // descriptor set via set_scene_texture(). We never vkCreate / vkDestroy.
    VkDescriptorSet   scene_ds_     = nullptr;  // ImGui ImTextureID

    uint32_t          rt_width_       = 0;
    uint32_t          rt_height_      = 0;

public:
    // Queried by EditorApplication main loop to apply deferred resize.
    bool              resize_pending_  = false;
    uint32_t          pending_w_       = 0;
    uint32_t          pending_h_       = 0;

private:

    EditorCamera      camera_;
    GizmoSystem       gizmo_;
    float             scroll_accumulator_ = 0.f;
    int               blockout_tool_      = 0;
    int               blockout_place_seq_ = 0;
    int               blockout_shape_     = 0; // 0 box … 3 ramp (see BlockoutPrimitiveComponent)
    /// Frames remaining to show Play-enter failure banner after `gw_editor_enter_play` returns false.
    int               pie_transport_fail_flash_ = 0;
    bool              snap_grid_enabled_  = false;
    float             snap_grid_step_     = 1.f;
    bool              gizmo_was_using_    = false;
    bool              gizmo_undo_armed_   = false;
    EntityHandle      gizmo_undo_entity_  = kNullEntity;
    gw::editor::undo::TransformSnapshot gizmo_undo_before_{};

    // Reserved for GPU object-ID picking (render-to-texture + readback). Today
    // selection uses a world-space ray vs. transform AABBs (O(n) over visible
    // entities) — accurate enough for blockout, not mesh-accurate.
    [[maybe_unused]] bool              picking_requested_   = false;
    [[maybe_unused]] uint32_t          pick_pixel_x_        = 0;
    [[maybe_unused]] uint32_t          pick_pixel_y_        = 0;
    EntityHandle      hovered_entity_      = kNullEntity;

    // Viewport window size recorded last frame.
    float vp_x_ = 0.f, vp_y_ = 0.f, vp_w_ = 0.f, vp_h_ = 0.f;

    GLFWwindow* window_ = nullptr;  // borrowed from EditorApplication
public:
    void set_window(GLFWwindow* w) noexcept { window_ = w; }
};

}  // namespace gw::editor
