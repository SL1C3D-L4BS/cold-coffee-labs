// editor/panels/lighting_panel.cpp
// Ambient + per-light editor. `active_light` scopes the slider block to a
// single entry of RenderSettings::lighting.lights[]; `light_count` caps the
// subset the renderer iterates.
#include "lighting_panel.hpp"
#include "editor/render/render_settings.hpp"

#include <imgui.h>
#include <algorithm>

namespace gw::editor {

void LightingPanel::on_imgui_render(EditorContext& /*ctx*/) {
    if (!visible_) return;
    ImGui::Begin(name(), &visible_);

    if (!settings_) {
        ImGui::TextDisabled("No RenderSettings bound");
        ImGui::End();
        return;
    }

    auto& L = settings_->lighting;

    ImGui::TextDisabled("Environment");
    ImGui::ColorEdit3("Ambient colour", L.ambient_color,
                       ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_Float);
    ImGui::SliderFloat("Ambient intensity", &L.ambient_intensity, 0.f, 4.f, "%.3f");

    ImGui::Separator();
    ImGui::TextDisabled("Dynamic lights");
    ImGui::SliderInt("Light count", &L.light_count, 0,
                      static_cast<int>(L.lights.size()));
    L.light_count = std::clamp(L.light_count, 0,
                                static_cast<int>(L.lights.size()));

    if (L.light_count > 0) {
        L.active_light = std::clamp(L.active_light, 0, L.light_count - 1);
        ImGui::SliderInt("Light index", &L.active_light, 0, L.light_count - 1);

        auto& li = L.lights[static_cast<std::size_t>(L.active_light)];
        ImGui::Checkbox  ("Enabled##light",           &li.enabled);
        ImGui::ColorEdit3("Colour##light",             li.color,
                           ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_Float);
        ImGui::SliderFloat("Intensity##light",        &li.intensity, 0.f, 10.f, "%.3f");
        ImGui::DragFloat3 ("Position##light",          li.position,  0.05f);
        ImGui::SliderFloat("Range##light",            &li.range,     0.1f, 100.f, "%.2f");
    } else {
        ImGui::TextDisabled("No active lights");
    }

    ImGui::End();
}

}  // namespace gw::editor
