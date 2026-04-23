#pragma once
// editor/panels/audit/asset_deps_panel.hpp — Part C §23 scaffold.

#include "editor/panels/panel.hpp"

namespace gw::editor::panels::audit {

class AssetDepsPanel final : public gw::editor::IPanel {
public:
    void on_imgui_render(gw::editor::EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "Asset Dependencies"; }
};

} // namespace gw::editor::panels::audit
