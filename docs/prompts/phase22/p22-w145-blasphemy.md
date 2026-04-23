# p22-w145-blasphemy - BlasphemySystem (5 canon + 3 unlockables) + StatComposition

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `phase-week` |
| Phase | 22 |
| Week | 145 |
| Tier | A |
| Depends on | `p22-w144-accrual-systems` |

## Inputs

- `gameplay/martyrdom/`
- `gameplay/acts/`

## Writes

- `gameplay/martyrdom/blasphemy_system.hpp`
- `gameplay/martyrdom/blasphemy_system.cpp`
- `gameplay/martyrdom/stat_composition_system.hpp`
- `gameplay/martyrdom/stat_composition_system.cpp`

## Exit criteria

- [ ] All 5 canon + 3 Story-Bible unlockables (Salt Pillar, Inverted Gravity, Hymn Needles) gated to Acts II/III
- [ ] StatCompositionSystem produces ResolvedStats = base + Blasphemy + Grace

## Prompt

Implement Blasphemies as data + behaviour. Act gating via
ActStateComponent. StatCompositionSystem produces the final
ResolvedStats consumed by the player controller.

