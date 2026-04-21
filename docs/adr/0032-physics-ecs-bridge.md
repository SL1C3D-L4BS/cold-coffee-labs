# ADR 0032 — Physics ↔ ECS bridge — components, transform sync, interpolation

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 12 Wave 12A)
- **Deciders:** Cold Coffee Labs engine + ECS group
- **Relates:** ADR-0004 (ECS World), ADR-0008 (TransformComponent with `dvec3`), ADR-0031 (physics facade), ADR-0037 (determinism contract)

## Context

Every physics-driven entity needs a stable mapping from `ecs::EntityHandle` ↔ `physics::BodyHandle`, with a canonical phase-ordering rule so the render thread sees a consistent transform. The `TransformComponent` uses `dvec3` positions (ADR-0008) to keep planet-scale scenes jitter-free; physics must accept + return the same precision without rounding every tick.

## Decision

### 1. ECS components (new, Phase 12)

```cpp
namespace gw::ecs::physics {

struct RigidBodyComponent {
    physics::BodyHandle body{};          // opaque; 0 = not yet created
    physics::BodyMotionType motion{physics::BodyMotionType::Dynamic};
    float mass_kg{1.0f};
    float linear_damping{0.05f};
    float angular_damping{0.05f};
    float friction{0.5f};
    float restitution{0.1f};
    physics::ObjectLayer layer{physics::ObjectLayer::Dynamic};
    bool  is_sensor{false};              // trigger when true
};

struct ColliderComponent {
    physics::ShapeHandle shape{};
    // Authoring offset applied at body-creation only.
    glm::vec3  local_offset{0};
    glm::quat  local_rot{1, 0, 0, 0};
};

struct CharacterComponent {
    physics::CharacterHandle ctrl{};
    float capsule_half_height{0.9f};
    float capsule_radius{0.3f};
    float step_up_max{0.3f};
    float slope_max_rad{0.872665f};      // 50°
    float jump_impulse_mps{5.0f};
};

struct PhysicsConstraintComponent {
    physics::ConstraintHandle joint{};
    // Authoring lives in `constraint.hpp` (Fixed / Distance / Hinge / Slider / Cone / SixDOF).
};

struct PhysicsVelocityComponent {
    glm::vec3 linear_mps{0.0f};
    glm::vec3 angular_rps{0.0f};
};

} // namespace gw::ecs::physics
```

`RigidBodyComponent` ownership: whoever adds the component is responsible for calling `rigid_body_system::materialize(world, entity)` during the next `physics_system::tick()` — the body is cooked lazily so the world can batch-add on the first substep.

### 2. Transform sync direction

Phase-ordering rule (enforced in `runtime::Engine::tick`):

```
1. input
2. physics.step(dt)                       ← writes body state forward one fixed dt
3. rigid_body_system::write_back_transforms(world, alpha)
   - Dynamic bodies: TransformComponent ← lerp(prev_pose, curr_pose, alpha)
   - Kinematic bodies: body target ← TransformComponent (pre-step path next frame)
4. ecs::tick()                            ← gameplay systems read post-physics transforms
5. ui.update
6. audio.update
7. ui.render
```

For `dvec3` world-space positions, the body's float-precision local position is relative to a per-body "anchor" stored alongside the handle. The anchor tracks the chunk origin (Phase 20 floating-origin) but in Phase 12 every anchor is origin (`dvec3(0,0,0)`) so body positions and `TransformComponent::world_position` are numerically equal modulo `float` vs. `double` precision.

Kinematic + trigger bodies are allowed to push transforms *into* the physics world; dynamic bodies are pull-only from ECS to physics except at initial spawn.

### 3. Interpolation alpha

`PhysicsWorld::interpolation_alpha()` returns the fraction of `physics.fixed_dt` accumulated but not yet simulated. The ECS bridge lerps linear positions, slerps orientations, and lerps linear/angular velocities between `prev` and `curr` substep state. In deterministic / headless mode `alpha == 0` is enforced so render output is a pure function of frame index.

### 4. Lazy materialization + deferred destruction

`physics_system::tick(World& w, PhysicsWorld& pw)` runs in two passes:

1. **Create** — iterate entities that have `RigidBodyComponent` with `body == 0` or `ColliderComponent` with `shape == 0`; create shape first, then body; store handle on the component.
2. **Destroy** — iterate `RigidBodyComponent`s flagged `pending_destroy` (set when the entity is despawned) and issue `pw.destroy_body(...)`. Destruction is deferred one frame so late subscribers still see the final `PhysicsContactRemoved`.

### 5. Systems ordering

```
physics_system::pre_step()          (ECS → body targets for kinematic)
physics_world.step(dt)              (fixed-step integrate)
physics_system::post_step(alpha)    (body → ECS write-back)
trigger_system::drain_events()      (TriggerEntered / TriggerExited → EventBus)
character_system::drain_events()    (CharacterGrounded → EventBus)
```

All five are registered through `ecs::SystemRegistry`; ordering is explicit via `register_after(...)` dependency edges rather than a global order list. (Matches ADR-0004 §6.)

### 6. Jobs granularity

Only `physics_world.step(dt)` is permitted to fan out into the job system (via `JoltJobBridge`). The ECS bridge itself runs on the main thread and iterates archetype chunks sequentially — the cost is bounded by **transform sync ≤ 0.25 ms for 2 048 bodies** (Phase-12 perf gate §7 of the plan).

## Consequences

- Gameplay code never calls `PhysicsWorld` directly; it edits components and lets the bridge do the push/pull.
- Phase 13 animation writes bone poses into kinematic ragdoll-part bodies via the same pre-step path.
- Phase 14 networking replicates `TransformComponent`+`PhysicsVelocityComponent` and re-derives body state on the client with rewind-and-replay.
- Floating origin (Phase 20) rebases the per-body anchor at a chunk boundary; `TransformComponent::world_position` stays `dvec3` and is untouched.

## Alternatives considered

- **Storing body handles in a side-table keyed by `EntityHandle`.** Rejected — a component is the ECS-native home and preserves archetype locality.
- **Writing body poses directly into `TransformComponent` every substep.** Rejected — shreds render interpolation and makes partial substeps non-deterministic per-frame.
- **Hard-realising bodies immediately on component add.** Rejected — makes scene-load a single-threaded blocker; lazy materialization batches.
