// editor/panels/audit/localization_panel.cpp — Part C §23 scaffold.

#include "editor/panels/audit/localization_panel.hpp"

namespace gw::editor::panels::audit {

void LocalizationPanel::on_imgui_render(gw::editor::EditorContext& /*ctx*/) {
    // Lives on top of engine/i18n; CSV round-trip via tools/loc.
}

} // namespace gw::editor::panels::audit
