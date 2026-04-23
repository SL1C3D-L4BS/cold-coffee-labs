#pragma once
// editor/panels/sacrilege/dialogue_graph_panel.hpp — Phase 22 scaffold.

#include "editor/panels/panel.hpp"

namespace gw::editor::panels::sacrilege {

class DialogueGraphPanel final : public gw::editor::IPanel {
public:
    void on_imgui_render(gw::editor::EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "Dialogue Graph"; }
};

} // namespace gw::editor::panels::sacrilege
