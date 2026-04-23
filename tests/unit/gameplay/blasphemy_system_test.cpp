// tests/unit/gameplay/blasphemy_system_test.cpp — Phase 22 W145

#include <doctest/doctest.h>

#include "engine/narrative/grace_meter.hpp"
#include "gameplay/martyrdom/blasphemy_system.hpp"
#include "gameplay/martyrdom/martyrdom_components.hpp"
#include "gameplay/martyrdom/stats.hpp"

#include <cstring>

using namespace gw::gameplay::martyrdom;

TEST_CASE("BlasphemyType / kBlasphemyTable are in sync at compile time") {
    CHECK(static_cast<int>(BlasphemyType::Count) == 8);
    CHECK(std::strcmp(blasphemy_spec(BlasphemyType::Stigmata).canon_name, "Stigmata") == 0);
    CHECK(std::strcmp(blasphemy_spec(BlasphemyType::Forgive).canon_name, "Forgive") == 0);
}

TEST_CASE("Canon Blasphemy costs / durations match docs/07 §2.3") {
    CHECK(blasphemy_spec(BlasphemyType::Stigmata).sin_cost        == doctest::Approx(25.f));
    CHECK(blasphemy_spec(BlasphemyType::Stigmata).duration_sec    == doctest::Approx(5.f));
    CHECK(blasphemy_spec(BlasphemyType::CrownOfThorns).sin_cost   == doctest::Approx(40.f));
    CHECK(blasphemy_spec(BlasphemyType::CrownOfThorns).duration_sec == doctest::Approx(8.f));
    CHECK(blasphemy_spec(BlasphemyType::WrathOfGod).sin_cost      == doctest::Approx(60.f));
    CHECK(blasphemy_spec(BlasphemyType::WrathOfGod).duration_sec  == doctest::Approx(0.f));
    CHECK(blasphemy_spec(BlasphemyType::FalseIdol).sin_cost       == doctest::Approx(30.f));
    CHECK(blasphemy_spec(BlasphemyType::UnholyCommunion).sin_cost == doctest::Approx(50.f));
    CHECK(blasphemy_spec(BlasphemyType::Forgive).sin_cost         == doctest::Approx(100.f));
    CHECK(blasphemy_spec(BlasphemyType::Forgive).grace_cost       == doctest::Approx(100.f));
}

TEST_CASE("Cast blocked on insufficient Sin") {
    SinComponent s{}; s.value = 10.f;
    gw::narrative::GraceComponent g{};
    RuinState r{};
    ActiveBlasphemies slots{};
    auto res = try_cast_blasphemy(BlasphemyType::Stigmata, s, g, r, slots, 0);
    CHECK(res.status == BlasphemyCastError::InsufficientSin);
    CHECK(s.value == doctest::Approx(10.f));
    CHECK(slots.count == 0);
}

TEST_CASE("Cast blocked while Ruin active") {
    SinComponent s{}; s.value = 80.f;
    gw::narrative::GraceComponent g{};
    RuinState r{}; r.active = true; r.time_remaining = 5.f;
    ActiveBlasphemies slots{};
    auto res = try_cast_blasphemy(BlasphemyType::Stigmata, s, g, r, slots, 0);
    CHECK(res.status == BlasphemyCastError::Ruin);
}

TEST_CASE("Cast Stigmata consumes 25 Sin, fills slot, 5 s duration") {
    SinComponent s{}; s.value = 30.f;
    gw::narrative::GraceComponent g{};
    RuinState r{};
    ActiveBlasphemies slots{};
    auto res = try_cast_blasphemy(BlasphemyType::Stigmata, s, g, r, slots, 0);
    CHECK(res.status == BlasphemyCastError::Ok);
    CHECK(s.value == doctest::Approx(5.f));
    CHECK(slots.count == 1);
    CHECK(slots.entries[0].time_remaining == doctest::Approx(5.f));
    CHECK(slots.entries[0].type == BlasphemyType::Stigmata);
}

TEST_CASE("Cast WrathOfGod is instant — no slot") {
    SinComponent s{}; s.value = 70.f;
    gw::narrative::GraceComponent g{};
    RuinState r{};
    ActiveBlasphemies slots{};
    auto res = try_cast_blasphemy(BlasphemyType::WrathOfGod, s, g, r, slots, 0);
    CHECK(res.status == BlasphemyCastError::Ok);
    CHECK(res.slot_idx == -1);
    CHECK(slots.count == 0);
    CHECK(s.value == doctest::Approx(10.f));
}

