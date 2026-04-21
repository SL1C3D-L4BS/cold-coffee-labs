# ADR 0042 — IK (two-bone / FABRIK / CCD) + morph targets + ragdoll bridge

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 13 Wave 13C)
- **Deciders:** Cold Coffee Labs engine group · Physics Lead co-owner for ragdoll
- **Related:** ADR-0039, ADR-0032 (physics ECS bridge), ADR-0033 (character controller)

## 1. Context

*Living Scene* (week 083) requires feet to plant on slopes, hand IK for weapon-holds, and a ragdoll fallback when the character dies. All three must run **after** physics for the frame so ground probes hit the authoritative collision world.

## 2. Decision

### 2.1 IK catalogue

Three solvers ship in Phase 13. Each runs per-character, per-frame, after blend-tree evaluation and before morph targets:

| Solver | Purpose | Source | Budget |
|---|---|---|---|
| TwoBoneIKJob | leg, arm, head-look (2-segment chain) | Ozz built-in | ≤ 0.03 ms / char |
| FABRIK | N-joint chain (up to 8 joints) | custom in `engine/anim/impl/fabrik_solver.cpp` | ≤ 0.12 ms / 5-joint char |
| CCD | fallback for non-convergent cases | custom in `engine/anim/impl/ccd_solver.cpp` | ≤ 0.25 ms / 5-joint char |

Common contract:

```cpp
struct IKChain {
    std::uint32_t joint_count{};
    std::uint16_t joint_indices[8]{};      // top → tip
    glm::vec3     target_ls{};             // local space
    glm::vec3     pole_ls{};               // optional pole hint (FABRIK)
    float         tolerance_m{0.002f};
    std::uint8_t  max_iterations{8};
    std::uint8_t  solver_kind{0};          // 0=TwoBone, 1=FABRIK, 2=CCD
    std::uint8_t  flags{0};                // bit0 = respect_joint_limits
};
```

Joint limits ship as per-joint min/max quaternion-swing bounds (two-cone), stored in `SkeletonDesc::joint_limits`. Solver honours bounds when `flags & 0x1`.

### 2.2 Determinism

- All three solvers are **fixed-iteration, deterministic**. No early-out on floating compare except via `tolerance_m`.
- Vectors are quantized to `1e-6 m` precision before entering `pose_hash`.
- CCD order iterates from tip to root (reverse index order) — stable.

### 2.3 Morph targets

Morph targets (blend shapes) are per-vertex delta positions + normals, applied in **model space after IK**:

```cpp
struct MorphTargetDesc {
    StringId         name{};
    std::uint32_t    vertex_count{};
    std::vector<glm::vec3> position_deltas;   // model-space
    std::vector<glm::vec3> normal_deltas;     // model-space (optional)
};

struct MorphTargetsRuntime {
    std::uint32_t    target_count{};
    float            weights[64]{};           // max 64 targets / mesh
};
```

Weights are driven by blend-tree leaf nodes (per-clip morph tracks, loaded from `.kanim` v2 — for Phase 13 we ship v1 without morph track compression; weights come from script events instead).

### 2.4 Ragdoll bridge

A `RagdollComponent` flag toggles a character between two authoritative sources:

```
animation_driven   → anim writes local-space pose → physics consumes character-controller frame
ragdoll_driven     → physics writes rigid-body state → anim derives model-space pose for render
```

Transition rule — **mutually exclusive per frame**, one-frame limbo:

1. Frame N: `RagdollActivated` event fires; component transitions to `limbo`.
2. Frame N: physics initializes rigid bodies from the current animation pose (hand-off).
3. Frame N+1: mode becomes `ragdoll_driven`; animation is read-only.

Recovery reverses the process through a `RagdollRecovered` event. Both states are deterministic because the hand-off pose is the output of the Phase-13 pose hash — byte-stable across OSes.

### 2.5 Interaction with physics (binding)

| Step | Owner | Notes |
|---|---|---|
| Physics `step()` | ADR-0031 | writes rigid-body state, character controller position |
| Animation `step()` | ADR-0039 | reads character-controller position (root motion consumer) |
| IK solve | this ADR | ground-probe raycast via `engine/physics/queries.hpp` — no IK geometry of its own |
| Morph apply | this ADR | in-place on extracted model-space pose |
| Render extract | Phase 5 | interpolated by `interpolation_alpha()` |

## 3. Consequences

- IK target-reach guaranteed within `tolerance_m` for TwoBone; FABRIK / CCD may miss on over-extended reach — `IKTargetMissed` event fires with residual.
- Morph weight array is capped at 64; hard-clamps at load time with a log warning.
- Ragdoll → recovery cycle costs 2 frames of latency; acceptable for the gameplay it targets.

## 4. Alternatives considered

- **Unreal-style Anim BP with IK nodes** — too much scope; our IK is 3 small solvers on top of Ozz.
- **Physics-authored IK** — rejected: IK runs in anim step, not physics step, so one-frame feedback is acceptable.
- **Morph in GPU compute** — deferred to Phase 17 VFX budget; Phase 13 keeps it CPU for determinism + memory clarity.
