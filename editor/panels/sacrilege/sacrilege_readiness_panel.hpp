#pragma once

#include "editor/panels/panel.hpp"

namespace gw::editor {

class PanelRegistry;

}

namespace gw::editor::panels::sacrilege {

class SacrilegeReadinessPanel final : public gw::editor::IPanel {
public:
    explicit SacrilegeReadinessPanel(gw::editor::PanelRegistry* registry) noexcept
        : registry_{registry} {}

    void on_imgui_render(gw::editor::EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "Sacrilege Readiness"; }

private:
    gw::editor::PanelRegistry* registry_{};
};

} // namespace gw::editor::panels::sacrilege
