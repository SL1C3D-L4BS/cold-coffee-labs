// tests/unit/gameplay/player_controller_test.cpp — Phase 22 W146

#include <doctest/doctest.h>

#include "gameplay/movement/player_controller.hpp"

#include <cmath>

using namespace gw::gameplay::movement;

TEST_CASE("Bunny hop — grounded jump multiplies horizontal velocity") {
    PlayerMotion m{};
    m.grounded = true;
    m.velocity = {5.f, 0.f, 0.f};
    PlayerInput in{};
    in.jump_pressed = true;
    PlayerTuning t{};
    const Vec3 v = apply_bunny_hop(m, in, t, 1.f / 60.f);
    CHECK(v.x == doctest::Approx(5.f * t.bhop_multiplier).epsilon(1e-3));
    CHECK(v.y == doctest::Approx(t.wall_kick_vertical));
}

TEST_CASE("Bunny hop — air speed is capped at max_air_speed") {
    PlayerMotion m{};
    m.grounded = false;
    m.velocity = {100.f, 0.f, 0.f};
    PlayerInput in{};
    PlayerTuning t{};
    const Vec3 v = apply_bunny_hop(m, in, t, 1.f / 60.f);
    const float speed = std::sqrt(v.x * v.x + v.z * v.z);
    CHECK(speed <= doctest::Approx(t.max_air_speed + 0.1f));
}

TEST_CASE("Wall kick requires jump, wall normal, not grounded, not used") {
    PlayerMotion m{};
    m.grounded = false;
    m.wall_normal = {1.f, 0.f, 0.f};
    m.velocity = {0.f, 0.f, 0.f};
    PlayerInput in{};
    in.jump_pressed = true;
    PlayerTuning t{};
    Vec3 v = apply_wall_kick(m, in, t);
    CHECK(v.x == doctest::Approx(t.wall_kick_horizontal));
    CHECK(v.y == doctest::Approx(t.wall_kick_vertical));

    // Used-flag blocks re-kick.
    m.wall_kick_used = true;
    v = apply_wall_kick(m, in, t);
    CHECK(v.x == doctest::Approx(0.f));
    CHECK(v.y == doctest::Approx(0.f));
}

TEST_CASE("Slide preserves horizontal velocity, scales by friction") {
    PlayerMotion m{};
    m.grounded = true;
    m.velocity = {10.f, 0.f, 10.f};
    PlayerInput in{};
    in.crouch_held = true;
    PlayerTuning t{};
    const Vec3 v = apply_slide(m, in, t, 1.f / 60.f);
    CHECK(v.x < 10.f);
    CHECK(v.z < 10.f);
    CHECK(v.x > 9.f); // friction 0.15 * dt = very small per-frame decay
}

TEST_CASE("swept_aabb_vs_plane — t_hit on a horizontal floor") {
    const SweptAabbResult r = swept_aabb_vs_plane(
        {0.f, 5.f, 0.f}, {0.f, -5.f, 0.f},  // origin + delta
        {0.f, 0.f, 0.f}, {0.f, 1.f, 0.f});  // plane_point + normal
    CHECK(r.hit);
    CHECK(r.t_hit == doctest::Approx(1.f));
}

TEST_CASE("swept_aabb_vs_plane — parallel motion misses") {
    const SweptAabbResult r = swept_aabb_vs_plane(
        {0.f, 5.f, 0.f}, {1.f, 0.f, 0.f},
        {0.f, 0.f, 0.f}, {0.f, 1.f, 0.f});
    CHECK_FALSE(r.hit);
}

TEST_CASE("step_player — gravity on not grounded, ground clamp on land") {
    PlayerMotion m{};
    m.velocity = {0.f, 0.f, 0.f};
    PlayerInput in{};
    EnvironmentSample env{};
    PlayerTuning t{};
    // In the air.
    env.ground_hit = false;
    step_player(m, in, env, t, 1.f / 60.f);
    CHECK(m.velocity.y < 0.f);

    // Land.
    env.ground_hit = true;
    step_player(m, in, env, t, 1.f / 60.f);
    CHECK(m.velocity.y == doctest::Approx(0.f));
    CHECK_FALSE(m.wall_kick_used); // ground reset
}

TEST_CASE("step_player — deterministic across identical inputs") {
    PlayerMotion a{};
    PlayerMotion b{};
    PlayerInput  in{};
    in.move_axis_y = 1.f;
    EnvironmentSample env{};
    env.ground_hit = true;
    PlayerTuning t{};
    for (int i = 0; i < 200; ++i) {
        in.jump_pressed = (i % 10 == 0);
        step_player(a, in, env, t, 1.f / 60.f);
        step_player(b, in, env, t, 1.f / 60.f);
    }
    CHECK(a.position.x == doctest::Approx(b.position.x));
    CHECK(a.position.y == doctest::Approx(b.position.y));
    CHECK(a.position.z == doctest::Approx(b.position.z));
    CHECK(a.velocity.x == doctest::Approx(b.velocity.x));
}

TEST_CASE("Mirror step fires only when Sin >= 80 AND unlocked AND not in Ruin") {
    CHECK_FALSE(try_mirror_step(50.f, true,  true,  false));
    CHECK_FALSE(try_mirror_step(80.f, true,  false, false)); // not unlocked
    CHECK_FALSE(try_mirror_step(80.f, false, true,  false)); // no input
    CHECK_FALSE(try_mirror_step(80.f, true,  true,  true));  // Ruin
    CHECK(try_mirror_step(80.f, true, true, false));
    CHECK(try_mirror_step(99.f, true, true, false));
}
