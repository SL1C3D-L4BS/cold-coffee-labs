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
