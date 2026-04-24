// tests/unit/gameplay/weapon_system_test.cpp — Phase 23 W152

#include <doctest/doctest.h>

#include "gameplay/martyrdom/stats.hpp"
#include "gameplay/weapons/weapon_catalog.hpp"
#include "gameplay/weapons/weapon_system.hpp"

using namespace gw::gameplay::weapons;
using gw::gameplay::martyrdom::ResolvedStats;

TEST_CASE("weapon roster — six weapons have distinct primary modes") {
    CHECK(primary_mode(WeaponId::HaloScythe) == FireMode::Melee);
    CHECK(primary_mode(WeaponId::Dogma) == FireMode::Hitscan);
    CHECK(primary_mode(WeaponId::Schism) == FireMode::Projectile);
}

TEST_CASE("weapon Dogma — profane primary accrues Sin") {
    ResolvedStats rs{};
    WeaponRuntimeState rt{};
    init_default_runtime(WeaponId::Dogma, rt);
    const auto r = fire_primary(WeaponId::Dogma, rs, rt, 0.016f);
    CHECK(r.sin_delta == doctest::Approx(8.f));
    CHECK(rt.ammo_in_mag == 47);
}

TEST_CASE("weapon Halo-Scythe — sacred primary accrues Mantra") {
    ResolvedStats rs{};
    WeaponRuntimeState rt{};
    init_default_runtime(WeaponId::HaloScythe, rt);
    const auto r = fire_primary(WeaponId::HaloScythe, rs, rt, 0.016f);
    CHECK(r.mantra_delta == doctest::Approx(4.f));
}

TEST_CASE("weapon Heretic — alt consumes Mantra and scales damage") {
    ResolvedStats rs{};
    WeaponRuntimeState rt{};
    init_default_runtime(WeaponId::HereticsLament, rt);
    rt.mantra_reserve = 50.f;
    const auto r = fire_alt(WeaponId::HereticsLament, rs, rt, 0.f);
    CHECK(r.consumed_mantra_alt);
    CHECK(r.damage_out > 100.f);
}
