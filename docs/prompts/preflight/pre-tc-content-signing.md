# pre-tc-content-signing - Cook worker calls content_signing::sign_artifact()

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `preflight` |
| Tier | A |
| Depends on | *(none)* |

## Inputs

- `tools/cook/content_signing.hpp`

## Writes

- `tools/cook/cook_worker.cpp`

## Exit criteria

- [ ] Cooked artifact on disk carries Ed25519 signature in manifest

## Prompt

In cook_worker post-finalise, call
`gw::cook::content_signing::sign_artifact(path, private_key)` and
record signature in cook manifest (ADR-0114).

