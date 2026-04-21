# Accessibility Self-Check — Phase 12

**Status:** open — signed off at Phase-12 exit gate
**Owner:** Cold Coffee Labs engine group
**Source:** [Game Accessibility Guidelines — full list](https://gameaccessibilityguidelines.com/full-list/)
**ADRs:** 0031–0038

Phase 12 adds physics + character movement + physics debug draw. The only player-facing surface is the dev-console overlay (physics debug commands) and, transitively, any HUD affordance that reads physics state (e.g. interact-prompt raycast). This doc is deliberately slim — the major accessibility pass is Phase 16.

## Vision

| # | Guideline | Tier | Implementation | Verification |
|---|---|---|---|---|
| V2 | Provide high-contrast colour scheme(s) | Basic | `physics.debug.*` line colours read from the boosted palette when `ui.contrast.boost = true` | `unit/physics/debug_contrast_test.cpp` |
| V3 | No essential info by colour alone | Basic | Interact-prompt is shape+icon+caption; physics debug surfaces are dev-only | visual review |
| V6 | Safe-zone respect | Intermediate | Debug overlay uses RCSS `@media (safe-area-inset)` — inherited from Phase 11 | manual verify on Steam Deck |

## Motor

| # | Guideline | Tier | Implementation | Verification |
|---|---|---|---|---|
| M11 | Gamepad reachable | Basic | Physics-debug toggles bound to CVars → reachable through the Accessibility settings tab (debug build only) | `unit/ui/gamepad_nav_test.cpp` (covers) |

## Vestibular / Motion

| # | Guideline | Tier | Implementation | Verification |
|---|---|---|---|---|
| VM1 | Reduce motion option respected | Basic | `ui.reduce_motion = true` suppresses continuous camera shake on large contact impulses; `physics.char.camera.shake_scale` scales to 0 under reduce-motion | `unit/physics/camera_shake_reduce_motion_test.cpp` |
| VM2 | Avoid photosensitive flash | Basic | Physics debug draw never strobes; RCSS forbids >3 Hz flashing on the dev console | `unit/assets/rcss_flash_lint_test.cpp` (covers) |

## Cognitive

| # | Guideline | Tier | Implementation | Verification |
|---|---|---|---|---|
| C3 | Avoid flashing | Basic | As above | — |
| C5 | Provide option to turn off non-essential camera effects | Advanced | `physics.char.camera.shake_scale` CVar [0, 1]; defaults to 1.0; UI slider ships in settings Accessibility tab | `unit/config/cvar_registry_test.cpp` (value range) |

## General

| # | Guideline | Tier | Implementation | Verification |
|---|---|---|---|---|
| G5 | Dev cheats do not leak to release | Basic | `physics.pause`, `physics.step`, `physics.debug` are all `kCmdDevOnly`; release builds strip them unless `dev.console.allow_release = true` | `unit/console/release_gate_test.cpp` (covers) |

## Seam reserved for Phase 16

- Haptic-contrast cue on contact events (accessibility rumble for hard-of-sight users): emits `haptic.tap` through the Phase-10 input haptics pipeline; currently latched to `ui.contrast.boost`. Formal Phase-16 item.
- Subtitle for contact SFX — piggy-backs on the Phase-11 caption channel; Phase 16 adds speaker-id mapping.

## Sign-off

- [ ] Rows above verified at Phase-12 exit
- [ ] `unit/physics/camera_shake_reduce_motion_test.cpp` green
- [ ] `unit/physics/debug_contrast_test.cpp` green
- [ ] `ctest --preset dev-win` + `ctest --preset dev-linux` green
- [ ] ADR-0038 §9 exit-gate checklist complete

Signed — _(founder signature at Phase-12 close)_
