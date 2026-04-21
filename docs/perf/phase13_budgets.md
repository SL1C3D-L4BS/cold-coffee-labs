# docs/perf/phase13_budgets.md — Phase 13 performance gates

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
