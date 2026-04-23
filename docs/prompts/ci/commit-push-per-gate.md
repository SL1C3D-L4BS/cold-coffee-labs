# commit-push-per-gate - Commit + push after each exit gate

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `ci` |
| Tier | A |
| Depends on | *(none)* |

## Exit criteria

- [ ] Branch pivot/phase-21-22-execution pushed after each gate
- [ ] PR opened after P21 exit + P22 exit

## Prompt

Branch: pivot/phase-21-22-execution. Commit + push after: P20
preflight green, each P21 week green, each P22 week green, P21
exit, P22 exit. PR after P21 exit for early review; second PR
after P22 exit.

