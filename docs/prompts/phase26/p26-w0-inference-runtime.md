# p26-w0-inference-runtime - inference_runtime + model_registry

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `phase-week` |
| Phase | 26 |
| Tier | A |
| Depends on | `phase20-verify` |

## Inputs

- `engine/ai_runtime/`

## Writes

- `engine/ai_runtime/inference_runtime.hpp`
- `engine/ai_runtime/inference_runtime.cpp`
- `engine/ai_runtime/model_registry.hpp`
- `engine/ai_runtime/model_registry.cpp`

## Exit criteria

- [ ] ggml CPU wrapper + BLAKE3 + Ed25519 verify works offline

## Prompt

Implement per §2.2.2 carve-out of docs/01.

