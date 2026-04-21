# ADR 0037 — Determinism contract + replay gate

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 12 Wave 12E)
- **Deciders:** Cold Coffee Labs engine + CI group
- **Relates:** ADR-0031, ADR-0032, ADR-0033, CLAUDE.md non-negotiables #5, #16

## Context

Determinism is the bedrock for:

1. Rewind-and-replay lag compensation (Phase 14).
2. Crash repro on `.gwreplay` files (Phase 15).
3. Procedural world determinism (Phase 19).
4. Multiplayer lockstep validation (Phase 14 *Two Client Night*).

Jolt 5.5 ships with `CROSS_PLATFORM_DETERMINISTIC ON`, but determinism is a **whole-system contract**, not a flag. This ADR captures what we own.

## Decision

### 1. `determinism_hash`

```cpp
namespace gw::physics {

struct DeterminismHash {
    std::uint64_t value{0};
};

DeterminismHash determinism_hash(const PhysicsWorld&, std::uint64_t frame_index);

} // namespace gw::physics
```

Implementation:

1. Gather all `BodyHandle`s and sort ascending by raw ID.
2. For each body, emit a canonicalized record:
   - `id` (u32)
   - `position_x/y/z` (f32, raw bit pattern, `-0.0 → 0.0`)
   - `orientation_x/y/z/w` (f32, canonical sign: if `w < 0` flip all four)
   - `linear_velocity_x/y/z` (f32, raw bits)
   - `angular_velocity_x/y/z` (f32, raw bits)
   - `motion_type` (u8)
   - `is_sleeping` (u8)
3. FNV-1a-64 over the record stream. Append `frame_index` at the end.

Budget: **≤ 80 µs for 1 024 bodies**. Enforced in `unit/physics/determinism_hash_test.cpp`.

### 2. Canonicalization rules (exhaustive)

| Field | Rule |
|---|---|
| Floats | Raw bit pattern except `-0.0 → 0.0`, NaN treated as error (asserts in debug) |
| Quaternion sign | `w ≥ 0` enforced by negating all four if `w < 0` |
| Body order | Ascending by `BodyID` |
| Sleeping bodies | Included, tagged with `is_sleeping` byte |
| Transient-only state (accumulated forces) | Excluded (reset every substep) |
| Frame index | Appended once at the end |

### 3. Replay format (`.gwreplay`)

Binary, little-endian, versioned:

```
magic   = "GWRP"
version = u16 (1)
flags   = u16 (1 = fixed-step, 2 = physics, 4 = character)
seed    = u64                 (RNG seed as of record)
fixed_dt= f32
frame_count = u32

// per-frame record, repeated frame_count times:
frame_index    : u32
wall_dt_us     : u32
input_blob_len : u16
input_blob     : bytes
hash           : u64          (determinism_hash at end-of-frame)
```

Inputs are a CRC-framed blob of `InputService::Snapshot` + `character_input` deltas. Replay format is forward-compatible: readers skip unknown `flags` bits.

### 4. `ReplayRecorder`

```cpp
class ReplayRecorder {
public:
    explicit ReplayRecorder(std::string output_path);
    void capture(std::uint32_t frame_index, std::uint32_t wall_dt_us,
                 std::span<const std::byte> input_blob,
                 DeterminismHash hash);
    ~ReplayRecorder();   // finalizes the file
};
```

Writes to a temp file and renames on success so a crash mid-record leaves no half-written capture.

### 5. `ReplayPlayer`

```cpp
class ReplayPlayer {
public:
    explicit ReplayPlayer(std::string input_path);
    std::size_t frame_count() const noexcept;
    void seek(std::uint32_t frame_index);
    const FrameRecord& current() const noexcept;
    // Advance to the next frame. Returns false at EOF.
    bool advance();
};
```

In deterministic replay mode, the engine:

1. Reads `FrameRecord::input_blob` and injects into the action system.
2. Runs a physics substep.
3. Calls `determinism_hash(world, frame_index)`.
4. Compares against `FrameRecord::hash`; on mismatch, asserts with a diff dump and a `physics.determinism.hash_miss` counter increment.

### 6. Upgrade-gate procedure

When a Jolt point-release is proposed:

1. Capture 60 s of the `physics_sandbox` scene on the current Jolt version (Windows + Linux).
2. Commit the capture under `tests/determinism/fixtures/<jolt-tag>.gwreplay`.
3. In the upgrade PR:
   - Bump `cmake/dependencies.cmake` Jolt pin.
   - Re-run the capture on **the new Jolt version**.
   - Re-play both captures cross-platform; each must replay to hash parity.
4. If hash parity is lost, attach a Jolt-upstream issue link and re-run the PR once the regression is fixed. **No mixed-Jolt PRs.**

### 7. CI jobs

| Job | Runs on | What it does |
|---|---|---|
| `physics_determinism_windows` | Win11 + clang-cl 22.1.x, ctest label `determinism` | Records a 60-frame replay of `physics_sandbox`; uploads as artifact |
| `physics_determinism_linux`   | Ubuntu 24.04 + clang 22.x, ctest label `determinism` | Downloads the Windows-recorded replay; replays it; asserts hash parity |

Both jobs are **required** before merge to `main`. Either failing fails the PR.

### 8. Hash frequency

Debug / deterministic headless mode → hash every frame. Release / retail → hash every `physics.determinism.hash_every_n` frames (default 30). A hash emit cost of 80 µs × 2 ≈ 0.16 ms per enforcement frame is budgeted.

### 9. What breaks determinism (hard errors)

- `std::chrono` drifting the fixed step.
- Any random-number generation inside `PhysicsWorld::step` that does not read from `rng.seed`.
- `std::unordered_map` / `std::unordered_set` iteration anywhere in the physics integration path.
- AVX / AVX2 / AVX-512 / FMA reordering — Jolt pin forbids.
- LTO / IPO — Jolt pin forbids.

## Consequences

- Phase 14 can implement client-side prediction + server rewind without "mostly deterministic" caveats.
- `.gwreplay` captures are safe to ship to QA as bug repros; any test box reproduces the hash.
- Jolt upgrades are a one-day procedure, not a surprise.

## Alternatives considered

- **Hash only positions.** Rejected — misses rotation + sleep-bit regressions that routinely cause multiplayer desyncs.
- **CRC-32 instead of FNV-1a-64.** Rejected — 32 bits is too narrow for a 60-frame, 1 024-body game; collision probability is non-negligible.
- **Hash in release only every 60 frames.** Considered; default 30 keeps the detection window tight enough to surface regressions within the Phase-13 `Living Scene` 60-second demo.
