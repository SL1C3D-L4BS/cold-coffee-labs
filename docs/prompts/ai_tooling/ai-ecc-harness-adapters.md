# ai-ecc-harness-adapters - Install Codex/OpenCode/Kiro/Trae/Gemini/Antigravity/Copilot adapters

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `ai-tooling` |
| Tier | A |
| Depends on | `ai-ecc-install-global` |

## Writes

- `.codex/`
- `.opencode/`
- `.kiro/`
- `.trae/`
- `.gemini/`
- `.agents/`

## Exit criteria

- [ ] Dormant harness adapters present; Cursor + Claude paths actively wired

## Prompt

Install every ECC harness adapter. Those without a local runtime
are registered dormant so they ship but don't run.

