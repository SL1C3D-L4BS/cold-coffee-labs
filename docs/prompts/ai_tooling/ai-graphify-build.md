# ai-graphify-build - Run initial graphify build and commit graph.json

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `ai-tooling` |
| Tier | A |
| Depends on | `ai-graphify-install` |

## Writes

- `graphify-out/GRAPH_REPORT.md`
- `graphify-out/graph.json`
- `.gitignore`

## Exit criteria

- [ ] graphify-out/GRAPH_REPORT.md non-empty

## Prompt

Run `graphify .`. Commit `graphify-out/GRAPH_REPORT.md` and
`graphify-out/graph.json`. Add cache/, manifest.json, cost.json to
.gitignore.

