// editor/theme/distressed_draw.cpp — Corrupted Relic chrome via ImDrawList.

#include "editor/theme/distressed_draw.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>

namespace gw::editor::theme {
namespace {

[[nodiscard]] float hash01(std::uint32_t x) noexcept {
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return (x & 0xFFFFFFu) / 16777216.f;
}

} // namespace

void draw_jagged_border(float x0, float y0, float x1, float y1,
                        const DistressedParams& params) noexcept {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (!dl) return;

    const float thick = std::max(1.f, params.crack_thickness_px);
    const ImU32 col   = params.accent_col;

    const int   segments = std::clamp<int>(params.notch_count * 3, 6, 24);
    const float w        = x1 - x0;
    const float h        = y1 - y0;

    auto edge_top = [&](int i) {
        const float t = segments > 1 ? static_cast<float>(i) / static_cast<float>(segments - 1) : 0.f;
        const float x = x0 + w * t;
        const float jig =
            std::sin(t * 6.2831853f * 1.7f) * params.jitter_amplitude_px * 0.5f;
        return ImVec2{x, y0 + jig};
    };
    auto edge_bottom = [&](int i) {
        const float t = segments > 1 ? static_cast<float>(i) / static_cast<float>(segments - 1) : 0.f;
        const float x = x1 - w * t;
        const float jig =
            std::sin(t * 6.2831853f * 2.1f) * params.jitter_amplitude_px * 0.5f;
        return ImVec2{x, y1 - jig};
    };
    auto edge_left = [&](int i) {
        const float t = segments > 1 ? static_cast<float>(i) / static_cast<float>(segments - 1) : 0.f;
        const float y = y1 - h * t;
        const float jig =
            std::sin(t * 6.2831853f * 1.3f) * params.jitter_amplitude_px * 0.5f;
        return ImVec2{x0 + jig, y};
    };
    auto edge_right = [&](int i) {
        const float t = segments > 1 ? static_cast<float>(i) / static_cast<float>(segments - 1) : 0.f;
        const float y = y0 + h * t;
        const float jig =
            std::sin(t * 6.2831853f * 1.9f) * params.jitter_amplitude_px * 0.5f;
        return ImVec2{x1 - jig, y};
    };

    for (int i = 0; i < segments - 1; ++i) {
        dl->AddLine(edge_top(i), edge_top(i + 1), col, thick);
        dl->AddLine(edge_bottom(i), edge_bottom(i + 1), col, thick);
        dl->AddLine(edge_left(i), edge_left(i + 1), col, thick);
        dl->AddLine(edge_right(i), edge_right(i + 1), col, thick);
    }
}

void draw_pixel_jitter_hover(float x0, float y0, float x1, float y1,
                             std::uint32_t imgui_id,
                             const DistressedParams& params) noexcept {
    if (!ImGui::IsItemHovered()) return;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (!dl) return;

    const float jx =
        (hash01(imgui_id * 3u) - 0.5f) * 2.f * params.jitter_amplitude_px;
    const float jy =
        (hash01(imgui_id * 7u) - 0.5f) * 2.f * params.jitter_amplitude_px;
    const ImU32 col = (params.accent_col & 0xFFFFFFu) |
                      (static_cast<std::uint32_t>(90) << 24);
    dl->AddRect(ImVec2{x0 + jx, y0 + jy}, ImVec2{x1 + jx, y1 + jy}, col, 2.f, 0,
                1.5f);
}

void draw_crack_separator(float x0, float y0, float x1, float y1,
                          const DistressedParams& params) noexcept {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (!dl) return;

    const int   N     = 12;
    const ImU32 col   = params.accent_col;
    const float thick = std::max(1.f, params.crack_thickness_px);
    ImVec2 pts[32];
    int    count = 0;
    pts[count++] = ImVec2{x0, y0};
    for (int i = 1; i < N && count < 31; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(N);
        const float x = x0 + (x1 - x0) * t;
        const float y = y0 + (y1 - y0) * t +
                        std::sin(t * 9.5f) * params.jitter_amplitude_px * 1.2f;
        pts[count++] = ImVec2{x, y};
    }
    pts[count++] = ImVec2{x1, y1};
    dl->AddPolyline(pts, count, col, 0, thick);
}

} // namespace gw::editor::theme
