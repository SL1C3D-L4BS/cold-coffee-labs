# ADR 0041 — Blend trees + glTF → kanim cook pipeline

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 13 Wave 13B)
- **Deciders:** Cold Coffee Labs engine group
- **Related:** ADR-0039, ADR-0040

## 1. Context

Ozz supplies `Blend2Job` / `BlendNJob` primitives. We wrap them in a graph structure so designers author locomotion + attack state machines without touching Ozz API. The editor graph (Phase 17) projects onto the same data — node editor is a view, not a second authoring path.

## 2. Decision

### 2.1 Graph vocabulary

```cpp
enum class BlendNodeKind : std::uint8_t {
    Clip,            // leaf: plays one clip at (time, weight)
    Blend1D,         // param → weights over N clips on a 1-D axis
    Blend2D,         // (x, y) → bilinear weights over 4 clips
    StateMachine,    // named states + timed transitions + conditions
    AdditiveLayer,   // base + delta clip overlay
};
```

Every node has:
- stable `NodeId` (u32, assigned at graph-build time, sorted by topological order)
- input ports (0..N child refs)
- output port (pose slot — Ozz `SoaTransform` span, sized by skeleton joint count)

### 2.2 Determinism contract

- Blend weights are **normalized then quantized to fixed-point** (`1e-6` precision) before entering `pose_hash`.
- `anim.blend.snap_threshold` (default 0.99) snaps a weight to 1.0 if its neighbours are below `1 - threshold` — prevents precision noise at the pure-animation boundary.
- State machine transitions use a fixed-point duration counter (integer microseconds) — no float time accumulation.

### 2.3 Authoring

Phase 13 v1 ships JSON-in-scene authoring:

```json
"animation_graph": {
  "skeleton": "player.skeleton",
  "root_motion": true,
  "state_machine": {
    "default_state": "Idle",
    "states": [
      { "name": "Idle", "clip": "player_idle.kanim" },
      { "name": "Walk", "clip": "player_walk.kanim" },
      { "name": "Run",  "clip": "player_run.kanim"  }
    ],
    "transitions": [
      { "from": "Idle", "to": "Walk", "duration_ms": 150, "condition": "speed > 0.1" },
      { "from": "Walk", "to": "Run",  "duration_ms": 200, "condition": "speed > 3.0" },
      { "from": "Run",  "to": "Walk", "duration_ms": 200, "condition": "speed < 3.0" },
      { "from": "Walk", "to": "Idle", "duration_ms": 200, "condition": "speed < 0.1" }
    ]
  }
}
```

The ImNodes projection (Phase 17) produces the exact same JSON — round-trip is a CI gate in Phase 17.

### 2.4 Cook pipeline — shape

```
asset.gltf ──(fastgltf)──► glTF AST
                              │
                              ├── skeleton joints ──► SkeletonDesc
                              ├── animations       ──► RawAnimation × N
                              └── meshes (skinned) ──► SkinnedMeshDesc
                                                        (Phase 6 mesh_cooker handles this part)

RawAnimation  ──(ozz offline)──► sampled keyframes
              ──(acl compress)──► compressed_tracks
              ──(kanim_cooker)──► .kanim blob
```

`kanim_cooker` is deterministic given identical source bytes + settings. The cooker settings struct is hashed into the content hash alongside the source gltf bytes.

### 2.5 Additive layer semantics

An additive layer computes `pose_base + (pose_ref_to_pose_delta)` in SoaTransform local space. Reference pose is always frame 0 of the delta clip. Weight scales the delta only. Interactions with IK are layered in the order: blend tree → additive → IK → morph → root motion strip (ADR-0042).

## 3. Consequences

- Graph evaluation cost is dominated by sampling, not blending — Ozz blend is ~0.02 ms / 50 joints / char.
- The JSON authoring path is stable enough to ship; a future binary `.kanimgraph` (Phase 17) can replace it without touching runtime.
- BLD can author graphs via `vscript.*` tools from ADR-0014 because the JSON is tokenizable by tree-sitter JSON grammar (RAG already pinned).

## 4. Alternatives considered

- **Hand-coded graph for each character** — rejected: not authorable by designers, not by BLD.
- **Binary graph format v1** — deferred; scope bloat for Phase 13.
- **State machine as a separate subsystem** — rejected: folds into the same graph; transitions are just timed blends.
