# pre-ed-audit-panels - Register 7 audit panels

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `preflight` |
| Tier | A |
| Depends on | *(none)* |

## Inputs

- `editor/panels/audit/`
- `editor/app/editor_app.cpp`

## Writes

- `editor/app/editor_app.cpp`

## Exit criteria

- [ ] All 7 audit panels listed in Window menu and render without crashing

## Prompt

Register shader_hotreload_panel, profiler_panel, asset_deps_panel,
localization_panel, heatmap_panel, bake_panel, shader_permutations_panel
in EditorApplication::init_imgui() alongside existing panels.

