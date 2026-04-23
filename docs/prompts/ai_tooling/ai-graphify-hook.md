# ai-graphify-hook - Install graphify post-commit + post-checkout hooks

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `ai-tooling` |
| Tier | B |
| Depends on | `ai-graphify-build` |

## Writes

- `.git/hooks/post-commit`
- `.git/hooks/post-checkout`

## Exit criteria

- [ ] Hook runs without interfering with cpm-cache / ninja git state

## Prompt

Run `graphify hook install`. Verify no conflict with existing hooks;
adjust adapter chaining if needed.

