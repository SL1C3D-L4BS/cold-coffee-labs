# Phase 12 — Replay protocol

**Status:** operational
**Binding on:** engine/physics, runtime, CI

## 1. File format — `.gwreplay` v1

Binary, little-endian.

### 1.1 Header

| Offset | Size | Field | Value |
|---|---|---|---|
| 0    | 4  | magic          | "GWRP" |
| 4    | 2  | version        | `1` (u16) |
| 6    | 2  | flags          | bit 0 = fixed-step, bit 1 = physics, bit 2 = character, bit 3 = input |
| 8    | 8  | seed           | `rng.seed` CVar value at record time (u64) |
| 16   | 4  | fixed_dt_bits  | raw bits of f32 fixed-dt |
| 20   | 4  | frame_count    | u32 |
| 24   | 4  | input_fmt_ver  | input-blob sub-format (u32) |
| 28   | 4  | reserved       | 0 |

### 1.2 Per-frame record (repeated `frame_count` times)

| Offset | Size | Field |
|---|---|---|
| +0  | 4  | frame_index (u32) |
| +4  | 4  | wall_dt_us (u32) |
| +8  | 2  | input_blob_len (u16) |
| +10 | 2  | reserved (u16) |
| +12 | N  | input_blob (bytes) |
| ... | 8  | hash (u64)  — `determinism_hash(world, frame_index).value` |

### 1.3 Input blob sub-format (`input_fmt_ver = 1`)

Packed array of:

| Field | Size | Notes |
|---|---|---|
| tag    | u8 | `0x01` = action_pulse, `0x02` = action_value, `0x03` = character_input, `0x04` = cvar_set |
| length | u8 | bytes in the rest of this record |
| payload| variable | depends on tag |

`character_input` payload is `entity_id (u32) + linear_vel (3×f32) + angular_vel (3×f32) + flags (u8)`. `action_value` payload is `action_hash (u32) + value (f32)`. `cvar_set` payload is `cvar_id (u32) + type (u8) + value bytes`.

## 2. Recording

```bash
gw_runtime --self-test --deterministic --frames=120 --record=path/to/out.gwreplay
```

The runtime opens a `physics::ReplayRecorder` at boot and calls `capture(...)` at the end of every frame. A CRC is computed over the whole tail and appended once at close; a crash mid-record leaves the temp file and does not corrupt the destination.

## 3. Replaying

```bash
gw_runtime --replay=path/to/in.gwreplay --enforce-hash
```

`ReplayPlayer` injects the captured input blob into the input system each frame, steps physics, recomputes `determinism_hash`, and compares. Any mismatch:

1. Prints the frame index, both hashes, and a diff of the first 16 bodies that differ.
2. Increments the `physics.determinism.hash_miss` counter CVar.
3. Exits non-zero (exit-code 3 = hash mismatch).

## 4. CI gate

### 4.1 `physics_determinism_windows`

```bash
cmake --preset physics-win
cmake --build --preset physics-win
ctest --preset physics-win -L determinism
# artifact: build/physics-win/bin/sandbox_physics.gwreplay
```

### 4.2 `physics_determinism_linux`

```bash
cmake --preset physics-linux
cmake --build --preset physics-linux
# Download Windows-recorded replay as artifact → scene_fixtures/
cp artifacts/sandbox_physics.gwreplay tests/determinism/fixtures/recorded.gwreplay
ctest --preset physics-linux -L determinism
```

Both CI entries are required to pass before the Phase-12 tag can move.

## 5. Fixtures

`tests/determinism/fixtures/` ships three captures:

| Fixture | Scene | Frames |
|---|---|---|
| `stack_of_crates.gwreplay`    | 8-high stack of cubes; one perturbation at frame 30 | 120 |
| `four_client_pile.gwreplay`   | 64-body tumble; no external perturbation | 180 |
| `character_ramp.gwreplay`     | character walks 30° ramp + jumps | 240 |

Each fixture is < 32 KB. They live in git (not Git LFS).

## 6. Upgrade procedure for Jolt bumps

Per ADR-0037 §6. Any Jolt pin bump PR must:

1. Re-record all three fixtures locally on the new Jolt.
2. Commit the new captures alongside the pin bump.
3. Cross-replay passes (Windows-recorded → Linux-replay, Linux-recorded → Windows-replay) must be green.
4. If parity is lost, link the upstream Jolt issue in the PR description and do not merge until the regression is fixed.

## 7. Local workflow

```bash
# Record
build/dev-win/bin/sandbox_physics.exe --record=out.gwreplay

# Replay + enforce
build/dev-win/bin/sandbox_physics.exe --replay=out.gwreplay --enforce-hash
```

`sandbox_physics --print-hash` prints the determinism hash of the final frame — useful for quick parity checks outside the full `.gwreplay` flow.
