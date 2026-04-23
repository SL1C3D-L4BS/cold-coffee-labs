// editor/theme/corruption_pulse.cpp

#include "editor/theme/corruption_pulse.hpp"
#include "editor/theme/theme_registry.hpp"

#include <imgui.h>

#include <algorithm>

namespace gw::editor::theme {

void tick_pulse(CorruptionPulseState& s, float dt_sec) noexcept {
    const float step = s.decay_per_sec * dt_sec;
    if (s.amplitude < s.target) {
        s.amplitude = std::min(s.amplitude + step * 4.f, s.target);
    } else {
        s.amplitude = std::max(s.amplitude - step, 0.f);
    }
    s.target = std::max(s.target - step * 0.5f, 0.f);
}

void trigger_pulse(CorruptionPulseState& s, float magnitude) noexcept {
    s.target = std::clamp(s.target + magnitude, 0.f, 1.f);
}

void draw_pulse_overlay(const CorruptionPulseState& s) noexcept {
    if (s.amplitude <= 0.001f) return;
    const ThemeEffects& fx = ThemeRegistry::instance().active().effects;
    if (!fx.has(EF_Vignette)) return;

    ImGuiViewport* vp = ImGui::GetMainViewport();
    if (!vp) return;

    const ImVec2 a = vp->Pos;
    const ImVec2 b{a.x + vp->Size.x, a.y + vp->Size.y};
    ImDrawList*    dl = ImGui::GetForegroundDrawList();
    if (!dl) return;

    const int edge_alpha =
        static_cast<int>(200.f * std::clamp(s.amplitude + s.sin_signature * 0.15f, 0.f, 1.f));
    const int mid_alpha = static_cast<int>(edge_alpha * 0.25f);

    dl->AddRectFilledMultiColor(
        a, b, IM_COL32(0, 0, 0, edge_alpha), IM_COL32(0, 0, 0, mid_alpha),
        IM_COL32(0, 0, 0, mid_alpha), IM_COL32(0, 0, 0, edge_alpha));
}

} // namespace gw::editor::theme
