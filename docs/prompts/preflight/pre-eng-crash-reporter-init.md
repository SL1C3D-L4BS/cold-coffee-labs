# pre-eng-crash-reporter-init - crash_reporter::install() in every main()

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `preflight` |
| Tier | A |
| Depends on | *(none)* |

## Inputs

- `engine/core/crash_reporter.hpp`

## Writes

- `editor/main.cpp`
- `apps/sandbox_infinite_seed/main.cpp`
- `apps/sandbox_playable/main.cpp`
- `tools/cook/main.cpp`

## Exit criteria

- [ ] Every shipped binary calls gw::core::crash_reporter::install() as first line of main()

## Prompt

Add `gw::core::crash_reporter::install();` as the first line of each
main() (editor, every sandbox app, cook worker). Include
"engine/core/crash_reporter.hpp".

