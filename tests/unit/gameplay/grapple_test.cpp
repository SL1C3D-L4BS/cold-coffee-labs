// tests/unit/gameplay/grapple_test.cpp — Phase 22 W147

#include <doctest/doctest.h>

#include "gameplay/movement/grapple.hpp"

using namespace gw::gameplay::movement;

TEST_CASE("Idle → Flying on tap within range") {
    GrappleState g{};
    GrappleInput in{};
    in.tap = true;
    in.hit = true;
    in.distance = 10.f;
    in.target_point = {5.f, 0.f, 5.f};
    PlayerMotion m{};
    PlayerTuning t{};
    (void)step_grapple(g, m, in, t, 1.f / 60.f);
    CHECK(g.mode == GrappleMode::Flying);
    CHECK(g.anchor_point.x == doctest::Approx(5.f));
}

TEST_CASE("Hold on small enemy → Reeling then glory kill on close") {
    GrappleState g{};
    PlayerMotion m{};
    PlayerTuning t{};
    GrappleInput in{};
    in.hold       = true;
    in.hit        = true;
    in.hit_enemy  = true;
    in.distance   = 8.f;
    in.target_point = {0.f, 0.f, 8.f};
    (void)step_grapple(g, m, in, t, 1.f / 60.f);
    CHECK(g.mode == GrappleMode::Reeling);

    // Target reached.
    g.anchor_point = {0.f, 0.f, 0.2f};
    const auto r = step_grapple(g, m, in, t, 1.f / 60.f);
    CHECK(r.trigger_glory_kill);
    CHECK(g.mode == GrappleMode::Idle);
}

TEST_CASE("Over-range target breaks the tether") {
    GrappleState g{};
    g.mode = GrappleMode::Flying;
    g.anchor_point = {100.f, 0.f, 0.f};
    g.origin_point = {0.f, 0.f, 0.f};
    g.tether_length = 20.f;
    PlayerMotion m{};
    PlayerTuning t{};
    GrappleInput in{};
    const auto r = step_grapple(g, m, in, t, 1.f / 60.f);
    CHECK(r.broken_this_tick);
    CHECK(g.mode == GrappleMode::Broken);
}
