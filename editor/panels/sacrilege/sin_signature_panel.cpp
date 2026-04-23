// editor/panels/sacrilege/sin_signature_panel.cpp — Phase 22 W148.

#include "editor/panels/sacrilege/sin_signature_panel.hpp"

#if __has_include(<imgui.h>)
#  include <imgui.h>
#  define GW_HAS_IMGUI 1
#else
#  define GW_HAS_IMGUI 0
#endif

namespace gw::editor::panels::sacrilege {

void SinSignaturePanel::on_imgui_render(gw::editor::EditorContext& /*ctx*/) {
#if GW_HAS_IMGUI
    if (!visible_) return;
    ImGui::Begin(name(), &visible_);
    ImGui::TextUnformatted("Live signature");
    ImGui::Separator();
    ImGui::Text("God Mode uptime: %.2f", live_.god_mode_uptime_ratio);
    ImGui::Text("Precision:       %.2f", live_.precision_ratio);
    ImGui::Text("Cruelty:         %.2f", live_.cruelty_ratio);
    ImGui::Text("Hitless streak:  %u",   static_cast<unsigned>(live_.hitless_streak));
    ImGui::Text("Deaths/area avg: %.2f", live_.deaths_per_area_avg);
    ImGui::Separator();
    ImGui::TextUnformatted("Reaction thresholds");
    ImGui::SliderFloat("cruelty→Malakor",   &thresholds_.cruelty_to_malakor,    0.f, 1.f);
    ImGui::SliderFloat("precision→Niccolo", &thresholds_.precision_to_niccolo,  0.f, 1.f);
    ImGui::SliderFloat("god-mode warning",  &thresholds_.god_mode_uptime_warning, 0.f, 1.f);
    ImGui::SliderFloat("deaths warning",    &thresholds_.deaths_per_area_warning, 0.f, 10.f);
    ImGui::End();
#else
    (void)live_; (void)thresholds_;
#endif
}

} // namespace gw::editor::panels::sacrilege
