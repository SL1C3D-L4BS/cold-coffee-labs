// tests/unit/physics/constraints_test.cpp — Phase 12 Wave 12C.

#include <doctest/doctest.h>

#include "engine/physics/physics_world.hpp"

using namespace gw::physics;

namespace {

std::pair<BodyHandle, BodyHandle> pair_of_dynamics(PhysicsWorld& w) {
    ShapeHandle s = w.create_shape(SphereShapeDesc{0.1f});
    RigidBodyDesc d{}; d.shape = s; d.motion_type = BodyMotionType::Dynamic; d.entity = 1;
    BodyHandle a = w.create_body(d);
    d.entity = 2;
    d.position_ws = {1.0, 0.0, 0.0};
    BodyHandle b = w.create_body(d);
    return {a, b};
}

} // namespace

TEST_CASE("All six constraint descriptors materialize handles") {
    PhysicsWorld w; REQUIRE(w.initialize(PhysicsConfig{}));
    auto [a, b] = pair_of_dynamics(w);

    FixedConstraintDesc fc{}; fc.a = a; fc.b = b;
    CHECK(w.create_constraint(fc).valid());

    DistanceConstraintDesc dc{}; dc.a = a; dc.b = b; dc.rest_length_m = 1.0f;
    CHECK(w.create_constraint(dc).valid());

    HingeConstraintDesc hc{}; hc.a = a; hc.b = b;
    CHECK(w.create_constraint(hc).valid());

    SliderConstraintDesc sc{}; sc.a = a; sc.b = b;
    CHECK(w.create_constraint(sc).valid());

    ConeConstraintDesc cc{}; cc.a = a; cc.b = b;
    CHECK(w.create_constraint(cc).valid());

    SixDOFConstraintDesc six{}; six.a = a; six.b = b;
    CHECK(w.create_constraint(six).valid());
}

TEST_CASE("Constraint destruction recycles the handle id") {
    PhysicsWorld w; REQUIRE(w.initialize(PhysicsConfig{}));
    auto [a, b] = pair_of_dynamics(w);
    FixedConstraintDesc fc{}; fc.a = a; fc.b = b;
    ConstraintHandle h1 = w.create_constraint(fc);
    ConstraintHandle h2 = w.create_constraint(fc);
    w.destroy_constraint(h1);
    ConstraintHandle h3 = w.create_constraint(fc);
    CHECK(h3.id == h1.id);
    CHECK(h3.id != h2.id);
}

TEST_CASE("ConstraintBroken event fires when break_impulse is exceeded") {
    PhysicsConfig cfg{};
    cfg.gravity = glm::vec3{0.0f};
    PhysicsWorld w; REQUIRE(w.initialize(cfg));
    auto [a, b] = pair_of_dynamics(w);

    DistanceConstraintDesc dc{};
    dc.a = a; dc.b = b;
    dc.rest_length_m = 1.0f;
    dc.break_impulse = 2.0f;  // Ns
    (void)w.create_constraint(dc);

    // Pull the bodies apart hard — relative velocity > 2 m/s.
    w.set_body_linear_velocity(a, {-10.0f, 0.0f, 0.0f});
    w.set_body_linear_velocity(b, { 10.0f, 0.0f, 0.0f});

    int broken = 0;
    w.bus_constraint_broken().subscribe([&](const gw::events::ConstraintBroken& e) {
        (void)e; ++broken;
    });
    w.step_fixed();
    CHECK(broken == 1);
}

TEST_CASE("Vehicle descriptor materializes and accepts input") {
    PhysicsWorld w; REQUIRE(w.initialize(PhysicsConfig{}));
    auto [chassis, _] = pair_of_dynamics(w);
    (void)_;

    VehicleDesc vd{};
    vd.chassis = chassis;
    VehicleHandle v = w.create_vehicle(vd);
    CHECK(v.valid());

    VehicleInput in{1.0f, 0.25f, 0.0f, 0.0f};
    w.set_vehicle_input(v, in);  // should not crash
    w.destroy_vehicle(v);
}
