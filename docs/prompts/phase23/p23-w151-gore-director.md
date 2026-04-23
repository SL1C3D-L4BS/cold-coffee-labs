# p23-w151-gore-director - Gore system + hybrid AI Director integration

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `phase-week` |
| Phase | 23 |
| Week | 151 |
| Tier | A |
| Depends on | `p23-w150-enemies-b`, `pre-eng-ai-runtime-dispatch` |

## Inputs

- `engine/services/gore/`
- `engine/services/director/`

## Writes

- `gameplay/combat/gore_system.hpp`
- `gameplay/combat/gore_system.cpp`

## Exit criteria

- [ ] Dismemberment + limb entities + decal placement on hit
- [ ] Director RL params active and bounded

## Prompt

Dismemberment with limbs as nav obstacles; hits place blood decals.
Hybrid Director wires from Phase 26 output.

