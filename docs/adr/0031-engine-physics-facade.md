# ADR 0031 — engine/physics — facade, layers, fixed-step loop

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 12 Wave 12A)
- **Deciders:** Cold Coffee Labs engine group
- **Supersedes / relates:** ADR-0023 (events), ADR-0024 (CVars), ADR-0029 (runtime boot), CLAUDE.md non-negotiables #2, #3, #5, #6, #9, #10, #11

## Context

Phase 11 closed the Playable Runtime with a null event / CVar / UI / console / input / audio stack wired through `runtime::Engine`. Phase 12 layers deterministic rigid-body dynamics onto that stack. The *Living Scene* milestone in Phase 13 (animation + BT + navmesh) is the downstream consumer, so Phase 12 must produce:

1. a stable public C++23 surface that does **not** leak `<Jolt/**>` into any header under `engine/`,
2. a deterministic, fixed-step simulation loop,
3. an RAII owner graph with no hidden globals,
4. a null backend that compiles and ticks identically on Windows + Linux at the `dev-*` preset, and
5. a Jolt backend reachable from `physics-*` and `playable-*` presets, behind `GW_ENABLE_JOLT=ON`.

## Decision

### 1. PIMPL facade

`engine/physics/physics_world.hpp` declares a single public class `PhysicsWorld`. All implementation state lives in a concrete `Impl` class owned via `std::unique_ptr<IBackend>`. Two backends ship:

| Backend | Entry | Gate | Used by |
|---|---|---|---|
| Null | `make_null_backend()` | always available | `dev-*` preset CI, all unit tests that do not explicitly want Jolt |
| Jolt | `make_jolt_backend()` | `GW_PHYSICS_JOLT` | `physics-*` + `playable-*` presets |

`PhysicsWorld::PhysicsWorld(const PhysicsConfig&)` resolves the backend. When `GW_PHYSICS_JOLT=OFF` the Jolt factory is absent from the TU and the null backend is always chosen.

### 2. Object + broadphase layers

```cpp
enum class ObjectLayer : std::uint16_t {
    Static        = 0,
    Dynamic       = 1,
    Trigger       = 2,
    Character     = 3,
    Projectile    = 4,
    Debris        = 5,
    WaterVolume   = 6,
    UserA         = 7,  // reserved for gameplay authors
    UserB         = 8,
    // 9..15 reserved for future engine use
};
enum class BroadPhaseLayer : std::uint8_t {
    Static  = 0,
    Moving  = 1,
};
```

Layer-vs-layer collision matrix is a single `std::uint32_t` per `ObjectLayer` (one bit per opponent). `OBJECT_LAYER_BITS=16` in the Jolt CMake pin matches this; the 16-bit ceiling is intentional (matches `JPH::ObjectLayer` default and keeps the matrix comfortably cache-resident).

### 3. Lifecycle

`PhysicsWorld` is owned by `runtime::Engine::Impl` and constructed **after** `CVarRegistry` is seeded (so gravity / solver iterations / sleep thresholds are read from standard CVars) and **before** any ECS system that wants to spawn bodies. Destruction runs in reverse. No `init()`/`shutdown()` pair exists on the public API — CLAUDE.md non-negotiable #6.

### 4. Fixed-step simulation

`PhysicsWorld::step(double wall_dt)` drains a single-precision accumulator and calls `IBackend::substep(fixed_dt)` zero or more times:

```
accumulator += min(wall_dt, fixed_dt * substeps_max)
while (accumulator >= fixed_dt):
    backend_->substep(fixed_dt)
    accumulator -= fixed_dt
    ++substep_index
interpolation_alpha = accumulator / fixed_dt
```

Rendering reads `PhysicsWorld::interpolation_alpha()` (0..1) to interpolate body transforms between the current and previous substep's cached poses. `fixed_dt` defaults to `1.0 / 60.0` and is bound to the `physics.fixed_dt` CVar; `substeps_max` is bound to `physics.substeps.max`. Phase 12 ships with no substeps below a single fixed step per `step()` call in headless / deterministic mode (`alpha == 0`).

### 5. Ownership + thread rules

| Object | Owner | Thread rules |
|---|---|---|
| `PhysicsWorld` | `runtime::Engine::Impl` | Main thread only |
| `IBackend` (null / Jolt) | `PhysicsWorld` | Implementation-private; Jolt path submits into `engine/jobs/` (ADR-0032) |
| `BodyHandle`, `ShapeHandle`, `ConstraintHandle`, `CharacterHandle` | 32-bit opaque IDs, ref-counted via `PhysicsWorld::retain_*` / `release_*` | Main thread only |
| Contact listener | Internal; pushes into `PhysicsWorld`'s `InFrameQueue<PhysicsContactAdded>` etc. | Jobs thread produces; main drains |

No raw `new`/`delete` in engine/physics — every handle is a typed ID and resources live behind unique_ptr inside the backend. No `<Jolt/**>` outside `engine/physics/impl/jolt_*.cpp` (and the single `impl/jolt_bridge.hpp` TU-local header).

### 6. Boot order (revised `runtime::Engine`)

```
Tier 0: CVarRegistry (+ standard + physics CVars), event buses
Tier 1: LocaleBridge, FontLibrary, GlyphAtlas, TextShaper
Tier 2: AudioService, InputService
Tier 3: PhysicsWorld                        ← new, Phase 12
Tier 4: UIService, SettingsBinder
Tier 5: ConsoleService (+ built-ins including physics.*)
```

Tick order (ADR-0029 revision §3):

```
1. input.update(now_ms)
2. drain CrossSubsystemBus → engine buses
3. physics.step(dt)                         ← new, Phase 12
4. drain physics event queues               ← new
5. ecs::tick()  (transform_sync after physics.step)
6. ui.update(dt)
7. audio.update(dt, listener)
8. ui.render()
9. ++frame_index
```

## Consequences

- Phase 13 animation can drive rigid-body-pose targets without ever touching `<Jolt/**>`.
- Phase 14 networking replicates `BodyHandle` → `TransformState` without replicating physics state; physics re-derives on the client via rewind-and-replay (ADR-0037 is the determinism contract enabling that).
- The CVar-driven solver knobs mean a content creator never edits C++ to tune a scene.
- Header quarantine makes the Jolt version bump a one-file change (ADR-0037 §upgrade-gate then replays the determinism capture).

## Alternatives considered

- **Exposing `JPH::Body*` directly.** Rejected — would couple the editor, gameplay, Phase 13 AI, and Phase 14 replication to the Jolt version.
- **Two-phase `init()`/`shutdown()`**. Rejected — violates non-negotiable #6; RAII is the house pattern.
- **Variable-step simulation.** Rejected — breaks determinism (ADR-0037).
- **Spawning Jolt threads.** Rejected — violates non-negotiable #10; `JoltJobBridge` adapts into `engine/jobs/` (ADR-0032).
