# pre-tc-bld-tool-dispatch - BLD tool dispatch: sacrilege_tools branch

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `preflight` |
| Tier | A |
| Depends on | *(none)* |

## Inputs

- `bld/bld-tools/src/sacrilege_tools.rs`
- `bld/bld-tools/src/lib.rs`

## Writes

- `bld/bld-tools/src/lib.rs`

## Exit criteria

- [ ] tool_dispatch() routes sacrilege::* names into sacrilege_tools::dispatch

## Prompt

Extend tool_dispatch() with a `name.starts_with(\"sacrilege::\")` arm
delegating to sacrilege_tools::dispatch(name, args). Export the
module publicly.

