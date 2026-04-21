#pragma once
// editor/viewport/debug_draw.hpp
// Immediate-mode debug line/shape drawing for the editor viewport.
// Spec ref: Phase 7 §8 — ~200 line roll-in, no external library.

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

namespace gw::editor::debug_draw {

// ---------------------------------------------------------------------------
// API — callable from any panel's on_imgui_render().
// All state is accumulated for the current frame and flushed by flush().
// ---------------------------------------------------------------------------

// Line between two world-space points.
void line(glm::vec3 from, glm::vec3 to, uint32_t rgba = 0xFFFFFFFF);

// Axis-aligned bounding box.
void aabb(glm::vec3 min, glm::vec3 max, uint32_t rgba = 0xFF00FF00);

// Circle approximation of a sphere (3 cross-section rings).
void sphere(glm::vec3 center, float radius, uint32_t rgba = 0xFFFF0000,
            int subdivisions = 16);

// Three coloured axes from a world transform.
void axis(const glm::mat4& world_transform, float length = 0.5f);

// Frustum from inverse proj-view matrix.
void frustum(const glm::mat4& proj_view_inv, uint32_t rgba = 0xFFFFFFFF);

// XZ ground plane grid.
void grid(float size = 20.f, float step = 1.f, uint32_t rgba = 0xFF404040);

// ---------------------------------------------------------------------------
// Internal: called by the ViewportPanel renderer.
// ---------------------------------------------------------------------------
struct LineVertex {
    float    x, y, z;
    uint32_t rgba;
};

// Returns all pending vertices and clears the buffer.
[[nodiscard]] const std::vector<LineVertex>& get_pending() noexcept;
void clear() noexcept;

}  // namespace gw::editor::debug_draw
