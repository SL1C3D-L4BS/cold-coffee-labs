// tests/unit/physics/ecs_bridge_test.cpp — Phase 12 Wave 12A (ADR-0032).

#include <doctest/doctest.h>

#include "engine/physics/ecs/physics_systems.hpp"
#include "engine/physics/physics_world.hpp"

#include <array>
#include <span>

using namespace gw::physics;

TEST_CASE("rigid_body_system_materialize creates backend handles") {
    PhysicsWorld w; REQUIRE(w.initialize(PhysicsConfig{}));
    ShapeHandle s = w.create_shape(BoxShapeDesc{glm::vec3{0.5f}});

    std::array<RigidBodyComponent, 3> comps{};
    for (auto& c : comps) { c.desc.shape = s; c.desc.motion_type = BodyMotionType::Dynamic; c.dirty = true; }

    rigid_body_system_materialize(w, comps);
    for (const auto& c : comps) CHECK(c.handle.valid());
    CHECK(w.body_count() == 3);
}

TEST_CASE("rigid_body_system_push_to_world moves kinematic bodies") {
    PhysicsWorld w; REQUIRE(w.initialize(PhysicsConfig{}));
    ShapeHandle s = w.create_shape(BoxShapeDesc{glm::vec3{0.5f}});

    RigidBodyComponent c{};
    c.desc.shape = s;
    c.desc.motion_type = BodyMotionType::Kinematic;
    c.dirty = true;
    std::array<RigidBodyComponent, 1> comps{c};
    rigid_body_system_materialize(w, comps);

    BridgeTransform t{};
    t.position_ws = {4.0, 5.0, 6.0};
    std::array<BridgeTransform, 1> tx{t};
    rigid_body_system_push_to_world(w, comps, tx);

    const auto st = w.body_state(comps[0].handle);
    CHECK(st.position_ws.x == doctest::Approx(4.0));
    CHECK(st.position_ws.y == doctest::Approx(5.0));
    CHECK(st.position_ws.z == doctest::Approx(6.0));
}

TEST_CASE("rigid_body_system_pull_from_world syncs transforms + velocities") {
    PhysicsConfig cfg{};
    cfg.gravity = glm::vec3{0.0f};
    PhysicsWorld w; REQUIRE(w.initialize(cfg));
    ShapeHandle s = w.create_shape(SphereShapeDesc{0.1f});

    RigidBodyComponent c{};
    c.desc.shape = s; c.desc.motion_type = BodyMotionType::Dynamic;
    c.desc.linear_velocity = glm::vec3{3.0f, 0.0f, 0.0f};
    c.dirty = true;
    std::array<RigidBodyComponent, 1> comps{c};
    rigid_body_system_materialize(w, comps);

    w.step_fixed();

    std::array<BridgeTransform, 1>         out_tx{};
    std::array<PhysicsVelocityComponent, 1> out_vel{};
    rigid_body_system_pull_from_world(w, comps, out_tx, out_vel);

    CHECK(out_tx[0].position_ws.x > 0.0);
    CHECK(out_vel[0].linear.x > 0.0f);
}

TEST_CASE("character_system_materialize creates handles; pull_transform syncs pose") {
    PhysicsWorld w; REQUIRE(w.initialize(PhysicsConfig{}));

    CharacterComponent c{};
    c.desc.position_ws = {10.0, 0.0, 0.0};
    c.dirty = true;
    std::array<CharacterComponent, 1> comps{c};
    character_system_materialize(w, comps);
    CHECK(comps[0].handle.valid());

    std::array<BridgeTransform, 1> out_tx{};
    character_system_pull_transform(w, comps, out_tx);
    CHECK(out_tx[0].position_ws.x == doctest::Approx(10.0));
}

TEST_CASE("trigger_system_materialize marks bodies as sensors") {
    PhysicsWorld w; REQUIRE(w.initialize(PhysicsConfig{}));
    ShapeHandle s = w.create_shape(BoxShapeDesc{glm::vec3{1.0f}});

    std::array<TriggerComponent, 1>   triggers{};
    std::array<BridgeTransform, 1>    transforms{};
    RigidBodyDesc d{}; d.shape = s;
    std::array<RigidBodyDesc, 1>      descs{d};

    trigger_system_materialize(w, triggers, transforms, descs);
    CHECK(triggers[0].handle.valid());
    CHECK(w.body_count() == 1);
}
