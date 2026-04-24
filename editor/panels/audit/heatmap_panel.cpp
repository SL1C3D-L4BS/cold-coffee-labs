// editor/panels/audit/heatmap_panel.cpp — Part C §23 scaffold.

#include "editor/panels/audit/heatmap_panel.hpp"

#include "engine/services/director/director_service.hpp"
#include "engine/services/director/schema/director.hpp"

#include <imgui.h>

namespace gw::editor::panels::audit {

void HeatmapPanel::on_imgui_render(gw::editor::EditorContext& ctx) {
    (void)ctx;
    if (!visible_) return;
    ImGui::Begin(name(), &visible_);

    ImGui::TextDisabled("Encounter / heat → director (plan week 159)");
    static float heat_intensity = 0.5f;
    ImGui::SliderFloat("Heat (stub)", &heat_intensity, 0.f, 1.f);
    if (ImGui::Button("Evaluate director (BLD-tier scaffold)")) {
        gw::services::director::DirectorRequest dr{};
        dr.seed         = 0xE001u;
        dr.logical_tick = 1;
        dr.normalized_sin = heat_intensity;
        dr.current_state  = gw::services::director::IntensityState::BuildUp;
        (void)gw::services::director::evaluate(dr);
    }
    ImGui::End();
}

} // namespace gw::editor::panels::audit
