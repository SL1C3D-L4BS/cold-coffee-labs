#pragma once
// engine/physics/rigid_body.hpp — Phase 12 Wave 12A (ADR-0031, ADR-0032).
//
// Rigid-body authoring descriptor + the ECS component that wraps a BodyHandle.

#include "engine/physics/physics_types.hpp"

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstdint>

namespace gw::physics {

struct RigidBodyDesc {
    ShapeHandle     shape{};
    BodyMotionType  motion_type{BodyMotionType::Dynamic};
    ObjectLayer     layer{ObjectLayer::Dynamic};

    glm::dvec3      position_ws{0.0};                            // world-space (Phase-5 double transform convention)
    glm::quat       rotation_ws{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3       linear_velocity{0.0f};
    glm::vec3       angular_velocity{0.0f};

    float           mass_kg{1.0f};                               // only used if motion_type == Dynamic
    float           friction{0.5f};
    float           restitution{0.0f};
    float           linear_damping{0.05f};
    float           angular_damping{0.05f};
    float           gravity_factor{1.0f};

    bool            is_sensor{false};                            // contact resolution off, triggers still fire
    bool            allow_sleeping{true};
    bool            continuous_collision{false};                 // swept CCD for fast projectiles

    EntityId        entity{kEntityNone};                         // optional: feed events::EntityId on contact events
};

// Mirror of a body's dynamic state — returned by PhysicsWorld::body_state().
struct BodyState {
    glm::dvec3      position_ws{0.0};
    glm::quat       rotation_ws{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3       linear_velocity{0.0f};
    glm::vec3       angular_velocity{0.0f};
    BodyMotionType  motion_type{BodyMotionType::Static};
    bool            awake{false};
};

// -----------------------------------------------------------------------------
// ECS component (ADR-0032). Stored on entities that own a physics body.
// The RigidBodySystem materializes this into a backend body lazily.
// -----------------------------------------------------------------------------
struct RigidBodyComponent {
    RigidBodyDesc   desc{};       // authoring data
    BodyHandle      handle{};     // back-reference (populated by RigidBodySystem)
    bool            dirty{true};  // bump to re-materialize after shape/motion change
};

// Companion component — read-only cache of the simulation's linear / angular
// velocity for gameplay code that wants to inspect motion without calling back
// into the physics world.
struct PhysicsVelocityComponent {
    glm::vec3 linear{0.0f};
    glm::vec3 angular{0.0f};
};

} // namespace gw::physics
