#pragma once
// editor/panels/audit/shader_hotreload_panel.hpp — Part C §23 scaffold.

#include "editor/panels/panel.hpp"

namespace gw::editor::panels::audit {

class ShaderHotReloadPanel final : public gw::editor::IPanel {
public:
    void on_imgui_render(gw::editor::EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "Shader Hot-Reload"; }
};

} // namespace gw::editor::panels::audit
