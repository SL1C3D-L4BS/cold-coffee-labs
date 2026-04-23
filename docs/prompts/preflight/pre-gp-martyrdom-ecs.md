# pre-gp-martyrdom-ecs - Register Martyrdom/GodMode/Blasphemy ECS components

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `preflight` |
| Tier | A |
| Depends on | `p22-w143-ecs-components` |

## Inputs

- `gameplay/martyrdom/god_mode.hpp`
- `gameplay/martyrdom/blasphemy_state.hpp`
- `gameplay/martyrdom/grace_meter.hpp`

## Writes

- `gameplay/CMakeLists.txt`

## Exit criteria

- [ ] world.register_component<SinComponent>() called at gameplay world bootstrap

## Prompt

In gameplay world bootstrap, register: SinComponent, MantraComponent,
RaptureState, RuinState, ResolvedStats, BlasphemyState. Wiring only —
real logic lands in P22 W143.

