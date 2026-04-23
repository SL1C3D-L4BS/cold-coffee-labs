#pragma once
// editor/panels/sacrilege/pcg_node_graph_panel.hpp — Phase 27 scaffold.

#include "editor/panels/panel.hpp"

namespace gw::editor::panels::sacrilege {

class PcgNodeGraphPanel final : public gw::editor::IPanel {
public:
    void on_imgui_render(gw::editor::EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "PCG Node Graph"; }
};

} // namespace gw::editor::panels::sacrilege
