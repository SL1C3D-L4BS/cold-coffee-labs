// gameplay/hud/diegetic_hud.cpp — Phase 21 W140 (ADR-0108).

#include "gameplay/hud/diegetic_hud.hpp"

#include <algorithm>

namespace gw::gameplay::hud {

namespace {

constexpr float clamp01(float v) noexcept {
    return std::clamp(v, 0.0f, 1.0f);
}

} // namespace

DiegeticHudModel build_hud_model(const DiegeticHudInputs& in) noexcept {
    DiegeticHudModel m{};
    const float health01 = clamp01(in.health_norm);
    const float armor01  = clamp01(in.armor_norm);
    const float mantra01 = clamp01(in.mantra_norm);
    const float grace01  = clamp01(in.grace_norm);
    const float rap01    = clamp01(in.rapture_norm);
    const float ruin01   = clamp01(in.ruin_norm);

    // Health drains pallor into the weapon + pushes blood into the vignette.
    m.weapon_pallor_01   = 1.0f - health01;
    m.vignette_blood_01  = (1.0f - health01) * (1.0f - health01);

    // Armor shows as frost / grime build-up on the weapon.
    m.weapon_frost_01    = armor01;

    // Mantra halo — shimmer ramps cubically to favour high Mantra states.
    m.weapon_halo_01     = mantra01 * mantra01 * mantra01;

    // Grace rim — golden rim light fades in at higher Grace.
    m.weapon_rim_gold_01 = grace01;

    // Rapture whiteout — punchy; Ruin desaturation — sustained.
    m.vignette_whiteout_01 = rap01;
    m.vignette_desat_01    = ruin01;

    // Sin tendril — ties to the post-processing tendril vignette.
    m.sin_tendril_01     = clamp01(in.sin_norm);

    // Chamber LEDs proportional to ammo/capacity (clamped to [0, 1]).
    if (in.ammo_capacity > 0u) {
        m.chamber_fill_01 = clamp01(static_cast<float>(in.ammo_current) /
                                     static_cast<float>(in.ammo_capacity));
    }
    m.weapon_slot = in.weapon_slot;
    return m;
}

bool needs_a11y_fallback(const DiegeticHudInputs& in,
                          bool                     large_print_enabled) noexcept {
    // A11y always wins when Large Print / Numeric HUD is requested. In
    // addition, when the player is in Rapture or Ruin the diegetic post
    // effects deliberately suppress visual detail — fall back so the text
    // panel remains readable.
    if (large_print_enabled) {
        return true;
    }
    return in.rapture_norm >= 0.75f || in.ruin_norm >= 0.75f;
}

} // namespace gw::gameplay::hud
