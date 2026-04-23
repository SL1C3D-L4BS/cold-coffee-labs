// editor/panels/viewport_panel.cpp
// Spec ref: Phase 7 §6 — offscreen render target, camera, gizmos, debug draw.
#include "viewport_panel.hpp"
#include "editor/theme/palette_imgui.hpp"
#include "editor/viewport/debug_draw.hpp"
#include "editor/scene/components.hpp"
#include "editor/undo/commands.hpp"
#include "engine/ecs/world.hpp"

#include <imgui.h>
#include <ImGuizmo.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cstdio>

namespace gw::editor {

// ---------------------------------------------------------------------------
ViewportPanel::~ViewportPanel() = default;

// ---------------------------------------------------------------------------
// apply_resize — flag the EditorApplication to rebuild the RT. The panel only
// records the intended dimensions; EditorApplication owns the GPU lifecycle.
// ---------------------------------------------------------------------------
void ViewportPanel::apply_resize(uint32_t w, uint32_t h) {
    rt_width_       = w;
    rt_height_      = h;
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
    const auto& pal = gw::editor::theme::ThemeRegistry::instance().active().palette;
    using gw::editor::theme::color32_to_im_u32;
    ImVec2 pos{vp_x_ + 8.f, vp_y_ + 8.f};
    dl->AddText(pos, color32_to_im_u32(pal.text, 180.f / 255.f), mode_str);

    // FPS counter (matches stats panel — positive / live metric).
    char fps_buf[32];
    std::snprintf(fps_buf, sizeof(fps_buf), "%.0f FPS",
                  1.f / std::max(ctx.delta_time_s, 0.0001f));
    ImVec2 fps_pos{vp_x_ + vp_w_ - 80.f, vp_y_ + 8.f};
    dl->AddText(fps_pos, color32_to_im_u32(pal.positive, 200.f / 255.f), fps_buf);
}

// ---------------------------------------------------------------------------
void ViewportPanel::on_imgui_render(EditorContext& ctx) {
    // NOTE: ImGuizmo::BeginFrame() is called once per frame in
    // EditorApplication::begin_frame(), next to ImGui::NewFrame().
    // Calling it twice resets hover/ID state between docked viewports.
    // SetOrthographic is configured below inside GizmoSystem::draw via the
    // ortho flag threaded through draw(), so this panel no longer sets it here.

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
        const ImVec4 border = ImGui::GetStyleColorVec4(ImGuiCol_Border);
        ImGui::Image(reinterpret_cast<ImTextureID>(scene_ds_), {vp_w_, vp_h_},
                     ImVec2{0.f, 0.f}, ImVec2{1.f, 1.f},
                     ImVec4{1.f, 1.f, 1.f, 1.f}, border);
    } else {
        const auto& pal = gw::editor::theme::ThemeRegistry::instance().active().palette;
        using gw::editor::theme::color32_to_im_u32;
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled({vp_x_, vp_y_},
                          {vp_x_ + vp_w_, vp_y_ + vp_h_},
                          color32_to_im_u32(pal.panel));
        ImGui::Dummy({vp_w_, vp_h_});
    }

    // Drop target for Asset Browser → Viewport drag-to-scene.
    // Creates an entity at the origin with standard components; the actual
    // mesh/texture resolve lands once the asset database is wired.
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload =
                ImGui::AcceptDragDropPayload("ASSET_PATH")) {
            const char* raw = static_cast<const char*>(payload->Data);
            std::string asset_path{raw, raw + payload->DataSize};
            if (!asset_path.empty() && asset_path.back() == '\0')
                asset_path.pop_back();

            std::string leaf = asset_path;
            if (auto slash = leaf.find_last_of("/\\"); slash != std::string::npos)
                leaf = leaf.substr(slash + 1);

            auto* world = ctx.world;
            ctx.cmd_stack.push(std::make_unique<undo::CreateEntityCommand>(
                std::move(leaf),
                [world, path = asset_path](const char* nm) -> EntityHandle {
                    if (!world) return kNullEntity;
                    auto e = world->create_entity();
                    world->add_component(e, scene::NameComponent{nm});
                    world->add_component(e, scene::TransformComponent{});
                    world->add_component(e, scene::VisibilityComponent{});
                    (void)path;  // asset wiring lands in Phase 8
                    return e;
                },
                [world](EntityHandle h) {
                    if (world) world->destroy_entity(h);
                }));
        }
        ImGui::EndDragDropTarget();
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
        const bool ortho = (camera_.mode() == CameraMode::Ortho);
        if (gizmo_.draw(ctx.selection,
                        glm::value_ptr(view), glm::value_ptr(proj),
                        vp_x_, vp_y_, vp_w_, vp_h_, ortho)) {
            // Apply delta to selected entities.
            // Phase 7 stub — Phase 8 writes through ECS.
            (void)gizmo_.delta_matrix();
        }
    }

    draw_overlay(ctx);

    // Debug-draw overlay (Phase 7 §8): ground grid + origin axis + a sphere
    // marker on each selected entity. Additional primitives come from any
    // panel/system that called debug_draw::line/sphere/... this frame.
    debug_draw::grid(20.f, 1.f);
    debug_draw::axis(glm::mat4{1.f}, 1.0f);
    if (ctx.world) {
        for (auto e : ctx.selection.selected()) {
            if (const auto* tc = ctx.world->get_component<scene::TransformComponent>(e)) {
                // `position` is float64 (floating-origin); debug lines operate in
                // camera-relative float32 — the cast is safe because the gizmo
                // and overlay both render near the camera, well within float32
                // precision (CLAUDE.md non-negotiable #17).
                debug_draw::sphere(glm::vec3(tc->position), 0.25f, 0xFF80FFFF, 12);
            }
        }
    }

    // Flush all accumulated lines into the viewport's draw list.
    if (rt_width_ > 0 && rt_height_ > 0) {
        const float aspect = vp_w_ / std::max(vp_h_, 1.f);
        const glm::mat4 view = camera_.view();
        const glm::mat4 proj = camera_.projection(aspect);
        debug_draw::flush_to_imgui(view, proj, vp_x_, vp_y_, vp_w_, vp_h_);
    } else {
        debug_draw::clear();
    }

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