TEST_CASE("Forgive requires unlock AND 100 Sin AND 100 Grace") {
    SinComponent s{}; s.value = 100.f;
    gw::narrative::GraceComponent g{};
    g.value = 100.f;
    RuinState r{};
    ActiveBlasphemies slots{};
    // Missing unlock.
    auto r1 = try_cast_blasphemy(BlasphemyType::Forgive, s, g, r, slots, 0);
    CHECK(r1.status == BlasphemyCastError::UnlockRequired);

    // With unlock.
    const std::uint8_t unlocked_mask = 1u << 0; // Forgive is bit 0 of the unlockables sub-mask
    auto r2 = try_cast_blasphemy(BlasphemyType::Forgive, s, g, r, slots, unlocked_mask);
    CHECK(r2.status == BlasphemyCastError::Ok);
    CHECK(s.value == doctest::Approx(0.f));
    CHECK(g.value == doctest::Approx(0.f));
}

TEST_CASE("Forgive fails if Grace insufficient") {
    SinComponent s{}; s.value = 100.f;
    gw::narrative::GraceComponent g{};
    g.value = 50.f;
    RuinState r{};
    ActiveBlasphemies slots{};
    const std::uint8_t unlocked_mask = 1u << 0;
    auto res = try_cast_blasphemy(BlasphemyType::Forgive, s, g, r, slots, unlocked_mask);
    CHECK(res.status == BlasphemyCastError::InsufficientGrace);
    CHECK(s.value == doctest::Approx(100.f)); // not consumed
}

TEST_CASE("Duration slots: 4th cast evicts oldest (FIFO)") {
    SinComponent s{}; s.value = 500.f;
    gw::narrative::GraceComponent g{};
    RuinState r{};
    ActiveBlasphemies slots{};
    (void)try_cast_blasphemy(BlasphemyType::Stigmata, s, g, r, slots, 0);
    (void)try_cast_blasphemy(BlasphemyType::CrownOfThorns, s, g, r, slots, 0);
    (void)try_cast_blasphemy(BlasphemyType::FalseIdol, s, g, r, slots, 0);
    CHECK(slots.count == 3);
    auto r4 = try_cast_blasphemy(BlasphemyType::UnholyCommunion, s, g, r, slots, 0);
    CHECK(r4.status == BlasphemyCastError::Ok);
    CHECK(slots.count == 3);
    CHECK(slots.entries[0].type == BlasphemyType::CrownOfThorns);
    CHECK(slots.entries[1].type == BlasphemyType::FalseIdol);
    CHECK(slots.entries[2].type == BlasphemyType::UnholyCommunion);
}

TEST_CASE("StatCompositionSystem — Rapture 3× and Ruin 0.25× incoming") {
    RaptureState rap{}; rap.active = true;
    RuinState    ruin{};
    ActiveBlasphemies slots{};
    StatCompositionInput in{};
    in.rapture = &rap;
    in.ruin    = &ruin;
    in.slots   = &slots;
    auto rs = compose_resolved_stats(in);
    CHECK(rs.damage_mult == doctest::Approx(3.f));
    CHECK(rs.speed_mult  == doctest::Approx(3.f));

    rap.active = false;
    ruin.active = true;
    rs = compose_resolved_stats(in);
    CHECK(rs.speed_mult == doctest::Approx(0.5f));
    CHECK(rs.damage_taken_mult == doctest::Approx(0.25f));
}

TEST_CASE("StatCompositionSystem — Stigmata sets reflect, UnholyCommunion sets lifesteal") {
    ActiveBlasphemies slots{};
    slots.count = 2;
    slots.entries[0] = {BlasphemyType::Stigmata, 5.f};
    slots.entries[1] = {BlasphemyType::UnholyCommunion, 12.f};
    StatCompositionInput in{};
    in.slots = &slots;
    auto rs = compose_resolved_stats(in);
    CHECK(rs.reflect_fraction == doctest::Approx(1.f));
    CHECK(rs.lifesteal        == doctest::Approx(0.5f));
    CHECK(rs.overshield_active);
}
