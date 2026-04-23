// tests/unit/gameplay/martyrdom_systems_test.cpp — Phase 22 W144

#include <doctest/doctest.h>

#include "gameplay/martyrdom/martyrdom_components.hpp"
#include "gameplay/martyrdom/martyrdom_systems.hpp"

using namespace gw::gameplay::martyrdom;

TEST_CASE("Sin accrual — profane hits scale by weapon multiplier") {
    SinComponent s{};
    MartyrdomFrameEvents ev{};
    ev.profane_hits = 4;
    ev.profane_weapon_mult = 1.5f; // +5 * 4 * 1.5 = 30
    tick_sin_accrual(s, ev);
    CHECK(s.value == doctest::Approx(30.f));
}

TEST_CASE("Sin accrual — Painweaver web pauses accrual") {
    SinComponent s{};
    MartyrdomFrameEvents ev{};
    ev.profane_hits = 10;
    ev.painweaver_web_active = true;
    tick_sin_accrual(s, ev);
    CHECK(s.value == doctest::Approx(0.f));
}

TEST_CASE("Sin accrual — clamps at max") {
    SinComponent s{};
    s.value = 95.f;
    MartyrdomFrameEvents ev{};
    ev.profane_hits = 10;
    tick_sin_accrual(s, ev);
    CHECK(s.value == doctest::Approx(100.f));
}

TEST_CASE("Mantra accrual from sacred / parry / glory kill") {
    MantraComponent m{};
    MartyrdomFrameEvents ev{};
    ev.sacred_kills      = 2;
    ev.precision_parries = 1;
    ev.glory_kills       = 1;
    // 2*12 + 1*20 + 1*15 = 59
    tick_mantra_accrual(m, ev);
    CHECK(m.value == doctest::Approx(59.f));
}

TEST_CASE("Rapture check fires at Sin=max, zeroes Sin, sets 6s timer") {
    SinComponent s{};
    s.value = 100.f;
    RaptureState r{};
    RuinState    ruin{};
    RaptureTriggerCounter counter{};

    const auto res = tick_rapture_check(s, r, ruin, counter);
    CHECK(res.just_triggered);
    CHECK(r.active);
    CHECK(r.time_remaining == doctest::Approx(kRaptureDurationSec));
    CHECK(s.value == doctest::Approx(0.f));
    CHECK(r.trigger_count == 1);
    CHECK(counter.this_circle == 1);
    CHECK(counter.lifetime == 1);
}

TEST_CASE("Rapture check will NOT fire while Ruin is active") {
    SinComponent s{};
    s.value = 100.f;
    RaptureState r{};
    RuinState ruin{};
    ruin.active = true;
    ruin.time_remaining = 4.f;
    RaptureTriggerCounter counter{};
    const auto res = tick_rapture_check(s, r, ruin, counter);
    CHECK_FALSE(res.just_triggered);
    CHECK_FALSE(r.active);
    CHECK(s.value == doctest::Approx(100.f));
}

TEST_CASE("Rapture expiry transitions to Ruin with 12s") {
    RaptureState r{};
    r.active = true;
    r.time_remaining = 0.1f;
    RuinState ruin{};
    const auto res = tick_rapture(r, ruin, 0.2f);
    CHECK(res.rapture_expired_this_tick);
    CHECK_FALSE(r.active);
    CHECK(ruin.active);
    CHECK(ruin.time_remaining == doctest::Approx(kRuinDurationSec));
    CHECK(kRuinDurationSec == doctest::Approx(12.f));
}

TEST_CASE("Ruin tick expires and clears") {
    RuinState ruin{};
    ruin.active = true;
    ruin.time_remaining = 0.05f;
    const auto res = tick_ruin(ruin, 0.1f);
    CHECK(res.ruin_expired_this_tick);
    CHECK_FALSE(ruin.active);
    CHECK(ruin.time_remaining == doctest::Approx(0.f));
}

TEST_CASE("Blasphemy tick drops expired slots and compacts") {
    ActiveBlasphemies slots{};
    slots.count = 3;
    slots.entries[0] = {BlasphemyType::Stigmata,        0.05f};
    slots.entries[1] = {BlasphemyType::UnholyCommunion, 5.0f};
    slots.entries[2] = {BlasphemyType::CrownOfThorns,   0.05f};

    tick_blasphemies(slots, 0.1f);
    CHECK(slots.count == 1);
    CHECK(slots.entries[0].type == BlasphemyType::UnholyCommunion);
}

TEST_CASE("observe_sin_signature fills SinSignatureStepInput deterministically") {
    MartyrdomFrameEvents ev{};
    ev.sacred_kills = 3;
    ev.glory_kills  = 2;
    ev.precision_parries = 1;
    ev.aoe_sin_ticks = 1;

    RaptureState r{};
    r.active = true;
    const auto obs = observe_sin_signature(ev, r, false, true, 1.f);
    CHECK(obs.rapture_active);
    CHECK(obs.entered_new_area);
    CHECK(obs.deaths_in_area == doctest::Approx(1.f));
    CHECK(obs.kills_this_tick >= 5u);
    CHECK(obs.precision_kills_this_tick >= 4u);
    CHECK(obs.cruelty_kills_this_tick   >= 3u);
}
