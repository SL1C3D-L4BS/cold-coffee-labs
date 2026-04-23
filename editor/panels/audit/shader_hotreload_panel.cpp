// editor/panels/audit/shader_hotreload_panel.cpp — Part C §23 scaffold.

#include "editor/panels/audit/shader_hotreload_panel.hpp"

namespace gw::editor::panels::audit {

void ShaderHotReloadPanel::on_imgui_render(gw::editor::EditorContext& /*ctx*/) {
    // Wires into `shaders/add_shader_hot_reload` (GREYWATER_DEVELOPMENT_MODE).
    // Surfaces compile errors and triggers rebuilds.
}

} // namespace gw::editor::panels::audit
