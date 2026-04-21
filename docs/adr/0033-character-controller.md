# ADR 0033 — Character controller model

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 12 Wave 12B)
- **Deciders:** Cold Coffee Labs engine + gameplay group
- **Relates:** ADR-0031 (physics facade), ADR-0032 (ECS bridge), ADR-0021 (input actions)

## Context

Greywater's first characters are humanoid survivors, hostile fauna, and small bipedal drones. A full rigid-body character is overkill and brittle (`T`-pose tipping on slopes). Jolt ships `JPH::CharacterVirtual` which is the industry pattern: a capsule swept through the world, resolving penetrations analytically. The null backend mirrors the analytical semantics with a capsule-against-AABB sweep.

## Decision

### 1. Model

| Property | Default | CVar |
|---|---|---|
| Capsule radius | 0.30 m | — (per-character component) |
| Capsule half-height | 0.90 m | — |
| Step-up max | 0.30 m | `physics.char.step_up_max` |
| Slope max | 50° (0.872665 rad) | `physics.char.slope_max_deg` |
| Stick-to-ground distance | 0.05 m | `physics.char.stick_dist` |
| Gravity mode | `World` (world gravity applied) | `physics.char.gravity_mode` (`world`/`custom`/`off`) |
| Jump coyote window | 150 ms | `physics.char.coyote_ms` |
| Ground probe rays | 1 downward + 4 skirt | — |

The character is a `JPH::CharacterVirtual` under Jolt; under the null backend it's a capsule-vs-AABB sweep with analytical depenetration on up to 4 contact manifolds per substep. Both paths return the same `CharacterStepResult` struct so gameplay sees one model.

### 2. Public API (shape)

```cpp
struct CharacterDesc {
    glm::vec3 initial_position{0};
    glm::quat initial_rotation{1, 0, 0, 0};
    float     capsule_radius{0.30f};
    float     capsule_half_height{0.90f};
    float     step_up_max{0.30f};
    float     slope_max_rad{0.872665f};
    float     jump_impulse_mps{5.0f};
    ObjectLayer layer{ObjectLayer::Character};
};

struct CharacterInput {
    glm::vec3 desired_velocity_mps{0};   // horizontal + vertical
    bool      jump_requested{false};
};

struct CharacterStepResult {
    glm::vec3 position_after{0};
    glm::quat rotation_after{1, 0, 0, 0};
    glm::vec3 velocity_after{0};
    bool      is_grounded{false};
    glm::vec3 ground_normal{0, 1, 0};
    std::uint32_t ground_surface_id{0};
    bool      touched_ceiling{false};
    bool      touched_wall{false};
};

class PhysicsWorld {
    CharacterHandle create_character(const CharacterDesc&);
    void            destroy_character(CharacterHandle);
    CharacterStepResult step_character(CharacterHandle, const CharacterInput&, float dt);
    void            set_character_position(CharacterHandle, glm::vec3 pos);
};
```

### 3. Ground detection + events

After each character sub-step:

- If `is_grounded == true` and the previous state was `false` → publish `events::CharacterGrounded{ entity, ground_normal, surface_id }` on the `InFrameQueue<CharacterGrounded>` owned by `PhysicsWorld`.
- `surface_id` is copied from `RigidBodyComponent::surface_id` or zero for geometry that has no dedicated surface ID.

`character_system::drain_events(World&, EventBus&)` pumps these into the per-event-type `EventBus<CharacterGrounded>` on the main thread.

### 4. Jump + coyote window

The controller keeps a `last_grounded_at_ms` timestamp. When `CharacterInput::jump_requested` is observed and `now_ms - last_grounded_at_ms <= physics.char.coyote_ms`, the upward velocity is set to `jump_impulse_mps` regardless of the *current* grounded state. This implements coyote-time without any per-frame bookkeeping on the gameplay side.

Jump peak height (for unit test):

```
h_peak = jump_impulse_mps² / (2 * |gravity|)
       = 5.0²    / (2 * 9.81)   ≈ 1.274 m
```

Wave-12B test `character_jump_peak_height` checks this within 1 cm.

### 5. Slope + step-up tests (unit)

- `character_climbs_30deg`: ramp at 30°, character walks up → position_z_after > position_z_before, grounded.
- `character_refuses_60deg`: ramp at 60° (> slope_max), character slides, does not ascend.
- `character_steps_up_0_3m`: step-up geometry at 0.3 m, character moves over; step-up at 0.31 m, character blocked.
- `character_gravity_mode_off`: `physics.char.gravity_mode = off`, character does not fall in idle.
- `character_wall_blocks`: facing a wall, desired velocity into wall → position_after unchanged on the wall axis.
- `character_grounded_event_fires_once_per_touch`: drop character onto ground; `CharacterGrounded` count == 1 across N frames.

### 6. Input bridge

The Phase-10 `ActionSet` emits `Move`, `Jump`, `Crouch`. The `character_system::apply_input(entity, action_state)` helper converts action deltas to a `CharacterInput`:

```
desired_velocity = rotate_by(Move.xy, yaw) * speed
                 + (Crouch ? -speed*0.3 : 0) (on Z)
jump_requested = Jump.edge
```

Speed constants are CVars:

- `physics.char.walk_speed_mps` default 3.5
- `physics.char.run_speed_mps` default 6.0
- `physics.char.crouch_speed_mps` default 1.5

## Consequences

- Characters never tip over or ragdoll unexpectedly on slopes; ragdolls are explicit and constraint-driven (ADR-0034).
- Coyote-time is built-in; gameplay code does not re-implement it.
- The same controller runs under the null backend; unit tests do not require Jolt.
- Phase 13's `CharacterAnimator` consumes `CharacterStepResult::velocity_after` + `is_grounded` directly.

## Alternatives considered

- **Rigid-body character with constraints.** Rejected — unpredictable on stairs; requires per-scene tuning.
- **Per-bone ragdoll as the primary locomotion model.** Rejected — ragdoll is a death + impact response; locomotion is analytical.
- **Capsule + step-up via a second capsule sweep.** Considered; Jolt's `CharacterVirtual` already handles this internally — we rely on it.
