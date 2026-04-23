// editor/theme/corruption_pulse.cpp — Part B §12.1 scaffold.

#include "editor/theme/corruption_pulse.hpp"

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

void draw_pulse_overlay(const CorruptionPulseState& /*s*/) noexcept {
    // Scaffold: the fullscreen ImGui overlay lands in Phase 21. A real
    // implementation would call `ImGui::GetForegroundDrawList()->AddRectFilled(...)`
    // with a radial alpha ramp keyed on `s.amplitude`.
}

} // namespace gw::editor::theme
