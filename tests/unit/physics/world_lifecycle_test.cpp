// tests/unit/physics/world_lifecycle_test.cpp — Phase 12 Wave 12A.

#include <doctest/doctest.h>

#include "engine/physics/physics_world.hpp"

#include <string>

using namespace gw::physics;

TEST_CASE("PhysicsWorld default-constructs and reports null backend") {
    PhysicsWorld w;
    CHECK_FALSE(w.initialized());
    CHECK(w.backend() == BackendKind::Null);
    CHECK(std::string{w.backend_name()} == "null");
}

TEST_CASE("PhysicsWorld::initialize is idempotent and applies config") {
    PhysicsWorld w;
    PhysicsConfig cfg{};
    cfg.gravity = glm::vec3{0.0f, -3.7f, 0.0f};
    cfg.fixed_dt = 1.0f / 120.0f;
    REQUIRE(w.initialize(cfg));
    CHECK(w.initialized());
    CHECK(w.config().fixed_dt == doctest::Approx(1.0f / 120.0f));
    CHECK(w.gravity().y == doctest::Approx(-3.7f));

    // Second initialize should return true without resetting.
    CHECK(w.initialize(cfg));
}

TEST_CASE("PhysicsWorld::shutdown clears bodies and can be reinitialized") {
    PhysicsWorld w;
    REQUIRE(w.initialize(PhysicsConfig{}));

    ShapeHandle shape = w.create_shape(BoxShapeDesc{glm::vec3{1.0f}});
    REQUIRE(shape.valid());
    RigidBodyDesc d{}; d.shape = shape; d.motion_type = BodyMotionType::Dynamic;
    BodyHandle body = w.create_body(d);
    REQUIRE(body.valid());
    CHECK(w.body_count() == 1);

    w.shutdown();
    CHECK_FALSE(w.initialized());

    REQUIRE(w.initialize(PhysicsConfig{}));
    CHECK(w.body_count() == 0);
}

TEST_CASE("PhysicsWorld::reset preserves config but clears bodies") {
    PhysicsWorld w;
    PhysicsConfig cfg{};
    cfg.gravity.y = -5.0f;
    REQUIRE(w.initialize(cfg));

    ShapeHandle shape = w.create_shape(SphereShapeDesc{0.5f});
    RigidBodyDesc d{}; d.shape = shape;
    (void)w.create_body(d);
    (void)w.create_body(d);
    CHECK(w.body_count() == 2);

    w.reset();
    CHECK(w.body_count() == 0);
    CHECK(w.gravity().y == doctest::Approx(-5.0f));
}

TEST_CASE("PhysicsWorld pause gates step progression") {
    PhysicsWorld w;
    REQUIRE(w.initialize(PhysicsConfig{}));
    const auto f0 = w.frame_index();
    w.pause(true);
    (void)w.step(1.0f / 60.0f);
    CHECK(w.frame_index() == f0);
    w.pause(false);
    (void)w.step(1.0f / 60.0f);
    CHECK(w.frame_index() == f0 + 1);
}
