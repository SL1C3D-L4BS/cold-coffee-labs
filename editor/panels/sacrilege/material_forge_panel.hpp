#pragma once
// editor/panels/sacrilege/material_forge_panel.hpp — Thin wrapper; grid lives in ambient_cg_browser.

#include "editor/panels/panel.hpp"
#include "editor/panels/sacrilege/ambient_cg_browser.hpp"

namespace gw::editor::panels::sacrilege {

class MaterialForgePanel final : public gw::editor::IPanel {
public:
    void on_imgui_render(gw::editor::EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "Material Forge"; }

    [[nodiscard]] AmbientCgBrowser& browser() noexcept { return browser_; }
    [[nodiscard]] const AmbientCgBrowser& browser() const noexcept { return browser_; }

private:
    AmbientCgBrowser browser_{};
};

} // namespace gw::editor::panels::sacrilege
