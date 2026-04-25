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
#include <cmath>
#include <unordered_set>

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
std::uint64_t GizmoSystem::pack_cell_index(int ix, int iy, int iz) noexcept {
    const auto u = [](int v) -> std::uint64_t {
        return static_cast<std::uint64_t>(static_cast<std::uint32_t>(v));
    };
    return (u(ix) << 42) | (u(iy) << 21) | u(iz);
}

void GizmoSystem::insert_spatial_bucket(std::uint64_t entity_bits,
                                        const glm::vec3& translation) {
    if (spatial_cell_world_ <= 0.f) {
        return;
    }
    const float inv = 1.f / spatial_cell_world_;
    const int ix = static_cast<int>(std::floor(translation.x * inv));
    const int iy = static_cast<int>(std::floor(translation.y * inv));
    const int iz = static_cast<int>(std::floor(translation.z * inv));
    const std::uint64_t key = pack_cell_index(ix, iy, iz);
    auto& cell = spatial_buckets_[key];
    if (std::find(cell.begin(), cell.end(), entity_bits) == cell.end()) {
        cell.push_back(entity_bits);
    }
}

void GizmoSystem::gather_entities_near(const glm::vec3& world,
                                       float radius_world,
                                       std::vector<EntityHandle>& out) const {
    if (radius_world <= 0.f || spatial_cell_world_ <= 0.f) {
        return;
    }
    const float inv = 1.f / spatial_cell_world_;
    const int ix0 = static_cast<int>(std::floor((world.x - radius_world) * inv));
    const int ix1 = static_cast<int>(std::floor((world.x + radius_world) * inv));
    const int iy0 = static_cast<int>(std::floor((world.y - radius_world) * inv));
    const int iy1 = static_cast<int>(std::floor((world.y + radius_world) * inv));
    const int iz0 = static_cast<int>(std::floor((world.z - radius_world) * inv));
    const int iz1 = static_cast<int>(std::floor((world.z + radius_world) * inv));

    std::unordered_set<std::uint64_t> seen;
    for (int ix = ix0; ix <= ix1; ++ix) {
        for (int iy = iy0; iy <= iy1; ++iy) {
            for (int iz = iz0; iz <= iz1; ++iz) {
                const auto it = spatial_buckets_.find(pack_cell_index(ix, iy, iz));
                if (it == spatial_buckets_.end()) {
                    continue;
                }
                for (const std::uint64_t bits : it->second) {
                    if (seen.insert(bits).second) {
                        out.push_back(EntityHandle::from_raw_bits(bits));
                    }
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
void GizmoSystem::set_entity_matrix(EntityHandle h, const glm::mat4& m) {
    entity_mats_[h.raw_bits()] = m;
    insert_spatial_bucket(h.raw_bits(), glm::vec3{m[3][0], m[3][1], m[3][2]});
}

glm::mat4 GizmoSystem::get_entity_matrix(EntityHandle h) const {
    const auto it = entity_mats_.find(h.raw_bits());
    if (it != entity_mats_.end()) return it->second;
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
