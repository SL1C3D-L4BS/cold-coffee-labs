# pre-eng-fsr2-hal-frame-graph - FSR2 + HDR output + present timing in frame graph

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `preflight` |
| Tier | A |
| Depends on | *(none)* |

## Inputs

- `engine/render/post/fsr2/`
- `engine/render/hal/hdr_output.hpp`
- `engine/render/hal/present_timing.hpp`
- `engine/render/frame_graph/frame_graph.cpp`

## Writes

- `engine/render/frame_graph/frame_graph.cpp`

## Exit criteria

- [ ] Frame graph inserts FSR2 pass between scene-color and tonemap when cvar.r_fsr2=1
- [ ] HDR10 metadata set via hdr_output when cvar.r_hdr=1

## Prompt

Register FSR2 upscale pass + HDR10 swapchain hooks + present_timing
in the frame graph build(). Gate each on RenderSettings.

