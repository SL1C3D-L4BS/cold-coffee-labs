// engine/physics/ecs/physics_systems.cpp — Phase 12 (ADR-0032).

#include "engine/physics/ecs/physics_systems.hpp"

namespace gw::physics {

namespace {

void ensure_body(PhysicsWorld& world, RigidBodyComponent& comp) {
    if (comp.handle.valid() && !comp.dirty) return;
    if (comp.handle.valid()) {
        world.destroy_body(comp.handle);
        comp.handle = BodyHandle{};
    }
    comp.handle = world.create_body(comp.desc);
    comp.dirty = false;
}

} // namespace

// ---------------------------------------------------------------------------
// RigidBody
// ---------------------------------------------------------------------------
void rigid_body_system_materialize(PhysicsWorld& world,
                                   std::span<RigidBodyComponent> bodies) {
    for (auto& c : bodies) {
        ensure_body(world, c);
    }
}

void rigid_body_system_push_to_world(PhysicsWorld& world,
                                     std::span<const RigidBodyComponent> bodies,
                                     std::span<const BridgeTransform>     transforms) {
    const std::size_t n = std::min(bodies.size(), transforms.size());
    for (std::size_t i = 0; i < n; ++i) {
        if (!bodies[i].handle.valid()) continue;
        if (bodies[i].desc.motion_type == BodyMotionType::Kinematic ||
            bodies[i].desc.motion_type == BodyMotionType::Static) {
            world.set_body_transform(bodies[i].handle, transforms[i].position_ws, transforms[i].rotation_ws);
        }
    }
}

void rigid_body_system_pull_from_world(const PhysicsWorld& world,
                                       std::span<const RigidBodyComponent>    bodies,
                                       std::span<BridgeTransform>             transforms_out,
                                       std::span<PhysicsVelocityComponent>    velocities_out) {
    const std::size_t n = bodies.size();
    for (std::size_t i = 0; i < n; ++i) {
        if (!bodies[i].handle.valid()) continue;
        const BodyState s = world.body_state(bodies[i].handle);
        if (i < transforms_out.size()) {
            transforms_out[i].position_ws = s.position_ws;
            transforms_out[i].rotation_ws = s.rotation_ws;
        }
        if (i < velocities_out.size()) {
            velocities_out[i].linear  = s.linear_velocity;
            velocities_out[i].angular = s.angular_velocity;
        }
    }
}

// ---------------------------------------------------------------------------
// Character
// ---------------------------------------------------------------------------
void character_system_materialize(PhysicsWorld& world,
                                  std::span<CharacterComponent> characters) {
    for (auto& c : characters) {
        if (c.handle.valid() && !c.dirty) continue;
        if (c.handle.valid()) {
            world.destroy_character(c.handle);
            c.handle = CharacterHandle{};
        }
        c.handle = world.create_character(c.desc);
        c.dirty = false;
    }
}

void character_system_push_input(PhysicsWorld& world,
                                 std::span<const CharacterComponent> characters,
                                 std::span<const CharacterInput>     inputs) {
    const std::size_t n = std::min(characters.size(), inputs.size());
    for (std::size_t i = 0; i < n; ++i) {
        if (!characters[i].handle.valid()) continue;
        world.set_character_input(characters[i].handle, inputs[i]);
    }
}

void character_system_pull_transform(PhysicsWorld& world,
                                     std::span<CharacterComponent> characters,
                                     std::span<BridgeTransform>    transforms_out) {
    for (std::size_t i = 0; i < characters.size(); ++i) {
        if (!characters[i].handle.valid()) continue;
        const CharacterState s = world.character_state(characters[i].handle);
        characters[i].last_state = s;
        if (i < transforms_out.size()) {
            transforms_out[i].position_ws = s.position_ws;
            transforms_out[i].rotation_ws = s.rotation_ws;
        }
    }
}

// ---------------------------------------------------------------------------
// Triggers
// ---------------------------------------------------------------------------
void trigger_system_materialize(PhysicsWorld& world,
                                std::span<TriggerComponent> triggers,
                                std::span<const BridgeTransform> transforms,
                                std::span<const RigidBodyDesc>   descs) {
    const std::size_t n = triggers.size();
    for (std::size_t i = 0; i < n; ++i) {
        auto& t = triggers[i];
        if (t.handle.valid() && !t.dirty) continue;
        if (t.handle.valid()) {
            world.destroy_body(t.handle);
            t.handle = BodyHandle{};
        }
        RigidBodyDesc desc = (i < descs.size()) ? descs[i] : RigidBodyDesc{};
        desc.layer = ObjectLayer::Trigger;
        desc.is_sensor = true;
        desc.motion_type = BodyMotionType::Static;
        if (i < transforms.size()) {
            desc.position_ws = transforms[i].position_ws;
            desc.rotation_ws = transforms[i].rotation_ws;
        }
        t.handle = world.create_body(desc);
        t.dirty = false;
    }
}

} // namespace gw::physics
