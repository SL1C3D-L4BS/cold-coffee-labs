# ai-graphify-install - Install graphify via pipx + Cursor rule

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `ai-tooling` |
| Tier | A |
| Depends on | *(none)* |

## Writes

- `tools/ai/requirements.txt`
- `tools/ai/README.md`
- `.cursor/rules/graphify.mdc`
- `.graphifyignore`

## Exit criteria

- [ ] pipx list shows graphifyy at pinned version
- [ ] .cursor/rules/graphify.mdc exists

## Prompt

Install graphifyy via pipx from PyPI; pin version in
tools/ai/requirements.txt. Run `graphify cursor install` to drop the
Cursor rule. Add a .graphifyignore excluding build/, third_party/,
target/, docs/translations/.

