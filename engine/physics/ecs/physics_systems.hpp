#pragma once
// engine/physics/ecs/physics_systems.hpp — Phase 12 (ADR-0032).
//
// Bridge between the ECS World and the physics facade. These systems are
// plain functions over parallel arrays so they slot into any archetype
// walk the ECS exposes (Phase-7 gw::ecs::World).

#include "engine/physics/ecs/physics_components.hpp"
#include "engine/physics/physics_world.hpp"

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstddef>
#include <span>

namespace gw::physics {

// A minimal transform structure so the physics bridge does not pull the
// scene/transform_system header (header quarantine non-negotiable).
struct BridgeTransform {
    glm::dvec3 position_ws{0.0};
    glm::quat  rotation_ws{1.0f, 0.0f, 0.0f, 0.0f};
};

// ---------------------------------------------------------------------------
// RigidBodySystem — materialize / destroy bodies and sync transforms both
// ways between the ECS and the physics world.
// ---------------------------------------------------------------------------
void rigid_body_system_materialize(PhysicsWorld& world,
                                   std::span<RigidBodyComponent> bodies);

void rigid_body_system_push_to_world(PhysicsWorld& world,
                                     std::span<const RigidBodyComponent> bodies,
                                     std::span<const BridgeTransform>     transforms);

void rigid_body_system_pull_from_world(const PhysicsWorld& world,
                                       std::span<const RigidBodyComponent>    bodies,
                                       std::span<BridgeTransform>             transforms_out,
                                       std::span<PhysicsVelocityComponent>    velocities_out);

// ---------------------------------------------------------------------------
// CharacterSystem — updates character input from gameplay + pulls transform.
// ---------------------------------------------------------------------------
void character_system_materialize(PhysicsWorld& world,
                                  std::span<CharacterComponent> characters);

void character_system_push_input(PhysicsWorld& world,
                                 std::span<const CharacterComponent> characters,
                                 std::span<const CharacterInput>     inputs);

void character_system_pull_transform(PhysicsWorld& world,
                                     std::span<CharacterComponent> characters,
                                     std::span<BridgeTransform>    transforms_out);

// ---------------------------------------------------------------------------
// TriggerSystem — materializes sensor bodies; events flow through the bus.
// ---------------------------------------------------------------------------
void trigger_system_materialize(PhysicsWorld& world,
                                std::span<TriggerComponent> triggers,
                                std::span<const BridgeTransform> transforms,
                                std::span<const RigidBodyDesc>   descs);

} // namespace gw::physics
