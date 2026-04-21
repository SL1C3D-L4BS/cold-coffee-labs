// editor/viewport/debug_draw.cpp
// ~200-line immediate-mode debug draw accumulator.
// Spec ref: Phase 7 §8.
#include "debug_draw.hpp"

#include <imgui.h>

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

// ---------------------------------------------------------------------------
// ImGui flush — projects accumulated world-space segments to screen and
// emits them as foreground lines on the current ImGui window's draw list.
// Segments are clipped against the near plane (view-space z >= -epsilon) so
// lines straddling the camera don't snap. Segments fully behind or fully
// outside the viewport rect are dropped. The buffer is cleared after flush.
// ---------------------------------------------------------------------------
namespace {

// Project a view-space point to screen; returns false when z >= near clip.
bool project_view(const glm::vec4& v_view,
                  const glm::mat4& proj,
                  float vp_x, float vp_y, float vp_w, float vp_h,
                  ImVec2& out) {
    constexpr float kNearEps = -1e-4f;
    if (v_view.z >= kNearEps) return false;  // behind near plane
    glm::vec4 clip = proj * v_view;
    if (clip.w <= 0.f) return false;
    glm::vec3 ndc{clip.x / clip.w, clip.y / clip.w, clip.z / clip.w};
    // Vulkan-style NDC: x in [-1,1], y in [-1,1] (flipped for screen).
    out.x = vp_x + (ndc.x * 0.5f + 0.5f) * vp_w;
    out.y = vp_y + (1.f - (ndc.y * 0.5f + 0.5f)) * vp_h;
    return true;
}

}  // namespace

void flush_to_imgui(const glm::mat4& view,
                    const glm::mat4& proj,
                    float vp_x, float vp_y, float vp_w, float vp_h) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (!dl || vp_w <= 0.f || vp_h <= 0.f) {
        g_vertices.clear();
        return;
    }

    const ImVec2 clip_min{vp_x, vp_y};
    const ImVec2 clip_max{vp_x + vp_w, vp_y + vp_h};
    dl->PushClipRect(clip_min, clip_max, true);

    const size_t n = g_vertices.size() & ~size_t{1};
    for (size_t i = 0; i + 1 < n; i += 2) {
        const LineVertex& a = g_vertices[i];
        const LineVertex& b = g_vertices[i + 1];

        glm::vec4 va = view * glm::vec4{a.x, a.y, a.z, 1.f};
        glm::vec4 vb = view * glm::vec4{b.x, b.y, b.z, 1.f};

        // Near-plane clip in view space (camera looks down -Z).
        constexpr float kNear = -1e-3f;
        bool a_behind = va.z > kNear;
        bool b_behind = vb.z > kNear;
        if (a_behind && b_behind) continue;
        if (a_behind != b_behind) {
            // Clip the behind endpoint to the near plane.
            float t = (kNear - va.z) / (vb.z - va.z);
            glm::vec4 hit = va + (vb - va) * t;
            if (a_behind) va = hit; else vb = hit;
        }

        ImVec2 sa, sb;
        if (!project_view(va, proj, vp_x, vp_y, vp_w, vp_h, sa)) continue;
        if (!project_view(vb, proj, vp_x, vp_y, vp_w, vp_h, sb)) continue;

        dl->AddLine(sa, sb, a.rgba, 1.0f);
    }

    dl->PopClipRect();
    g_vertices.clear();
}

}  // namespace gw::editor::debug_draw
