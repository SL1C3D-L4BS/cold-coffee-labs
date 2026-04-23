#pragma once
// editor/panels/sacrilege/act_state_panel.hpp — Phase 22 W148.

#include "editor/panels/panel.hpp"
#include "engine/narrative/act_state.hpp"

namespace gw::editor::panels::sacrilege {

class ActStatePanel final : public gw::editor::IPanel {
public:
    void on_imgui_render(gw::editor::EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "Act / Phase"; }

    void set_state(const gw::narrative::ActState& s) noexcept { state_ = s; }
    [[nodiscard]] const gw::narrative::ActState& state() const noexcept { return state_; }

    void set_pending_transition(const gw::narrative::ActTransitionInput& t) noexcept { pending_ = t; }
    [[nodiscard]] const gw::narrative::ActTransitionInput& pending_transition() const noexcept { return pending_; }

private:
    gw::narrative::ActState             state_{};
    gw::narrative::ActTransitionInput   pending_{};
};

} // namespace gw::editor::panels::sacrilege
