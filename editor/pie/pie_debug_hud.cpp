// editor/pie/pie_debug_hud.cpp — Wave 1: PIE telemetry overlay (ADR-0106).

#include "editor/pie/pie_debug_hud.hpp"

#include <imgui.h>

namespace gw::editor::pie {

void draw_pie_debug_hud(PieDebugHudState& state) noexcept {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + 12.f, vp->WorkPos.y + 32.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.82f);
    if (ImGui::Begin("PIE Debug", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav)) {
        const char* phase = state.pie_phase == 0 ? "Playing" : "Paused";
        ImGui::Text("Phase: %s", phase);
        ImGui::Text("dt: %.4f s", static_cast<double>(state.dt_last_s));
        ImGui::Text("ECS entities: %u", state.entity_count);
        ImGui::Text("Gameplay DLL: %s", state.gameplay_dll_loaded ? "loaded" : "optional / absent");
        ImGui::Separator();
        ImGui::TextUnformatted("Overlay toggles");
        ImGui::Checkbox("Martyrdom meters", &state.flags.martyrdom_meters);
        ImGui::Checkbox("God mode state", &state.flags.god_mode_state);
        ImGui::Checkbox("Grace meter", &state.flags.grace_meter);
        ImGui::Checkbox("Director decisions", &state.flags.director_decisions);
        ImGui::Checkbox("Sin signature", &state.flags.sin_signature);
        ImGui::Checkbox("Frame timing", &state.flags.frame_timing);
        ImGui::Checkbox("ECS archetypes", &state.flags.ecs_archetypes);
        ImGui::Separator();
        ImGui::Text("Overlay budget cap: %.2f ms", static_cast<double>(state.budget_ms_cap));
        ImGui::TextDisabled("Measured editor-only overlay cost is sampled in pie_perf_guard.");
    }
    ImGui::End();
}

} // namespace gw::editor::pie
