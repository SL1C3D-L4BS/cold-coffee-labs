# ADR 0027 — RmlUi render backend via Vulkan HAL

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 11 Wave 11D)
- **Deciders:** Cold Coffee Labs engine group
- **Related:** ADR-0026 (UI service), blueprint §3.20, `engine/render/frame_graph/`

## Context

RmlUi 6.2 ships an optional Vulkan backend as PR #836 (still draft upstream at Phase 11 kickoff). Greywater already owns a Vulkan HAL in `engine/render/hal/` — the RmlUi backend must talk to *that*, never to a raw `VkDevice`.

## Decision

Implement `engine/ui/backends/rml_render_hal.{hpp,cpp}` as an `Rml::RenderInterface`:

- Every `RenderCompiledGeometry` call binds one of a small cache of graphics pipelines keyed by `{vertex_layout, scissor_enabled, texture_bound}`. The cache lives in the HAL and is shared with the debug-draw pipeline cache.
- `CompileGeometry` returns an engine-side `hal::MeshHandle` packed into the RmlUi handle slot. Geometry is uploaded once to a device-local `hal::VulkanBuffer`; updates are rare (layout change) and go through the HAL's transient uploader.
- `LoadTexture` / `GenerateTexture` map to `hal::VulkanImage` creation. Static RmlUi textures (icons, atlas pages) are encoded as **BC7** at cook-time; dynamic glyph atlas (ADR-0028) is `R8_UNORM` live-uploaded. Mipmaps disabled for pixel-exact UI.
- `EnableScissorRegion` / `SetScissorRegion` map to Vulkan dynamic scissor state (`vkCmdSetScissor`) on the HAL command buffer.
- `EnableClipMask` / `RenderToClipMask` implement the stencil-based rounded-corner clipping that landed in RmlUi 6.x; the HAL exposes a depth/stencil dynamic-rendering path that PR #836 requires.
- Transform stack (`SetTransform`) multiplies into a push-constant block; no uniform buffer per draw.

### Frame-graph integration

A new `ui_pass` `FrameGraphNode` is registered at engine init:

- **Reads:** `UIColorInput` (the resolved scene colour, `LOAD_OP_LOAD`)
- **Writes:** `UIColorOutput` (a transient rename of the swapchain colour attachment)
- **Runs after:** `scene_resolve_pass`
- **Runs before:** `present_pass`
- **Command buffer:** allocated by the frame-graph scheduler; RmlUi's backend records into it via HAL primitives.
- **Contexts drained per pass:** HUD (primary), console (debug overlay), rml-debugger (debug overlay). Ordered by z-layer — console on top.

### Vendoring strategy

PR #836 is pulled as a vendored patch (`third_party/rmlui_vulkan_backend.patch`) applied on top of the 6.2 tag in CPM. The patch is rewritten so the backend never touches a raw `VkDevice`; all calls funnel through `hal::VulkanDevice`. An upstream exit path is planned: once upstream merges #836, we open a follow-up PR that routes their code through our HAL interfaces (or loops back to the canonical Vulkan objects, configurable at compile time).

### Gating

Same as ADR-0026: `GW_UI_RMLUI=ON` pulls the backend; `GW_UI_RMLUI=OFF` substitutes a null `Rml::RenderInterface` that records draw-counts for tests but never touches the GPU.

## Perf contract

| Gate | Target | Hardware basis |
|---|---|---|
| UI pass GPU time at 1080p (HUD visible) | ≤ 0.8 ms | GTX 1060 |
| Pipeline-cache hit ratio steady-state | ≥ 98 % | HUD + console open |
| Texture-cache hit ratio steady-state | ≥ 98 % | Cold Drip theme |
| Glyph-atlas upload delta per frame | ≤ 64 KB steady, ≤ 512 KB worst | see ADR-0028 |

## Tests (≥ 3 in this ADR)

1. Pipeline-cache eviction under rapid scissor toggle
2. Texture-cache lifecycle: upload → bind × 100 frames → destroy, zero leaks (VMA stats)
3. Frame-graph pass correctly orders HUD above console above debugger across one full frame

## Alternatives considered

- **Write the backend directly against `VkDevice`** — violates the HAL quarantine (CLAUDE.md #2 non-negotiables).
- **Render RmlUi to an offscreen target and blit** — doubles the fillrate cost at 1080p; measurable on an RX 580.
- **Wait for upstream #836 to merge** — blocks the *Playable Runtime* milestone on an external cadence. Unacceptable.

## Consequences

- A studio fork of RmlUi at `third_party/rmlui/` is introduced. Rebasing on each upstream tag is a Phase-18 maintenance task.
- The `ui_pass` frame-graph node is the first post-resolve pass that does not run at the scene resolution; it always targets the swapchain (to avoid a second resolve). Phase 17's VFX work inherits this pattern.
