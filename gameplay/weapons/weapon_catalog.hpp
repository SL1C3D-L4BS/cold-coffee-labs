#pragma once
// gameplay/weapons/weapon_catalog.hpp — Phase 23 W152 (docs/07 §4)

#include "gameplay/weapons/weapon_types.hpp"

namespace gw::gameplay::weapons {

[[nodiscard]] constexpr WeaponAllegiance allegiance(WeaponId w) noexcept {
    switch (w) {
    case WeaponId::HaloScythe:
    case WeaponId::HereticsLament:
    case WeaponId::Cathedral:
        return WeaponAllegiance::Sacred;
    default:
        return WeaponAllegiance::Profane;
    }
}

[[nodiscard]] constexpr FireMode primary_mode(WeaponId w) noexcept {
    switch (w) {
    case WeaponId::HaloScythe:
        return FireMode::Melee;
    case WeaponId::Dogma:
    case WeaponId::HereticsLament:
    case WeaponId::Confessor:
        return FireMode::Hitscan;
    case WeaponId::Schism:
    case WeaponId::Cathedral:
        return FireMode::Projectile;
    default:
        return FireMode::Hitscan;
    }
}

[[nodiscard]] constexpr WeaponShotParams primary_params(WeaponId w) noexcept {
    switch (w) {
    case WeaponId::HaloScythe:
        return {22.f, 0.f, 0.f, 4.f, 12.f, 0.f};
    case WeaponId::Dogma:
        return {9.f, 8.f, 0.f, 0.f, 0.f, 1.f};
    case WeaponId::HereticsLament:
        return {55.f, 0.f, 0.f, 0.f, 0.f, 0.f};
    case WeaponId::Confessor:
        return {14.f, 12.f, 0.f, 0.f, 0.f, 1.f};
    case WeaponId::Schism:
        return {18.f, 6.f, 0.f, 0.f, 0.f, 1.f};
    case WeaponId::Cathedral:
        return {40.f, 0.f, 0.f, 0.f, 0.f, 0.f};
    default:
        return {};
    }
}

} // namespace gw::gameplay::weapons
