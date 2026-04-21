// tests/integration/physics/phase12_exit_gate_test.cpp — Phase 12 closeout.

#include <doctest/doctest.h>

#include "engine/physics/physics_world.hpp"

using namespace gw::physics;

namespace {

std::uint64_t run_stack_scene(int frames) {
    PhysicsConfig cfg{};
    cfg.fixed_dt = 1.0f / 60.0f;
    cfg.gravity = glm::vec3{0.0f, -9.81f, 0.0f};
    PhysicsWorld w; REQUIRE(w.initialize(cfg));

    // Static floor.
    ShapeHandle fs = w.create_shape(BoxShapeDesc{glm::vec3{10.0f, 0.1f, 10.0f}});
    RigidBodyDesc floor{}; floor.shape = fs; floor.motion_type = BodyMotionType::Static;
    floor.entity = 1;
    (void)w.create_body(floor);

    // Three stacked boxes with slightly offset entities.
    ShapeHandle box = w.create_shape(BoxShapeDesc{glm::vec3{0.5f}});
    for (int i = 0; i < 3; ++i) {
        RigidBodyDesc d{};
        d.shape = box;
        d.motion_type = BodyMotionType::Dynamic;
        d.position_ws = glm::dvec3{0.0, 2.0 + 1.05 * i, 0.0};
        d.entity = static_cast<EntityId>(10 + i);
        (void)w.create_body(d);
    }

    for (int i = 0; i < frames; ++i) w.step_fixed();
    return w.determinism_hash();
}

} // namespace

TEST_CASE("Phase 12 exit gate: identical scenes produce identical hashes") {
    const auto a = run_stack_scene(180);
    const auto b = run_stack_scene(180);
    CHECK(a == b);
}

TEST_CASE("Phase 12 exit gate: determinism_hash varies between two different scenes") {
    // Different world seeds / gravities -> different post-step hashes.
    PhysicsConfig cfg_a{}; cfg_a.gravity = glm::vec3{0.0f, -9.81f, 0.0f};
    PhysicsConfig cfg_b{}; cfg_b.gravity = glm::vec3{0.0f, -4.00f, 0.0f};

    auto run_one = [](const PhysicsConfig& cfg) {
        PhysicsWorld w; REQUIRE(w.initialize(cfg));
        ShapeHandle s = w.create_shape(SphereShapeDesc{0.5f});
        RigidBodyDesc d{}; d.shape = s; d.motion_type = BodyMotionType::Dynamic;
        d.position_ws = glm::dvec3{0.0, 10.0, 0.0};
        (void)w.create_body(d);
        for (int i = 0; i < 60; ++i) w.step_fixed();
        return w.determinism_hash();
    };
    CHECK(run_one(cfg_a) != run_one(cfg_b));
}
