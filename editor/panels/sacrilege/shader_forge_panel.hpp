#pragma once
// editor/panels/sacrilege/shader_forge_panel.hpp — Shader Forge (ImGui shell; graph wiring per ADR-0108).

#include "editor/panels/panel.hpp"

namespace gw::editor::panels::sacrilege {

class ShaderForgePanel final : public gw::editor::IPanel {
public:
    void on_imgui_render(gw::editor::EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "Shader Forge"; }
};

} // namespace gw::editor::panels::sacrilege
