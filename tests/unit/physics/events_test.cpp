// tests/unit/physics/events_test.cpp — Phase 12 Wave 12A/12D.

#include <doctest/doctest.h>

#include "engine/physics/events_physics.hpp"
#include "engine/physics/physics_world.hpp"

using namespace gw::physics;

TEST_CASE("ContactAdded fires once per new pair, ContactPersisted for overlap frames") {
    PhysicsConfig cfg{};
    cfg.gravity = glm::vec3{0.0f};       // keep dynamics still until we position them
    PhysicsWorld w; REQUIRE(w.initialize(cfg));

    ShapeHandle s = w.create_shape(BoxShapeDesc{glm::vec3{0.5f}});
    // Kinematic bodies: overlap does not push them apart, so the contact
    // pair persists across steps (dynamic bodies would separate).
    RigidBodyDesc d{}; d.shape = s; d.motion_type = BodyMotionType::Kinematic; d.entity = 1;
    (void)w.create_body(d);
    d.entity = 2; d.position_ws = {0.5, 0.0, 0.0};       // deep overlap
    (void)w.create_body(d);

    int added     = 0;
    int persisted = 0;
    w.bus_contact_added().subscribe([&](const gw::events::PhysicsContactAdded&)     { ++added; });
    w.bus_contact_persisted().subscribe([&](const gw::events::PhysicsContactPersisted&){ ++persisted; });

    w.step_fixed();
    CHECK(added == 1);
    CHECK(persisted == 0);

    w.step_fixed();
    CHECK(added == 1);
    CHECK(persisted >= 1);
}

TEST_CASE("TriggerEntered fires when a sensor pair overlaps") {
    PhysicsConfig cfg{}; cfg.gravity = glm::vec3{0.0f};
    PhysicsWorld w; REQUIRE(w.initialize(cfg));

    ShapeHandle s = w.create_shape(BoxShapeDesc{glm::vec3{0.5f}});
    RigidBodyDesc trig{}; trig.shape = s; trig.motion_type = BodyMotionType::Static;
    trig.layer = ObjectLayer::Trigger; trig.is_sensor = true; trig.entity = 777;
    (void)w.create_body(trig);

    RigidBodyDesc mover{}; mover.shape = s; mover.motion_type = BodyMotionType::Dynamic;
    mover.position_ws = {0.4, 0.0, 0.0}; mover.entity = 42;
    (void)w.create_body(mover);

    int triggered = 0;
    gw::physics::EntityId trig_id = 0;
    w.bus_trigger_entered().subscribe([&](const gw::events::TriggerEntered& e) {
        ++triggered; trig_id = e.trigger;
    });
    w.step_fixed();
    CHECK(triggered == 1);
    CHECK(trig_id == 777);
}

TEST_CASE("ContactRemoved fires when bodies separate") {
    PhysicsConfig cfg{}; cfg.gravity = glm::vec3{0.0f};
    PhysicsWorld w; REQUIRE(w.initialize(cfg));

    ShapeHandle s = w.create_shape(BoxShapeDesc{glm::vec3{0.5f}});
    RigidBodyDesc d{}; d.shape = s; d.motion_type = BodyMotionType::Dynamic;
    d.entity = 1; d.position_ws = {0.0, 0.0, 0.0};
    BodyHandle a = w.create_body(d);
    d.entity = 2; d.position_ws = {0.9, 0.0, 0.0};
    BodyHandle b = w.create_body(d);

    int removed = 0;
    w.bus_contact_removed().subscribe([&](const gw::events::PhysicsContactRemoved&) { ++removed; });

    w.step_fixed();   // touching
    w.set_body_transform(b, glm::dvec3{10.0, 0.0, 0.0}, glm::quat{1.0f, 0.0f, 0.0f, 0.0f});
    (void)a;
    w.step_fixed();   // separated
    CHECK(removed == 1);
}
