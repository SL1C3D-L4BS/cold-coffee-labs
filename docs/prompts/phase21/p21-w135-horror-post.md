# p21-w135-horror-post - horror_post.hlsl — single-pass compute + CPU wire

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `phase-week` |
| Phase | 21 |
| Week | 135 |
| Tier | A |
| Depends on | `phase20-verify` |

## Inputs

- `shaders/CMakeLists.txt`
- `engine/render/post/`
- `engine/render/frame_graph/`

## Writes

- `shaders/horror_post.hlsl`
- `engine/render/post/horror_post.hpp`
- `engine/render/post/horror_post.cpp`
- `tests/perf/horror_post_perf_test.cpp`

## Exit criteria

- [ ] horror_post pass registered in frame graph after tonemap
- [ ] gw_perf_gate_horror_post: ≤ 0.25 ms @ 1080p on RX 580

## Prompt

Write a single-pass compute shader `horror_post.hlsl` combining
chromatic aberration + film grain + screen shake. Drive by three
cvars (r.horror.ca_strength, r.horror.grain, r.horror.shake).
Register in frame_graph after tonemap. Add a perf gate.

