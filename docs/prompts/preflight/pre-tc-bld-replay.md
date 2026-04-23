# pre-tc-bld-replay - bld-replay gameplay_schema reached by recorder

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `preflight` |
| Tier | A |
| Depends on | *(none)* |

## Inputs

- `bld/bld-replay/src/gameplay_schema.rs`
- `bld/bld-replay/src/lib.rs`

## Writes

- `bld/bld-replay/src/lib.rs`

## Exit criteria

- [ ] record() serialises GameplayEvent at tick cadence; playback round-trips to identical state hash

## Prompt

`pub use gameplay_schema::*;`; add record(event, tick) + playback()
public APIs. Used by P22 W148 determinism test.

