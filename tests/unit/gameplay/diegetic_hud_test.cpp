// tests/unit/gameplay/diegetic_hud_test.cpp — Phase 21 W140 (ADR-0108).

#include "gameplay/hud/diegetic_hud.hpp"

#include <doctest/doctest.h>

using gw::gameplay::hud::DiegeticHudInputs;
using gw::gameplay::hud::build_hud_model;
using gw::gameplay::hud::needs_a11y_fallback;

TEST_CASE("diegetic_hud: full health yields no pallor/no blood vignette") {
    DiegeticHudInputs in{};
    in.health_norm = 1.0f;
    const auto m = build_hud_model(in);
    CHECK(m.weapon_pallor_01 == doctest::Approx(0.0f));
    CHECK(m.vignette_blood_01 == doctest::Approx(0.0f));
}

TEST_CASE("diegetic_hud: zero health fully pallors + full blood vignette") {
    DiegeticHudInputs in{};
    in.health_norm = 0.0f;
    const auto m = build_hud_model(in);
    CHECK(m.weapon_pallor_01 == doctest::Approx(1.0f));
    CHECK(m.vignette_blood_01 == doctest::Approx(1.0f));
}

TEST_CASE("diegetic_hud: chamber fill reflects ammo ratio") {
    DiegeticHudInputs in{};
    in.ammo_current  = 4;
    in.ammo_capacity = 16;
    const auto m = build_hud_model(in);
    CHECK(m.chamber_fill_01 == doctest::Approx(0.25f));
}

TEST_CASE("diegetic_hud: large print always triggers a11y fallback") {
    DiegeticHudInputs in{};
    CHECK(needs_a11y_fallback(in, /*large_print_enabled=*/true));
    CHECK_FALSE(needs_a11y_fallback(in, /*large_print_enabled=*/false));
}

TEST_CASE("diegetic_hud: deep Rapture/Ruin suppresses HUD -> a11y fallback") {
    DiegeticHudInputs r{};
    r.rapture_norm = 0.9f;
    CHECK(needs_a11y_fallback(r, /*large_print_enabled=*/false));

    DiegeticHudInputs ruin{};
    ruin.ruin_norm = 0.85f;
    CHECK(needs_a11y_fallback(ruin, /*large_print_enabled=*/false));
}
