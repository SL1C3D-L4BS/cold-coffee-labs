#pragma once
// editor/theme/corruption_pulse.hpp — Part B §12.1 scaffold.
//
// Fullscreen vignette pulse that visualises destructive commands and is
// modulated by Sin-signature during PIE. Single extra ImGui draw-list pass,
// ≤ 0.05 ms on RX 580. Gated by `ThemeEffectFlags::EF_Vignette`.

namespace gw::editor::theme {

struct CorruptionPulseState {
    float amplitude       = 0.f;   // 0..1 current pulse
    float target          = 0.f;   // 0..1 target pulse
    float decay_per_sec   = 1.5f;  // exponential decay rate
    float sin_signature   = 0.f;   // 0..1 live value during PIE (read-only)
};

void tick_pulse(CorruptionPulseState& s, float dt_sec) noexcept;

/// Add the fullscreen vignette overlay to the current ImGui foreground draw
/// list. Called by `EditorApplication::end_frame` after all panels rendered.
void draw_pulse_overlay(const CorruptionPulseState& s) noexcept;

/// Raise the pulse target (e.g. on destructive command, delete confirmation,
/// Sin-signature spike during PIE). Amplitude then decays back to zero.
void trigger_pulse(CorruptionPulseState& s, float magnitude) noexcept;

} // namespace gw::editor::theme
