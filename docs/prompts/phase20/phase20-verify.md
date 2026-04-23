# phase20-verify - Phase 20 preflight: build + run sandbox_nine_circles

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `phase-week` |
| Phase | 20 |
| Week | 134 |
| Tier | A |
| Depends on | *(none)* |

## Inputs

- `apps/sandbox_infinite_seed/main.cpp`
- `engine/world/gptm/`

## Writes

- `docs/02_ROADMAP_AND_OPERATIONS.md`

## Exit criteria

- [ ] sandbox_nine_circles prints: NINE CIRCLES GROUND — circles=9 chunks=81 lod=OK veg=OK origin=OK
- [ ] All 7 acceptance criteria in docs/02 Phase 20 ticked
- [ ] CI green dev-win + dev-linux

## Prompt

Configure + build `sandbox_nine_circles`; run it; verify every Phase
20 acceptance criterion in docs/02. If any fail, back-fill the
failing week (127-134). Only after all green do we proceed to Phase
21.

