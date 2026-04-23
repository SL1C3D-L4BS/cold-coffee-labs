# p21-w139-audio-dialogue - MartyrdomAudioController + dialogue-graph line triggers

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `phase-week` |
| Phase | 21 |
| Week | 139 |
| Tier | A |
| Depends on | `p21-w138-circle-skins`, `pre-eng-narrative-tick` |

## Inputs

- `engine/audio/`
- `engine/narrative/dialogue_graph.hpp`

## Writes

- `engine/audio/martyrdom_audio_controller.hpp`
- `engine/audio/martyrdom_audio_controller.cpp`
- `engine/narrative/dialogue_graph.cpp`

## Exit criteria

- [ ] Sin choir / Rapture thunder / Ruin drone layers BPM-crossfade deterministically
- [ ] dialogue_graph fires voice lines on Sin threshold events

## Prompt

MartyrdomAudioController owns four stem layers (base, sin_choir,
rapture_thunder, ruin_drone) and BPM-crossfades between them.
dialogue_graph receives SinComponent threshold events and triggers
Malakor/Niccolò lines.

