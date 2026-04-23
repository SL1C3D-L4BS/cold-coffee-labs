# pre-ed-widgets-firstuse - First use of distressed widgets + spline editor

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `preflight` |
| Tier | B |
| Depends on | `pre-ed-sacrilege-panels` |

## Inputs

- `editor/widgets/distressed_widgets.hpp`
- `editor/widgets/spline_editor.hpp`

## Writes

- `editor/panels/sacrilege/circle_editor_panel.cpp`
- `editor/panels/sacrilege/encounter_editor_panel.cpp`

## Exit criteria

- [ ] Circle Editor panel calls at least one distressed_button() per frame
- [ ] Encounter Editor panel exposes at least one spline surface

## Prompt

Use `gw::editor::widgets::distressed_button(...)` inside
circle_editor_panel.cpp render(). Use
`gw::editor::widgets::SplineEditor` inside encounter_editor_panel.cpp
render() for spawn-path authoring. Placeholder data is fine.

