// editor/pie/pie_perf_guard.cpp — Part B §12.5 scaffold.

#include "editor/pie/pie_perf_guard.hpp"

namespace gw::editor::pie {

void tick_perf_guard(PiePerfGuardState& state, float frame_ms) noexcept {
    const float alpha = 0.05f;
    state.frame_ms_ewma = state.frame_ms_ewma * (1.f - alpha) + frame_ms * alpha;
    state.warning_active = state.frame_ms_ewma > state.frame_ms_cap;
}

} // namespace gw::editor::pie
