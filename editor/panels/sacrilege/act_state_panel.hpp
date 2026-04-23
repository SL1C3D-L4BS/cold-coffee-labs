#pragma once
// editor/panels/sacrilege/act_state_panel.hpp — Phase 21 scaffold.

#include "editor/panels/panel.hpp"

namespace gw::editor::panels::sacrilege {

class ActStatePanel final : public gw::editor::IPanel {
public:
    void on_imgui_render(gw::editor::EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "Act / Phase"; }
};

} // namespace gw::editor::panels::sacrilege
