// editor/panels/sacrilege/ai_director_sandbox_panel.cpp — Phase 23 / 26

#include "editor/panels/sacrilege/ai_director_sandbox_panel.hpp"

#include "engine/services/director/director_service.hpp"

#if __has_include(<imgui.h>)
#  include <imgui.h>
#  define GW_HAS_IMGUI 1
#else
#  define GW_HAS_IMGUI 0
#endif

namespace gw::editor::panels::sacrilege {

void AiDirectorSandboxPanel::on_imgui_render(gw::editor::EditorContext& /*ctx*/) {
#if GW_HAS_IMGUI
    static gw::services::director::DirectorRequest req{};
    static gw::services::director::DirectorResult  last{};
    static bool                                    evaluated = false;

    ImGui::TextUnformatted("Read-only policy preview — matches apps/sandbox_director (ADR-0107).");
    ImGui::SliderFloat("player_health_ratio", &req.player_health_ratio, 0.f, 1.f);
    ImGui::SliderFloat("normalized_sin", &req.normalized_sin, 0.f, 1.f);
    ImGui::SliderFloat("normalized_grace", &req.normalized_grace, 0.f, 1.f);
    ImGui::SliderFloat("intensity_ewma", &req.intensity_ewma, 0.f, 1.f);
    ImGui::SliderFloat("time_in_state_s", &req.time_in_state_s, 0.f, 60.f);

    int st = static_cast<int>(req.current_state);
    if (ImGui::SliderInt("intensity_state (0-3)", &st, 0, 3)) {
        req.current_state = static_cast<gw::services::director::IntensityState>(st);
    }

    if (ImGui::Button("Evaluate policy")) {
        last      = gw::services::director::evaluate(req);
        evaluated = true;
    }
    if (evaluated) {
        ImGui::Separator();
        ImGui::Text("spawn_rate_scale: %.3f", static_cast<double>(last.spawn_rate_scale));
        ImGui::Text("item_drop_scale:  %.3f", static_cast<double>(last.item_drop_scale));
        ImGui::Text("intensity_target: %.3f", static_cast<double>(last.intensity_target));
        ImGui::ProgressBar(static_cast<float>(last.intensity_target), ImVec2(0, 0), "");
    }
#else
    (void)0;
#endif
}

} // namespace gw::editor::panels::sacrilege
