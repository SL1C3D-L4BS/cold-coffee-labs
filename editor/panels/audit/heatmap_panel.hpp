#pragma once
// editor/panels/audit/heatmap_panel.hpp — Director policy preview (Wave 1 audit).

#include "editor/panels/panel.hpp"

namespace gw::editor::panels::audit {

class HeatmapPanel final : public gw::editor::IPanel {
public:
    void on_imgui_render(gw::editor::EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "Heatmap"; }
};

} // namespace gw::editor::panels::audit
