# ai-ecc-vendor-full - Vendor everything-claude-code at pinned SHA

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `ai-tooling` |
| Tier | A |
| Depends on | *(none)* |

## Writes

- `third_party/_vendor/everything-claude-code/`

## Exit criteria

- [ ] Full ECC upstream surface present at pinned commit

## Prompt

git clone https://github.com/affaan-m/everything-claude-code.git
into third_party/_vendor/. Pin to latest tagged release. Vendor
full surface: all agents, skills, commands, rules, hooks, MCP
configs, plugin manifests.

