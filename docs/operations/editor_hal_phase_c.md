# Editor HAL — Phase C audit (Wave 1C.9)

**Purpose:** Operational checklist for the “editor on full HAL” row in [stub_completion_matrix_W1C_followups.md](./stub_completion_matrix_W1C_followups.md) (W1C.9). This is a **review artifact**, not a runtime gate: confirm the editor’s Vulkan path stays aligned with ADR-0003 (dynamic rendering, sync2, timeline semaphores where applicable) before expanding post-FX or alternate backends.

## Scope

1. **Dynamic rendering** — Editor scene pass and swapchain presentation use the HAL’s dynamic-rendering entry points (no legacy `VkRenderPass`-only assumptions in new code).
2. **Resource truth** — Render targets panel and histogram paths consume **live** `VkImage` / readback surfaces from the same HAL handles the scene pass uses (see W1C.5–W1C.6).
3. **FSR 2 / HDR toggles** — [`editor/render/render_settings.hpp`](../../editor/render/render_settings.hpp) documents scaffold wiring (`fsr2_enabled`, `hdr_enabled`). Audit that toggles route to [`engine/render/post/upscale_and_hdr_bridge.cpp`](../../engine/render/post/upscale_and_hdr_bridge.cpp) (or successor) when the post-FX wave registers passes; until then, UI may be inert but must not assert or leak handles.
4. **Cross-read** — `docs/06_ARCHITECTURE.md` (HAL + frame graph) and `docs/05_RESEARCH_BUILD_AND_STANDARDS.md` (HAL commitments) stay the contract; this file tracks **editor-specific** verification steps.

## Exit criteria (W1C.9)

- [ ] No placeholder “synthetic” luminance or RT slots in default editor paths (W1C.6 closed).
- [ ] `RenderSettings::fsr2_enabled` / `hdr_enabled` either no-op safely or drive registered passes; document which in code comments when behavior changes.
- [ ] `rg` for known stub strings in `editor_app` / `editor/render` reviewed after each HAL-facing change.

**Last reviewed:** _— (append date on each audit pass)._
