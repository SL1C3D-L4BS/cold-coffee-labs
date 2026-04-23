# p24-w158-platform-builds - Windows + Linux + Steam Deck platform builds signed

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `phase-week` |
| Phase | 24 |
| Week | 158 |
| Tier | A |
| Depends on | `p24-w157-wcag-audit` |

## Inputs

- `tools/cook/content_signing.hpp`

## Writes

- `.github/workflows/release.yml`

## Exit criteria

- [ ] Installer + AppImage/DEB + Steam Deck (steamrt-sniper) builds; Ed25519-signed cooked assets verified at load in Release

## Prompt

Build + sign + verify per ADR-0114.

