// editor/panels/viewport_panel.cpp
// Spec ref: Phase 7 §6 — offscreen render target, camera, gizmos, debug draw.
#include "viewport_panel.hpp"
#include "editor/bld_api/editor_bld_api.hpp"
#include "editor/theme/palette_imgui.hpp"
#include "editor/viewport/debug_draw.hpp"
#include "editor/scene/components.hpp"
#include "editor/undo/commands.hpp"
#include "engine/ecs/entity.hpp"
#include "engine/ecs/world.hpp"

#include <imgui.h>
#include <ImGuizmo.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>

namespace gw::editor {

namespace {

glm::vec3 blockout_hit_on_ground_plane(float mouse_x, float mouse_y, float vp_x, float vp_y,
                                       float vp_w, float vp_h, const glm::mat4& view,
                                       const glm::mat4& proj, glm::vec3 fallback_xz) {
    if (vp_w < 1.f || vp_h < 1.f)
        return {fallback_xz.x, 0.f, fallback_xz.z};
    const float ndc_x = (2.f * (mouse_x - vp_x) / vp_w) - 1.f;
    const float ndc_y = 1.f - (2.f * (mouse_y - vp_y) / vp_h);
    const glm::mat4 inv_vp = glm::inverse(proj * view);
    glm::vec4       w0     = inv_vp * glm::vec4(ndc_x, ndc_y, -1.f, 1.f);
    glm::vec4       w1     = inv_vp * glm::vec4(ndc_x, ndc_y, 1.f, 1.f);
    w0 /= w0.w;
    w1 /= w1.w;
    const glm::vec3 ray_o(w0);
    const glm::vec3 ray_d = glm::normalize(glm::vec3(w1 - w0));
    constexpr float kEps = 1e-5f;
    if (std::abs(ray_d.y) < kEps)
        return {fallback_xz.x, 0.f, fallback_xz.z};
    const float t = -ray_o.y / ray_d.y;
    if (t < 0.f)
        return {fallback_xz.x, 0.f, fallback_xz.z};
    return ray_o + ray_d * t;
}

[[nodiscard]] glm::mat4 trs_to_mat4(const scene::TransformComponent& tc) {
    const glm::mat4 t = glm::translate(glm::mat4{1.f}, glm::vec3{tc.position});
    const glm::mat4 r = glm::mat4_cast(tc.rotation);
    const glm::mat4 s = glm::scale(glm::mat4{1.f}, tc.scale);
    return t * r * s;
}

void snap_translation_in_place(glm::vec3& v, float step) {
    if (!(step > 0.f)) return;
    v.x = std::round(v.x / step) * step;
    v.y = std::round(v.y / step) * step;
    v.z = std::round(v.z / step) * step;
}

void apply_pivot_mat_to_transform(const glm::mat4& pivot, scene::TransformComponent& tc,
                                  bool snap, float snap_step) {
    glm::vec3 skew;
    glm::vec4 persp;
    glm::vec3 sc;
    glm::quat rot;
    glm::vec3 trans;
    glm::decompose(pivot, sc, rot, trans, skew, persp);
    if (snap)
        snap_translation_in_place(trans, snap_step);
    tc.position = glm::dvec3{trans};
    tc.rotation = rot;
    tc.scale    = sc;
}

[[nodiscard]] undo::TransformSnapshot snapshot_from_tc(const scene::TransformComponent& tc) {
    return {glm::vec3{tc.position}, tc.rotation, tc.scale};
}

[[nodiscard]] bool snapshots_differ(const undo::TransformSnapshot& a,
                                    const undo::TransformSnapshot& b) {
    return a.translation != b.translation || a.rotation != b.rotation || a.scale != b.scale;
}

} // namespace

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

    if (pie_transport_fail_flash_ > 0)
        --pie_transport_fail_flash_;

    bool in_pie = ctx.in_pie;
    if (in_pie) ImGui::BeginDisabled();
    if (ImGui::Button("  Play  ", {0.f, btn_size})) {
        if (!gw_editor_enter_play())
            pie_transport_fail_flash_ = 120;
    }
    if (in_pie) ImGui::EndDisabled();

    ImGui::SameLine();
    if (!in_pie) ImGui::BeginDisabled();
    const char* pause_label = ctx.pie_paused ? "Resume" : " Pause ";
    if (ImGui::Button(pause_label, {0.f, btn_size})) {
        if (ctx.pie_pause_toggle)
            ctx.pie_pause_toggle(ctx.pie_pause_user_data);
    }
    if (ImGui::Button("  Stop  ", {0.f, btn_size})) {
        (void)gw_editor_stop();
    }
    if (!in_pie) ImGui::EndDisabled();

