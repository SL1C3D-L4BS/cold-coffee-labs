// tests/unit/gameplay/blood_surf_test.cpp — Phase 22 W147

#include <doctest/doctest.h>

#include "gameplay/movement/blood_surf.hpp"

using namespace gw::gameplay::movement;

TEST_CASE("Surf active on slope > 15°") {
    BloodSurfQuery q{};
    q.slope_degrees = 20.f;
    PlayerTuning t{};
    const auto r = apply_blood_surf({10.f, 0.f, 0.f}, q, t, 1.f / 60.f);
    CHECK(r.surf_active);
    CHECK(r.effective_friction == doctest::Approx(t.surf_friction));
}

TEST_CASE("Surf active on blood decal cell") {
    BloodSurfQuery q{};
    q.on_blood_decal = true;
    PlayerTuning t{};
    const auto r = apply_blood_surf({10.f, 0.f, 0.f}, q, t, 1.f / 60.f);
    CHECK(r.surf_active);
}

TEST_CASE("Surf inactive on flat dry ground") {
    BloodSurfQuery q{};
    q.slope_degrees = 5.f;
    q.on_blood_decal = false;
    PlayerTuning t{};
    const auto r = apply_blood_surf({10.f, 0.f, 0.f}, q, t, 1.f / 60.f);
    CHECK_FALSE(r.surf_active);
    CHECK(r.effective_friction == doctest::Approx(t.slide_friction));
}

TEST_CASE("Vertical velocity preserved across surf") {
    BloodSurfQuery q{};
    q.on_blood_decal = true;
    PlayerTuning t{};
    const auto r = apply_blood_surf({10.f, -5.f, 0.f}, q, t, 1.f / 60.f);
    CHECK(r.post_friction_velocity.y == doctest::Approx(-5.f));
    CHECK(r.post_friction_velocity.x < 10.f);
}
