# ADR 0046 — Phase 13 cross-cutting decisions (summary)

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 13 exit gate)
- **Deciders:** Cold Coffee Labs engine group
- **Summarises:** ADRs 0039–0045

This ADR is the cross-cutting summary for Phase 13 — dependency pins, perf gates, thread ownership, upgrade procedure, and hand-off to Phase 14.

## 1. Dependency pins

| Library | Pin | Gate | Default |
|---|---|---|---|
| Ozz-animation | 0.16.0 (2025-01-19) | `GW_ENABLE_OZZ` → `GW_ANIM_OZZ` | OFF for `dev-*`, ON for `living-*` |
| ACL | v2.1.0 (2023-12-07) | `GW_ENABLE_ACL` → `GW_ANIM_ACL` (implies OZZ) | OFF / ON |
| Recast/Detour | v1.6.0 + `main` commit (PR #791) | `GW_ENABLE_RECAST` → `GW_GAMEAI_RECAST` | OFF / ON |
| Recast build flags | `RECASTNAVIGATION_DT_POLYREF64 ON`, demos / tests / examples OFF | — | — |
| Ozz build flags | `ozz_build_{tools,fbx,gltf,data,samples,howtos,tests} OFF` | — | — |
| ACL build flags | `ACL_IMPL_BUILD_{TESTS,TOOLS} OFF`, `ACL_IMPL_ENABLE_INSTALL OFF` | — | — |

Rationale for `ozz_build_{fbx,gltf} OFF`: Phase 6 already owns glTF import via fastgltf; FBX SDK is forbidden repo-wide.

Clean-checkout `dev-*` builds the null animation, null behavior-tree, and null navmesh backends (deterministic all-zero hashes) so no external Phase-13 dep is required for CI green.

## 2. Hot-path budgets (CI-enforced via `gw_perf_gate`)

| Scenario | Budget | ADR |
|---|---|---|
| `AnimationWorld::step` — 100 chars, 1 clip, no IK | ≤ 1.80 ms | 0039 |
| `AnimationWorld::step` — 100 chars, 3-clip SM blend + L2M | ≤ 2.50 ms | 0039 / 0041 |
| 1-char two-bone IK | ≤ 0.03 ms | 0042 |
| 1-char 5-joint FABRIK | ≤ 0.12 ms | 0042 |
| 1-char 5-joint CCD | ≤ 0.25 ms | 0042 |
| `BehaviorTreeWorld::tick` — 100 entities @ 10 Hz | ≤ 0.35 ms | 0043 |
| `BehaviorTreeWorld::tick` — 1 000 entities @ 10 Hz | ≤ 3.00 ms | 0043 |
| Navmesh tile bake (32×32 m, background) | ≤ 25.0 ms (off main) | 0044 |
| Path query 200 m | ≤ 0.25 ms | 0044 |
| 64 batched path queries | ≤ 2.00 ms | 0044 |
| `pose_hash` over 100 characters | ≤ 0.08 ms | 0039 |
| `bt_state_hash` over 100 entities | ≤ 0.05 ms | 0043 |
| `nav_content_hash` over 1 tile | ≤ 0.03 ms | 0044 |
| `living_hash` full-scene | ≤ 0.20 ms | 0045 |
| `animation_debug_pass` emit (100 skel) | ≤ 0.40 ms | 0039 |
| `navmesh_debug_pass` emit (4 tiles + 10 paths) | ≤ 0.30 ms | 0044 |

All gates ship under CTest label `phase13`. Any Ozz / ACL / Recast pin bump reruns all fifteen.

## 3. Thread-ownership summary

| Thread | Owns | Phase-13 duties |
|---|---|---|
| Main | `AnimationWorld`, `BehaviorTreeWorld`, `NavmeshWorld` lifetime + API; drains their `InFrameQueue` buses | `step`, `tick`, `drain_bake_results`, `create/destroy_*` |
| Jobs pool — `sim` lane | Ozz sampling / blending / L2M batches; BT subtree tick batches; IK solves | via `Scheduler::parallel_for` |
| Jobs pool — `query` lane | Path query batches | `NavmeshWorld::query_batch` |
| Jobs pool — `background` lane | Recast tile bakes; `.kanim` / `.knav` async load | `Scheduler::submit(Background, …)` |
| Render | `animation_debug_pass` + `navmesh_debug_pass` record; pose consumer | const-only reads |
| Audio | Subscribes to `AnimationKey` for footsteps / foley | never touches Ozz types |

Zero `std::thread` / `std::async` introduced. Ozz's sample `std::async` fanout is **not** used — we replace it with `ozz_job_pool.cpp`.

## 4. Tick ordering (enforced in `runtime::Engine::tick`)

```
physics.step()
 → anim.step()                   // consumes root-motion feedback from char controller
 → bt.tick()                     // 10 Hz sub-sampled off anim clock
 → nav.drain_bake_results()      // pulls NavmeshBaked events + corridor updates
 → ecs.transform_sync_system()
 → render.extract()
```

Hard invariant: a single BT step never crosses a fixed-step boundary. Batching is across entities, not across BT nodes of one entity.

## 5. Determinism ladder

1. Ozz runtime is deterministic under `-fno-fast-math` on Clang 17+.
2. ACL uniform decoder is deterministic — non-uniform is forbidden by ADR-0040.
3. Recast per-tile bake is deterministic given identical input + config; parallel tile bakes are independent because outputs don't cross-reference.
4. BT executor has no random branch; any `Random` node takes an explicit seed from `BlackboardComponent`.
5. `pose_hash` + `bt_state_hash` + `nav_content_hash` are canonical FNV-1a-64 of sorted entries with quantized floats.
6. `living_hash` = xor-chain fold of physics ⊕ anim ⊕ bt ⊕ nav hashes.
7. CI `living_determinism_{win,linux}` replays 30 s and asserts byte-identical hash frame-by-frame.

## 6. Upgrade-gate procedure (Ozz / ACL / Recast)

Any PR that bumps a Phase-13 dep pin **must**:

1. Re-cook the canonical test corpus (`tests/fixtures/anim/corpus.gltf`, `tests/fixtures/nav/corpus.gwscene`).
2. Run `kanim_cook_parity` + `knav_cook_parity` CI gates Win ↔ Linux — bytes must match.
3. Re-record `living_determinism_windows` artifact and replay on Linux — `living_hash` must match.
4. Replay the existing golden `.gwreplay` files from the previous pin; any divergence blocks the bump.
5. Annotate the new pin in `docs/adr/0046-phase13-cross-cutting.md` §1 with the upgrade date.

No exceptions; Ozz-animation has a documented history of SoaTransform math-path drift between point releases.

## 7. Risks & retrospective (to be filled at phase exit)

| Risk | Outcome |
|---|---|
| Ozz 0.16 → 0.17 determinism drift | pin held at 0.16.0 for Phase 13; no drift observed under `-fno-fast-math` Clang 17. |
| ACL non-uniform decoder accidentally linked | compile-time `#error` added in `acl_decoder.cpp`; verified clean. |
| Recast tile bake non-determinism under parallel bake | per-tile seed from `(tile_x, tile_y, world_seed)` — serial inside tile, parallel between tiles; verified in `nav_bake_parity_test`. |
| BT hash drifts on blackboard floats | blackboard floats quantized on hash input; `bt_state_hash_float_stability_test` green. |
| IK fights physics | IK runs **after** physics in tick order; ground probe goes through `engine/physics/queries.hpp`. |
| Navmesh bake races ECS writes | bake takes a snapshot copy of triangle soup before dispatch; writes during bake silently re-queue. |
| BLD-authored BT bit-identical to hand-written | `.gwvs` canonicalized on save; `bt_bld_parity` green. |

## 8. Hand-off to Phase 14 (Networking)

Phase 14 inherits three locked contracts:

- **Anim replication:** serialize `(clip_hash, time_quant, blend_weights_quant[])` per character.
- **BT replication:** serialize `(tree_content_hash, active_node_path_id, bb_writes_delta[])`.
- **Nav corridor replication:** serialize `(path_hash, current_waypoint_index)` per agent.

Netcode ECS replication framework (ADR-0047, Phase 14 Wave 14A) builds on these.

The `living-*` preset is the base for `netcode-*` in Phase 14.

## 9. Exit-gate checklist

- [x] ADRs 0039–0046 landed with Status: Accepted.
- [x] `engine/anim/` + `engine/gameai/` module trees match ADR §4.
- [x] Headers contain **zero** `<ozz/**>`, `<acl/**>`, `<Recast*.h>`, `<Detour*.h>` in public API.
- [x] `pose_hash`, `bt_state_hash`, `nav_content_hash`, `living_hash` wired through console + CI.
- [x] 22 Phase-13 CVars registered; 11 console commands registered.
- [x] `sandbox_living_scene` prints `LIVING OK`; CTest `living_sandbox` green.
- [x] ≥ 55 new tests; CI target ≥ 383/383.
- [x] Roadmap Phase 13 → `completed` with closeout paragraph.
- [x] Risk-table retrospective filled in above §7.
