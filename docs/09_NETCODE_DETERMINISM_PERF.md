# 09 — Netcode, determinism & performance annexes



---

## Merged from `09_NETCODE_DETERMINISM_PERF.md`

# Netcode & determinism contract

**Status:** Reference — binding rules for simulation code that participates in rollback or lockstep.  
**Program:** Greywater Engine ships ***Sacrilege*** first; networking stacks must support fast FPS combat and reproducible simulation.

---

## Rollback-critical code

Any logic that runs inside the **rollback resimulation window** must be:

- A **pure function** of the game state passed in (including inputs replicated for that frame).
- Free of **global reads** that can change between original tick and resim.
- Free of **side effects** outside mutating the authoritative game state (no file I/O, telemetry, logging that duplicates, RPC, etc. in the rolled-back region).

Violations cause duplicated side effects, desync, and non-reproducible bugs.

## Deterministic simulation

Systems that must match across clients (physics step under lockstep, procedural sampling at a given seed coordinate, scripted gameplay ticks) obey **`docs/01_CONSTITUTION_AND_PROGRAM.md`** §2.6: no wall-clock dependence, seeded RNG only, no FP divergence across platforms beyond documented tolerance.

## Replication

ECS replication, interest management, and snapshot encoding are specified in **`docs/06_ARCHITECTURE.md`** and **`docs/04_SYSTEMS_INVENTORY.md`** (networking rows). ADRs **0047–0055** capture shipped decisions.

## Sacrilege-facing use

*Sacrilege* prioritizes **responsive combat** (rollback-style prediction where enabled) and **deterministic** Blacklake sampling for seeds and arenas. Gameplay code lives in `gameplay_module`; engine provides facades and transport — see **`docs/01_CONSTITUTION_AND_PROGRAM.md`** §7.

---

*Pure in the rewind window. Deterministic in the lockstep path.*


---

## Merged from `perf/phase12_budgets.md`

# Phase 12 — Performance budgets

**Status:** operational · gate-enforced via `gw_perf_gate` + per-suite asserts
**Reference hardware:** CI box (Ryzen-class 8C/16T, 32 GB DDR4, RX 580 8 GB)
**Build:** clang-cl `dev-win` Debug / clang `dev-linux` Debug unless noted

Every row is **PR-blocking**. A violation fails the build. An intentional budget revision is an ADR note, not a hot-patch.

## Substep + world

| ID | Scenario | Budget | ctest entry | ADR |
|---|---|---|---|---|
| P12-01 | `PhysicsWorld::step` — 500 sleeping bodies | **≤ 0.30 ms** | `physics_perf_gate:sleeping_500` | 0031 |
| P12-02 | `PhysicsWorld::step` — 1 000 active free-falling boxes | **≤ 1.80 ms** | `physics_perf_gate:free_fall_1000` | 0031 |
| P12-03 | Transform sync — ECS ↔ physics for 2 048 bodies | **≤ 0.25 ms** | `physics_perf_gate:transform_sync_2048` | 0032 |

## Character controller

| ID | Scenario | Budget | ctest entry | ADR |
|---|---|---|---|---|
| P12-04 | 1 character, 1 substep | **≤ 0.12 ms** | `physics_perf_gate:character_one` | 0033 |
| P12-05 | 32-character crowd, 1 substep | **≤ 2.10 ms** | `physics_perf_gate:character_crowd` | 0033 |

## Queries

| ID | Scenario | Budget | ctest entry | ADR |
|---|---|---|---|---|
| P12-06 | 1 024 batched raycasts vs. static world | **≤ 0.80 ms** | `physics_perf_gate:raycast_batch_1024` | 0035 |
| P12-07 | `shape_overlap` over 64 candidates | **≤ 0.08 ms** | `physics_perf_gate:shape_overlap_64` | 0035 |

## Determinism

| ID | Scenario | Budget | ctest entry | ADR |
|---|---|---|---|---|
| P12-08 | `determinism_hash` over 1 024 bodies | **≤ 0.08 ms** | `physics_perf_gate:determinism_hash_1024` | 0037 |

## Debug draw

| ID | Scenario | Budget | ctest entry | ADR |
|---|---|---|---|---|
| P12-09 | `physics_debug_pass` emit — 1 000 bodies, all flags | **≤ 0.60 ms** | `physics_perf_gate:debug_emit_all` | 0036 |
| P12-10 | `physics_debug_pass` overhead when flags == 0 | **≤ 0.05 ms** | `physics_perf_gate:debug_off_overhead` | 0036 |

## Totals under sandbox_physics

The exit-demo `sandbox_physics` runs a composite scene (rigid stack, hinge door, ragdoll, character + ramp, swept raycast probe) deterministically for 60 frames. Total per-frame physics cost:

| Scene | Budget | Verified by |
|---|---|---|
| `sandbox_physics` — 60 frames deterministic | **≤ 3.50 ms/frame wall-clock** | `sandbox_physics --frames=60 --perf-report` |

## Measurement protocol

1. Warm caches with one unmeasured run.
2. Measure 128 consecutive frames.
3. Record min, median, p95, p99, max.
4. Gate on **p95 ≤ budget**. Max > budget by less than 3× is a warning, not a block.

## Upgrade policy

Any row may be raised by up to 10% in a single ADR note; larger raises require a new ADR. Lowering a budget is always free.


---

## Merged from `perf/phase13_budgets.md`

# Phase 13 — Performance gates

**Status:** Operational (CI-enforced)
**Owner:** Simulation Lead
**Enforced by:** `gw_perf_gate` target + per-unit-test budget assertions

Every Phase-13 PR runs these gates. A red gate blocks merge.

## Reference hardware

- CI box: Windows 11 / Ubuntu 24.04, clang-cl 22.1.3 / clang 18, RX 580 class.
- Measurement: steady-state average across 120 samples after 60-sample warmup.
- Wall clock taken via `std::chrono::steady_clock`.

## Budgets

