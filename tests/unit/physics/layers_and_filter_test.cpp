// tests/unit/physics/layers_and_filter_test.cpp — Phase 12 Wave 12D.

#include <doctest/doctest.h>

#include "engine/physics/physics_world.hpp"

using namespace gw::physics;

TEST_CASE("layer_bit produces 1-hot masks for all object layers") {
    CHECK(layer_bit(ObjectLayer::Static)    == 0x0001u);
    CHECK(layer_bit(ObjectLayer::Dynamic)   == 0x0002u);
    CHECK(layer_bit(ObjectLayer::Trigger)   == 0x0004u);
    CHECK(layer_bit(ObjectLayer::Character) == 0x0008u);
}

TEST_CASE("QueryFilter layer_mask suppresses misses on excluded layers") {
    PhysicsWorld w; REQUIRE(w.initialize(PhysicsConfig{}));
    ShapeHandle s = w.create_shape(BoxShapeDesc{glm::vec3{1.0f, 1.0f, 1.0f}});

    RigidBodyDesc sa{}; sa.shape = s; sa.motion_type = BodyMotionType::Static;
    sa.layer = ObjectLayer::Static;  sa.position_ws = {0.0, 0.0, -5.0};
    (void)w.create_body(sa);

    RigidBodyDesc sb = sa; sb.layer = ObjectLayer::Dynamic; sb.position_ws = {0.0, 0.0, -8.0};
    (void)w.create_body(sb);

    RaycastInput ray{};
    ray.origin_ws = {0.0f, 0.0f, 0.0f};
    ray.direction_ws = {0.0f, 0.0f, -1.0f};
    ray.max_distance_m = 100.0f;

    QueryFilter f_all{};  // defaults to kLayerMaskAll -> hits the first (nearest) wall
    RaycastHit hit{};
    REQUIRE(w.raycast_closest(ray, f_all, hit));
    CHECK(hit.point_ws.z > -6.0f);  // first wall at z=-5 (+/- half-extent)

    QueryFilter f_dyn_only{};
    f_dyn_only.layer_mask = layer_bit(ObjectLayer::Dynamic);
    RaycastHit hit2{};
    REQUIRE(w.raycast_closest(ray, f_dyn_only, hit2));
    CHECK(hit2.point_ws.z < -6.0f);  // second wall at z=-8 only when static excluded
}

TEST_CASE("QueryFilter.exclude skips the self body") {
    PhysicsWorld w; REQUIRE(w.initialize(PhysicsConfig{}));
    ShapeHandle s = w.create_shape(BoxShapeDesc{glm::vec3{1.0f}});

    RigidBodyDesc d{}; d.shape = s; d.motion_type = BodyMotionType::Static;
    d.position_ws = {0.0, 0.0, -3.0};
    BodyHandle near_body = w.create_body(d);

    d.position_ws = {0.0, 0.0, -8.0};
    (void)w.create_body(d);

    RaycastInput ray{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, 100.0f};

    QueryFilter f{};
    f.exclude = near_body;
    RaycastHit hit{};
    REQUIRE(w.raycast_closest(ray, f, hit));
    CHECK(hit.point_ws.z < -6.0f);
}

TEST_CASE("QueryFilter.predicate rejects hits by EntityId") {
    PhysicsWorld w; REQUIRE(w.initialize(PhysicsConfig{}));
    ShapeHandle s = w.create_shape(BoxShapeDesc{glm::vec3{1.0f}});

    RigidBodyDesc d{}; d.shape = s; d.motion_type = BodyMotionType::Static;
    d.entity = 0x1001; d.position_ws = {0.0, 0.0, -3.0};
    (void)w.create_body(d);

    d.entity = 0x1002; d.position_ws = {0.0, 0.0, -8.0};
    (void)w.create_body(d);

    RaycastInput ray{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, 100.0f};
    QueryFilter f{};
    f.predicate = [](EntityId id) { return id == 0x1002; };
    RaycastHit hit{};
    REQUIRE(w.raycast_closest(ray, f, hit));
    CHECK(hit.entity == 0x1002);
    CHECK(hit.point_ws.z < -6.0f);
}
