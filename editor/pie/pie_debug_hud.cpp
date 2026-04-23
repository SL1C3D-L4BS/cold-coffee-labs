// editor/pie/pie_debug_hud.cpp — Part B §12.5 scaffold.

#include "editor/pie/pie_debug_hud.hpp"

namespace gw::editor::pie {

void draw_pie_debug_hud(PieDebugHudState& /*state*/) noexcept {
    // Phase 22: ImGui overlay windows for each flag. CI perf gate verifies
    // total editor overlay time stays under 1 ms on RX 580.
}

} // namespace gw::editor::pie
