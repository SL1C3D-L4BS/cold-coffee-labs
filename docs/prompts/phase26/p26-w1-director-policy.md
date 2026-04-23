# p26-w1-director-policy - L4D-style Director policy + bounded RL

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `phase-week` |
| Phase | 26 |
| Tier | A |
| Depends on | `p26-w0-inference-runtime` |

## Writes

- `engine/ai_runtime/director_policy.hpp`
- `engine/ai_runtime/director_policy.cpp`

## Exit criteria

- [ ] Build-Up / Sustained Peak / Peak Fade / Relax transitions + reset-on-rewind

## Prompt

State machine with bounded RL parameter clamps; rollback-safe.

