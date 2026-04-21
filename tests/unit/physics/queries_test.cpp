// tests/unit/physics/queries_test.cpp — Phase 12 Wave 12D.

#include <doctest/doctest.h>

#include "engine/physics/physics_world.hpp"

#include <array>

using namespace gw::physics;

TEST_CASE("raycast_closest returns the nearest body first") {
    PhysicsWorld w; REQUIRE(w.initialize(PhysicsConfig{}));
    ShapeHandle s = w.create_shape(BoxShapeDesc{glm::vec3{1.0f}});

    RigidBodyDesc d{}; d.shape = s; d.motion_type = BodyMotionType::Static;
    d.position_ws = {0.0, 0.0, -3.0}; d.entity = 1;
    (void)w.create_body(d);
    d.position_ws = {0.0, 0.0, -8.0}; d.entity = 2;
    (void)w.create_body(d);

    RaycastInput ray{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, 100.0f};
    RaycastHit hit{};
    REQUIRE(w.raycast_closest(ray, QueryFilter{}, hit));
    CHECK(hit.entity == 1);
    CHECK(hit.point_ws.z > -6.0f);
}

TEST_CASE("raycast against empty world returns false") {
    PhysicsWorld w; REQUIRE(w.initialize(PhysicsConfig{}));
    RaycastInput ray{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, 10.0f};
    RaycastHit hit{};
    CHECK_FALSE(w.raycast_closest(ray, QueryFilter{}, hit));
}

TEST_CASE("Overlap reports intersecting bodies and skips the query shape") {
    PhysicsWorld w; REQUIRE(w.initialize(PhysicsConfig{}));
    ShapeHandle probe = w.create_shape(BoxShapeDesc{glm::vec3{1.0f}});
    ShapeHandle body_shape = w.create_shape(BoxShapeDesc{glm::vec3{0.5f}});

    RigidBodyDesc d{}; d.shape = body_shape; d.motion_type = BodyMotionType::Static;
    d.position_ws = {0.0, 0.0, 0.0}; d.entity = 1; (void)w.create_body(d);
    d.position_ws = {0.5, 0.0, 0.0}; d.entity = 2; (void)w.create_body(d);
    d.position_ws = {5.0, 5.0, 5.0}; d.entity = 3; (void)w.create_body(d);  // far away

    OverlapInput in{probe, glm::vec3{0.0f, 0.0f, 0.0f}};
    std::array<ShapeQueryHit, 8> hits{};
    const std::size_t n = w.overlap(in, QueryFilter{}, hits);
    CHECK(n == 2);
}

TEST_CASE("shape_sweep_closest behaves like a raycast in the null backend") {
    PhysicsWorld w; REQUIRE(w.initialize(PhysicsConfig{}));
    ShapeHandle probe = w.create_shape(SphereShapeDesc{0.1f});
    ShapeHandle wall = w.create_shape(BoxShapeDesc{glm::vec3{1.0f}});
    RigidBodyDesc d{}; d.shape = wall; d.motion_type = BodyMotionType::Static;
    d.position_ws = {0.0, 0.0, -3.0}; d.entity = 7;
    (void)w.create_body(d);

    ShapeSweepInput in{probe, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, 100.0f};
    ShapeQueryHit hit{};
    REQUIRE(w.shape_sweep_closest(in, QueryFilter{}, hit));
    CHECK(hit.entity == 7);
}

TEST_CASE("raycast_batch fills hits and flags for every input") {
    PhysicsWorld w; REQUIRE(w.initialize(PhysicsConfig{}));
    ShapeHandle wall = w.create_shape(BoxShapeDesc{glm::vec3{1.0f}});
    RigidBodyDesc d{}; d.shape = wall; d.motion_type = BodyMotionType::Static;
    d.position_ws = {0.0, 0.0, -3.0}; (void)w.create_body(d);

    std::array<RaycastInput, 4> inputs{
        RaycastInput{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, 100.0f},
        RaycastInput{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f,  0.0f}, 100.0f},  // miss
        RaycastInput{{0.0f, 0.0f, 0.0f}, {0.0f, 1.0f,  0.0f}, 100.0f},  // miss
        RaycastInput{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, 1.0f},    // too short
    };
    std::array<RaycastHit, 4> hits{};
    std::array<std::uint8_t, 4> flags{};
    RaycastBatch batch{inputs, hits, flags};
    w.raycast_batch(batch, QueryFilter{});
    CHECK(flags[0] == 1);
    CHECK(flags[1] == 0);
    CHECK(flags[2] == 0);
    CHECK(flags[3] == 0);
}

TEST_CASE("raycast_any returns (deterministically) a valid hit when any exists") {
    PhysicsWorld w; REQUIRE(w.initialize(PhysicsConfig{}));
    ShapeHandle wall = w.create_shape(BoxShapeDesc{glm::vec3{1.0f}});
    RigidBodyDesc d{}; d.shape = wall; d.motion_type = BodyMotionType::Static;
    d.position_ws = {0.0, 0.0, -3.0}; d.entity = 99;
    (void)w.create_body(d);
    RaycastHit hit{};
    RaycastInput ray{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, 100.0f};
    REQUIRE(w.raycast_any(ray, QueryFilter{}, hit));
    CHECK(hit.entity == 99);
}
