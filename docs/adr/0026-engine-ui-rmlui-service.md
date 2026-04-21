# ADR 0026 — `engine/ui/`: RmlUi service, context lifecycle, system/file interfaces

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 11 Wave 11D)
- **Deciders:** Cold Coffee Labs engine group
- **Related:** ADR-0027 (Vulkan-HAL backend), ADR-0028 (text shaping), blueprint §3.20, CLAUDE.md #12

## Context

Blueprint §3.20 and CLAUDE.md #12 name **RmlUi** as the in-game UI library. Phase 11 must make it shippable. Upstream RmlUi 6.2 (released 2026-01-11) carries the semaphore fix from PR #874 and ships native touch + a data-model debugger; 6.2 is our pin.

## Decision

Ship `engine/ui/` with a `UIService` that:

- Owns the single `Rml::Context` per window. A secondary context is allowed for overlays (console, debugger) but they share the one frame-graph pass (ADR-0027).
- Drives `context->Update()` + `context->Render()` from the main thread, after `InputService::update` and before the render-submit.
- Implements `Rml::SystemInterface` (`rml_system.cpp`): time via `gw::core::Time`, logs via `gw::core::logger`, clipboard via a new `engine/platform/clipboard` seam.
- Implements `Rml::FileInterface` (`rml_filesystem.cpp`) against the virtual FS used by cooked assets — so `.rml`, `.rcss`, and font files all go through the same path as textures/meshes.
- Forwards input via Phase-10's `RawSnapshot` through a `rml_input_bridge` that translates `KeyCode`/`MouseButton` to RmlUi key/modifier enums. Gamepad nav uses the same bridge with a synthesised focus-cursor driven by `input.menu.*` actions.
- Exposes `UIService::set_locale(StringView tag)` which routes to the `LocaleBridge` in ADR-0028.
- Integrates the `rml_debugger` plugin in debug builds only; console command `ui.debugger` toggles its overlay.

### Gating

`GW_UI_RMLUI` (default **ON** for the `gw_runtime` preset, **OFF** for `dev-win` / `dev-linux` CI) pulls the RmlUi CPM package. When off, a stub implementation of `UIService` compiles and runs — it registers the same command handlers, accepts the same CVar changes, and exposes the same `update()/render()` entry points, but draws nothing. Every test that asserts "3 UI elements rendered" is skipped (tagged `[skip-if-no-ui]`) when gated off.

### Theming

A single `theme.rcss` under `assets/ui/` defines the **Cold Drip** palette as CSS custom properties (`--obsidian`, `--greywater`, `--brew`, `--crema`, `--signal`, `--bean`). `ui.contrast.boost = true` swaps to a high-contrast token set; no UI code is hand-tinted.

### Context lifecycle and focus

- `FocusChanged` events on the Phase-11A bus steer RmlUi focus via `Rml::ElementDocument::Focus`.
- Gamepad navigation emulates `tab`/`shift-tab` / `space` / `enter` via `input.menu.nav_*` actions. Every focusable element has a visible 2-px `--signal`-coloured ring (WCAG 2.2 AA contrast ≥ 3:1 against every background).
- `WindowResized` resizes every context and reloads the layout.

## Perf contract

See ADR-0028 for text-shape budgets and ADR-0027 for GPU-pass budgets. `UIService::update()` itself (no layout invalidation) is budgeted at **≤ 0.1 ms** per frame at 1080p.

## Tests (≥ 4 in this ADR)

1. `UIService` null-backend constructs and tears down cleanly
2. `set_locale` propagates through `LocaleBridge` and triggers a re-layout
3. `WindowResized` event rebuilds the context dimensions
4. Debugger plugin toggles cleanly when enabled in debug builds

Additional UI tests live in ADR-0027/0028/0029 territory.

## Alternatives considered

- **Noesis GUI** — commercial; license costs + vendor lock.
- **Ultralight** — AGPL / commercial; unclear embedded runtime size, no Vulkan HAL.
- **Write our own** — explicitly forbidden by CLAUDE.md #13.
- **Delay UI integration to Phase 16** — makes the *Playable Runtime* milestone unreachable. Non-starter.

## Consequences

- `engine/platform/clipboard` is introduced in this wave as a thin OS-seam (Win32 `OpenClipboard`, Wayland `wl_data_device`, X11 `XClipboard`). Three-file module; 50 lines each.
- `rml_debugger` lives in a debug-only `.cpp` compiled under `#ifdef GW_UI_RMLUI_DEBUGGER` (default ON in Debug, OFF in Release).
- Phase 12's physics-anchored HUD (world-space → screen-space name tags) needs a `RmlUi::DataModel` feature that's already in 6.2; no retrofit needed.
