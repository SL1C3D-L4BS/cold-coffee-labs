// tests/unit/gameplay/gore_system_test.cpp — Phase 23 W151

#include <doctest/doctest.h>

#include "engine/services/gore/gore_service.hpp"
#include "gameplay/combat/encounter_director_bridge.hpp"
#include "gameplay/combat/gore_system.hpp"

TEST_CASE("Gore apply_hit produces decals for heavy damage") {
    gw::services::gore::GoreHitRequest req{};
    req.seed      = 0xA11;
    req.damage    = 90.f;
    req.gore_level = 1.f;
    const auto r  = gw::services::gore::apply_hit(req);
    CHECK(r.decals_spawned >= 1u);
    CHECK(r.limbs_severed_mask != 0u);
}

TEST_CASE("Gameplay gore facade matches service") {
    gw::services::gore::GoreHitRequest req{};
    req.damage = 50.f;
    req.seed   = 42;
    const auto a = gw::gameplay::combat::apply_gore_hit(req);
    const auto b = gw::services::gore::apply_hit(req);
    CHECK(a.decals_spawned == b.decals_spawned);
    CHECK(a.limbs_severed_mask == b.limbs_severed_mask);
}

TEST_CASE("Director bridge evaluates bounded spawn scale") {
    gw::services::director::DirectorRequest req{};
    req.seed            = 0xBEEF;
    req.normalized_sin  = 0.5f;
    req.current_state   = gw::services::director::IntensityState::Relax;
    const auto out      = gw::gameplay::combat::evaluate_encounter_director(req);
    CHECK(out.spawn_rate_scale >= 0.f);
    CHECK(out.spawn_rate_scale <= 4.f);
}

TEST_CASE("Limb obstacle bookkeeping counts bits") {
    gw::gameplay::combat::GoreNavObstacleBookkeeping bk{};
    gw::gameplay::combat::register_limb_obstacles(bk, 0b1011u);
    CHECK(bk.active_limb_obstacles == 3);
}
