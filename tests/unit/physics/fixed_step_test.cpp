// tests/unit/physics/fixed_step_test.cpp — Phase 12 Wave 12A.

#include <doctest/doctest.h>

#include "engine/physics/physics_world.hpp"

using namespace gw::physics;

TEST_CASE("step() accumulates wall-time into a fixed-step integer count") {
    PhysicsConfig cfg{};
    cfg.fixed_dt = 1.0f / 60.0f;
    cfg.max_substeps = 8;
    PhysicsWorld w; REQUIRE(w.initialize(cfg));

    CHECK(w.step(1.0f / 120.0f) == 0);    // under a full dt
    CHECK(w.step(1.0f / 120.0f) == 1);    // 2 × 1/120 == 1/60
}

TEST_CASE("max_substeps clamps catch-up after a long hitch") {
    PhysicsConfig cfg{};
    cfg.fixed_dt = 1.0f / 60.0f;
    cfg.max_substeps = 4;
    PhysicsWorld w; REQUIRE(w.initialize(cfg));
    // Feed 1 second (60 steps) in one call — should cap at max_substeps.
    const std::uint32_t n = w.step(1.0f);
    CHECK(n == cfg.max_substeps);
}

TEST_CASE("interpolation_alpha reports 0..1 across a partial step") {
    PhysicsConfig cfg{};
    cfg.fixed_dt = 1.0f / 60.0f;
    PhysicsWorld w; REQUIRE(w.initialize(cfg));
    (void)w.step(0.5f * cfg.fixed_dt);
    const float a = w.interpolation_alpha();
    CHECK(a >= 0.0f);
    CHECK(a <= 1.0f);
    CHECK(a == doctest::Approx(0.5f).epsilon(0.01f));
}

TEST_CASE("step_fixed advances frame_index by exactly 1") {
    PhysicsWorld w; REQUIRE(w.initialize(PhysicsConfig{}));
    const auto f0 = w.frame_index();
    w.step_fixed();
    CHECK(w.frame_index() == f0 + 1);
}
