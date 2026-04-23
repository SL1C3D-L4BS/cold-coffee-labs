# pre-eng-ai-runtime-dispatch - Dispatch AI runtime from services

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `preflight` |
| Tier | A |
| Depends on | *(none)* |

## Inputs

- `engine/ai_runtime/director_policy.hpp`
- `engine/ai_runtime/music_symbolic.hpp`
- `engine/ai_runtime/material_eval.hpp`
- `engine/services/director/director_service.cpp`
- `engine/services/audio_weave/audio_weave.cpp`
- `engine/services/material_forge/material_forge.cpp`

## Writes

- `engine/services/director/director_service.cpp`
- `engine/services/audio_weave/audio_weave.cpp`
- `engine/services/material_forge/material_forge.cpp`

## Exit criteria

- [ ] Each service includes and calls the matching ai_runtime header
- [ ] gw_perf_gate_ai_runtime remains green

## Prompt

Add one call-site per service:
- director_service → `gw::ai::director_policy::tick(...)`
- audio_weave      → `gw::ai::music_symbolic::step(...)`
- material_forge   → `gw::ai::material_eval::evaluate(...)`
Keep stubs no-op on non-ggml builds (guarded by GW_AI_RUNTIME).

