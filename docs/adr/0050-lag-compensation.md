# ADR 0050 — Server rewind + lag compensation

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 14 Wave 14E)
- **Deciders:** Cold Coffee Labs engine group
- **Scope:** Phase 14 (week 088), requires ADR-0037 + ADR-0048

## 1. Context

When a client fires at a target, the client's view is older than the server by `rtt/2 + client_interp_delay`. If the server runs the hit test at "now" it systematically misses. Valve-style lag compensation rewinds the authoritative world to the tick the shooter actually saw, runs the hit test, and restores. It requires physics that can checkpoint and replay (ADR-0037 §3).

## 2. Decision

### 2.1 API

```cpp
class ServerRewind {
public:
    void   archive_frame(Tick, const PhysicsWorld&, const TransformBuffer&);
    [[nodiscard]] RewindScope scoped_rewind(Tick target);
    [[nodiscard]] std::size_t history_frames() const;
    [[nodiscard]] std::uint64_t archive_hash(Tick) const;   // ADR-0037 oracle
};
```

`RewindScope` is RAII: the destructor snaps physics + transforms back by writing the archived state in reverse. Hit queries call:

```cpp
[[nodiscard]] RaycastHit hit_query_with_lag_comp(PeerId shooter,
                                                  const RaycastInput& ray,
                                                  Tick now,
                                                  const QueryFilter& filter);
```

which internally does `auto scope = rewind.scoped_rewind(now - peer_rtt/2)`, runs `PhysicsWorld::raycast_closest`, and returns the hit.

### 2.2 Window

`net.lagcomp.window_frames` = 12 (200 ms @ 60 Hz default). The archive is a ring buffer of snapshots — position, orientation, linear velocity, angular velocity per replicated body — not full physics state. The character controller capsule is archived separately because its collision volume is not a Jolt body in the null backend.

### 2.3 Hash restore

Every archived frame stores its ADR-0037 `determinism_hash`. After the `RewindScope` destructor restores, the hash is recomputed and compared against the archive entry; any mismatch fires `PhysicsHashMismatch` on the error bus and writes a `.rewind-mismatch.bin` diff dump. This is the oracle that proves rewind is bit-stable.

### 2.4 Serialization

Rewind never reaches for heap. The ring is a `gw::Vector<RewindFrame>` pre-sized to `window_frames × max_bodies` bytes at `NetworkWorld::initialize`.

### 2.5 Threading

All rewinds run on the server main thread inside `NetworkWorld::step_fixed()`. A rewind scope is *never* concurrent with a physics step (§4 tick order, ADR-0046 §4 pattern).

## 3. Consequences

- Hit tests are authoritative on the server, so clients only need to predict locally, not correct server truth.
- Rewind cost is O(bodies) per frame archive + O(bodies) per scoped_rewind; on a 2c+server sandbox with 2 048 bodies the archive is ≤ 1.1 ms per frame, within the budget.
- Changing `net.lagcomp.window_frames` invalidates any in-flight replay (forces rebaseline).

## 4. Alternatives considered

- **Client-authoritative hit tests** — rejected as a cheating vector.
- **Replay physics from fixed-step seed** — rejected; too expensive (re-stepping is ~10× archive).
- **Archive full physics state (contacts included)** — rejected for memory; velocities + transforms suffice because contact resolution is re-derived in the rewind.
