// editor/panels/sacrilege/circle_editor_panel.cpp — Phase 21 W140 (ADR-0108).

#include "editor/panels/sacrilege/circle_editor_panel.hpp"

#include <cstdlib>

#if __has_include(<imgui.h>)
#  include <imgui.h>
#  define GW_HAS_IMGUI 1
#else
#  define GW_HAS_IMGUI 0
#endif

namespace gw::editor::panels::sacrilege {

namespace {

const char* circle_label(gw::world::CircleId id) noexcept {
    switch (id) {
        case gw::world::CircleId::Violence:  return "Violence — Vatican Necropolis";
        case gw::world::CircleId::Heresy:    return "Heresy — Colosseum of Echoes";
        case gw::world::CircleId::Lust:      return "Lust — Sewer of Grace";
        case gw::world::CircleId::Gluttony:  return "Gluttony — Hell's Factory";
        case gw::world::CircleId::Greed:     return "Greed — Rome Above";
        case gw::world::CircleId::Wrath:     return "Wrath — Palazzo of the Unspoken";
        case gw::world::CircleId::Pride:     return "Pride — Open Arena";
        case gw::world::CircleId::Treachery: return "Treachery — Heaven's Back Door";
        case gw::world::CircleId::Fraud:     return "Fraud — Silentium";
        default: break;
    }
    return "Unknown";
}

} // namespace

void CircleEditorPanel::on_imgui_render(gw::editor::EditorContext& /*ctx*/) {
#if GW_HAS_IMGUI
    if (!visible_) {
        return;
    }
    ImGui::Begin(name(), &visible_);

    if (const char* seed = std::getenv("GW_HELL_SEED"); seed && seed[0] != '\0')
        ImGui::TextDisabled("Hell Seed (GW_HELL_SEED): %s", seed);
    else
        ImGui::TextDisabled("Hell Seed: (set GW_HELL_SEED for deterministic Blacklake preview)");

    const auto& profiles = gw::world::circle_profiles();

    // Circle picker.
    if (ImGui::BeginCombo("Circle", circle_label(selected_))) {
        for (std::size_t i = 0; i < profiles.size(); ++i) {
            const auto id = static_cast<gw::world::CircleId>(i);
            const bool selected = (id == selected_);
            if (ImGui::Selectable(circle_label(id), selected)) {
                selected_ = id;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    const auto& profile = profiles[static_cast<std::size_t>(selected_)];
    const auto& skin    = profile.location_skin;

    ImGui::Separator();
    ImGui::TextUnformatted("Location Skin");
    ImGui::BulletText("Name: %.*s", static_cast<int>(skin.name.size()), skin.name.data());
    ImGui::BulletText("Skybox: %.*s",
                      static_cast<int>(skin.skybox_ref.size()), skin.skybox_ref.data());
    ImGui::BulletText("Audio bed: %.*s",
                      static_cast<int>(skin.ambient_audio_bed.size()),
                      skin.ambient_audio_bed.data());

    ImGui::Separator();
    ImGui::TextUnformatted("Fog");
    float fog[3] = {profile.fog_params.color_rgb[0], profile.fog_params.color_rgb[1],
                    profile.fog_params.color_rgb[2]};
    ImGui::ColorEdit3("Fog tint (preview)", fog, ImGuiColorEditFlags_NoInputs);
    ImGui::Text("Density %.4f  Height falloff %.4f", profile.fog_params.density,
                profile.fog_params.height_falloff);

    ImGui::Separator();
    ImGui::TextUnformatted("Horror / Circle post-stack baselines");
    ImGui::Text("CA=%.2f  Grain=%.2f  Shake=%.2f",
                skin.horror_chromatic_aberration, skin.horror_film_grain,
                skin.horror_screen_shake);
    ImGui::Text("SinTendril=%.2f  RuinDesat=%.2f  RaptureWhiteout=%.2f",
                skin.sin_tendril_baseline, skin.ruin_desat_baseline,
                skin.rapture_whiteout_baseline);

    ImGui::End();
#else
    (void)selected_;
#endif
}

} // namespace gw::editor::panels::sacrilege
