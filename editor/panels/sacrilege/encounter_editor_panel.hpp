#pragma once
// editor/panels/sacrilege/encounter_editor_panel.hpp — Phase 22 scaffold.

#include "editor/panels/panel.hpp"

namespace gw::editor::panels::sacrilege {

class EncounterEditorPanel final : public gw::editor::IPanel {
public:
    void on_imgui_render(gw::editor::EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "Encounter Editor"; }
};

} // namespace gw::editor::panels::sacrilege
