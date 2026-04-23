#pragma once
// editor/panels/sacrilege/circle_editor_panel.hpp — Phase 21 scaffold.
// Nine-Circle live tuning + Location Skin selector.

#include "editor/panels/panel.hpp"

namespace gw::editor::panels::sacrilege {

class CircleEditorPanel final : public gw::editor::IPanel {
public:
    void on_imgui_render(gw::editor::EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "Circle Editor"; }
};

} // namespace gw::editor::panels::sacrilege
