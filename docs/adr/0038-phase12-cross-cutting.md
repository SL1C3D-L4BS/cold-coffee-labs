# ADR 0038 — Phase 12 cross-cutting decisions (summary)

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 12 exit gate)
- **Deciders:** Cold Coffee Labs engine group
- **Summarises:** ADRs 0031–0037

This ADR is the cross-cutting summary for Phase 12. It pins dependencies, perf gates, and thread-ownership rules that span more than one wave.

## 1. Dependency pins

| Library | Pin | Gate | Default |
|---|---|---|---|
| Jolt Physics | v5.5.0 (2025-12-28) | `GW_ENABLE_JOLT` → `GW_PHYSICS_JOLT` | ON for `physics-*` / `playable-*`, OFF for `dev-*` |
| Jolt build flags | `CROSS_PLATFORM_DETERMINISTIC=ON`, `USE_SSE4_1=ON`, `USE_SSE4_2=ON`, `USE_AVX*=OFF`, `INTERPROCEDURAL_OPTIMIZATION=OFF`, `FLOATING_POINT_EXCEPTIONS_ENABLED=OFF`, `OBJECT_LAYER_BITS=16`, `USE_STD_VECTOR=OFF` | — | — |

Reason for AVX off: determinism parity across CI hosts and dev laptops. FMA reordering is an explicit determinism killer for Jolt ≥5.x.

Clean-checkout `dev-*` CI builds the **null physics backend** (stub world, all-zero hashes) so the repo ships with zero external physics deps by default.

## 2. Hot-path budgets (CI-enforced via `gw_perf_gate`)

| Scenario | Budget | ADR |
|---|---|---|
| `PhysicsWorld::step` — 500 sleeping bodies | ≤ 0.30 ms | 0031 |
| `PhysicsWorld::step` — 1 000 active free-falling boxes | ≤ 1.80 ms | 0031 |
| Character step (1 character) | ≤ 0.12 ms | 0033 |
| Character step (32-character crowd) | ≤ 2.10 ms | 0033 |
| 1 024 batched raycasts vs. static world | ≤ 0.80 ms | 0035 |
| `determinism_hash(1 024 bodies)` | ≤ 0.08 ms | 0037 |
| `physics_debug_pass` emit (1 000 bodies, all flags) | ≤ 0.60 ms | 0036 |
| Transform sync — ECS ↔ Jolt for 2 048 bodies | ≤ 0.25 ms | 0032 |
| `physics_debug_pass` overhead when flags == 0 | ≤ 0.05 ms | 0036 |

All gates ship with a ctest entry under label `phase12`. Any Jolt pin bump re-runs all nine.

## 3. Thread-ownership summary

| Thread | Owns | Calls made in Phase 12 |
|---|---|---|
| Main | `PhysicsWorld` lifetime + API; drains physics `InFrameQueue`s | `step`, `create/destroy_*`, `raycast*`, event drains |
| Jobs pool (engine/jobs) | Internal Jolt work via `JoltJobBridge`; `raycast_batch` chunks | — (all through `Scheduler::submit` / `parallel_for`) |
| Render | `physics_debug_pass` record | `debug_draw(sink)` (const, safe) |
| I/O (jobs pool) | `.kphys` decode | `kphys_loader::load` |
| Audio | Subscribes to `PhysicsContactAdded` for impact SFX | — |

No `std::thread` / `std::async` introduced. Jolt's `JobSystemThreadPool` is replaced by `JoltJobBridge`.

## 4. Thread-ownership table for Jolt internals

`JoltJobBridge` implements `JPH::JobSystem`:

| `JPH::JobSystem` method | Bridge target |
|---|---|
| `CreateBarrier` | Adapter over `gw::jobs::WaitGroup` |
| `DestroyBarrier` | deleter |
| `WaitForJobs` | `WaitGroup::wait` |
| `CreateJob` | `Scheduler::submit` at `JobPriority::Normal` |
| `QueueJob` | `Scheduler::submit` |
| `FreeJob` | ref-count-decrement (no-op post-submit) |

At runtime, `pstree` / `procexp` shows only `engine/jobs` worker threads — no Jolt thread names.

## 5. Determinism posture

- Cross-platform bit-exact across `dev-win` (clang-cl 22.1.3) and `dev-linux` (clang 22.x).
- CI gate `physics_determinism_windows` records; `physics_determinism_linux` replays.
- Hash is FNV-1a-64 over canonicalized state + frame index (ADR-0037 §1).

## 6. Cross-platform notes

- **Windows**: Jolt compiles with `/arch:SSE4.2` (implicit for x64); no `/arch:AVX`.
- **Linux**: clang `-msse4.2 -mno-avx -mno-avx2`. The compiler flags live in `engine/physics/CMakeLists.txt`, not in the Jolt fetch options, so any new compilation unit inherits them.
- **Steam Deck**: treated as Linux; SSE 4.2 is present; determinism replays parity-match.

## 7. Accessibility cross-walk

Physics touches accessibility only through debug draw. `physics.debug.enabled = false` by default; debug-draw colours honour `ui.contrast.boost` (when boost is on, outlines use the boosted palette). See `docs/a11y_phase12_selfcheck.md`.

## 8. What did not land in Phase 12 (deferred by design)

| Deferred | Target phase | Why |
|---|---|---|
| Soft-body / cloth | Phase 17 (VFX maturity) | Not gameplay-critical before then |
| Destruction / voxel fracture | Phase 22 (*Gather & Build*) | Needs survival system |
| Reverb probe baking | Phase 17 | Uses physics meshes but needs VFX maturity |
| GPU broadphase / cloth | Phase 20 (planetary) | Scale-dependent |
| Navmesh physics-query coupling | Phase 13 | Lands with Recast |
| Vehicle audio impostor | Phase 17 | Vehicle ships; audio polish later |

## 9. Exit-gate checklist

- [x] Week rows 072–077 merged behind the `phase-12` tag
- [x] ADRs 0031–0037 land as Accepted
- [x] ADR 0038 lands as this summary
- [x] `gw_tests` green with ≥ 35 new physics cases
- [x] `ctest --preset dev-win` and `dev-linux` green
- [x] Performance gates §2 green
- [x] `docs/a11y_phase12_selfcheck.md` signed off
- [x] `docs/perf/phase12_budgets.md` green on the CI box
- [x] `docs/determinism/phase12_replay_protocol.md` accepted
- [x] `docs/05` marks Phase 12 *shipped*
- [x] Git tag `v0.12.0-physics` pushed
