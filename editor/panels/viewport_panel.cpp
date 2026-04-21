// editor/panels/viewport_panel.cpp
// Spec ref: Phase 7 §6 — offscreen render target, camera, gizmos, debug draw.
#include "viewport_panel.hpp"
#include "editor/viewport/debug_draw.hpp"
#include "editor/commands/editor_commands.hpp"

#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <ImGuizmo.h>

#define VOLK_IMPLEMENTATION_HEADER_ONLY
#include <volk.h>
#include <vk_mem_alloc.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cstring>

namespace gw::editor {

// Forward decl — device helpers accessed via the HAL public API.
namespace render::hal { class VulkanDevice; }

// ---------------------------------------------------------------------------
ViewportPanel::ViewportPanel(gw::render::hal::VulkanDevice& device)
    : device_(device) {
    // Sensible defaults.
    camera_.state().position = {0.f, 3.f, 8.f};
    camera_.state().target   = {0.f, 0.f, 0.f};
}

ViewportPanel::~ViewportPanel() {
    destroy_scene_image();
}

// ---------------------------------------------------------------------------
// Scene image lifecycle (called on resize).
// Full VMA allocation path — Phase 7 §6.1.
// ---------------------------------------------------------------------------
void ViewportPanel::create_scene_image(uint32_t w, uint32_t h) {
    if (w == 0 || h == 0) return;
    rt_width_  = w;
    rt_height_ = h;

    // Full Vulkan image/view/sampler/descriptor creation happens here.
    // For Phase 7 we provide the correct structure; actual vk calls require
    // the VulkanDevice's raw handles which we access via the public device API.
    // This stub records the dimensions and returns; the real GPU path is
    // wired once EditorApplication can pass the VulkanDevice internal handles.
    //
    // See docs/05 Phase 7 §6.1 for the full protocol.
    // TODO: wire VMA image creation once VulkanDevice exposes allocate_image().
}

void ViewportPanel::destroy_scene_image() {
    // TODO: vmaDestroyImage + destroy view/sampler when VulkanDevice API is wired.
    scene_image_   = nullptr;
    scene_view_    = nullptr;
    scene_sampler_ = nullptr;
    scene_alloc_   = nullptr;
    scene_ds_      = nullptr;
}

void ViewportPanel::apply_resize(uint32_t w, uint32_t h) {
    destroy_scene_image();
    create_scene_image(w, h);
    resize_pending_ = false;
}

// ---------------------------------------------------------------------------
void ViewportPanel::draw_toolbar(EditorContext& ctx) {
    // Play / Pause / Stop buttons.
    float btn_size = 24.f;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {4.f, 2.f});

    bool in_pie = ctx.in_pie;
    if (in_pie) ImGui::BeginDisabled();
    if (ImGui::Button("  Play  ", {0.f, btn_size})) {
        // PIE enter — wired in EditorApplication via callback.
    }
    if (in_pie) ImGui::EndDisabled();

    ImGui::SameLine();
    if (!in_pie) ImGui::BeginDisabled();
    if (ImGui::Button(" Pause ", {0.f, btn_size})) {}
    if (ImGui::Button("  Stop  ", {0.f, btn_size})) {}
    if (!in_pie) ImGui::EndDisabled();

    ImGui::SameLine(0.f, 16.f);

    // Gizmo op selector.
    const char* op_labels[] = {"W Translate", "E Rotate", "R Scale"};
    int cur_op = static_cast<int>(gizmo_.op());
    ImGui::SetNextItemWidth(110.f);
    if (ImGui::Combo("##gizmo_op", &cur_op, op_labels, 3))
        gizmo_.set_op(static_cast<GizmoOp>(cur_op));

    ImGui::SameLine();
    const char* sp_label = (gizmo_.space() == GizmoSpace::World) ? "World" : "Local";
    if (ImGui::Button(sp_label, {60.f, btn_size})) gizmo_.toggle_space();

    ImGui::PopStyleVar();
}

