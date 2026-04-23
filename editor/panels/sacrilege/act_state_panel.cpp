// editor/panels/sacrilege/act_state_panel.cpp — Phase 22 W148.

#include "editor/panels/sacrilege/act_state_panel.hpp"

#if __has_include(<imgui.h>)
#  include <imgui.h>
#  define GW_HAS_IMGUI 1
#else
#  define GW_HAS_IMGUI 0
#endif

namespace gw::editor::panels::sacrilege {

#if GW_HAS_IMGUI
namespace {

const char* act_label(gw::narrative::Act a) noexcept {
    switch (a) {
        case gw::narrative::Act::I:    return "I — Rite of Unspeaking";
        case gw::narrative::Act::II:   return "II — Blasphemy Circuit";
        case gw::narrative::Act::III:  return "III — Unmaking";
        case gw::narrative::Act::None: return "None";
    }
    return "?";
}

} // namespace
#endif // GW_HAS_IMGUI

void ActStatePanel::on_imgui_render(gw::editor::EditorContext& /*ctx*/) {
#if GW_HAS_IMGUI
    if (!visible_) return;
    ImGui::Begin(name(), &visible_);
    ImGui::Text("Act:             %s", act_label(state_.current_act));
    ImGui::Text("Phase in act:    %u", static_cast<unsigned>(state_.phase_in_act));
    ImGui::Text("Time in phase:   %.2f s", state_.time_in_phase_s);
    ImGui::Text("God Mode ever:   %s", state_.god_mode_ever_triggered ? "YES" : "no");
    ImGui::Text("Witness seen:    %s", state_.witness_seen ? "YES" : "no");
    ImGui::Separator();
    ImGui::TextUnformatted("Pending transition");
    ImGui::Checkbox("first_rapture_survived", &pending_.first_rapture_survived);
    ImGui::Checkbox("colosseum_cleared",      &pending_.colosseum_cleared);
    ImGui::Checkbox("logos_phase_entered",    &pending_.logos_phase_entered);
    ImGui::Checkbox("grace_threshold_met",    &pending_.grace_threshold_met);
    ImGui::End();
#else
    (void)state_; (void)pending_;
#endif
}

} // namespace gw::editor::panels::sacrilege
