// editor/panels/audit/heatmap_panel.cpp — Wave 1: director encounter pressure preview.

#include "editor/panels/audit/heatmap_panel.hpp"

#include "engine/services/director/director_service.hpp"
#include "engine/services/director/schema/director.hpp"

#include <imgui.h>

namespace gw::editor::panels::audit {

void HeatmapPanel::on_imgui_render(gw::editor::EditorContext& ctx) {
    (void)ctx;
    if (!visible_) {
        return;
    }
    ImGui::Begin(name(), &visible_);

    ImGui::TextUnformatted(
        "Encounter pressure → director policy preview. Values feed a live "
        "`gw::services::director::evaluate` call so designers can feel spawn cadence "
        "before shipping a Blacklake slice.");
    static float normalized_sin_driver = 0.5f;
    ImGui::SliderFloat("Normalized sin driver", &normalized_sin_driver, 0.f, 1.f);
    if (ImGui::Button("Evaluate director policy")) {
        gw::services::director::DirectorRequest dr{};
        dr.seed           = 0xE001u;
        dr.logical_tick   = 1;
        dr.normalized_sin = normalized_sin_driver;
        dr.current_state  = gw::services::director::IntensityState::BuildUp;
        (void)gw::services::director::evaluate(dr);
    }
    ImGui::End();
}

} // namespace gw::editor::panels::audit
