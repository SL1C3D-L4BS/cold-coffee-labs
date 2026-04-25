#pragma once
// editor/viewport/gizmo_system.hpp
// ImGuizmo wrapper — translate/rotate/scale gizmo with multi-select support.
// Spec ref: Phase 7 §7.

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "editor/selection/selection_context.hpp"
#include "editor/viewport/editor_camera.hpp"

#include <cstdint>
#include <unordered_map>

namespace gw::editor {

enum class GizmoOp    { Translate, Rotate, Scale };
enum class GizmoSpace { World, Local };

// GizmoSystem draws the ImGuizmo overlay inside the viewport ImGui window.
// It coordinates multi-entity pivot, transaction bracketing, and undo push.
class GizmoSystem {
public:
    GizmoSystem() = default;

    // Call inside the viewport BeginChild/EndChild block.
    // `view_mat` and `proj_mat` are column-major floats (glm layout).
    // `ortho` must match the projection actually in use (ImGuizmo requires
    //   SetOrthographic() to match the camera or handles are wrong).
    // Returns true if a transform was applied this frame (triggers a command).
    [[nodiscard]] bool draw(const SelectionContext& sel,
                            const float* view_mat,
                            const float* proj_mat,
                            float viewport_x, float viewport_y,
                            float viewport_w, float viewport_h,
                            bool ortho = false);

    // Query / set the current operation.
    void set_op(GizmoOp op)       noexcept { current_op_    = op; }
    void set_space(GizmoSpace sp) noexcept { current_space_ = sp; }
    [[nodiscard]] GizmoOp    op()    const noexcept { return current_op_; }
    [[nodiscard]] GizmoSpace space() const noexcept { return current_space_; }

    // Toggle world/local (Space bar hotkey from viewport).
    void toggle_space() noexcept {
        current_space_ = (current_space_ == GizmoSpace::World)
            ? GizmoSpace::Local : GizmoSpace::World;
    }

    // Call from viewport panel's on_event().
    void on_key(int glfw_key) noexcept;

    // True while the user is dragging (transaction is open).
    [[nodiscard]] bool is_using() const noexcept { return using_gizmo_; }

    // Entity → world-space transform accessors (set by the viewport panel
    // each frame before calling draw).
    void set_entity_matrix(EntityHandle h, const glm::mat4& m);
    [[nodiscard]] glm::mat4 get_entity_matrix(EntityHandle h) const;

    // Delta applied this frame (only valid when draw() returns true).
    [[nodiscard]] const glm::mat4& delta_matrix() const noexcept { return delta_; }

    /// Clears cached entity → matrix map (call each frame before set_entity_matrix).
    void clear_entity_matrices() noexcept {
        entity_mats_.clear();
        entity_mats_.reserve(256);
    }

    /// World matrix ImGuizmo manipulated last (single-selection pivot).
    [[nodiscard]] const glm::mat4& pivot_matrix() const noexcept { return pivot_mat_; }

private:
    GizmoOp    current_op_    = GizmoOp::Translate;
    GizmoSpace current_space_ = GizmoSpace::World;
    bool       using_gizmo_   = false;
    [[maybe_unused]] bool tx_open_ = false; // command transaction open? (undo wiring)

    glm::mat4  pivot_mat_ = glm::mat4{1.f};
    glm::mat4  delta_     = glm::mat4{1.f};

    // Generational entity id → world matrix (O(1) lookup for multi-select pivots).
    // Wave 1C: selection is small (≪256); `unordered_map` is fine. A `FlatMap` or
    // sorted `std::vector` is only worth it if profiling shows this as hot.
    std::unordered_map<std::uint64_t, glm::mat4> entity_mats_;
};

}  // namespace gw::editor