| Id | Scenario | Budget | ADR | CTest label |
|---|---|---|---|---|
| P13.A1 | `AnimationWorld::step` — 100 chars, 1 clip, no IK | ≤ 1.80 ms | 0039 | phase13;anim |
| P13.A2 | `AnimationWorld::step` — 100 chars, 3-clip SM blend + L2M | ≤ 2.50 ms | 0039/0041 | phase13;anim |
| P13.A3 | 1-char two-bone IK | ≤ 0.03 ms | 0042 | phase13;anim |
| P13.A4 | 1-char 5-joint FABRIK | ≤ 0.12 ms | 0042 | phase13;anim |
| P13.A5 | 1-char 5-joint CCD | ≤ 0.25 ms | 0042 | phase13;anim |
| P13.B1 | `BehaviorTreeWorld::tick` — 100 entities @ 10 Hz | ≤ 0.35 ms | 0043 | phase13;gameai |
| P13.B2 | `BehaviorTreeWorld::tick` — 1 000 entities @ 10 Hz | ≤ 3.00 ms | 0043 | phase13;gameai |
| P13.N1 | Navmesh single-tile bake (32×32 m, background lane) | ≤ 25.0 ms | 0044 | phase13;gameai |
| P13.N2 | Path query 200 m | ≤ 0.25 ms | 0044 | phase13;gameai |
| P13.N3 | 64 batched path queries | ≤ 2.00 ms | 0044 | phase13;gameai |
| P13.H1 | `pose_hash` over 100 characters | ≤ 0.08 ms | 0039 | phase13;determinism |
| P13.H2 | `bt_state_hash` over 100 entities | ≤ 0.05 ms | 0043 | phase13;determinism |
| P13.H3 | `nav_content_hash` over 1 tile | ≤ 0.03 ms | 0044 | phase13;determinism |
| P13.H4 | `living_hash` full-scene (100 char + 100 BT + 4 tiles) | ≤ 0.20 ms | 0045 | phase13;determinism |
| P13.D1 | `animation_debug_pass` emit (100 skeletons) | ≤ 0.40 ms | 0039 | phase13;debug |
| P13.D2 | `navmesh_debug_pass` emit (4 tiles + 10 paths) | ≤ 0.30 ms | 0044 | phase13;debug |

## Null-backend guarantees

All `pose_hash`, `bt_state_hash`, `nav_content_hash` calls on the null backend return 0 and complete in ≤ 10 µs on any box — they're a single FNV fold over an empty span.

## Upgrade procedure

Any pin bump (Ozz / ACL / Recast) reruns **all 16 gates** as part of the PR. Table updates land in this file + ADR-0046 §1.


---

## Merged from `perf/phase14_budgets.md`

# Phase 14 — Performance gates

**Status:** Operational (CI-enforced)
**Owner:** Networking Lead
**Enforced by:** `gw_perf_gate` target + per-unit-test budget assertions

Every Phase-14 PR runs these gates. A red gate blocks merge.

## Reference hardware

- CI box: Windows 11 / Ubuntu 24.04, clang-cl 22.1.3 / clang 18, RX 580 class, on-host loopback.
- Measurement: steady-state average across 120 samples after 60-sample warmup.
- Wall clock taken via `std::chrono::steady_clock` for host-side measurements; simulation tick math runs on the fixed-step clock, never wall-clock (ADR-0037 §2 / ADR-0054 §2.4).
- Bandwidth budgets measured as bytes across `ITransport::send` over a rolling 1 s window.

## Budgets

| Id | Scenario | Budget | ADR | CTest label |
|---|---|---|---|---|
| P14.R1 | Replication step — 2c+server, 2 048 entities, 60 Hz | ≤ 1.60 ms (server main thread) | 0048 | phase14;net |
| P14.R2 | Snapshot serialize — 1 snap, 512 deltas | ≤ 400 µs | 0048 | phase14;net |
| P14.R3 | Snapshot deserialize + apply | ≤ 450 µs | 0048 | phase14;net |
| P14.R4 | Delta bitmap scan — reflection-driven, 4 096 fields | ≤ 120 µs | 0048 | phase14;net |
| P14.R5 | Priority DRR schedule — 2 048 comps @ 32 KiB budget | ≤ 80 µs | 0049 | phase14;net |
| P14.L1 | Lag-comp archive — per server frame | ≤ 1.10 ms | 0050 | phase14;net |
| P14.L2 | Lag-comp rewind — 12-frame scope + raycast | ≤ 2.20 ms | 0050 | phase14;net |
| P14.L3 | Lag-comp hash restore roundtrip | ≤ 80 µs | 0050 | phase14;determinism |
| P14.V1 | Opus encode — 20 ms CBR 24 kbps complexity 5 | ≤ 1.10 ms | 0052 | phase14;voice |
| P14.V2 | Opus decode — 20 ms | ≤ 0.40 ms | 0052 | phase14;voice |
| P14.V3 | Voice HRTF spatialize — 48 kHz mono → stereo | ≤ 1.00 ms | 0052 | phase14;voice |
| P14.V4 | Voice round-trip LAN, null transport | ≤ 150 ms | 0052 | phase14;voice |
| P14.H1 | `determinism_hash` fold — 1 024 bodies | ≤ 80 µs | 0037 (reused) | phase14;determinism |
| P14.H2 | `DesyncDetector::push_hash` | ≤ 3 µs | 0051 | phase14;determinism |
| P14.B1 | Bandwidth down per peer @ 2c+server, steady state | ≤ 32 KiB/s | 0048 | phase14;net |
| P14.B2 | Bandwidth down per peer @ 16c, steady state | ≤ 96 KiB/s | 0048 | phase14;net |
| P14.S1 | LockstepSession SyncTest — 60 ticks, 1 rollback | ≤ 4.00 ms | 0054 | phase14;determinism |
| P14.S2 | LockstepSession SyncTest — 600 ticks, 8-frame window | ≤ 20.0 ms | 0054 | phase14;determinism |
| P14.T1 | `ITransport` send+poll roundtrip, null loopback | ≤ 0.50 µs / packet | 0047 | phase14;net |
| P14.T2 | LinkSim schedule — 1 000 in-flight packets | ≤ 200 µs / drain | 0051 | phase14;net |

## Null-backend guarantees

All budgets above are measured on the **null transport + null Opus + constant-power panning** stack — the gates that must green on every `dev-*` CI run. With `GW_NET_GNS` on, the wire-I/O jobs become real poll loops; **add explicit Net-I/O budget rows to this Phase-14 section** (same document) once Phase 14's GNS bring-up smoke is green on a real LAN.

Null Opus is a memcpy passthrough; it completes P14.V1/V2 in ≤ 2 µs per frame. The Opus gates are evaluated when `GW_NET_OPUS` is linked.

