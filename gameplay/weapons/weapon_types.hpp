#pragma once
// gameplay/weapons/weapon_types.hpp — Phase 23 W152

#include <cstdint>

namespace gw::gameplay::weapons {

enum class WeaponId : std::uint8_t {
    HaloScythe      = 0,
    Dogma           = 1,
    HereticsLament  = 2,
    Confessor       = 3,
    Schism          = 4,
    Cathedral       = 5,
    Count           = 6,
};

enum class FireMode : std::uint8_t {
    Melee      = 0,
    Hitscan    = 1,
    Projectile = 2,
};

enum class WeaponAllegiance : std::uint8_t {
    Profane = 0,
    Sacred  = 1,
};

struct WeaponShotParams {
    float base_damage        = 10.f;
    float sin_on_hit         = 0.f;
    float sin_on_kill        = 0.f;
    float mantra_on_hit      = 0.f;
    float mantra_on_kill     = 0.f;
    float profane_weapon_mult_applies = 1.f; // scales Sin accrual path
};

struct WeaponRuntimeState {
    std::uint16_t ammo_in_mag   = 0;
    std::uint16_t mag_capacity  = 30;
    float         alt_cooldown_s = 0.f;
    float         mantra_reserve = 0.f; // cached from MantraComponent for alts
};

struct FireResult {
    float damage_out      = 0.f;
    float sin_delta       = 0.f;
    float mantra_delta    = 0.f;
    bool  consumed_mantra_alt = false;
    float audio_bus_gain_suggest = 1.f; // SFX_Weapons layer
};

} // namespace gw::gameplay::weapons
