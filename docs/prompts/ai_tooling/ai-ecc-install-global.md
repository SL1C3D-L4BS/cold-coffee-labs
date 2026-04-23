# ai-ecc-install-global - Install ECC globally + project-scoped adapters

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `ai-tooling` |
| Tier | A |
| Depends on | `ai-ecc-vendor-full` |

## Writes

- `tools/ai/install_ecc.ps1`
- `tools/ai/install_ecc.sh`
- `tools/ai/uninstall_ecc.ps1`
- `tools/ai/uninstall_ecc.sh`
- `.cursor/hooks/adapter.js`

## Exit criteria

- [ ] install_ecc --profile full places agents/commands/skills/hooks under ~/.claude and .cursor/

## Prompt

Author install/uninstall scripts (PowerShell + bash). Profile=full
copies the full upstream surface into ~/.claude and mirrors
Cursor-compatible assets into `.cursor/`.

