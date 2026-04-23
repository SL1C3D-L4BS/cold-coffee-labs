# p21-w140-hud-editor-panels - Diegetic HUD + Circle/Encounter editor panels

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `phase-week` |
| Phase | 21 |
| Week | 140 |
| Tier | A |
| Depends on | `p21-w139-audio-dialogue`, `pre-ed-sacrilege-panels` |

## Inputs

- `engine/ui/rml/`

## Writes

- `engine/ui/hud/diegetic_hud.hpp`
- `engine/ui/hud/diegetic_hud.cpp`
- `editor/panels/sacrilege/circle_editor_panel.cpp`
- `editor/panels/sacrilege/encounter_editor_panel.cpp`

## Exit criteria

- [ ] HUD shows blood vial + runic Sin ring + Mantra counter in RmlUi
- [ ] Circle Editor authors Location Skin dropdown (9 skins); Encounter Editor exposes spawn-spline surface

## Prompt

Build RmlUi HUD (3 elements). Land authoring surfaces in the
Circle + Encounter editor panels — dropdown for Location Skins,
spline editor for spawn paths.

