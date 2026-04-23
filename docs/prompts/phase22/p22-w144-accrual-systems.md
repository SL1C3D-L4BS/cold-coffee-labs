# p22-w144-accrual-systems - Sin/Mantra/Rapture/Ruin accrual + signature

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `phase-week` |
| Phase | 22 |
| Week | 144 |
| Tier | A |
| Depends on | `p22-w143-ecs-components` |

## Inputs

- `engine/narrative/sin_signature.hpp`

## Writes

- `gameplay/martyrdom/sin_accrual_system.hpp`
- `gameplay/martyrdom/sin_accrual_system.cpp`
- `gameplay/martyrdom/mantra_accrual_system.hpp`
- `gameplay/martyrdom/mantra_accrual_system.cpp`
- `gameplay/martyrdom/rapture_check_system.hpp`
- `gameplay/martyrdom/rapture_check_system.cpp`
- `gameplay/martyrdom/rapture_tick_system.hpp`
- `gameplay/martyrdom/rapture_tick_system.cpp`
- `gameplay/martyrdom/ruin_tick_system.hpp`
- `gameplay/martyrdom/ruin_tick_system.cpp`

## Exit criteria

- [ ] All five systems register in canonical order
- [ ] sin_signature rolling fingerprint deterministic

## Prompt

Five ECS systems; ordered per design. sin_signature updates on every
SinComponent delta; fingerprint used by Logos Phase 4.

