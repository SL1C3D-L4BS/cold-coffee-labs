# ai-ecc-all-rules - Copy all ECC language rule packs

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `ai-tooling` |
| Tier | A |
| Depends on | `ai-ecc-install-global` |

## Writes

- `.cursor/rules/ecc-common/`
- `.cursor/rules/ecc-typescript/`
- `.cursor/rules/ecc-python/`
- `.cursor/rules/ecc-golang/`
- `.cursor/rules/ecc-swift/`
- `.cursor/rules/ecc-php/`

## Exit criteria

- [ ] All language rule packs present under .cursor/rules/ecc-*/

## Prompt

Copy every upstream ECC rule pack (common + typescript + python +
golang + swift + php). No language allowlist.

