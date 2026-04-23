// editor/panels/audit/heatmap_panel.cpp — Part C §23 scaffold.

#include "editor/panels/audit/heatmap_panel.hpp"

namespace gw::editor::panels::audit {

void HeatmapPanel::on_imgui_render(gw::editor::EditorContext& /*ctx*/) {
    // Records position / damage / deaths from PIE or headless replay; overlays
    // on the viewport. Wires to bld-replay gameplay_schema traces.
}

} // namespace gw::editor::panels::audit
