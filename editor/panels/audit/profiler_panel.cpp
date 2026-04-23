// editor/panels/audit/profiler_panel.cpp — Part C §23 scaffold.

#include "editor/panels/audit/profiler_panel.hpp"

namespace gw::editor::panels::audit {

void ProfilerPanel::on_imgui_render(gw::editor::EditorContext& /*ctx*/) {
    // Tracy-compatible capture panel. Reads from the existing Tracy client
    // instrumentation mentioned in docs/10.
}

} // namespace gw::editor::panels::audit
