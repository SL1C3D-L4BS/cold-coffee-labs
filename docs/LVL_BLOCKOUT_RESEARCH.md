# Level blockout — research parity (ProBuilder / Unreal / Godot)

## Reference tiers

| Tier | Unity ProBuilder | Unreal | Godot CSG | Greywater Phase 1 (plan) |
|------|------------------|--------|-----------|---------------------------|
| Primitives | Cube, cylinder, … | Basic shapes | CSG shapes | Box / cylinder / plane / ramp stub |
| Transform | Move/rotate/scale | Same | Same | ImGuizmo + **snap** (`editor/panels/viewport_panel.cpp`) |
| Edit mesh | Vertex/edge/face | Modeling mode | CSG tree | **Deferred** — Phase 2 |
| UV | Unwrap / auto | Editor UV tools | Per-face limited | Deferred |
| Booleans | Limited / export | CSG | CSG combine | **Phase 2** — `docs/LVL_CSG_BACKLOG.md` |
| Export | Merge to mesh | Merge actors | MeshInstance | Optional **.gwmesh** bake (backlog) |

## Sacrilege layering

- **Designer blockout** — ECS entities tagged for hand-placed greybox (`BlockoutPrimitiveComponent`).
- **Blacklake / GPTM mesh** — procedural arena; **must not** remove blockout on regen unless user confirms.
- **Viewport** — Phase 8 records real mesh draws; until then, wire AABBs + grid (`docs/13_THIRD_PARTY_ASSETS.md` viewport note).

## Greywater Phase-1 scope (implemented as stubs + hooks)

- Tool modes: Place / Move / Rotate / Scale (persisted via `editor/config/editor_config.hpp` when wired).
- Primitives create entities with `TransformComponent` + `BlockoutPrimitiveComponent`.
- Library / Material Forge drag-drop paths feed **material assign** (Inspector integration backlog).
