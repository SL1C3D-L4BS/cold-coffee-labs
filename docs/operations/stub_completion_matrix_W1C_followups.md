# Stub completion matrix — Wave 1C (stub follow-ups)

**Program:** Cursor plan `stub_completion_program_1d2f4e69` (plan id `1d2f4e69`).  
**Scope:** Items left after `wave1c-landed-2026-04` — see plan todo `wave1c-stub-followups` and plan section **1C Editor API, a11y, viewport, render partials**.  
**Exit:** each row has a named test, `rg` gate, or review artifact; **execute in the order below** (later rows may depend on earlier invariants).

**Execution log (append-only):** 2026-04-25 — **W1C.1** header aligned with Kahn topo-sort; new doctest for parent-without-Transform. **W1C.4** viewport ray–AABB pick + debug wire AABBs use `WorldMatrixComponent` for world center/half when present. **W1C.3** gizmo header: entity map documented vs viewport picking. **W1C.5–6** `sync_render_targets_thumbnails()` after `create_scene_rt` (all slots bound to live VkImage/ImGui DS); `ExposureParams` no synthetic default lum; `luminance_sample_valid` + `clear_editor_luminance_readout` when readback missing; render settings panel shows “—” until GPU sample. **PIE** `tick_perf_guard` runs before the PIE HUD; `PieDebugHudState` shows **frame time EWMA** and surfaces `pie_perf_guard` over-budget (144 Hz) as a warning line. **W1C.8** parity line added atop [`bld_set_field_v1_matrix.md`](./bld_set_field_v1_matrix.md). **W1C.9** checklist: [`editor_hal_phase_c.md`](./editor_hal_phase_c.md). **W1C.7** a11y: doctests in `tests/unit/editor/editor_a11y_toml_test.cpp` round-trip all **seven** `EditorA11yConfig` bools; no extra fields pending.

2026-04-25 — **W1C.3 (spatial index):** `GizmoSystem` adds uniform-grid buckets on entity translation (4 world units per cell) plus `gather_entities_near()`; picking remains in `viewport_panel.cpp`. **Wave 2 (BLD):** `RegistryDispatchHandler` in `bld-mcp` (tests default), `RagDispatcher` pre-dispatches `registry::find_dispatch`, `gw_bld_server` built with `bld-tools/seq-tools` via the `server` feature; `sacrilege.*` calls return structured `deferred` JSON from `RagDispatcher` until host pipelines land.

## Execution order (mandatory)

| # | ID | Deliverable | Primary paths | Why this order |
|---|----|--------------|---------------|----------------|
| 1 | W1C.1 | **Hierarchy / transforms** — deferred topo-sort, correct world matrices every frame | `editor/scene/transform_system.cpp`, `transform_system.hpp` | Downstream gizmo, picking, and BLD `WorldMatrix` reads assume a stable, ordered graph. |
| 2 | W1C.2 | **Editor camera + input** — GLFW vs `engine::InputEvent` unified; no orphan Phase-10 input debt | `editor/viewport/editor_camera.cpp`, `editor/viewport/editor_camera.hpp`, call sites in viewport / app | Gizmo and viewport need one coherent input and camera model before pick rays and manipulators. |
| 3 | W1C.3 | **Gizmo system** — replace flat map with **spatial index** (or equivalent) for entity/pivot association | `editor/viewport/gizmo_system.hpp`, `gizmo_system.cpp` | Scales with selection; should use correct transforms from (1) and input from (2). |
| 4 | W1C.4 | **Viewport** — full mesh/material draw path, **entity picking** per Phase 7 §6.4 (depth / ID / ray), polish beyond “mesh visible” | `editor/panels/viewport_panel.cpp`, `editor/render/*` as wired by `EditorScenePass` | Picks and manipulators need geometry, depth, and selection unified with (1)–(3). |
| 5 | W1C.5 | **Render targets panel** — every G-buffer / RT slot bound to **live** Vulkan images from the editor HAL; no placeholder rects | `editor/panels/render_targets_panel.cpp`, `.hpp` | Thumbnails and labels must reflect real surfaces before histogram and settings are trustworthy. |
| 6 | W1C.6 | **Render settings** — **only** live GPU luminance readback; **no** synthetic luminance branch | `editor/panels/render_settings_panel.cpp` | Depends on (5) and a real scene/tonemap path; closes plan “synthetic” debt. |
| 7 | W1C.7 | **A11y** — full TOML round-trip for every `EditorA11yConfig` field (close gaps vs tests) | `editor/a11y/editor_a11y.cpp`, `tests/*a11y*` if present | Independent of render order but should not lag documented v1; verify after main editor shell stable. |
| 8 | W1C.8 | **BLD / editor C API** — `gw_editor_get_field` and inspector **parity** with `gw_editor_set_field` for all v1 types; document matrix in `docs/operations/bld_set_field_v1_matrix.md`; scene I/O as exercised by BLD + File menu | `editor/bld_api/editor_bld_api.cpp`, `editor/bld_api/bld_host_table.cpp` | API surface is safer once ECS/transform and viewport agree on components (1)–(4). |
| 9 | W1C.9 | **Editor on full HAL** — dynamic rendering / HAL swap (plan “Phase C audit”), FSR/HDR registration as in `editor/render/render_settings.hpp` comments | `editor/app/editor_app.hpp`, `editor/app/editor_app.cpp`, `editor/render/*` | Cross-cutting; last so it does not destabilize (1)–(6) while they are still landing. |

## Wave-1 exit gates (plan 1C)

When rows **1–6** and **8** are at least at “no known stub path” for their scope:

- `rg` on `editor/panels/audit` for empty `on_imgui_render` bodies = **0** (per plan; stricter AST lint optional).
- No **“Phase 11”** string in `editor/panels/console_panel.*`.
- Histogram / luminance path: **no** synthetic branch in release/editor default (`W1C.6`).

## Cross-links

- **Wave 0 matrix:** [stub_completion_matrix_W0.md](./stub_completion_matrix_W0.md)
- **Graphify:** Community 4 (`on_imgui_render`, editor UI) — `graphify-out/GRAPH_REPORT.md`

**Same program, sequenced after this matrix (not excluded):**

| Wave | Deliverable (stub completion program) |
|------|----------------------------------------|
| **2** | BLD MCP `StubHandler` → production dispatch; `bld-tools` / bridge live; editor copilot panel wired to live BLD/HITL |
| **10** | `modules_builtin` / `module_manifest` non-null panel and tool factories; `editor_app` factory registration complete |
