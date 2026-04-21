// editor/viewport/gizmo_system.cpp
#include "gizmo_system.hpp"

// imgui.h MUST precede ImGuizmo.h — ImGuizmo depends on ImDrawList, ImVec2, etc.
#include <imgui.h>
#include <ImGuizmo.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <numeric>

namespace gw::editor {

namespace {
ImGuizmo::OPERATION to_imguizmo_op(GizmoOp op) noexcept {
    switch (op) {
    case GizmoOp::Translate: return ImGuizmo::TRANSLATE;
    case GizmoOp::Rotate:    return ImGuizmo::ROTATE;
    case GizmoOp::Scale:     return ImGuizmo::SCALE;
    }
    return ImGuizmo::TRANSLATE;
}

ImGuizmo::MODE to_imguizmo_mode(GizmoSpace sp) noexcept {
    return (sp == GizmoSpace::World) ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
}
}  // namespace

// ---------------------------------------------------------------------------
void GizmoSystem::set_entity_matrix(EntityHandle h, const glm::mat4& m) {
    for (auto& e : entity_mats_) {
        if (e.handle == h) { e.mat = m; return; }
    }
    entity_mats_.push_back({h, m});
}

glm::mat4 GizmoSystem::get_entity_matrix(EntityHandle h) const {
    for (const auto& e : entity_mats_)
        if (e.handle == h) return e.mat;
    return glm::mat4{1.f};
}

// ---------------------------------------------------------------------------
bool GizmoSystem::draw(const SelectionContext& sel,
                       const float* view_mat,
                       const float* proj_mat,
                       float vx, float vy, float vw, float vh,
                       bool ortho)
{
    if (sel.empty()) { using_gizmo_ = false; return false; }

    ImGuizmo::SetOrthographic(ortho);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(vx, vy, vw, vh);

    // Compute pivot matrix from selection.
    if (sel.count() == 1) {
        pivot_mat_ = get_entity_matrix(sel.primary());
    } else {
        // Centroid pivot — average translation.
        glm::vec3 sum{0.f};
        for (auto h : sel.selected())
            sum += glm::vec3{get_entity_matrix(h)[3]};
        sum /= static_cast<float>(sel.count());
        pivot_mat_ = glm::translate(glm::mat4{1.f}, sum);
    }

    float mat_copy[16];
    std::copy_n(glm::value_ptr(pivot_mat_), 16, mat_copy);

    bool manipulated = ImGuizmo::Manipulate(
        view_mat, proj_mat,
        to_imguizmo_op(current_op_),
        to_imguizmo_mode(current_space_),
        mat_copy,
        glm::value_ptr(delta_));

    bool currently_using = ImGuizmo::IsUsing();

    if (manipulated) {
        pivot_mat_ = glm::make_mat4(mat_copy);
    }

    // Detect begin/end of drag for transaction bracketing.
    // The caller (ViewportPanel) reads is_using() to open/close the tx.
    using_gizmo_ = currently_using;

    return manipulated;
}

// ---------------------------------------------------------------------------
void GizmoSystem::on_key(int key) noexcept {
    switch (key) {
    case GLFW_KEY_W: current_op_ = GizmoOp::Translate; break;
    case GLFW_KEY_E: current_op_ = GizmoOp::Rotate;    break;
    case GLFW_KEY_R: current_op_ = GizmoOp::Scale;     break;
    case GLFW_KEY_SPACE: toggle_space();                 break;
    default: break;
    }
}

}  // namespace gw::editor