    if (pie_transport_fail_flash_ > 0) {
        ImGui::TextColored(ImVec4{1.f, 0.4f, 0.35f, 1.f},
            "Could not enter Play (snapshot or gameplay module). See stderr.");
    }

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

    ImGui::SameLine(0.f, 16.f);
    ImGui::TextDisabled("Blockout");
    ImGui::SameLine();
    const char* bt_labels[] = {"Place", "Move", "Rotate", "Scale"};
    int bt = blockout_tool_;
    ImGui::SetNextItemWidth(140.f);
    if (ImGui::Combo("##blockout_tool", &bt, bt_labels, 4))
        blockout_tool_ = bt;
    if (blockout_tool_ == 1) gizmo_.set_op(GizmoOp::Translate);
    else if (blockout_tool_ == 2) gizmo_.set_op(GizmoOp::Rotate);
    else if (blockout_tool_ == 3) gizmo_.set_op(GizmoOp::Scale);

    ImGui::SameLine(0.f, 12.f);
    const char* sh_labels[] = {"Box", "Cylinder", "Plane", "Ramp"};
    int sh = blockout_shape_;
    ImGui::SetNextItemWidth(100.f);
    if (ImGui::Combo("##prim", &sh, sh_labels, 4))
        blockout_shape_ = sh;

    ImGui::SameLine(0.f, 12.f);
    ImGui::Checkbox("Snap", &snap_grid_enabled_);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(72.f);
    ImGui::DragFloat("##snap_step", &snap_grid_step_, 0.05f, 0.125f, 8.f, "%.2f");

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

    ImVec2 grid_pos{vp_x_ + 8.f, vp_y_ + vp_h_ - 22.f};
    char grid_line[160];
    std::snprintf(grid_line, sizeof(grid_line),
                  "Grid 1u  %s  Blockout = designer layer; Blacklake/GPTM mesh is separate "
                  "(regen must not wipe blockout unless opted-in). Vulkan mesh draw: Phase 8.",
                  snap_grid_enabled_ ? "snap on" : "snap off");
    dl->AddText(grid_pos, color32_to_im_u32(pal.text_muted, 160.f / 255.f), grid_line);
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

    const bool rt_hovered = ImGui::IsItemHovered();

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

