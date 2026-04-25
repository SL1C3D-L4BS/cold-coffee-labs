# BLD C-ABI: `gw_editor_set_field` / `gw_editor_get_field` (v1)

**Surface:** BLD and fuzz hooks call these from [`editor/bld_api/editor_bld_api.cpp`](../../editor/bld_api/editor_bld_api.cpp) **after** [`ensure_authoring_scene_types_for_load`](../../editor/scene/authoring_scene_load.hpp) has run on the target world (the editor’s authoring ECS).

**Path syntax:**

- `ComponentName.field` — e.g. `TransformComponent.position`
- `Name` — legacy shorthand for the display name; equivalent to `NameComponent.value` when only the label is needed

**Not in v1 (inspector-only or other APIs):** components registered for reflection in the outliner/inspector that do not appear below must be edited in-editor or via a future C-ABI extension (sequencer, gameplay components, etc.).

| Component | Field | `set` JSON | `get` JSON | Notes |
|-----------|--------|------------|------------|--------|
| `NameComponent` | `value` | `"string"` (JSON string) | `"string"` | `set` also accepts bare `Name` path to write `NameComponent.value` (special case in C API) |
| `Name` (special) | — | `"string"` | N/A in table; use `get` with path `Name` for quoted name | `set` special path: field_path exactly `Name` if no `NameComponent` reflection path |
| `TransformComponent` | `position` | `[x,y,z]` (f64) | same | World-space, `dvec3` in ECS |
| `TransformComponent` | `scale` | `[x,y,z]` (f32) | same | |
| `TransformComponent` | `rotation` | `[w,x,y,z]` quaternion | same | `glm::quat` storage order w,x,y,z |
| `VisibilityComponent` | `visible` | `true` / `false` | unquoted `true`/`false` in JSON string | |
| `BlockoutPrimitiveComponent` | `shape` | unsigned (JSON number) | number string | 0=box, etc. (see component) |
| `BlockoutPrimitiveComponent` | `gwmat_rel` | `"content/…"` JSON string, escapes per JSON | quoted string | Relative material path, fixed buffer |
| `WorldMatrixComponent` | `dirty` | unsigned `0`/`1` (JSON number) | number string | **Write-only for cache** — use transform update to refresh world |
| `WorldMatrixComponent` | `world` | — (not settable) | 16 `double` JSON list in column order (`glm::dmat4` columns then rows as emitted by the C API) | **Read-only**; output of `update_transforms` — not editable via Surface P in v1 |

**Threading:** Per ADR-0007, caller holds the editor’s single-threaded tick; BLD does not run concurrent writers.

**Memory:** `gw_editor_get_field` returns heap JSON; call [`gw_free_string`](../../editor/bld_api/editor_bld_api.hpp) to release.
