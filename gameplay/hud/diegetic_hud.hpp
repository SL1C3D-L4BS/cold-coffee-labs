#pragma once
// gameplay/hud/diegetic_hud.hpp — Phase 21 W140 (ADR-0108).
//
// Diegetic HUD — player-state readouts rendered *inside the world* as
// iconography on the weapon, stigmata on the character mesh, blood in the
// vignette, etc. Phase 21 ships a deterministic data model; the RmlUi/render
// binding sites are in engine/render/hud_binding.* (Phase 23).
//
// The design intent (docs/07 §5) is:
//
//   • No bars, no numbers, no floating icons.
//   • Health — pallor of the weapon and vignette blood. 0..1 where 1 == full.
//   • Armor — frost/grime on the weapon. 0..1 where 1 == full armour.
//   • Sin   — tendril vignette driven by post stack (Phase 21 W136).
//   • Mantra — halo shimmer on the weapon. 0..1 where 1 == full Mantra.
//   • Rapture — whiteout bloom ramp; Phase 21 W136.
//   • Ruin  — desaturation ramp; Phase 21 W136.
//   • Grace — golden rim-light on the weapon.
//   • Ammo  — chamber LEDs on the weapon (per-weapon UV atlas slot).
//
// Everything is derivable from the ECS state so the HUD is deterministic
// and replay-safe.

#include <cstdint>

namespace gw::gameplay::hud {

// Lightweight input record — built from ECS components each frame.
struct DiegeticHudInputs {
    float         health_norm{1.0f};   // 0..1
    float         armor_norm{0.0f};    // 0..1
    float         sin_norm{0.0f};      // 0..1  (SinComponent::current / 100)
    float         mantra_norm{0.0f};   // 0..1  (MantraComponent::current / 100)
    float         rapture_norm{0.0f};  // 0..1  (0 outside Rapture; ramps 0→1→0)
    float         ruin_norm{0.0f};     // 0..1
    float         grace_norm{0.0f};    // 0..1
    std::uint16_t ammo_current{0u};
    std::uint16_t ammo_capacity{0u};
    std::uint8_t  weapon_slot{0u};     // 0..7
};

// HUD output model consumed by the RmlUi / render binding.
struct DiegeticHudModel {
    float weapon_pallor_01{0.0f};       // health blend
    float weapon_frost_01{0.0f};        // armor blend
    float weapon_halo_01{0.0f};         // mantra shimmer
    float weapon_rim_gold_01{0.0f};     // grace rim
    float vignette_blood_01{0.0f};      // pallor → blood vignette
    float vignette_whiteout_01{0.0f};   // rapture whiteout ramp
    float vignette_desat_01{0.0f};      // ruin desaturation ramp
    float sin_tendril_01{0.0f};         // drives sin_tendril.frag
    float chamber_fill_01{0.0f};        // ammo / capacity
    std::uint8_t weapon_slot{0u};
};

// Deterministic data transform — no randomness, no time sampling.
DiegeticHudModel build_hud_model(const DiegeticHudInputs& in) noexcept;

// Diegetic HUD never shows numerical text. This helper is exposed for
// accessibility overlays (Part C §24) — returns true when Large Print /
// Numeric HUD is enabled, the a11y system may fall back to a text panel.
[[nodiscard]] bool needs_a11y_fallback(const DiegeticHudInputs& in,
                                       bool                     large_print_enabled) noexcept;

} // namespace gw::gameplay::hud
