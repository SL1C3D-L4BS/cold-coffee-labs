# p27-w4-editor-copilot - Editor Co-Pilot wrapping BLD + HITL approval

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `phase-week` |
| Phase | 27 |
| Tier | A |
| Depends on | `p27-w3-audio-director` |

## Inputs

- `bld/bld-tools/`

## Writes

- `engine/services/editor_copilot/schema/`

## Exit criteria

- [ ] All BLD tool invocations require HITL approval entry in audit log

## Prompt

HITL approval gate on every BLD tool call.

