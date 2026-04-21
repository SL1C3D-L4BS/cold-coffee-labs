#pragma once
// editor/panels/viewport_panel.hpp
// Offscreen render target + camera + gizmos + debug draw + entity picking.
// Spec ref: Phase 7 §6.

#include "panel.hpp"
#include "editor/viewport/editor_camera.hpp"
#include "editor/viewport/gizmo_system.hpp"

// Vulkan forward declares — avoids pulling in volk in the header.
#include <cstdint>

struct VkImage_T;     using VkImage      = VkImage_T*;
struct VkImageView_T; using VkImageView  = VkImageView_T*;
struct VkSampler_T;   using VkSampler    = VkSampler_T*;
struct VkDescriptorSet_T; using VkDescriptorSet = VkDescriptorSet_T*;
struct VmaAllocation_T;   using VmaAllocation   = VmaAllocation_T*;

namespace gw::editor {

class ViewportPanel final : public IPanel {
public:
    // Phase 7: offscreen render target allocation is a TODO (§6.1). The panel
    // therefore has no Vulkan dependency at construction; it will acquire the
    // required device + allocator handles via a setter when Phase 8 wires
    // real scene-image creation.
    ViewportPanel()           = default;
    ~ViewportPanel() override;

    void on_imgui_render(EditorContext& ctx) override;
    void on_event(int glfw_key, int action) override;
    [[nodiscard]] const char* name() const override { return "Viewport"; }

    // Called by EditorApplication when a resize is safe (after wait_idle).
    void apply_resize(uint32_t w, uint32_t h);

    [[nodiscard]] const EditorCamera& camera() const noexcept { return camera_; }
    [[nodiscard]] EditorCamera& camera() noexcept { return camera_; }

    // Scroll delta injected by GLFW scroll callback.
    void inject_scroll(float delta) noexcept { scroll_accumulator_ += delta; }

private:
    void create_scene_image(uint32_t w, uint32_t h);
    void destroy_scene_image();
    void draw_toolbar(EditorContext& ctx);
    void draw_overlay(EditorContext& ctx);

    // Offscreen scene image — displayed via ImGui::Image.
    VkImage           scene_image_  = nullptr;
    VkImageView       scene_view_   = nullptr;
    VkSampler         scene_sampler_= nullptr;
    VmaAllocation     scene_alloc_  = nullptr;
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

    // Entity picking staging buffer — written to by GPU, read by CPU next frame.
    // Full picking pipeline wired in Phase 7 §6.4.
    bool              picking_requested_   = false;
    uint32_t          pick_pixel_x_        = 0;
    uint32_t          pick_pixel_y_        = 0;
    EntityHandle      hovered_entity_      = kNullEntity;

    // Viewport window size recorded last frame.
    float vp_x_ = 0.f, vp_y_ = 0.f, vp_w_ = 0.f, vp_h_ = 0.f;

    GLFWwindow* window_ = nullptr;  // borrowed from EditorApplication
public:
    void set_window(GLFWwindow* w) noexcept { window_ = w; }
};

}  // namespace gw::editor
