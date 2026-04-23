# pre-gp-malakor-owner - VoiceDirector owns Malakor/Niccolò state

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `preflight` |
| Tier | A |
| Depends on | `p22-w146-player-controller` |

## Inputs

- `gameplay/characters/malakor_niccolo/character_state.hpp`
- `engine/narrative/voice_director.hpp`

## Writes

- `engine/narrative/voice_director.hpp`
- `engine/narrative/voice_director.cpp`

## Exit criteria

- [ ] VoiceDirector::tick() drives CharacterState from Sin thresholds

## Prompt

Promote `CharacterState` into a member of VoiceDirector. Update per
tick; mirror-step fires at SinComponent >= 80 (Act II gated).

