// tests/unit/physics/debug_draw_test.cpp — Phase 12 Wave 12D.

#include <doctest/doctest.h>

#include "engine/physics/debug_draw.hpp"
#include "engine/physics/physics_world.hpp"

using namespace gw::physics;

TEST_CASE("DebugDrawFlag bit encoding matches ADR-0036") {
    CHECK(debug_flag_bit(DebugDrawFlag::Bodies)      == 0x01u);
    CHECK(debug_flag_bit(DebugDrawFlag::Contacts)    == 0x02u);
    CHECK(debug_flag_bit(DebugDrawFlag::BroadPhase)  == 0x04u);
    CHECK(debug_flag_bit(DebugDrawFlag::Character)   == 0x08u);
    CHECK(debug_flag_bit(DebugDrawFlag::Constraints) == 0x10u);
    CHECK(debug_flag_bit(DebugDrawFlag::Queries)     == 0x20u);
    CHECK(debug_flag_bit(DebugDrawFlag::All)         == 0x3Fu);
}

TEST_CASE("DebugDrawSink respects max_lines cap") {
    DebugDrawSink sink;
    sink.set_max_lines(3);
    sink.push_line({});
    sink.push_line({});
    sink.push_line({});
    CHECK_FALSE(sink.can_push_line());
    CHECK(sink.lines().size() == 3u);
    sink.clear();
    CHECK(sink.lines().empty());
    CHECK(sink.flag_mask() == 0u);
}

TEST_CASE("fill_debug_draw emits lines when Bodies flag is set") {
    PhysicsWorld w; REQUIRE(w.initialize(PhysicsConfig{}));
    ShapeHandle s = w.create_shape(BoxShapeDesc{glm::vec3{1.0f}});
    RigidBodyDesc d{}; d.shape = s; d.motion_type = BodyMotionType::Dynamic;
    (void)w.create_body(d);

    DebugDrawSink sink;
    sink.set_flag_mask(debug_flag_bit(DebugDrawFlag::Bodies));
    sink.set_max_lines(128);
    w.fill_debug_draw(sink);
    // 12 edges per AABB.
    CHECK(sink.lines().size() >= 12u);
}

TEST_CASE("fill_debug_draw is a no-op when the mask is zero") {
    PhysicsWorld w; REQUIRE(w.initialize(PhysicsConfig{}));
    ShapeHandle s = w.create_shape(BoxShapeDesc{glm::vec3{1.0f}});
    RigidBodyDesc d{}; d.shape = s; (void)w.create_body(d);

    DebugDrawSink sink;  // flag_mask default 0
    w.fill_debug_draw(sink);
    CHECK(sink.lines().empty());
}
