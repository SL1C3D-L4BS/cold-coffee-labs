#pragma once
// gameplay/weapons/weapon_system.hpp — Phase 23 W152

#include "gameplay/martyrdom/stats.hpp"
#include "gameplay/weapons/weapon_types.hpp"

namespace gw::gameplay::weapons {

void init_default_runtime(WeaponId id, WeaponRuntimeState& rt) noexcept;

/// Primary fire: applies ResolvedStats damage mult + allegiance-driven Sin/Mantra.
[[nodiscard]] FireResult fire_primary(WeaponId id, const martyrdom::ResolvedStats& rs,
                                      WeaponRuntimeState& rt, float dt) noexcept;

/// Alt-fire (Phase 23 model): Mantra spend or cooldown gate per weapon.
[[nodiscard]] FireResult fire_alt(WeaponId id, const martyrdom::ResolvedStats& rs,
                                  WeaponRuntimeState& rt, float dt) noexcept;

void tick_cooldowns(WeaponRuntimeState& rt, float dt) noexcept;

} // namespace gw::gameplay::weapons