    if (!ctx.in_pie && blockout_tool_ == 0 && rt_hovered && ctx.world &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGuizmo::IsUsing()) {
        ImVec2      mouse  = ImGui::GetMousePos();
        const float aspect = vp_w_ / std::max(vp_h_, 1.f);
        const glm::mat4 view = camera_.view();
        const glm::mat4 proj = camera_.projection(aspect);
        glm::vec3 hit =
            blockout_hit_on_ground_plane(mouse.x, mouse.y, vp_x_, vp_y_, vp_w_, vp_h_, view,
                                         proj, camera_.target());
        if (snap_grid_enabled_)
            snap_translation_in_place(hit, snap_grid_step_);
        ++blockout_place_seq_;
        char name_buf[72];
        std::snprintf(name_buf, sizeof(name_buf), "Blockout %d", blockout_place_seq_);
        std::string      name_copy(name_buf);
        gw::ecs::World*  world = ctx.world;
        const glm::dvec3 pos{static_cast<double>(hit.x), static_cast<double>(hit.y),
                             static_cast<double>(hit.z)};
        const auto shape_u8 = static_cast<std::uint8_t>(std::clamp(blockout_shape_, 0, 3));
        ctx.cmd_stack.push(std::make_unique<undo::CreateEntityCommand>(
            std::move(name_copy),
            [world, pos, shape_u8](const char* nm) -> EntityHandle {
                if (!world) return kNullEntity;
                auto e = world->create_entity();
                world->add_component(e, scene::NameComponent{nm});
                world->add_component(e, scene::TransformComponent{
                    pos, glm::quat{1.f, 0.f, 0.f, 0.f}, glm::vec3{1.f, 1.f, 1.f}});
                world->add_component(e, scene::VisibilityComponent{});
                scene::BlockoutPrimitiveComponent bp{};
                bp.shape = shape_u8;
                world->add_component(e, bp);
                return e;
            },
            [world](EntityHandle h) {
                if (world) world->destroy_entity(h);
            }));
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

    // Gizmo — feed world TRS, apply pivot back to ECS (single selection), undo on drag end.
    if (!ctx.selection.empty() && rt_width_ > 0 && rt_height_ > 0 && ctx.world) {
        float aspect = vp_w_ / std::max(vp_h_, 1.f);
        glm::mat4 view = camera_.view();
        glm::mat4 proj = camera_.projection(aspect);
        const bool ortho = (camera_.mode() == CameraMode::Ortho);

        gizmo_.clear_entity_matrices();
        for (const auto h : ctx.selection.selected()) {
            if (const auto* tc = ctx.world->get_component<scene::TransformComponent>(h))
                gizmo_.set_entity_matrix(h, trs_to_mat4(*tc));
        }

        const bool manipulated = gizmo_.draw(ctx.selection,
                                               glm::value_ptr(view), glm::value_ptr(proj),
                                               vp_x_, vp_y_, vp_w_, vp_h_, ortho);
        const bool using_now = gizmo_.is_using();

        if (ctx.selection.count() == 1) {
            const EntityHandle gh = ctx.selection.primary();
            auto* gtc = ctx.world->get_component<scene::TransformComponent>(gh);
            if (gtc) {
                if (!gizmo_was_using_ && using_now) {
                    gizmo_undo_before_ = snapshot_from_tc(*gtc);
                    gizmo_undo_entity_ = gh;
                    gizmo_undo_armed_  = true;
                }
                if (manipulated) {
                    apply_pivot_mat_to_transform(gizmo_.pivot_matrix(), *gtc,
                                                 snap_grid_enabled_, snap_grid_step_);
                }
                if (gizmo_was_using_ && !using_now && gizmo_undo_armed_ &&
                    gizmo_undo_entity_ == gh) {
                    const undo::TransformSnapshot after = snapshot_from_tc(*gtc);
                    if (snapshots_differ(gizmo_undo_before_, after)) {
                        gw::ecs::World* w = ctx.world;
                        undo::TransformSnapshot before = gizmo_undo_before_;
                        ctx.cmd_stack.push(std::make_unique<undo::SetEntityTransformCommand>(
                            gh, std::move(before), std::move(after),
                            [w](EntityHandle eh, const undo::TransformSnapshot& s) {
                                if (!w) return;
                                auto* t = w->get_component<scene::TransformComponent>(eh);
                                if (!t) return;
                                t->position = glm::dvec3{s.translation};
                                t->rotation = s.rotation;
                                t->scale    = s.scale;
                            }));
                    }
                    gizmo_undo_armed_  = false;
                    gizmo_undo_entity_ = kNullEntity;
                }
            }
        } else if (!using_now && gizmo_undo_armed_) {
            gizmo_undo_armed_  = false;
            gizmo_undo_entity_ = kNullEntity;
        }
        gizmo_was_using_ = using_now;
    }

    draw_overlay(ctx);

    // Debug-draw overlay (Phase 7 §8): ground grid + origin axis + a sphere
    // marker on each selected entity. Additional primitives come from any
    // panel/system that called debug_draw::line/sphere/... this frame.
    debug_draw::grid(20.f, 1.f);
    debug_draw::axis(glm::mat4{1.f}, 1.0f);
    if (ctx.world) {
        // Wire AABB for every named transform so the default Cube/Sphere show
        // up without a selection (Vulkan mesh pass is still future Phase 8).
        ctx.world->for_each<scene::NameComponent, scene::TransformComponent, scene::VisibilityComponent>(
            [w = ctx.world](gw::ecs::Entity e, const scene::NameComponent& nm,
                            const scene::TransformComponent& tc, const scene::VisibilityComponent&) {
                if (w && w->get_component<scene::BlockoutPrimitiveComponent>(e)) return;
                const glm::vec3 c{static_cast<float>(tc.position.x),
                                  static_cast<float>(tc.position.y),
                                  static_cast<float>(tc.position.z)};
                const glm::vec3 h = tc.scale * 0.5f;
                std::uint32_t col = 0xFF5AC8E8; // default wire
                if (std::strcmp(nm.c_str(), "Cube") == 0) col = 0xFF55CC88;
                else if (std::strcmp(nm.c_str(), "Sphere") == 0) col = 0xFFCC88EE;
                else if (std::strcmp(nm.c_str(), "Scene Root") == 0) col = 0xFFAAAAAA;
                debug_draw::aabb(c - h, c + h, col);
            });
        ctx.world->for_each<scene::NameComponent, scene::TransformComponent, scene::VisibilityComponent,
                            scene::BlockoutPrimitiveComponent>(
            [](gw::ecs::Entity /*e*/, const scene::NameComponent& /*nm*/,
               const scene::TransformComponent& tc, const scene::VisibilityComponent&,
               const scene::BlockoutPrimitiveComponent&) {
                const glm::vec3 c{static_cast<float>(tc.position.x),
                                  static_cast<float>(tc.position.y),
                                  static_cast<float>(tc.position.z)};
                const glm::vec3 h = tc.scale * 0.5f;
                debug_draw::aabb(c - h, c + h, 0xFFFFAA44);
            });
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
