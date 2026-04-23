// editor/panels/audit/bake_panel.cpp — Part C §23 scaffold.

#include "editor/panels/audit/bake_panel.hpp"

namespace gw::editor::panels::audit {

void BakePanel::on_imgui_render(gw::editor::EditorContext& /*ctx*/) {
    // Long-running cook jobs with live progress + cancel. Wraps the existing
    // tools/cook job API.
}

} // namespace gw::editor::panels::audit
