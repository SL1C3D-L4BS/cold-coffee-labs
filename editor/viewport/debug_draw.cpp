// editor/viewport/debug_draw.cpp
// ~200-line immediate-mode debug draw accumulator.
// Spec ref: Phase 7 §8.
#include "debug_draw.hpp"

#include <cmath>
#include <numbers>

namespace gw::editor::debug_draw {

namespace {
    std::vector<LineVertex> g_vertices;
}

const std::vector<LineVertex>& get_pending() noexcept { return g_vertices; }
void clear() noexcept { g_vertices.clear(); }

// ---------------------------------------------------------------------------
static void push(glm::vec3 p, uint32_t rgba) {
    g_vertices.push_back({p.x, p.y, p.z, rgba});
}

// ---------------------------------------------------------------------------
void line(glm::vec3 from, glm::vec3 to, uint32_t rgba) {
    push(from, rgba);
    push(to,   rgba);
}

// ---------------------------------------------------------------------------
void aabb(glm::vec3 mn, glm::vec3 mx, uint32_t rgba) {
    // 12 edges of a box.
    // Bottom face
    line({mn.x, mn.y, mn.z}, {mx.x, mn.y, mn.z}, rgba);
    line({mx.x, mn.y, mn.z}, {mx.x, mn.y, mx.z}, rgba);
    line({mx.x, mn.y, mx.z}, {mn.x, mn.y, mx.z}, rgba);
    line({mn.x, mn.y, mx.z}, {mn.x, mn.y, mn.z}, rgba);
    // Top face
    line({mn.x, mx.y, mn.z}, {mx.x, mx.y, mn.z}, rgba);
    line({mx.x, mx.y, mn.z}, {mx.x, mx.y, mx.z}, rgba);
    line({mx.x, mx.y, mx.z}, {mn.x, mx.y, mx.z}, rgba);
    line({mn.x, mx.y, mx.z}, {mn.x, mx.y, mn.z}, rgba);
    // Vertical edges
    line({mn.x, mn.y, mn.z}, {mn.x, mx.y, mn.z}, rgba);
    line({mx.x, mn.y, mn.z}, {mx.x, mx.y, mn.z}, rgba);
    line({mx.x, mn.y, mx.z}, {mx.x, mx.y, mx.z}, rgba);
    line({mn.x, mn.y, mx.z}, {mn.x, mx.y, mx.z}, rgba);
}

// ---------------------------------------------------------------------------
void sphere(glm::vec3 center, float r, uint32_t rgba, int subs) {
    float step = 2.f * std::numbers::pi_v<float> / static_cast<float>(subs);
    for (int i = 0; i < subs; ++i) {
        float a0 = step * static_cast<float>(i);
        float a1 = step * static_cast<float>(i + 1);
        float c0 = std::cos(a0), s0 = std::sin(a0);
        float c1 = std::cos(a1), s1 = std::sin(a1);
        // XY ring
        line(center + glm::vec3{c0 * r, s0 * r, 0.f},
             center + glm::vec3{c1 * r, s1 * r, 0.f}, rgba);
        // XZ ring
        line(center + glm::vec3{c0 * r, 0.f, s0 * r},
             center + glm::vec3{c1 * r, 0.f, s1 * r}, rgba);
        // YZ ring
        line(center + glm::vec3{0.f, c0 * r, s0 * r},
             center + glm::vec3{0.f, c1 * r, s1 * r}, rgba);
    }
}

// ---------------------------------------------------------------------------
void axis(const glm::mat4& m, float len) {
    glm::vec3 origin{m[3]};
    glm::vec3 ax{m[0]}, ay{m[1]}, az{m[2]};
    line(origin, origin + ax * len, 0xFF0000FF); // X = red
    line(origin, origin + ay * len, 0xFF00FF00); // Y = green
    line(origin, origin + az * len, 0xFFFF0000); // Z = blue
}

// ---------------------------------------------------------------------------
void frustum(const glm::mat4& inv, uint32_t rgba) {
    // 8 NDC corners → world space.
    static const glm::vec4 kNDC[8] = {
        {-1,-1,-1,1},{1,-1,-1,1},{1,1,-1,1},{-1,1,-1,1},
        {-1,-1, 1,1},{1,-1, 1,1},{1,1, 1,1},{-1,1, 1,1},
    };
    glm::vec3 ws[8];
    for (int i = 0; i < 8; ++i) {
        glm::vec4 v = inv * kNDC[i];
        ws[i] = glm::vec3{v} / v.w;
    }
    // Near quad
    line(ws[0],ws[1],rgba); line(ws[1],ws[2],rgba);
    line(ws[2],ws[3],rgba); line(ws[3],ws[0],rgba);
    // Far quad
    line(ws[4],ws[5],rgba); line(ws[5],ws[6],rgba);
    line(ws[6],ws[7],rgba); line(ws[7],ws[4],rgba);
    // Connecting edges
    for (int i = 0; i < 4; ++i) line(ws[i], ws[i + 4], rgba);
}

// ---------------------------------------------------------------------------
void grid(float size, float step, uint32_t rgba) {
    for (float x = -size; x <= size + 0.001f; x += step) {
        // Slightly brighter on axis lines.
        uint32_t col = (std::abs(x) < 0.001f) ? 0xFF606060 : rgba;
        line({x, 0.f, -size}, {x, 0.f, size}, col);
    }
    for (float z = -size; z <= size + 0.001f; z += step) {
        uint32_t col = (std::abs(z) < 0.001f) ? 0xFF606060 : rgba;
        line({-size, 0.f, z}, {size, 0.f, z}, col);
    }
}

}  // namespace gw::editor::debug_draw
