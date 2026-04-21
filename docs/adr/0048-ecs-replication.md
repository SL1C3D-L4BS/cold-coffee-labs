# ADR 0048 — ECS replication, delta compression, interest management

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 14 Wave 14C)
- **Deciders:** Cold Coffee Labs engine group
- **Scope:** Phase 14 (weeks 086+), extending ADR-0047

## 1. Context

Replication is the spine of *Two Client Night*: a dedicated server pushes a snapshot of the authoritative world to each peer every tick, deltas are computed against the peer's last acknowledged baseline, and an interest set gates what any one peer receives. The framework must deliver ≤ 32 KiB/s/client on a 2c+server sandbox and ≤ 96 KiB/s/client on a 16-peer hypothetical — both on the RX 580 baseline (CLAUDE.md #1) — while maintaining byte-identical replays cross-platform (ADR-0037).

## 2. Decision

### 2.1 Wire shape

```cpp
struct Snapshot {
    Tick                           tick{};
    std::uint32_t                  baseline_id{};    // peer's last ack, 0 = full
    std::uint64_t                  determinism_hash{}; // ADR-0037 oracle
    gw::Vector<EntityDelta>        deltas;           // core-container, arena-backed
};

struct EntityDelta {
    EntityReplicationId id{};       // (entity << 16) | revision
    std::uint32_t       field_bitmap{};  // set bit ⇒ field changed
    BitWriter           payload{};       // fields packed in declaration order
};
```

`BitWriter` lives in `engine/net/replication/bit_stream.hpp`; it writes to a bump-arena provided by the calling job and never touches the heap on the hot path.

### 2.2 Schema

Each replicated component declares fields via a macro that emits compile-time `FieldDesc` entries:

```cpp
GW_NET_REPLICATE(TransformComponent,
    GW_NET_FIELD_ANCHOR_POS(position, 24 /*bits*/, 0.01f /*m*/),
    GW_NET_FIELD_QUAT_SMALLEST3(orientation, 10 /*bits/axis*/),
    GW_NET_FIELD_FLOAT8(scale_uniform, 0.0f, 32.0f))
```

Field kinds:

| Kind | Bits | Notes |
|---|---|---|
| `ANCHOR_POS` | 24 × 3 | Quantized offset against the receiver's floating-origin anchor (CLAUDE.md #17); raw `f64` never on the wire |
| `QUAT_SMALLEST3` | 2 + 3×N | N-bit components of the three smallest quat axes |
| `FLOAT8` / `FLOAT16` | 8 / 16 | Quantized into a [min, max] range |
| `INT_VAR` | variable | Var-int 1–5 bytes, LEB-128 style |
| `BITFLAG` | 1 | Raw bit |

### 2.3 Baseline cache

Every peer gets a `BaselineCache` of the last `net.rep.baseline_ring` snapshots (default 16 = ~266 ms @ 60 Hz). Snapshots beyond the ring roll off deterministically; a client whose last ack is older than the ring is force-rebaselined (full snapshot). Baseline eviction is timer-free; it happens in the same tick that pushes a new snapshot.

### 2.4 Interest management

```cpp
struct InterestSet {
    glm::dvec3              anchor_ws{};         // peer's floating-origin anchor
    float                   radius_m{64.0f};     // per-component overrides via hint
    std::uint32_t           chunk_mask{};        // OR of relevance zones
};
```

Server builds per-peer `InterestSet` at the top of the replicate job; each entity's chunk coord (Phase-19 compatible grid) is tested against the mask, then each component's per-type override radius. Entities outside the set drop out of the snapshot cleanly — no explicit "remove" message; the peer reaps by timeout.

### 2.5 Job plumbing

`replicate_job` runs per-tick via `jobs::Scheduler::parallel_for` across peers. It reads an immutable ECS snapshot copy taken at the top of the tick so the simulation thread is free. Zero `std::thread`, zero `std::async`.

### 2.6 Budgets

| Scenario | Budget |
|---|---|
| Replication step (2c+server, 2 048 entities, 60 Hz) | ≤ 1.6 ms server main thread |
| Snapshot serialize (1 snap, 512 deltas) | ≤ 400 µs |
| Snapshot deserialize + apply | ≤ 450 µs |
| Delta bitmap scan (reflection-driven, 4 096 fields) | ≤ 120 µs |

Gates live in `docs/perf/phase14_budgets.md`.

## 3. Consequences

- Replays are purely byte-reproductions of the replicated snapshot stream: no local state is required to replay.
- Changing a field's bit budget bumps the schema content-hash; baseline rings are invalidated on mismatch.
- Any component can opt in with one macro; non-replicated components never walk the replicate_job.
- Anim, BT, and nav replication (ADR-0046 §8 hand-off) land as plain `GW_NET_REPLICATE` decls.

## 4. Alternatives considered

- **Quantize world positions as raw `f32`** — rejected; breaks determinism under floating-origin relocation.
- **Run replication off the main thread directly** — rejected; we take a snapshot copy so the main thread can publish state freely while replicate_job reads.
- **Use flatbuffers / protobuf for the snapshot** — rejected; these are fine for session metadata but the per-tick hot path demands fixed-width bit-packing.
