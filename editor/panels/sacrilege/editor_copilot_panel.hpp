#pragma once
// editor/panels/sacrilege/editor_copilot_panel.hpp — Phase 23 scaffold.
// Merges with the existing agent_panel/ — this panel adds the structured
// tool catalogue UI on top.

#include "editor/panels/panel.hpp"

namespace gw::editor::panels::sacrilege {

class EditorCopilotPanel final : public gw::editor::IPanel {
public:
    void on_imgui_render(gw::editor::EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "Editor Copilot"; }
};

} // namespace gw::editor::panels::sacrilege
