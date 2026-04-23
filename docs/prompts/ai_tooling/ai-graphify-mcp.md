# ai-graphify-mcp - Register graphify MCP in .cursor/mcp.json

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `ai-tooling` |
| Tier | A |
| Depends on | `ai-graphify-build` |

## Writes

- `.cursor/mcp.json`

## Exit criteria

- [ ] .cursor/mcp.json contains a graphify server entry Opus can dispatch

## Prompt

Add a project-local MCP server definition running
`python -m graphify.serve graphify-out/graph.json` so Claude Opus
can call query_graph / get_neighbors / shortest_path.

