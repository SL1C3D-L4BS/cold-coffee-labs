// editor/panels/audit/shader_hotreload_panel.cpp — Part C §23 scaffold.

#include "editor/panels/audit/shader_hotreload_panel.hpp"

#include <imgui.h>

namespace gw::editor::panels::audit {

void ShaderHotReloadPanel::on_imgui_render(gw::editor::EditorContext& /*ctx*/) {
    // Wires into `shaders/add_shader_hot_reload` (GREYWATER_DEVELOPMENT_MODE).
    // Surfaces compile errors and triggers rebuilds.
    ImGui::TextDisabled(
        "Keep .gwmat / shader paths aligned with Sacrilege Library selections; "
        "hot reload pairs with Shader Forge once GREYWATER_DEVELOPMENT_MODE is on.");
}

} // namespace gw::editor::panels::audit
