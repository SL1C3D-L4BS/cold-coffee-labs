# Accessibility Self-Check — Phase 10

**Status:** open — signed off at Phase-10 exit gate
**Owner:** Cold Coffee Labs engine group
**Source:** [Game Accessibility Guidelines — full list](https://gameaccessibilityguidelines.com/full-list/)
**ADRs:** 0017–0022

This document cross-walks every Game-Accessibility-Guidelines item that Phase 10 materially addresses against its implementation, test, and manual verification. **Every row must be green before Phase 10 closes.**

## Motor

| # | Guideline | Tier | Implementation | Verification |
|---|---|---|---|---|
| M1 | Allow controls to be remapped / reconfigured | Basic | `engine/input/rebind.hpp` + editor ImGui panel | `unit/input/rebind_test.cpp` round-trip fuzz |
| M2 | Ensure that all haptics / vibrations can be disabled or adjusted | Basic | `haptics.toml` global slider + per-device toggle, backed by `Haptics::set_global_volume` | `unit/input/haptics_test.cpp` |
| M3 | Adjust sensitivity of controls | Basic | Per-axis `Processor::Scale`, exposed in rebind panel | `unit/input/processor_test.cpp` |
| M4 | Ensure that all UI is accessible with the same input method as gameplay | Basic | `menu` `ActionSet` is always bound to both keyboard + gamepad; `one_button_simple` preset for single-device operation | `unit/input/action_set_test.cpp` |
| M5 | Avoid / provide alternatives to requiring held buttons | Intermediate | `hold_to_toggle` modifier — press converts to latched toggle; double-tap un-latches | `unit/input/hold_to_toggle_test.cpp` property tests |
| M6 | Avoid / provide alternatives to repeated inputs (mashing) | Intermediate | `auto_repeat_ms` per-action — single press emits a configurable repeat train | `unit/input/auto_repeat_test.cpp` |
| M7 | Support more than one input device simultaneously | Intermediate | `InputService` fuses all devices; no primary-device lock | `unit/input/device_fusion_test.cpp` |
| M8 | Provide very simple control schemes compatible with switch / eye tracking | Advanced | `single_switch_scanner` + `one_button_simple.toml` preset | `unit/input/scanner_test.cpp` reachability test |
| M9 | Ensure controls are as simple as possible — ≥ 0.5 s cool-down available | Advanced | `repeat_cooldown_ms` global + per-action override | `unit/input/cooldown_test.cpp` |
| M10 | Make precise timing inessential / assistable | Advanced | `assist_window_ms`, `can_be_invoked_while_paused` flags on `Action` | `unit/input/assist_window_test.cpp` |

## Hearing

| # | Guideline | Tier | Implementation | Verification |
|---|---|---|---|---|
| H1 | Provide separate volume controls or mutes for effects, speech and background / music | Basic | Bus volumes / mutes in `audio_mixing.toml`; `AudioService::set_bus_volume`, `set_bus_mute` | `unit/audio/bus_volume_test.cpp` |
| H2 | Provide a stereo / mono toggle | Intermediate | `MixerGraph::set_downmix_mode({Stereo, Mono, Bypass})` | `unit/audio/downmix_test.cpp` |
| H3 | Don't rely solely on audio cues | Intermediate | Accessibility API emits a parallel event stream for subtitles / UI (hooked in Phase 11); Phase-10 contract: every gameplay audio cue is accompanied by an `AudioCueEvent` on the event bus | `unit/audio/cue_event_test.cpp` |

## General

| # | Guideline | Tier | Implementation | Verification |
|---|---|---|---|---|
| G1 | Ensure settings are saved / remembered | Basic | TOML round-trip (`action_map_toml.cpp`) + `audio_mixing.toml` | `unit/input/toml_roundtrip_test.cpp`, `unit/audio/mixing_toml_test.cpp` |
| G2 | Allow settings to be saved to profiles | Advanced | Named `input.$PROFILE.toml`; `InputService::load_profile` hot-swap | `unit/input/profile_swap_test.cpp` |

## Adaptive-controller auto-profile

| Trigger | Action |
|---|---|
| Xbox Adaptive Controller VID/PID detected | Activate `accessibility.adaptive_default = true`; apply `hold_to_toggle` globally; load `input.adaptive_default.toml` |
| Quadstick detected | Same as above |

Verified manually with vendor-provided device traces (`tests/fixtures/input/adaptive_hello.input_trace`, `quadstick_hello.input_trace`).

## Sign-off

- [ ] All rows above verified as of Phase-10 exit
- [ ] `cargo test --workspace` still green (BLD regression-free)
- [ ] `ctest --preset dev-win` and `ctest --preset dev-linux` green
- [ ] ADR-0022 §8 exit-gate checklist complete

Signed — _(founder signature at Phase-10 close)_
