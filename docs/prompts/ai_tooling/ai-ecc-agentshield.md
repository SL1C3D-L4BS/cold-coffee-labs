# ai-ecc-agentshield - Integrate AgentShield CI gate

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `ai-tooling` |
| Tier | A |
| Depends on | `ai-ecc-install-global` |

## Writes

- `.github/workflows/agentshield.yml`
- `docs/10_APPENDIX_ADRS_AND_REFERENCES.md`

## Exit criteria

- [ ] PRs touching .cursor/, .claude/, or .mcp.json run `npx ecc-agentshield scan`
- [ ] ADR-0120 landed

## Prompt

Wire AgentShield into CI (file-path triggered). Add `/security-scan`
slash entry. Write ADR-0120 (AI supply-chain hygiene).