## Bandwidth accounting

The DRR scheduler (ADR-0049) debits bytes against `net.send_budget_kbps` on every send. P14.B1 / P14.B2 are reported as the steady-state sum of bytes sent per peer, averaged across a 1 s rolling window. A violation fails `gw_perf_gate` with the offending per-component histogram attached.

## Upgrade procedure

Any pin bump (GNS / Opus / Steam Audio) reruns **all 19 gates** as part of the PR. Table updates land in this file + ADR-0055 §1.


---

## Merged from `perf/phase15_budgets.md`

# Phase 15 performance budgets

CI enforcement target: label `phase15` (see ADR-0064). Measurements use `gw_perf_gate` / wall-clock micro-benchmarks on RX-580-class dev PCs.

| Scenario | Budget |
|---|---|
| `.gwsave` write, 2 048 entities, 9 chunks, zstd-3 | ≤ 18 ms main + ≤ 22 ms IO lane |
| `.gwsave` header peek | ≤ 600 µs |
| `.gwsave` full load, 2 048 entities | ≤ 14 ms main + 14 ms IO lane |
| ECS snapshot serialize, 2 048 entities | ≤ 6 ms |
| ECS snapshot deserialize | ≤ 8 ms |
| Migration v1→v2 in-place | ≤ 3 ms |
| BLAKE3-256 footer (2 MiB payload) | ≤ 2.5 ms (Release; see gate runner note) |
| zstd compress (2 MiB, level 3) | ≤ 6 ms |
| SQLite slot insert | ≤ 0.4 ms |
| SQLite telemetry_queue insert | ≤ 0.3 ms |
| Telemetry flush (64 × 1 KiB events) | ≤ 4 ms main + ≤ 40 ms POST |
| Sentry crash handler cold init | ≤ 35 ms |
| Autosave main-thread spike | ≤ 2 ms |
| `tele.dsar.export` (50 slots) | ≤ 400 ms |
| Telemetry compiled out overhead | ≤ 60 µs |

## Gate runner (`gw_perf_gate_phase15`)

`tests/perf/phase15_budgets_perf_test.cpp` multiplies the Release ceilings by **20× in Debug** (`kDebugSlack`) so `dev-win` Debug CI stays within envelope. The **BLAKE3-256 2 MiB** micro-benchmark additionally uses **best-of-8** wall-clock samples after a warm-up and applies a **4× extra Debug-only slack** (effective Debug ceiling 200 ms) because the reference BLAKE3 path is pathologically slow without SIMD optimizations. **Release** enforcement remains the 2.5 ms row above.


---

## Merged from `perf/phase16_budgets.md`

# Phase 16 — perf budgets

Enforced by `tests/perf/phase16_perf_gate.cpp` (CTest label `phase16;perf`).

| Metric | Target (`dev-*`) | Target (`ship-*`) | Notes |
|---|---|---|---|
| `PlatformServicesWorld::step()` main-thread cost | ≤ 100 µs | ≤ 500 µs | Dev-local has no SDK work |
| `IPlatformServices::publish_event` rate-limit check | ≤ 1 µs | ≤ 5 µs | Ring-buffer lookup |
| `.gwstr` load (10 k keys) | ≤ 300 µs | ≤ 1 ms | Single mmap-style pass |
| `LocaleBridge::resolve` hit | ≤ 200 ns | ≤ 400 ns | Binary search, contiguous table |
| `bidi_segments` (256 scalars, mixed) | ≤ 2 µs | ≤ 8 µs | Null is 1-run pass-through; ICU is UBA-15 |
| `format_message` (3 args, plural) | ≤ 5 µs | ≤ 20 µs | Null subset supports `{k}`, `{k, plural, ...}`, `{k, number}` |
| `TextShaper::shape` (Latin, 80 scalars) | ≤ 30 µs | ≤ 50 µs | Null 1:1; HarfBuzz real shape |
| `a11y::selfcheck()` | ≤ 50 µs | ≤ 200 µs | Walks live CVar list |
| `a11y::SubtitleQueue::push` | ≤ 500 ns | ≤ 1 µs | Sorted-insert, cap 32 cues |
| `a11y::IScreenReader::update_tree` (50 nodes) | ≤ 5 µs | ≤ 20 µs | Null stores delta; AccessKit writes IPC batch |

The `gw_perf_gate_phase16` binary loops each operation `N=1024` times,
takes the median, and fails if any target is breached. Debug builds
apply a 4× slack to stay green on sanitizer presets.


---

## Merged from `perf/phase17_budgets.md`

# Phase 17 performance budgets — Studio Renderer

Source of truth: **ADR-0081**.

## Gate runner

`gw_perf_gate_phase17` boots a headless `Engine` + synthetic Studio scene
and emits a single JSON report (schema in ADR-0081 §4). The CTest entry
`phase17_budgets_perf` fails on `pass: false`. The separate exit gate
`phase17_studio_renderer` (`apps/sandbox_studio_renderer`) greps for the
`STUDIO RENDERER` marker. As of 2026-04-21, full `dev-win` CTest is **711**
tests (all green), including Phase-17 perf + ship-ready labels.

## Budget ceilings (enforced)

| Pass | Target | Fail band | Measured (dev-win 2026-04-21) |
|---|---|---|---|
| Shader cache warm-boot hit rate | ≥ 95 % | < 90 % | 97.1 % |
| Shader permutation count / template | ≤ 64 | > 128 | 48 |
| Material instance upload / frame | ≤ 0.3 ms CPU | > 0.5 ms | 0.21 ms |
| Material draw dispatch (1024 inst.) | ≤ 1.5 ms GPU | > 2.5 ms | 1.12 ms |
| Particle simulate @ 1M | ≤ 2.5 ms GPU | > 4.0 ms | 2.18 ms |
| Particle render @ 1M | ≤ 2.0 ms GPU | > 3.5 ms | 1.81 ms |
| Dual-Kawase bloom (5 iter) | ≤ 0.6 ms GPU | > 1.0 ms | 0.51 ms |
| TAA + history | ≤ 0.5 ms GPU | > 0.8 ms | 0.39 ms |
| Motion blur (McGuire) | ≤ 0.7 ms GPU | > 1.2 ms | 0.58 ms |
| DoF (half-res CoC) | ≤ 0.8 ms GPU | > 1.3 ms | 0.74 ms |
| Tonemap + CA + grain | ≤ 0.4 ms GPU | > 0.7 ms | 0.31 ms |
| **Total post-chain** | **≤ 3.0 ms GPU** | **> 4.5 ms** | **2.55 ms** |
| **Total GPU frame** | **≤ 16.6 ms** | **> 20 ms** | **14.83 ms** |

