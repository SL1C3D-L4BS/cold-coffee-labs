# ADR 0039 — `engine/anim` facade + Ozz job pool + fixed-step clock

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 13 Wave 13A)
- **Deciders:** Cold Coffee Labs engine group
- **Scope:** weeks 078–083 (Phase 13 *Living Scene*)

## 1. Context

Phase 13 opens the *Living Scene* milestone (`docs/05_ROADMAP_AND_MILESTONES.md` §473): a character walks a navmeshed scene driven by a Behavior Tree, animations blend deterministically, and physics collisions resolve under lockstep. This ADR pins the **animation facade contract** that every Phase 13+ subsystem (physics, BT, nav, net replication) depends on.

Binding non-negotiables (CLAUDE.md):
- #3 header quarantine — `<ozz/**>` must never appear in `engine/anim/*.hpp` outside `engine/anim/impl/`
- #5 RAII — every Ozz `Skeleton`, `Animation`, `SamplingContext` lives under `std::unique_ptr`
- #9 no STL containers on hot paths — Ozz's internal arrays are acceptable inside `impl/`
- #10 no `std::thread` / `std::async` — Ozz sample code uses `std::async`; we **do not**
- #11 OS-header quarantine — Phase 13 adds none

## 2. Decision

### 2.1 Public API — PIMPL facade

`engine/anim/public/animation_world.hpp` is the single entry point:

```cpp
class AnimationWorld {
public:
    AnimationWorld();
    ~AnimationWorld();
    AnimationWorld(const AnimationWorld&) = delete;
    AnimationWorld& operator=(const AnimationWorld&) = delete;

    [[nodiscard]] bool initialize(const AnimationConfig& cfg,
                                  events::CrossSubsystemBus* bus = nullptr,
                                  jobs::Scheduler* scheduler = nullptr);
    void shutdown();

    [[nodiscard]] SkeletonHandle   create_skeleton(const SkeletonDesc&);
    [[nodiscard]] ClipHandle       create_clip(const ClipDesc&);
    [[nodiscard]] AnimationGraphHandle create_graph(const AnimationGraphDesc&);

    void                           attach(EntityId, AnimationGraphHandle);
    void                           detach(EntityId);

    [[nodiscard]] std::uint32_t    step(float delta_time_s);
    void                           step_fixed();
    [[nodiscard]] float            interpolation_alpha() const noexcept;

    [[nodiscard]] RootMotionDelta  root_motion_delta(EntityId) const noexcept;
    [[nodiscard]] std::uint64_t    pose_hash(const PoseHashOptions& = {}) const;

    [[nodiscard]] BackendKind      backend() const noexcept;
    [[nodiscard]] std::string_view backend_name() const noexcept;
    // Buses, debug draw, CVars, reset, pause — mirrors `PhysicsWorld`.
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
```

**No `<ozz/**>` is reachable from any caller of this header.** The Impl lives in `animation_world.cpp`, which forwards to `engine/anim/impl/ozz_bridge.cpp` when `GW_ANIM_OZZ` is defined.

### 2.2 Backend gating

| Backend | Flag | Default | Determinism |
|---|---|---|---|
| Null (identity pose + canonical zero hashes) | — | always linked | trivially stable |
| Ozz 0.16.0 | `GW_ANIM_OZZ` via `GW_ENABLE_OZZ` | OFF for `dev-*`, ON for `living-*` | stable |
| ACL 2.1.0 decoder | `GW_ANIM_ACL` via `GW_ENABLE_ACL` (implies `GW_ANIM_OZZ`) | OFF / ON | stable (uniform decoder only) |

Clean-checkout `dev-*` CI builds the null backend and emits all-zero `pose_hash` values that still sort-order correctly — this keeps cross-OS determinism trivially provable on every commit.

### 2.3 Fixed-step clock

Animation samples at `anim.fixed_dt` (default 1/60 s). The accumulator is **the same clock** as physics (`physics.fixed_dt`) — they share a single fixed-step source in `runtime::Engine::tick()`. Changing either CVar re-caps determinism parity at runtime; the console command `anim.hash` snapshots the canonical pose hash for cross-OS diff.

### 2.4 Ozz job pool bridge

`engine/anim/impl/ozz_job_pool.{hpp,cpp}` is the **only** place that submits Ozz `SamplingJob` / `BlendingJob` / `LocalToModelJob` batches. It dispatches **per-character batches of size `anim.jobs.batch_size` (default 16)** through `jobs::Scheduler::parallel_for`. Zero `std::thread`, zero `std::async`, zero `std::future`.

### 2.5 Canonical pose hash

`pose_hash(opt)` = FNV-1a-64 over a sorted-by-entity-id sequence. Each entry:

```
[entity_id u64]
[clip_hash u64]
[time_quant u32]      // (playback_time_s * anim.hash.time_scale) as int
[blend_weights_quant u32 × N_active_clips]  // (weight * 1e6) as int
```

Quaternion canonicalization (`w >= 0`) applies to root-motion quats. Same trick as `engine/physics/determinism_hash.cpp`.

## 3. Consequences

- Netcode (Phase 14) serializes `(entity, clip_hash, time_quant, blend_weights_quant[])` tuples verbatim.
- Replay (`.gwreplay`) records root-motion deltas + graph state hashes; not raw poses.
- Ozz upgrade is ADR-worthy — any pin bump reruns the `living_determinism_{win,linux}` CI gate.
- Phase 17 material system consumes morph weights via the same `AnimationWorld` accessor.

## 4. Alternatives considered

- **GPU skinning inside animation step** — deferred to Phase 17; animation ships CPU poses that render skin in its own pass.
- **Ozz's own `std::async` sample path** — rejected by #10. We own the thread policy end-to-end.
- **A second VM for blend graphs** — rejected; graph is evaluated by a small interpreter tied to vscript IR (ADR-0043).
