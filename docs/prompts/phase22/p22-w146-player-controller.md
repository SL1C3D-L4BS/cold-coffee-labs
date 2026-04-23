# p22-w146-player-controller - Player controller + Malakor/Niccolò voice director

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `phase-week` |
| Phase | 22 |
| Week | 146 |
| Tier | A |
| Depends on | `p22-w145-blasphemy` |

## Inputs

- `engine/narrative/voice_director.hpp`
- `engine/world/gptm/`

## Writes

- `gameplay/movement/player_controller.hpp`
- `gameplay/movement/player_controller.cpp`
- `engine/narrative/voice_director.cpp`

## Exit criteria

- [ ] swept-AABB vs GPTM collision mesh; bunny hop + slide + wall kick all testable
- [ ] mirror-step Act-II ability fires when SinComponent ≥ 80

## Prompt

Player controller uses swept-AABB against the GPTM collision mesh.
Movement primitives: bunny hop, slide, wall kick. Wire
Malakor/Niccolò lines through VoiceDirector; Act II mirror-step
ability fires at Sin ≥ 80.

