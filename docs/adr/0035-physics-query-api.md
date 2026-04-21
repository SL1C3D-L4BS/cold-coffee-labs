# ADR 0035 — Physics query API — rays, sweeps, overlaps, batching

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 12 Wave 12D)
- **Deciders:** Cold Coffee Labs engine + gameplay group
- **Relates:** ADR-0031, ADR-0032, ADR-0034

## Context

Gameplay relies on spatial queries: ranged attacks (raycast), projectile capsule sweeps, volumetric triggers (overlap), ground detection (short downward sweep). A single-shot API is the common case; batched queries are necessary for AI perception (hundreds of rays per tick) and particle collision (thousands).

## Decision

### 1. Single-shot API

```cpp
namespace gw::physics {

struct RaycastHit {
    BodyHandle  body{};
    glm::vec3   point_ws{0};
    glm::vec3   normal{0, 1, 0};
    float       t{0};                    // 0 = origin, 1 = origin+dir*length
    std::uint32_t surface_id{0};
};

struct ShapeQueryHit {
    BodyHandle  body{};
    glm::vec3   point_ws{0};
    glm::vec3   normal{0, 1, 0};
    float       penetration_m{0};
    std::uint32_t surface_id{0};
};

struct QueryFilter {
    LayerMask layer_mask{kLayerMaskAll};
    // Optional predicate — evaluated per candidate body; return true to accept.
    bool (*predicate)(void* user, BodyHandle) {nullptr};
    void* predicate_user{nullptr};
    // Exclude bodies with these IDs (e.g. the shooter itself).
    std::span<const BodyHandle> excluded{};
};

class PhysicsWorld {
    std::optional<RaycastHit>
    raycast_closest(glm::vec3 origin, glm::vec3 dir, float max_distance,
                    const QueryFilter& filter = {}) const;

    bool raycast_any(glm::vec3 origin, glm::vec3 dir, float max_distance,
                     const QueryFilter& filter = {}) const;

    std::size_t raycast_all(glm::vec3 origin, glm::vec3 dir, float max_distance,
                            std::span<RaycastHit> out, const QueryFilter& filter = {}) const;

    std::optional<ShapeQueryHit>
    shape_sweep_closest(ShapeHandle shape, glm::vec3 start, glm::vec3 end,
                        const QueryFilter& filter = {}) const;

    std::size_t shape_overlap(ShapeHandle shape, glm::vec3 position, glm::quat rotation,
                              std::span<BodyHandle> out, const QueryFilter& filter = {}) const;
};
```

### 2. Batched API

Batched queries submit into `engine/jobs/` through the backend-owned bridge (ADR-0031 §JoltJobBridge). The API is a simple span-in / span-out:

```cpp
struct RaycastBatch {
    std::span<const glm::vec3> origins;
    std::span<const glm::vec3> directions;
    std::span<const float>      max_distances;
    std::span<RaycastHit>       out_hits;
    std::span<std::uint8_t>     out_hit_mask;   // 1 if hit, 0 if miss
    const QueryFilter*          filter{nullptr};
};

void PhysicsWorld::raycast_batch(const RaycastBatch&);
```

Batch size is capped by `physics.query.batch_cap` (default 4096). Over-cap batches are split. The batch is synchronous on the calling thread: `raycast_batch` does not return until all chunks complete. The null backend processes the batch sequentially; the Jolt backend splits into `std::max(1, n / chunk_size)` chunks via `parallel_for`.

### 3. Determinism

All queries read a `const` snapshot of the world. They never mutate body state. Jolt's broadphase query API is `const`-safe; the null backend keeps a sorted-axis-aligned-bounding-box list built lazily on `step()`. Results are ordered by `t` ascending for the `*_all` + `*_closest` variants.

### 4. Excluded bodies + predicate

The `QueryFilter::excluded` span is scanned linearly per candidate (typical size 0–3 — the shooter + its weapon + its mount). The `predicate` hook is the escape valve for gameplay filters the layer mask can't express ("pet-safe objects" where pets are tagged via a dynamic flag). Predicate functions must be `noexcept`, stateless on the physics side, and may touch ECS reads — they run on the calling thread.

### 5. Memory + performance

- `raycast_closest` on 1 candidate ≤ 0.5 µs (null backend) / ≤ 1.5 µs (Jolt) — unit test.
- `raycast_batch` of 1024 rays against a static-heavy world ≤ 0.8 ms — Phase-12 perf gate §7 of the plan.
- `shape_overlap` against a 10 m³ volume with 64 candidates ≤ 80 µs — unit test.
- Zero heap allocations after the caller-supplied `out` span is allocated. The null backend uses a `thread_local` `std::vector` reused per call and cleared on entry.

## Consequences

- Gameplay code uses the same API in Unreal-ish `LineTraceByChannel` style.
- Phase 13 navmesh cache + BT perception queries route through `raycast_batch` so they share the job pool.
- Phase 17 VFX particle collision uses `raycast_batch` at chunk granularity (1 call per particle chunk, not per particle).

## Alternatives considered

- **Async future-returning queries.** Rejected — added bookkeeping; synchronous-parallel is sufficient for Phase 12 call sites.
- **Exposing Jolt's `NarrowPhaseQuery` directly.** Rejected — would leak Jolt types into public headers (CLAUDE.md non-negotiable #3).
- **Returning `std::vector<RaycastHit>` from `raycast_all`.** Rejected — forces an allocation per call; span-out keeps the hot path zero-alloc.
