// gameplay/weapons/weapon_system.cpp — Phase 23 W152

#include "gameplay/weapons/weapon_system.hpp"
#include "gameplay/weapons/weapon_catalog.hpp"

#include <algorithm>

namespace gw::gameplay::weapons {

void init_default_runtime(WeaponId id, WeaponRuntimeState& rt) noexcept {
    rt.alt_cooldown_s = 0.f;
    rt.mantra_reserve = 100.f;
    switch (id) {
    case WeaponId::Dogma:
        rt.mag_capacity  = 48;
        rt.ammo_in_mag   = 48;
        break;
    case WeaponId::HereticsLament:
        rt.mag_capacity = 8;
        rt.ammo_in_mag  = 8;
        break;
    case WeaponId::Confessor:
        rt.mag_capacity = 16;
        rt.ammo_in_mag  = 16;
        break;
    case WeaponId::Schism:
        rt.mag_capacity = 1;
        rt.ammo_in_mag  = 1;
        break;
    case WeaponId::Cathedral:
        rt.mag_capacity = 12;
        rt.ammo_in_mag  = 12;
        break;
    default:
        rt.mag_capacity = 0;
        rt.ammo_in_mag  = 0;
        break;
    }
}

namespace {

void apply_ammo_for_shot(WeaponId id, WeaponRuntimeState& rt) noexcept {
    switch (id) {
    case WeaponId::HaloScythe:
        return;
    case WeaponId::Dogma:
    case WeaponId::Confessor:
    case WeaponId::Cathedral:
        if (rt.ammo_in_mag > 0) --rt.ammo_in_mag;
        return;
    case WeaponId::HereticsLament:
        if (rt.ammo_in_mag > 0) --rt.ammo_in_mag;
        return;
    case WeaponId::Schism:
        if (rt.ammo_in_mag > 0) --rt.ammo_in_mag;
        return;
    default:
        return;
    }
}

} // namespace

FireResult fire_primary(WeaponId id, const martyrdom::ResolvedStats& rs,
                        WeaponRuntimeState& rt, float dt) noexcept {
    (void)dt;
    FireResult out{};
    const auto  alleg = allegiance(id);
    const auto  p       = primary_params(id);
    out.damage_out      = p.base_damage * rs.damage_mult;
    apply_ammo_for_shot(id, rt);

    if (alleg == WeaponAllegiance::Profane) {
        out.sin_delta = p.sin_on_hit * p.profane_weapon_mult_applies;
    } else {
        out.mantra_delta = p.mantra_on_hit;
    }
    out.audio_bus_gain_suggest = 1.f;
    return out;
}

FireResult fire_alt(WeaponId id, const martyrdom::ResolvedStats& rs,
                    WeaponRuntimeState& rt, float dt) noexcept {
    (void)rs;
    tick_cooldowns(rt, dt);
    FireResult out{};
    if (rt.alt_cooldown_s > 0.f) return out;

    switch (id) {
    case WeaponId::HaloScythe:
        if (rt.mantra_reserve >= 25.f) {
            rt.mantra_reserve -= 25.f;
            out.damage_out           = 80.f * rs.damage_mult;
            out.mantra_delta         = -25.f;
            out.consumed_mantra_alt  = true;
        }
        break;
    case WeaponId::Dogma:
        rt.alt_cooldown_s = 40.f;
        out.sin_delta     = 4.f;
        out.damage_out    = 6.f * rs.damage_mult;
        break;
    case WeaponId::HereticsLament:
        if (rt.mantra_reserve >= 10.f) {
            rt.mantra_reserve -= 10.f;
            out.damage_out = primary_params(id).base_damage * 4.f * rs.damage_mult;
            out.mantra_delta        = -10.f;
            out.consumed_mantra_alt = true;
        }
        break;
    case WeaponId::Confessor:
        rt.alt_cooldown_s = 12.f;
        out.sin_delta     = 15.f;
        break;
    case WeaponId::Schism:
        rt.alt_cooldown_s = 2.f;
        out.sin_delta     = 3.f;
        break;
    case WeaponId::Cathedral:
        if (rt.mantra_reserve >= 75.f) {
            rt.mantra_reserve -= 75.f;
            out.mantra_delta        = -75.f;
            out.consumed_mantra_alt = true;
            rt.alt_cooldown_s       = 20.f;
        }
        break;
    default:
        break;
    }
    return out;
}

void tick_cooldowns(WeaponRuntimeState& rt, float dt) noexcept {
    rt.alt_cooldown_s = std::max(0.f, rt.alt_cooldown_s - dt);
}

} // namespace gw::gameplay::weapons
