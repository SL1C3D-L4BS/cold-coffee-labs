# p21-w136-circle-post - Sin / Rapture / Ruin post-process CVars

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `phase-week` |
| Phase | 21 |
| Week | 136 |
| Tier | A |
| Depends on | `p21-w135-horror-post` |

## Inputs

- `engine/render/post/`

## Writes

- `shaders/sin_tendril.hlsl`
- `shaders/ruin_desat.hlsl`
- `shaders/rapture_whiteout.hlsl`
- `engine/render/post/martyrdom_post.hpp`
- `engine/render/post/martyrdom_post.cpp`

## Exit criteria

- [ ] Three additive post stages toggleable via cvars r.sin_tendril / r.ruin_desat / r.rapture_whiteout

## Prompt

Add Sin tendril vignette, Ruin desat LUT, Rapture whiteout curve as
additive post stages reading SinComponent / RaptureState
(placeholder structs until P22 W143 lands).

