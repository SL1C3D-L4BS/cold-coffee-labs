# pre-ed-sacrilege-panels - Register 10 Sacrilege panels

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `preflight` |
| Tier | A |
| Depends on | *(none)* |

## Inputs

- `editor/panels/sacrilege/`
- `editor/app/editor_app.cpp`

## Writes

- `editor/app/editor_app.cpp`

## Exit criteria

- [ ] All 10 sacrilege panels appear in Window menu and render without crashing
- [ ] doctest: PanelRegistry::find() locates each panel by name

## Prompt

Register every panel under `editor/panels/sacrilege/` inside
EditorApplication::init_imgui() alongside the existing
panels_.add(...) calls (line ~219).

Panels: act_state_panel, ai_director_sandbox_panel, circle_editor_panel,
dialogue_graph_panel, editor_copilot_panel, encounter_editor_panel,
material_forge_panel, pcg_node_graph_panel, shader_forge_panel,
sin_signature_panel.

For panels that need the world / render settings / selection, pass
pointers to `scene_world_`, `render_settings_`, `selection_`.

