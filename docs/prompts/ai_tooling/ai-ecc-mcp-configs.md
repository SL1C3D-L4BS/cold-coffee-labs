# ai-ecc-mcp-configs - Merge ECC MCP servers into .cursor/mcp.json

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `ai-tooling` |
| Tier | A |
| Depends on | `ai-ecc-install-global`, `ai-graphify-mcp` |

## Writes

- `.cursor/mcp.json`

## Exit criteria

- [ ] GitHub / Context7 / Exa / Memory / Playwright / Sequential Thinking / Supabase entries present alongside graphify

## Prompt

Import ECC upstream mcp-configs/mcp-servers.json into the project
.cursor/mcp.json, keeping graphify first.

