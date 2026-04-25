#pragma once
// editor/panels/sacrilege/pcg_node_graph_panel.hpp — Blacklake PCG graph (Wave 1).

#include "editor/panels/panel.hpp"

namespace gw::editor::panels::sacrilege {

class PcgNodeGraphPanel final : public gw::editor::IPanel {
public:
    PcgNodeGraphPanel();
    ~PcgNodeGraphPanel() override;

    void on_imgui_render(gw::editor::EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "PCG Node Graph"; }

private:
    void* imnodes_ctx_ = nullptr;
};

} // namespace gw::editor::panels::sacrilege
