// tests/unit/physics/rigid_body_test.cpp — Phase 12 Wave 12A.

#include <doctest/doctest.h>

#include "engine/physics/physics_world.hpp"

#include <cmath>

using namespace gw::physics;

namespace {

PhysicsConfig make_config(float dt = 1.0f / 60.0f) {
    PhysicsConfig cfg{};
    cfg.fixed_dt = dt;
    cfg.max_substeps = 16;
    cfg.sleep_linear_threshold = 0.0f;
    cfg.sleep_angular_threshold = 0.0f;
    return cfg;
}

} // namespace

TEST_CASE("Body creation + destruction reuse slot via free list") {
    PhysicsWorld w; REQUIRE(w.initialize(make_config()));
    ShapeHandle s = w.create_shape(SphereShapeDesc{0.5f});
    RigidBodyDesc d{}; d.shape = s;
    BodyHandle a = w.create_body(d);
    BodyHandle b = w.create_body(d);
    CHECK(a.valid());
    CHECK(b.valid());
    CHECK(a.id != b.id);
    CHECK(w.body_count() == 2);

    w.destroy_body(a);
    CHECK(w.body_count() == 1);
    BodyHandle c = w.create_body(d);
    CHECK(c.valid());
    CHECK(c.id == a.id);  // slot reuse
    CHECK(w.body_count() == 2);
}

TEST_CASE("Free-fall under gravity advances position via fixed step") {
    PhysicsConfig cfg = make_config();
    cfg.gravity = glm::vec3{0.0f, -9.81f, 0.0f};
    PhysicsWorld w; REQUIRE(w.initialize(cfg));
    ShapeHandle s = w.create_shape(SphereShapeDesc{0.1f});

    RigidBodyDesc d{}; d.shape = s;
    d.motion_type = BodyMotionType::Dynamic;
    d.position_ws = glm::dvec3{0.0, 100.0, 0.0};
    BodyHandle body = w.create_body(d);

    for (int i = 0; i < 30; ++i) w.step_fixed();  // 0.5 s

    const BodyState st = w.body_state(body);
    CHECK(st.position_ws.y < 100.0);
    CHECK(st.linear_velocity.y < 0.0f);
}

TEST_CASE("Gravity factor scales apparent gravity") {
    PhysicsConfig cfg = make_config();
    cfg.gravity = glm::vec3{0.0f, -10.0f, 0.0f};
    PhysicsWorld w; REQUIRE(w.initialize(cfg));
    ShapeHandle s = w.create_shape(SphereShapeDesc{0.1f});

    RigidBodyDesc da{}; da.shape = s; da.gravity_factor = 0.0f;
    da.position_ws = glm::dvec3{0.0, 100.0, 0.0};
    RigidBodyDesc db = da; db.gravity_factor = 1.0f;
    BodyHandle a = w.create_body(da);
    BodyHandle b = w.create_body(db);

    for (int i = 0; i < 60; ++i) w.step_fixed();  // 1 s

    const auto sa = w.body_state(a);
    const auto sb = w.body_state(b);
    CHECK(std::fabs(sa.linear_velocity.y) < 1e-3f);     // no gravity
    CHECK(sb.linear_velocity.y < -5.0f);                // gravity active
}

TEST_CASE("Static bodies never move") {
    PhysicsConfig cfg = make_config();
    cfg.gravity = glm::vec3{0.0f, -9.81f, 0.0f};
    PhysicsWorld w; REQUIRE(w.initialize(cfg));
    ShapeHandle s = w.create_shape(BoxShapeDesc{glm::vec3{1.0f, 0.1f, 1.0f}});
    RigidBodyDesc d{}; d.shape = s; d.motion_type = BodyMotionType::Static;
    d.position_ws = glm::dvec3{0.0, 0.0, 0.0};
    BodyHandle body = w.create_body(d);
    for (int i = 0; i < 30; ++i) w.step_fixed();
    const auto st = w.body_state(body);
    CHECK(st.position_ws.y == doctest::Approx(0.0));
    CHECK(st.linear_velocity.y == doctest::Approx(0.0f));
}

TEST_CASE("Apply impulse changes velocity next step") {
    PhysicsConfig cfg = make_config();
    cfg.gravity = glm::vec3{0.0f};  // no gravity for clean impulse test
    PhysicsWorld w; REQUIRE(w.initialize(cfg));
    ShapeHandle s = w.create_shape(SphereShapeDesc{0.1f});
    RigidBodyDesc d{}; d.shape = s; d.mass_kg = 2.0f;
    BodyHandle body = w.create_body(d);
    w.apply_impulse(body, glm::vec3{0.0f, 6.0f, 0.0f});  // Δv = 3 m/s
    w.step_fixed();
    const auto st = w.body_state(body);
    CHECK(st.linear_velocity.y == doctest::Approx(3.0f).epsilon(0.01f));
}

TEST_CASE("Resting body on a static ground plane does not tunnel through") {
    PhysicsConfig cfg = make_config();
    cfg.gravity = glm::vec3{0.0f, -9.81f, 0.0f};
    PhysicsWorld w; REQUIRE(w.initialize(cfg));

    // Wide static floor.
    ShapeHandle floor_shape = w.create_shape(BoxShapeDesc{glm::vec3{10.0f, 0.1f, 10.0f}});
    RigidBodyDesc floor{}; floor.shape = floor_shape;
    floor.motion_type = BodyMotionType::Static;
    floor.position_ws = glm::dvec3{0.0, 0.0, 0.0};
    (void)w.create_body(floor);

    // Dynamic box just above it.
    ShapeHandle box = w.create_shape(BoxShapeDesc{glm::vec3{0.5f}});
    RigidBodyDesc bd{}; bd.shape = box;
    bd.motion_type = BodyMotionType::Dynamic;
    bd.position_ws = glm::dvec3{0.0, 2.0, 0.0};
    BodyHandle body = w.create_body(bd);

    for (int i = 0; i < 120; ++i) w.step_fixed();  // 2 s
    const auto st = w.body_state(body);
    // Body should be resting on top of the floor (not below it).
    CHECK(st.position_ws.y > 0.5);
    CHECK(st.position_ws.y < 1.1);
}
