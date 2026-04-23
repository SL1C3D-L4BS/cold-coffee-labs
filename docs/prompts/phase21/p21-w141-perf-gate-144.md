# p21-w141-perf-gate-144 - RX 580 @ 1080p / 144 FPS Hell Frame perf gate

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `phase-week` |
| Phase | 21 |
| Week | 141 |
| Tier | A |
| Depends on | `p21-w140-hud-editor-panels` |

## Inputs

- `tests/perf/`

## Writes

- `tests/perf/hell_frame_perf_test.cpp`
- `tests/perf/CMakeLists.txt`

## Exit criteria

- [ ] gw_perf_gate_hell_frame asserts 1080p @ 144 FPS RX 580 with horror_post + decals + HUD + dialogue triggers active

## Prompt

Author a deterministic perf gate that reproduces a Violence Circle
scene and asserts frame time ≤ 6.94 ms. Attach to ctest -L perf.

