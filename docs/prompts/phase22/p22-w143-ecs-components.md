# p22-w143-ecs-components - Martyrdom ECS components

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `phase-week` |
| Phase | 22 |
| Week | 143 |
| Tier | A |
| Depends on | `p21-w142-exit-gate` |

## Inputs

- `gameplay/martyrdom/`
- `engine/ecs/`

## Writes

- `gameplay/martyrdom/martyrdom_components.hpp`
- `gameplay/martyrdom/god_mode.hpp`
- `gameplay/martyrdom/blasphemy_state.hpp`
- `gameplay/martyrdom/stats.hpp`

## Exit criteria

- [ ] SinComponent / MantraComponent / RaptureState / RuinState / ResolvedStats register via ECS bootstrapper

## Prompt

Define canonical components. Register with ECS type registry. No
behaviour yet — data only.

