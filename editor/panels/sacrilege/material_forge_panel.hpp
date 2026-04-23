#pragma once
// editor/panels/sacrilege/material_forge_panel.hpp — Phase 28 scaffold.

#include "editor/panels/panel.hpp"

namespace gw::editor::panels::sacrilege {

class MaterialForgePanel final : public gw::editor::IPanel {
public:
    void on_imgui_render(gw::editor::EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "Material Forge"; }
};

} // namespace gw::editor::panels::sacrilege
