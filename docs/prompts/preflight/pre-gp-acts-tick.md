# pre-gp-acts-tick - Register Acts state-machine tick

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `preflight` |
| Tier | A |
| Depends on | `p22-w145-blasphemy` |

## Inputs

- `gameplay/acts/`

## Writes

- `gameplay/acts/acts_tick_system.hpp`
- `gameplay/acts/acts_tick_system.cpp`

## Exit criteria

- [ ] ActsStateSystem transitions Act I → II → III on thresholds verified by test

## Prompt

Create ActsStateSystem that consumes ActStateComponent and drives
transitions based on Sin / Grace totals. Register it after Martyrdom
accrual in the world tick order.