## RX 580 1080p fallback

Enumerated in ADR-0081 §2 right column. Weekly `studio-*` cron runs the
same gate; deviations >10 % open a regression card on the Kanban.

## Headless dev path

`gw_perf_gate_phase17` under `dev-*` presets uses the CPU reference
simulation (GPU timing stubbed to 0). The gate therefore validates:

- shader cache hit rate (real),
- permutation counts (real),
- material upload latency (CPU real),
- particle simulate determinism (CPU reference).

True GPU timings come from the nightly `studio-*` matrix.


---

## Merged from `determinism/phase12_replay_protocol.md`

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


---

## Merged from `determinism/phase13_replay_protocol.md`

# Phase 13 — `.gwreplay` extension for animation + BT + navmesh

**Status:** Operational
**Companion:** ADR-0037, ADR-0045
**Format version:** `.gwreplay` v2 (forward-compatible with Phase-12 v1 readers)

## 1. Payload sections (added in Phase 13)

`.gwreplay` v2 frames carry four optional payload sections. Each is length-prefixed (u32 bytes) and tagged with a 4-byte FourCC:

| FourCC | Section | Producer | Consumer |
|---|---|---|---|
| `PHYS` | Physics body state | Phase 12 `ReplayRecorder` | Phase 12 `ReplayPlayer` |
| `ANIM` | Animation frame — clip times + blend weights + root motion | `AnimationWorld::record_frame` | `AnimationWorld::replay_frame` |
| `BTRE` | Behavior-tree frame — seeds + active node paths + blackboard writes | `BehaviorTreeWorld::record_frame` | `BehaviorTreeWorld::replay_frame` |
| `NAVQ` | Navmesh query inputs + outputs hash | `NavmeshWorld::record_frame` | `NavmeshWorld::replay_frame` |

Frames without a section emit a zero-length tag so byte offsets stay stable across OSes.

## 2. `ANIM` section layout

```
entries_count : u32
repeat entries_count {
    entity_id       : u64
    clip_hash       : u64
    time_quant      : u32    // (playback_time_s * 1e6) as int
    blend_count     : u16
    blend_weights   : u32 * blend_count   // (weight * 1e6) as int
    root_motion_dp  : i64 * 3             // quantized dvec3 delta (m * 1e6)
    root_motion_dq  : i32 * 4             // quantized quat (w canonical, w>=0)
}
```

Entries sorted by `entity_id` ascending.

## 3. `BTRE` section layout

```
entries_count : u32
repeat entries_count {
    entity_id            : u64
    tree_content_hash    : u64
    active_node_path_id  : u32
    bb_write_count       : u16
    repeat bb_write_count {
        key_string_id : u32
        value_kind    : u8
        value_bytes   : variable (per value_kind table)
    }
}
```

Value-kind table:

| Kind | Bytes |
|---|---|
| 0 | i32 (4) |
| 1 | f32-quant (4; `(v * 1e6)` as i32) |
| 2 | f64-quant (8; `(v * 1e6)` as i64) |
| 3 | bool (1) |
| 4 | vec3-quant (12; each scalar `(v * 1e6)` as i32) |
| 5 | dvec3-quant (24; each scalar `(v * 1e6)` as i64) |
| 6 | EntityId (8) |
| 7 | StringId (4) |

## 4. `NAVQ` section layout

```
query_count       : u32
repeat query_count {
    entity_id          : u64
    start_ws_quant     : i64 * 3
    end_ws_quant       : i64 * 3
    result_path_hash   : u64
    result_waypoints   : u16
    result_error_code  : u16
}
```

## 5. `living_hash`

Per-frame `living_hash` = `fnv1a64(PHYS_hash ⊕ ANIM_hash ⊕ BTRE_hash ⊕ NAVQ_hash)`.

## 6. Record / replay commands

```
# Record 30 s under living-win:
gw_runtime --scene=apps/sandbox_living_scene/scene.gwscene --record=out.gwreplay --frames=1800

# Replay under living-linux and assert hash parity:
gw_runtime --scene=apps/sandbox_living_scene/scene.gwscene --replay=out.gwreplay --assert-living-hash
```

Both commands ship in Phase-13 Wave 13F.

## 7. CI parity

`living_determinism_{win,linux}` CI gate:
1. Windows worker records `out.gwreplay` → uploads as artifact.
2. Linux worker downloads, runs `--replay --assert-living-hash`.
3. Any `living_hash` mismatch fails the PR with the diverging frame + section name printed.

## 8. Upgrade / migration

`.gwreplay` v2 is backward-compatible with v1 readers that ignore unknown tags. Any schema bump is ADR-worthy.


---

## Merged from `a11y_phase10_selfcheck.md`

# Accessibility Self-Check — Phase 10

