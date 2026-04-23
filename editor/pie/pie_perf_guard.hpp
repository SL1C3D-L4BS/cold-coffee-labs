#pragma once
// editor/pie/pie_perf_guard.hpp — Part B §12.5 scaffold.
//
// Guards the 144 FPS Tier A floor while in PIE. Raises a non-fatal warning
// toast when the sample budget exceeds the cap.

namespace gw::editor::pie {

struct PiePerfGuardState {
    float frame_ms_ewma   = 0.f;
    float frame_ms_cap    = 6.94f;   // 1000 / 144
    float editor_ms_cap   = 1.0f;    // editor-only overhead
    bool  warning_active  = false;
};

void tick_perf_guard(PiePerfGuardState& state, float frame_ms) noexcept;

} // namespace gw::editor::pie
