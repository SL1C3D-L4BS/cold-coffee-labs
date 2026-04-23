# ai-ecc-hooks - Install ECC 15-event Cursor hook chain

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `ai-tooling` |
| Tier | A |
| Depends on | `ai-ecc-install-global`, `ai-graphify-hook` |

## Writes

- `.cursor/hooks/`

## Exit criteria

- [ ] 15-event chain coexists with graphify PreToolUse hook

## Prompt

Use .cursor/hooks/adapter.js to sequence all 15 ECC events
alongside graphify's PreToolUse hook. Adjust order if both claim
the same lifecycle point.