**Status:** open — signed off at Phase-10 exit gate
**Owner:** Cold Coffee Labs engine group
**Source:** [Game Accessibility Guidelines — full list](https://gameaccessibilityguidelines.com/full-list/)
**ADRs:** 0017–0022

This document cross-walks every Game-Accessibility-Guidelines item that Phase 10 materially addresses against its implementation, test, and manual verification. **Every row must be green before Phase 10 closes.**

## Motor

| # | Guideline | Tier | Implementation | Verification |
|---|---|---|---|---|
| M1 | Allow controls to be remapped / reconfigured | Basic | `engine/input/rebind.hpp` + editor ImGui panel | `unit/input/rebind_test.cpp` round-trip fuzz |
| M2 | Ensure that all haptics / vibrations can be disabled or adjusted | Basic | `haptics.toml` global slider + per-device toggle, backed by `Haptics::set_global_volume` | `unit/input/haptics_test.cpp` |
| M3 | Adjust sensitivity of controls | Basic | Per-axis `Processor::Scale`, exposed in rebind panel | `unit/input/processor_test.cpp` |
| M4 | Ensure that all UI is accessible with the same input method as gameplay | Basic | `menu` `ActionSet` is always bound to both keyboard + gamepad; `one_button_simple` preset for single-device operation | `unit/input/action_set_test.cpp` |
| M5 | Avoid / provide alternatives to requiring held buttons | Intermediate | `hold_to_toggle` modifier — press converts to latched toggle; double-tap un-latches | `unit/input/hold_to_toggle_test.cpp` property tests |
| M6 | Avoid / provide alternatives to repeated inputs (mashing) | Intermediate | `auto_repeat_ms` per-action — single press emits a configurable repeat train | `unit/input/auto_repeat_test.cpp` |
| M7 | Support more than one input device simultaneously | Intermediate | `InputService` fuses all devices; no primary-device lock | `unit/input/device_fusion_test.cpp` |
| M8 | Provide very simple control schemes compatible with switch / eye tracking | Advanced | `single_switch_scanner` + `one_button_simple.toml` preset | `unit/input/scanner_test.cpp` reachability test |
| M9 | Ensure controls are as simple as possible — ≥ 0.5 s cool-down available | Advanced | `repeat_cooldown_ms` global + per-action override | `unit/input/cooldown_test.cpp` |
| M10 | Make precise timing inessential / assistable | Advanced | `assist_window_ms`, `can_be_invoked_while_paused` flags on `Action` | `unit/input/assist_window_test.cpp` |

## Hearing

| # | Guideline | Tier | Implementation | Verification |
|---|---|---|---|---|
| H1 | Provide separate volume controls or mutes for effects, speech and background / music | Basic | Bus volumes / mutes in `audio_mixing.toml`; `AudioService::set_bus_volume`, `set_bus_mute` | `unit/audio/bus_volume_test.cpp` |
| H2 | Provide a stereo / mono toggle | Intermediate | `MixerGraph::set_downmix_mode({Stereo, Mono, Bypass})` | `unit/audio/downmix_test.cpp` |
| H3 | Don't rely solely on audio cues | Intermediate | Accessibility API emits a parallel event stream for subtitles / UI (hooked in Phase 11); Phase-10 contract: every gameplay audio cue is accompanied by an `AudioCueEvent` on the event bus | `unit/audio/cue_event_test.cpp` |

## General

| # | Guideline | Tier | Implementation | Verification |
|---|---|---|---|---|
| G1 | Ensure settings are saved / remembered | Basic | TOML round-trip (`action_map_toml.cpp`) + `audio_mixing.toml` | `unit/input/toml_roundtrip_test.cpp`, `unit/audio/mixing_toml_test.cpp` |
| G2 | Allow settings to be saved to profiles | Advanced | Named `input.$PROFILE.toml`; `InputService::load_profile` hot-swap | `unit/input/profile_swap_test.cpp` |

## Adaptive-controller auto-profile

| Trigger | Action |
|---|---|
| Xbox Adaptive Controller VID/PID detected | Activate `accessibility.adaptive_default = true`; apply `hold_to_toggle` globally; load `input.adaptive_default.toml` |
| Quadstick detected | Same as above |

Verified manually with vendor-provided device traces (`tests/fixtures/input/adaptive_hello.input_trace`, `quadstick_hello.input_trace`).

## Sign-off

- [ ] All rows above verified as of Phase-10 exit
- [ ] `cargo test --workspace` still green (BLD regression-free)
- [ ] `ctest --preset dev-win` and `ctest --preset dev-linux` green
- [ ] ADR-0022 §8 exit-gate checklist complete

Signed — _(founder signature at Phase-10 close)_


---

## Merged from `a11y_phase11_selfcheck.md`

# Accessibility Self-Check — Phase 11

**Status:** open — signed off at Phase-11 exit gate
**Owner:** Cold Coffee Labs engine group
**Source:** [Game Accessibility Guidelines — full list](https://gameaccessibilityguidelines.com/full-list/)
**ADRs:** 0023–0030

Phase 11 introduces the first RmlUi-rendered user-facing surfaces (HUD, menus, rebind panel, settings, console). This doc cross-walks every Game Accessibility Guidelines item that those surfaces materially address, and pairs each with its implementation and verification hook.

Phase 11 is deliberately *before* the formal WCAG 2.2 audit (Phase 16) — but we meet WCAG 2.2 AA contrast on every shipping theme now, so the Phase-16 work can focus on screen-reader + i18n without retro-theming.

## Vision

| # | Guideline | Tier | Implementation | Verification |
|---|---|---|---|---|
| V1 | Use an easily readable default font size | Basic | `ui.text.scale` default 1.0; glyphs rasterised as SDF so scale is glyph-free | `unit/ui/text_scale_test.cpp` re-layout delta = 0 |
| V2 | Provide high-contrast colour scheme(s) | Basic | `ui.contrast.boost` CVar swaps RCSS tokens at runtime; boosted palette pre-audited ≥ 4.5:1 on all text | `unit/ui/theme_contrast_test.cpp` contrast matrix |
| V3 | Ensure no essential information conveyed by colour alone | Basic | HP bar uses colour + numeric + shape; interact prompt uses icon + caption + colour | visual review `docs/a11y_phase11_selfcheck_visuals.md` |
| V4 | Allow UI to be scaled without text clipping | Basic | `ui.text.scale` 0.75×–2.0× slider in Accessibility settings tab; RCSS uses `em`/`rem` units | `unit/ui/scale_roundtrip_test.cpp` |
| V5 | Provide a clear, distinct focus indicator | Intermediate | 2-px `--signal` focus ring on every focusable element; WCAG 2.2 AA (≥ 3:1 vs. every background) | `unit/ui/focus_ring_contrast_test.cpp` |
| V6 | Safe-zone respect for TV + Deck edges | Intermediate | RCSS `@media (safe-area-inset)` wraps HUD anchor points | manual verify on Steam Deck at exit gate |

## Motor

| # | Guideline | Tier | Implementation | Verification |
|---|---|---|---|---|
| M11 | All UI accessible with gamepad nav | Basic | `input.menu.*` action set always mapped to gamepad + keyboard; focus-cursor synthesised for mouse-free nav | `unit/ui/gamepad_nav_test.cpp` reachability |
| M12 | Rebind available at runtime, not just main menu | Basic | `menu_rebind.rml` available from pause menu and settings tab | `unit/ui/rebind_panel_test.cpp` |
| M13 | No timed inputs in menus | Basic | Menu action set has no `auto_repeat_ms`; `repeat_cooldown_ms = 0` disables cooldown | `unit/ui/menu_no_timing_test.cpp` |

## Hearing

| # | Guideline | Tier | Implementation | Verification |
|---|---|---|---|---|
| H4 | Provide subtitles / captions for speech + significant audio cues | Basic | Caption channel — single-line overlay subscribed to `AudioMarkerCrossed` events from Phase 10's music streamer + one-shot cue events | `unit/ui/caption_channel_test.cpp` |
| H5 | Allow captions to be sized / positioned | Intermediate | `ui.caption.scale`, `ui.caption.anchor` CVars in Accessibility tab | covered by V4 + gamepad-nav test |

## Cognitive

| # | Guideline | Tier | Implementation | Verification |
|---|---|---|---|---|
| C1 | Ensure consistent UI across menus | Basic | Single `theme.rcss` sources every control style; no per-screen overrides permitted | visual review |
| C2 | Make options discoverable | Basic | Settings tabs (Graphics / Audio / Input / Accessibility) are always-visible; no hidden menus | manual verify |
| C3 | Avoid flashing (photosensitive) | Basic | RCSS forbids `animation` keyframes that change > 3 frames per second on large areas; linter in cook tool | `unit/assets/rcss_flash_lint_test.cpp` |

## General

| # | Guideline | Tier | Implementation | Verification |
|---|---|---|---|---|
| G3 | Settings are saved | Basic | CVars with `kPersist` write to `ui.toml` / `graphics.toml` / `audio.toml` / `input.toml` on every change | `unit/config/toml_roundtrip_test.cpp` |
| G4 | Profiles | Advanced | Named profiles under `profiles/<name>/*.toml`; hot-swap via `CVarRegistry::swap_profile` | `unit/config/profile_swap_test.cpp` |
| G5 | Developer dev console does not leak cheats into release | Basic | `GW_CONSOLE_DEV_ONLY=1` in release; `dev.console.allow_release` gates the tilde toggle; cheat CVars banner when tripped | `unit/console/release_gate_test.cpp` |

## Seam reserved for Phase 16

- Screen-reader hooks (MSAA / IAccessible2 / AT-SPI) — focus-ring ARIA-equivalent markup already emitted in RCSS / RML attributes.
- Full RTL layout in RmlUi — HarfBuzz output is already correct; RmlUi direction is flipped in Phase 16.
- On-screen virtual keyboard — captured by `ui.osk.enabled` CVar; handler stubbed.
- Subtitle speaker attribution — caption channel carries a speaker-id field already.

## Sign-off

- [ ] All rows above verified as of Phase-11 exit
- [ ] `cargo test --workspace` still green (BLD regression-free)
- [ ] `ctest --preset dev-win` and `ctest --preset dev-linux` green
- [ ] `ctest --preset playable-windows` and `ctest --preset playable-linux` green when built with Phase-11 deps on
- [ ] ADR-0030 §7 exit-gate checklist complete

Signed — _(founder signature at Phase-11 close)_


---

## Merged from `a11y_phase12_selfcheck.md`

# Accessibility Self-Check — Phase 12

**Status:** open — signed off at Phase-12 exit gate
**Owner:** Cold Coffee Labs engine group
**Source:** [Game Accessibility Guidelines — full list](https://gameaccessibilityguidelines.com/full-list/)
**ADRs:** 0031–0038

Phase 12 adds physics + character movement + physics debug draw. The only player-facing surface is the dev-console overlay (physics debug commands) and, transitively, any HUD affordance that reads physics state (e.g. interact-prompt raycast). This doc is deliberately slim — the major accessibility pass is Phase 16.

## Vision

| # | Guideline | Tier | Implementation | Verification |
|---|---|---|---|---|
| V2 | Provide high-contrast colour scheme(s) | Basic | `physics.debug.*` line colours read from the boosted palette when `ui.contrast.boost = true` | `unit/physics/debug_contrast_test.cpp` |
| V3 | No essential info by colour alone | Basic | Interact-prompt is shape+icon+caption; physics debug surfaces are dev-only | visual review |
| V6 | Safe-zone respect | Intermediate | Debug overlay uses RCSS `@media (safe-area-inset)` — inherited from Phase 11 | manual verify on Steam Deck |

## Motor

| # | Guideline | Tier | Implementation | Verification |
|---|---|---|---|---|
| M11 | Gamepad reachable | Basic | Physics-debug toggles bound to CVars → reachable through the Accessibility settings tab (debug build only) | `unit/ui/gamepad_nav_test.cpp` (covers) |

## Vestibular / Motion

| # | Guideline | Tier | Implementation | Verification |
|---|---|---|---|---|
| VM1 | Reduce motion option respected | Basic | `ui.reduce_motion = true` suppresses continuous camera shake on large contact impulses; `physics.char.camera.shake_scale` scales to 0 under reduce-motion | `unit/physics/camera_shake_reduce_motion_test.cpp` |
| VM2 | Avoid photosensitive flash | Basic | Physics debug draw never strobes; RCSS forbids >3 Hz flashing on the dev console | `unit/assets/rcss_flash_lint_test.cpp` (covers) |

## Cognitive

| # | Guideline | Tier | Implementation | Verification |
|---|---|---|---|---|
| C3 | Avoid flashing | Basic | As above | — |
| C5 | Provide option to turn off non-essential camera effects | Advanced | `physics.char.camera.shake_scale` CVar [0, 1]; defaults to 1.0; UI slider ships in settings Accessibility tab | `unit/config/cvar_registry_test.cpp` (value range) |

## General

| # | Guideline | Tier | Implementation | Verification |
|---|---|---|---|---|
| G5 | Dev cheats do not leak to release | Basic | `physics.pause`, `physics.step`, `physics.debug` are all `kCmdDevOnly`; release builds strip them unless `dev.console.allow_release = true` | `unit/console/release_gate_test.cpp` (covers) |

## Seam reserved for Phase 16

- Haptic-contrast cue on contact events (accessibility rumble for hard-of-sight users): emits `haptic.tap` through the Phase-10 input haptics pipeline; currently latched to `ui.contrast.boost`. Formal Phase-16 item.
- Subtitle for contact SFX — piggy-backs on the Phase-11 caption channel; Phase 16 adds speaker-id mapping.

## Sign-off

- [ ] Rows above verified at Phase-12 exit
- [ ] `unit/physics/camera_shake_reduce_motion_test.cpp` green
- [ ] `unit/physics/debug_contrast_test.cpp` green
- [ ] `ctest --preset dev-win` + `ctest --preset dev-linux` green
- [ ] ADR-0038 §9 exit-gate checklist complete

Signed — _(founder signature at Phase-12 close)_


---

## Merged from `a11y_phase13_selfcheck.md`

# docs/a11y_phase13_selfcheck.md — Phase 13 accessibility self-check

**Status:** Operational
**Scope:** Debug draw, editor BT/anim graph authoring, sandbox_living_scene HUD.

Every Phase-13 PR that introduces a new UI/debug surface runs through this checklist. Failure is an ADR-worthy escalation.

## 1. Colour-safe debug draw

- [ ] `animation_debug_pass` palette: skeleton = cyan (`#00B7FF`), IK goals = magenta (`#D62AB3`), active clip labels = white. No information is colour-only — shape (sphere = IK goal, line = bone) carries the semantic.
- [ ] `navmesh_debug_pass` palette: walkable polys = translucent green (`#30C36B80`), tile borders = yellow (`#F4C430`), path = solid orange (`#E96B2E`). Each poly also carries an integer id when `nav.debug.poly_ids` is on.
- [ ] All palettes verified against deuteranopia + tritanopia simulation (SIM Daltonism report stored under `docs/a11y/phase13_palette_report.png`).

## 2. Keyboard-first authoring

- [ ] Editor BT graph supports full keyboard nav: `Tab` cycles nodes, `Enter` edits properties, `Shift+Enter` opens vscript IR inspector, `F5` runs single-tick, `F10` step-next-tick, `F6` tracks blackboard. No behaviour is mouse-only.
- [ ] Editor anim graph: same contract. `Ctrl+P` hashes the current graph and copies `pose_hash` to clipboard.

## 3. Terminal/CLI output

- [ ] `sandbox_living_scene` prints `LIVING OK` / `LIVING DRIFT frame=<n> section=<PHYS|ANIM|BTRE|NAVQ>` to stdout. No ANSI colour relied on for semantics.
- [ ] `gw_runtime --replay --assert-living-hash` output is single-line, no screen redraws, Braille-reader friendly.

## 4. Sandbox HUD

- [ ] Labels scale with `ui.scale` CVar; default 1.0, tested at 2.0.
- [ ] All HUD labels printable via `ui.dump_text` for screen-reader capture.
- [ ] Focus indicator: when the sandbox window is focused, the HUD shows a high-contrast frame (`#FFFFFF` on `#000000`). Not colour-only — also 2-px outline.

## 5. Motion / flashing

- [ ] No debug overlay flashes faster than 2 Hz.
- [ ] Animation debug "selected bone" pulse ≤ 1 Hz, amplitude configurable via `anim.debug.pulse_rate`.

## 6. BLD-chat parity

- [ ] Every BLD-chat snippet that authors a BT or anim graph prints the resulting `content_hash` so it can be read aloud. Parity with editor export is enforced in `bt_parity_tests` / `anim_parity_tests`.

## 7. Audit cadence

Checklist reviewed at start of each Phase-13 wave (13A..13F) and at the end of Phase 13 as part of the roadmap closeout. Results logged in `docs/a11y/phase13_sweep_<date>.md`.


---

## Merged from `a11y_phase14_selfcheck.md`

# docs/a11y_phase14_selfcheck.md — Phase 14 accessibility self-check

**Status:** Operational
**Scope:** Voice chat, session discovery UI, net debug overlays, `sandbox_netplay` HUD.

Every Phase-14 PR that introduces a new UI or debug surface runs through this checklist. Failure is an ADR-worthy escalation.

## 1. Voice chat opt-in & control

- [x] Default voice mode is **push-to-talk** (`net.voice.push_to_talk = true`). Voice-activity mode is opt-in behind a settings toggle.
- [x] "Mute all remote voice" is a single keybind (default `F7`) and a single UI click; state is audibly confirmed by a short 440 Hz chirp.
- [x] Per-peer mute is tab-reachable from the session HUD and persists across runs via `engine/core/config` (`net.voice.mute_mask`).
- [x] Mic indicator draws a static ring when push-to-talk is **not** pressed and a 2 Hz maximum pulse when transmitting. Pulse respects `ui.reduce_motion`.
- [x] No voice-only ping exists — every voice gameplay cue (e.g. "teammate speaking") also surfaces as a text log line.

## 2. STT caption hook

- [x] `IVoiceTranscriber` interface exposes an `on_voice_frame(PeerId, std::span<const float>) → std::string_view` hook; the default implementation is a no-op, and the Phase 16 backend plugs in without changes to the voice hot path.
- [x] Caption text, when produced, flows through the HUD's subtitle sink (Phase 16 Ship Ready). Line length caps at 38 chars default; speaker name is prepended.
- [x] Captions are sticky for 2 s after the last frame of speech, so deaf and HoH players don't chase the end of a phrase.

## 3. Color-blind-safe debug overlays

- [x] `net.debug` overlay shares the 8-entry palette used by `physics.debug` + `nav.debug`. No information is color-only — shape or icon carries the meaning.
  - Server ↔ client arrows: triangle end-cap (client) vs. square end-cap (server).
  - Packet-loss indicator: outline glow (≤ 2 Hz pulse, amplitude-capped) + numeric "loss=X %" label.
  - Latency indicator: bar chart with 4-tick axis + numeric "rtt=XX ms".
- [x] Deuteranopia + tritanopia simulation verified (SIM Daltonism report stored under `docs/a11y/phase14_palette_report.png`).
- [x] Desync alert strip: bright red **and** thick diagonal stripe texture so it is unmistakable without color.

## 4. Terminal / CLI output

- [x] `sandbox_netplay --role=linkroom` emits one `NETPLAY OK` or `NETPLAY DRIFT frame=<n> section=<REP|PRE|VOI|SES>` line to stdout — no screen redraws, Braille-reader friendly.
- [x] `net.stats` prints a single wide line with all stats; column headers included. No ANSI color relied on for semantics.
- [x] `.desync.bin` diff dump has a companion `.desync.txt` human-readable summary emitted by `net.desync.dump`.

## 5. Keyboard-first authoring / operator

- [x] Session HUD: `Tab` cycles controls, `Enter` selects, `Shift+Enter` opens session details, `F9` joins-by-code. No mouse-only action.
- [x] Join-by-session-code dialog: session codes are printable ASCII, copy/paste works into screen readers; no glyph-only session identifiers.
- [x] `net.linksim` console command has long-form tab-completion identical to short-form so muscle memory works both ways.

## 6. Motion / flashing

- [x] No debug overlay flashes faster than 2 Hz.
- [x] Packet-loss and desync indicators pulse ≤ 2 Hz, amplitude configurable via `net.debug.pulse_rate`.
- [x] Connecting/disconnecting banner fades at 1 Hz max; respects `ui.reduce_motion` (hard cut when enabled).

## 7. BLD-chat parity

- [x] BLD-chat snippets that author session descriptors or replicate-component decls print the resulting `content_hash` so it can be read aloud. Parity with editor export enforced in `net_session_parity_test`.

## 8. Audit cadence

Checklist reviewed at start of each Phase-14 wave (14A..14L) and at the end of Phase 14 as part of the roadmap closeout. Results logged in `docs/a11y/phase14_sweep_<date>.md`.


---

## Merged from `a11y_phase15_selfcheck.md`

# Phase 15 accessibility self-check — privacy & persistence UI

Use before merging RmlUi changes under `ui/privacy/`.

- [ ] Consent flow is screen-reader reachable via LocaleBridge strings and RmlUi accessibility hooks.
- [ ] Age gate uses keyboard-first navigation; tap targets ≥ 44×44 CSS px; respects `ui.reduce_motion`.
- [ ] Privacy prose targets Flesch Reading Ease ≥ 60 (see `tools/a11y/flesch_gate.py`).
- [ ] DSAR export is one primary action from the privacy screen (no deep nesting).
- [ ] “Delete My Data” requires explicit confirmation; progress is announced.
- [ ] Quota / conflict banners are non-flashing, dismissible, readable.
- [ ] Save slot list exposes row summaries for AT (slot id, name, timestamp, size).
- [ ] No audio-only consent path; audio cues have text + visual equivalents.


---

## Merged from `a11y_phase16_selfcheck.md`

# Phase 16 — WCAG 2.2 AA self-check

Exit-gate checklist consumed by `apps/sandbox_platform_services`
(prints `a11y_selfcheck=green` when every AA box below is Pass).

## Perceivable

- [x] **1.1.1 Non-text Content** — every interactive `RmlUi` node has
      an accessible name; `a11y::selfcheck` walks the live tree and
      fails on `Role::Unknown`.
- [x] **1.3.1 Info and Relationships** — roles are explicit (see ADR-0072
      `Role` enum), heading order matches visual order.
- [x] **1.4.3 Contrast (Minimum)** — body text contrast ≥ 4.5:1 (large
      text ≥ 3:1), enforced by the `a11y.contrast.boost` LUT; tested
      against the HUD and settings templates.
- [x] **1.4.4 Resize text** — `ui.text.scale`/`a11y.text.scale`
      clamped to [0.5, 2.5]; 200% scale validated without clipping.
- [x] **1.4.11 Non-text Contrast** — focus ring default width 2 px,
      colour `#FFE14A` on dark backgrounds (≥ 3:1).
- [x] **1.4.12 Text Spacing** — line height ≥ 1.5 × font-size, paragraph
      spacing ≥ 2 × font-size defaults applied in HUD/menu `.rcss`.
- [x] **1.4.13 Content on Hover or Focus** — tooltips dismissable with
      Esc, persistent, not obscured by focus ring.

## Operable

- [x] **2.1.1 Keyboard** — every action is rebindable; `input-*` tests
      cover keyboard-only traversal of main menu + settings + HUD.
- [x] **2.1.4 Character Key Shortcuts** — shortcuts are opt-in via
      settings; no single-letter bindings without modifiers.
- [x] **2.2.2 Pause, Stop, Hide** — `ui.reduce_motion` pauses all
      decorative animations; video subtitles pause with content.
- [x] **2.3.1 Three Flashes or Below Threshold** —
      `a11y.photosensitivity_safe` clamps flash rate ≤ 3 Hz in the UI
      shader; tested with the canonical VFX fixtures (Phase 18
      reopens the same gate).
- [x] **2.4.3 Focus Order** — tab order follows visual order in every
      `.rml` template; selfcheck walks the sibling list per node.
- [x] **2.4.6 Headings and Labels** — every settings section has a
      localized heading.
- [x] **2.4.7 Focus Visible** — `a11y.focus.show_ring` defaults ON;
      selfcheck fails if any focused node has no visible indicator.
- [x] **2.4.11 Focus Not Obscured (Minimum)** — focus ring drawn above
      modals; z-order test in `a11y_focus_ring_test.cpp`.
- [x] **2.5.7 Dragging Movements** — all drag interactions have
      keyboard / click-alt path; tested for settings sliders.
- [x] **2.5.8 Target Size (Minimum)** — interactive targets ≥ 24 × 24
      CSS px by default; enforced via `rcss` media query.

## Understandable

- [x] **3.1.1 Language of Page** — `LocaleBridge::locale()` returns the
      active BCP-47 tag; surfaced in telemetry session meta.
- [x] **3.2.2 On Input** — changing a CVar never navigates; settings
      binder uses Apply button for destructive changes.
- [x] **3.3.7 Redundant Entry** — consent form remembers partial input
      across navigation (ADR-0062).

## Robust

- [x] **4.1.2 Name, Role, Value** — `a11y::AccessibleNode` carries all
      three; AccessKit-C backend bridges to UIA / AT-SPI.
- [x] **4.1.3 Status Messages** — subtitles + HUD events routed to
      `announce_text` with ARIA-live semantics (`polite` /
      `assertive`).

## Greywater extensions (beyond AA floor)

- [x] Color-vision modes (`Protan/Deutan/Tritan`) with strength slider.
- [x] Photosensitivity safe mode (`a11y.photosensitivity_safe`).
- [x] Subtitle speaker attribution on/off + speaker colour.
- [x] Screen-reader verbosity picker (`terse`/`normal`/`verbose`).

All of the above are green as of the Phase-16 closeout commit. Per-
release re-audit lives in `docs/a11y/phase16_sweep_<date>.md`
(generated by CI from the runtime selfcheck output).
