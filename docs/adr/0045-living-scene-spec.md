# ADR 0045 — *Living Scene* exit-demo scene spec + replay protocol

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 13 Wave 13F)
- **Deciders:** Simulation Lead · Gameplay Lead · CI Lead
- **Related:** ADR-0037 (physics replay), ADR-0039..0044

## 1. Context

*Living Scene* (week 083) is the Phase-13 gated demo. It must demonstrate the full stack (anim + BT + nav + physics) running deterministically under lockstep, with BT authored both via editor graph and BLD chat producing identical runtime behaviour. The demo is CI-gated and cross-OS-replayed — byte-identical `living_hash` is the gate.

## 2. Decision

### 2.1 Scene content (`apps/sandbox_living_scene/scene.gwscene`)

- 32 × 32 m rectangular patch with 1 m border walls.
- Two ramps (slopes 15°, 30°) dividing the patch into three regions.
- 4 patrol waypoints at `(8, 0, 8)`, `(24, 0, 8)`, `(24, 0, 24)`, `(8, 0, 24)`.
- 1 character (capsule: r=0.4 m, h=1.8 m) spawned at waypoint 0 with `Idle` → `Walk` locomotion graph.
- 3 clips baked: `idle.kanim`, `walk.kanim`, `run.kanim` (canonical placeholders — identity-pose animations that advance time; real content comes in Phase 20+ art pass).
- 1 BT (`patrol.gwvs`) — sequence of `move_to waypoint[i]` for i in 0..3, repeating.
- Physics: default Jolt if `GW_ENABLE_JOLT`, else null backend (still deterministic).

### 2.2 Exit-demo binary — `apps/sandbox_living_scene`

Prints, after 300 fixed-step frames:

```
[sandbox_living_scene] LIVING OK — frames=300 char_xy=<x,z> anim_hash=<hex> bt_hash=<hex> nav_hash=<hex> world_hash=<hex>
```

The `LIVING OK` token is the CTest regex gate. All four hashes appear alongside so cross-OS diffs surface the offending subsystem immediately.

### 2.3 Replay protocol (`.gwreplay` extension)

`.gwreplay` v2 (from Phase 12) gains three new frame-payload sections:

```
PhysicsFrame        — already defined (ADR-0037)
AnimationFrame      { clip_time_quant[] , blend_weights_quant[] , root_motion_delta }
BTFrame             { bt_step_seeds[] , active_node_path_id_by_entity[] , bb_writes[] }
NavFrame            { query_inputs[] , query_outputs_hash }
```

Each frame section is independently FNV-1a-64 hashed; `living_hash` combines the four with xor-chain fold.

### 2.4 Cross-OS CI gates

Two CI jobs:

- **`living_determinism_windows`** — records a `patrol.gwreplay` under the `living-win` preset. Uploads it as an artifact.
- **`living_determinism_linux`** — downloads the artifact, replays under `living-linux`, asserts byte-identical `living_hash` per frame.

Failure writes the diverging frame + section to the build log and fails the PR.

### 2.5 BLD-chat authoring parity gate

`bt_bld_parity` CTest:

1. Compile the hand-written `patrol.gwvs`.
2. Synthesize the same tree via `bld::tests::produce_patrol_bt()` (exercised through the `vscript.create_tree` tool path).
3. Run 300 BT ticks on both with the same blackboard seed.
4. Assert `state_hash_A == state_hash_B`.

This gate is **not** gated on BLD being online — it uses a recorded tool-response transcript checked into `tests/fixtures/bld/bt_patrol_transcript.json`.

### 2.6 Accessibility parity

`ui.motion.reduce` CVar (Phase 11) propagates into the demo:
- When ON, walk → idle transition collapses to immediate snap (no blend).
- Animation debug draw respects `ui.contrast.boost` for skeleton line colour — WCAG 2.2 AA palette.

## 3. Consequences

- Every Phase 13 commit must keep `sandbox_living_scene` green.
- Cross-OS CI jobs are the gating contract — PRs that diverge fail.
- Phase 14 netcode serializes the same four hashes to diagnose desyncs in flight.

## 4. Alternatives considered

- **Smaller demo (1 character, 1 clip, no BT)** — rejected; does not exercise the *Living Scene* claim.
- **Random-seed BT instead of canned patrol** — rejected; makes cross-OS replay non-trivial.
- **Separate recording/replay binaries** — rejected; one binary + `--record` / `--replay` flags.