// ---------------------------------------------------------------------------
void ViewportPanel::draw_overlay(EditorContext& ctx) {
    // Camera mode indicator.
    const char* mode_str = "";
    switch (camera_.mode()) {
    case CameraMode::Orbit: mode_str = "Orbit"; break;
    case CameraMode::Fly:   mode_str = "Fly";   break;
    case CameraMode::Ortho: mode_str = "Ortho"; break;
    }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pos{vp_x_ + 8.f, vp_y_ + 8.f};
    dl->AddText(pos, IM_COL32(200, 200, 200, 180), mode_str);

    // FPS counter.
    char fps_buf[32];
    std::snprintf(fps_buf, sizeof(fps_buf), "%.0f FPS",
                  1.f / std::max(ctx.delta_time_s, 0.0001f));
    ImVec2 fps_pos{vp_x_ + vp_w_ - 80.f, vp_y_ + 8.f};
    dl->AddText(fps_pos, IM_COL32(160, 220, 160, 200), fps_buf);
}

// ---------------------------------------------------------------------------
void ViewportPanel::on_imgui_render(EditorContext& ctx) {
    ImGuizmo::SetOrthographic(camera_.mode() == CameraMode::Ortho);
    ImGuizmo::BeginFrame();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.f, 0.f});
    ImGui::Begin(name(), &visible_, ImGuiWindowFlags_NoScrollbar |
                                     ImGuiWindowFlags_NoScrollWithMouse);

    draw_toolbar(ctx);

    // Scene image area.
    ImVec2 content = ImGui::GetContentRegionAvail();
    uint32_t cw = static_cast<uint32_t>(std::max(content.x, 1.f));
    uint32_t ch = static_cast<uint32_t>(std::max(content.y, 1.f));

    if (cw != rt_width_ || ch != rt_height_) {
        pending_w_       = cw;
        pending_h_       = ch;
        resize_pending_  = true;
        // Actual resize deferred to next frame start (main thread, after wait_idle).
    }

    // Record viewport rect for gizmo/overlay.
    ImVec2 vp_min = ImGui::GetCursorScreenPos();
    vp_x_ = vp_min.x; vp_y_ = vp_min.y;
    vp_w_ = static_cast<float>(rt_width_);
    vp_h_ = static_cast<float>(rt_height_);

    // Display scene image (or checkerboard if not yet allocated).
    if (scene_ds_) {
        ImGui::Image(reinterpret_cast<ImTextureID>(scene_ds_),
                     {vp_w_, vp_h_});
    } else {
        // Draw a placeholder until the GPU image is wired.
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled({vp_x_, vp_y_},
                          {vp_x_ + vp_w_, vp_y_ + vp_h_},
                          IM_COL32(20, 22, 30, 255));
        ImGui::Dummy({vp_w_, vp_h_});
    }

    // Camera update.
    bool hovered = ImGui::IsWindowHovered();
    if (window_) {
        // Apply scroll → dolly.
        if (hovered && std::abs(scroll_accumulator_) > 0.001f) {
            camera_.state().orbit_dist =
                std::max(0.05f, camera_.state().orbit_dist -
                         scroll_accumulator_ * camera_.state().orbit_dist * 0.1f);
            scroll_accumulator_ = 0.f;
        }
        camera_.update(ctx.delta_time_s, hovered, window_);
    }

    // Gizmo.
    if (!ctx.selection.empty() && rt_width_ > 0 && rt_height_ > 0) {
        float aspect = vp_w_ / std::max(vp_h_, 1.f);
        glm::mat4 view = camera_.view();
        glm::mat4 proj = camera_.projection(aspect);
        // ImGuizmo needs right-handed, reversed-Z doesn't apply here.
        if (gizmo_.draw(ctx.selection,
                        glm::value_ptr(view), glm::value_ptr(proj),
                        vp_x_, vp_y_, vp_w_, vp_h_)) {
            // Apply delta to selected entities.
            // Phase 7 stub — Phase 8 writes through ECS.
            (void)gizmo_.delta_matrix();
        }
    }

    draw_overlay(ctx);

    // Ground grid via debug draw.
    debug_draw::grid(20.f, 1.f);

    ImGui::End();
    ImGui::PopStyleVar();
}

// ---------------------------------------------------------------------------
void ViewportPanel::on_event(int glfw_key, int /*action*/) {
    gizmo_.on_key(glfw_key);

    // Camera mode shortcuts.
    if (glfw_key == GLFW_KEY_F) {
        if (!camera_.state().position.x && !camera_.state().position.z)
            camera_.frame({0.f, 0.f, 0.f}, 3.f);
        else
            camera_.frame(camera_.state().target, 3.f);
    }
}

}  // namespace gw::editor
