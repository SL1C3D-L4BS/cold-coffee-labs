# pre-eng-narrative-tick - Narrative tick system

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `preflight` |
| Tier | A |
| Depends on | *(none)* |

## Inputs

- `engine/narrative/`

## Writes

- `engine/narrative/narrative_tick_system.hpp`
- `engine/narrative/narrative_tick_system.cpp`
- `engine/narrative/CMakeLists.txt`

## Exit criteria

- [ ] NarrativeTickSystem::tick(world, dt) advances act_state → dialogue_graph → sin_signature → voice_director → grace_meter
- [ ] Doctest with fake world ticks 60 frames without assertion

## Prompt

Create NarrativeTickSystem that owns references to all five narrative
subsystems and ticks them in canonical order. Register it inside the
gameplay world init of each sandbox main that uses narrative.

