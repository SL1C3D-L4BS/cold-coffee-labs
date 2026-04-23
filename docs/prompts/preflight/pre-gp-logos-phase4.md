# pre-gp-logos-phase4 - Logos Phase4 selector stub-wire

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `preflight` |
| Tier | B |
| Depends on | *(none)* |

## Inputs

- `gameplay/boss/logos/phase4_selector.hpp`

## Writes

- `engine/services/director/director_service.hpp`
- `engine/services/director/director_service.cpp`

## Exit criteria

- [ ] DirectorService holds an optional<Phase4Selector> zero-overhead on non-Logos levels

## Prompt

Add an `std::optional<gameplay::boss::logos::Phase4Selector>` member
to DirectorService. Instantiate when a level flagged `logos_arena`
is loaded. No behaviour — stub-wire only; real Phase 23 impl later.

