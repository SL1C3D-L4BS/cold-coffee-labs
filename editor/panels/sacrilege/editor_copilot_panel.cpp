// editor/panels/sacrilege/editor_copilot_panel.cpp — Phase 23 W154

#include "editor/panels/sacrilege/editor_copilot_panel.hpp"

#if __has_include(<imgui.h>)
#  include <imgui.h>
#  define GW_HAS_IMGUI 1
#else
#  define GW_HAS_IMGUI 0
#endif

namespace gw::editor::panels::sacrilege {

void EditorCopilotPanel::on_imgui_render(gw::editor::EditorContext& /*ctx*/) {
#if GW_HAS_IMGUI
    ImGui::TextUnformatted("BLD tool catalogue (HITL approval required for mutate-tier actions).");
    ImGui::Separator();
    ImGui::BulletText("Suggest encounter — spawn heatmap + narrative beats");
    ImGui::BulletText("Detect exploits — geometry / line-of-sight probes");
    ImGui::BulletText("Scene heatmap — Sin density / Mantra flow");
    ImGui::BulletText("Voice lines — reachability on .gwdlg");
    ImGui::BulletText("Concept to material — Material Forge dispatch");
    static bool hitl = true;
    ImGui::Checkbox("Require human approval before apply", &hitl);
    (void)hitl;
#else
    (void)0;
#endif
}

} // namespace gw::editor::panels::sacrilege
