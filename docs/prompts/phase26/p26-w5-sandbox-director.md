# p26-w5-sandbox-director - apps/sandbox_director — standalone headless + ImGui

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `phase-week` |
| Phase | 26 |
| Tier | A |
| Depends on | `p26-w4-cvars-perf` |

## Writes

- `apps/sandbox_director/main.cpp`
- `apps/sandbox_director/CMakeLists.txt`

## Exit criteria

- [ ] gw_perf_gate_director ≤ 0.1 ms; prints DIRECTOR SANDBOX OK

## Prompt

Standalone app; both headless-CI and interactive ImGui paths.

