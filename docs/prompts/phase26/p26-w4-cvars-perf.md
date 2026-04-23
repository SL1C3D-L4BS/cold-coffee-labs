# p26-w4-cvars-perf - ai_cvars + gw_perf_gate_ai_runtime

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `phase-week` |
| Phase | 26 |
| Tier | A |
| Depends on | `p26-w3-material-eval` |

## Writes

- `engine/ai_runtime/ai_cvars.hpp`
- `engine/ai_runtime/ai_cvars.cpp`
- `tests/perf/director_budgets_perf_test.cpp`

## Exit criteria

- [ ] Total AI budget ≤ 0.74 ms on RX 580

## Prompt

All AI runtime surfaces controlled via cvars; perf gate enforces
budget.

