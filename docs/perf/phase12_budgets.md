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
