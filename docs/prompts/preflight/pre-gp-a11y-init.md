# pre-gp-a11y-init - Gameplay a11y init + HUD surface

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `preflight` |
| Tier | A |
| Depends on | `p21-w140-hud-editor-panels` |

## Inputs

- `gameplay/a11y/gameplay_a11y.hpp`

## Writes

- `apps/sandbox_playable/main.cpp`

## Exit criteria

- [ ] initialize_defaults()+apply() runs at gameplay world init
- [ ] HUD gates gore level + colour-blind override from GameplayA11yConfig

## Prompt

Initialise GameplayA11yConfig at world start. Photosensitivity
pre-warning fires on first launch. HUD reads cfg each frame.

