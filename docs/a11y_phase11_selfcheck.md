# Accessibility Self-Check — Phase 11

**Status:** open — signed off at Phase-11 exit gate
**Owner:** Cold Coffee Labs engine group
**Source:** [Game Accessibility Guidelines — full list](https://gameaccessibilityguidelines.com/full-list/)
**ADRs:** 0023–0030

Phase 11 introduces the first RmlUi-rendered user-facing surfaces (HUD, menus, rebind panel, settings, console). This doc cross-walks every Game Accessibility Guidelines item that those surfaces materially address, and pairs each with its implementation and verification hook.

Phase 11 is deliberately *before* the formal WCAG 2.2 audit (Phase 16) — but we meet WCAG 2.2 AA contrast on every shipping theme now, so the Phase-16 work can focus on screen-reader + i18n without retro-theming.

## Vision

| # | Guideline | Tier | Implementation | Verification |
|---|---|---|---|---|
| V1 | Use an easily readable default font size | Basic | `ui.text.scale` default 1.0; glyphs rasterised as SDF so scale is glyph-free | `unit/ui/text_scale_test.cpp` re-layout delta = 0 |
| V2 | Provide high-contrast colour scheme(s) | Basic | `ui.contrast.boost` CVar swaps RCSS tokens at runtime; boosted palette pre-audited ≥ 4.5:1 on all text | `unit/ui/theme_contrast_test.cpp` contrast matrix |
| V3 | Ensure no essential information conveyed by colour alone | Basic | HP bar uses colour + numeric + shape; interact prompt uses icon + caption + colour | visual review `docs/a11y_phase11_selfcheck_visuals.md` |
| V4 | Allow UI to be scaled without text clipping | Basic | `ui.text.scale` 0.75×–2.0× slider in Accessibility settings tab; RCSS uses `em`/`rem` units | `unit/ui/scale_roundtrip_test.cpp` |
| V5 | Provide a clear, distinct focus indicator | Intermediate | 2-px `--signal` focus ring on every focusable element; WCAG 2.2 AA (≥ 3:1 vs. every background) | `unit/ui/focus_ring_contrast_test.cpp` |
| V6 | Safe-zone respect for TV + Deck edges | Intermediate | RCSS `@media (safe-area-inset)` wraps HUD anchor points | manual verify on Steam Deck at exit gate |

## Motor

| # | Guideline | Tier | Implementation | Verification |
|---|---|---|---|---|
| M11 | All UI accessible with gamepad nav | Basic | `input.menu.*` action set always mapped to gamepad + keyboard; focus-cursor synthesised for mouse-free nav | `unit/ui/gamepad_nav_test.cpp` reachability |
| M12 | Rebind available at runtime, not just main menu | Basic | `menu_rebind.rml` available from pause menu and settings tab | `unit/ui/rebind_panel_test.cpp` |
| M13 | No timed inputs in menus | Basic | Menu action set has no `auto_repeat_ms`; `repeat_cooldown_ms = 0` disables cooldown | `unit/ui/menu_no_timing_test.cpp` |

## Hearing

| # | Guideline | Tier | Implementation | Verification |
|---|---|---|---|---|
| H4 | Provide subtitles / captions for speech + significant audio cues | Basic | Caption channel — single-line overlay subscribed to `AudioMarkerCrossed` events from Phase 10's music streamer + one-shot cue events | `unit/ui/caption_channel_test.cpp` |
| H5 | Allow captions to be sized / positioned | Intermediate | `ui.caption.scale`, `ui.caption.anchor` CVars in Accessibility tab | covered by V4 + gamepad-nav test |

## Cognitive

| # | Guideline | Tier | Implementation | Verification |
|---|---|---|---|---|
| C1 | Ensure consistent UI across menus | Basic | Single `theme.rcss` sources every control style; no per-screen overrides permitted | visual review |
| C2 | Make options discoverable | Basic | Settings tabs (Graphics / Audio / Input / Accessibility) are always-visible; no hidden menus | manual verify |
| C3 | Avoid flashing (photosensitive) | Basic | RCSS forbids `animation` keyframes that change > 3 frames per second on large areas; linter in cook tool | `unit/assets/rcss_flash_lint_test.cpp` |

## General

| # | Guideline | Tier | Implementation | Verification |
|---|---|---|---|---|
| G3 | Settings are saved | Basic | CVars with `kPersist` write to `ui.toml` / `graphics.toml` / `audio.toml` / `input.toml` on every change | `unit/config/toml_roundtrip_test.cpp` |
| G4 | Profiles | Advanced | Named profiles under `profiles/<name>/*.toml`; hot-swap via `CVarRegistry::swap_profile` | `unit/config/profile_swap_test.cpp` |
| G5 | Developer dev console does not leak cheats into release | Basic | `GW_CONSOLE_DEV_ONLY=1` in release; `dev.console.allow_release` gates the tilde toggle; cheat CVars banner when tripped | `unit/console/release_gate_test.cpp` |

## Seam reserved for Phase 16

- Screen-reader hooks (MSAA / IAccessible2 / AT-SPI) — focus-ring ARIA-equivalent markup already emitted in RCSS / RML attributes.
- Full RTL layout in RmlUi — HarfBuzz output is already correct; RmlUi direction is flipped in Phase 16.
- On-screen virtual keyboard — captured by `ui.osk.enabled` CVar; handler stubbed.
- Subtitle speaker attribution — caption channel carries a speaker-id field already.

## Sign-off

- [ ] All rows above verified as of Phase-11 exit
- [ ] `cargo test --workspace` still green (BLD regression-free)
- [ ] `ctest --preset dev-win` and `ctest --preset dev-linux` green
- [ ] `ctest --preset playable-windows` and `ctest --preset playable-linux` green when built with Phase-11 deps on
- [ ] ADR-0030 §7 exit-gate checklist complete

Signed — _(founder signature at Phase-11 close)_
