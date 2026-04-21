# ADR 0022 — Phase 10 cross-cutting decisions (summary)

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 10 Wave 10E close / exit gate)
- **Deciders:** Cold Coffee Labs engine group
- **Summarises:** ADRs 0017–0021

This ADR is the **cross-cutting summary** for Phase 10. It records decisions that span more than one wave or that fell out during implementation.

## 1. Dependency pins

| Library | Pin | Notes |
|---|---|---|
| miniaudio | 0.11.25 (2026-03-04) | Vendored via a single-TU wrapper `miniaudio_unity.c` that includes `miniaudio.c` instead of using the `MINIAUDIO_IMPLEMENTATION` macro. This is forward-compatible with the 0.12 split that separates `.c` from `.h`. Optional CPM package (gated by `GW_AUDIO_MINIAUDIO`). |
| Steam Audio SDK | 4.8.1 (2026-02-11) | Binary DLL/SO from GitHub Releases — open-source since 4.5.2. Gated by `GW_AUDIO_STEAM`. |
| SDL3 | 3.4.0 (2026-01-01) | FetchContent from `libsdl-org/SDL@release-3.4.0`. Gated by `GW_INPUT_SDL3`. |
| libopus | 1.6 | Submodule `xiph/opus`. Gated by `GW_AUDIO_OPUS`. |
| libopusfile | 0.12 | Depends on libopus + libogg 1.3.5. Gated by `GW_AUDIO_OPUS`. |

All four `GW_*` gates default **OFF** in Phase 10's initial landing. The null/trace backends are the default production path; gate-on builds are the integration path that Phase 11 consumes for the Playable-Runtime demo.

**Why gate off by default in Phase 10?** The phase's Definition-of-Done asks for *architecture*, *determinism*, and *accessibility* — all of which are testable without the binary dependencies. Gate-on lights up the full production path and does so behind flags that flip in Phase 11 without code changes. This keeps CI quick, avoids vendoring a 12 MB Steam Audio binary across a million git clones, and makes every decision made in ADRs 0017–0021 survive on its own merits.

## 2. Determinism contract

- The null audio backend pumps N frames from a known seed and produces bit-identical PCM across platforms.
- The trace-replay input backend emits identical `RawEvent` streams across platforms.
- Golden-PCM fixtures under `tests/fixtures/audio/` are the canonical reference.
- Steam Audio and miniaudio, when gated on, are *allowed* to differ within ±0.5 dB; tests that gate on SDK paths relax bit-identity to envelope equality.

## 3. Hot-path budgets (CI-enforced via `gw_perf_gate`)

| Gate | Target | Hardware basis |
|---|---|---|
| Audio CPU (32 voices + 10 s occlusion barrage) | ≤ 3 % | i5-8400 equivalent |
| Steam Audio per-frame | ≤ 1.0 ms | at 60 FPS |
| Audio callback alloc/free | 0 / 0 | miniaudio custom alloc hook |
| Music streamer underruns / 10 min | 0 | 48 kHz stereo |
| Input poll | ≤ 0.1 ms / frame | 4 gamepads |
| Rebind round-trip | ≤ 2 ms | TOML save + reload |

## 4. Thread-ownership summary

| Thread | Owns | Calls |
|---|---|---|
| Main | `AudioService`, `InputService`, bus volumes, listener update | backend `poll`, `update(dt)`, rebind, profile swap |
| Audio | `IAudioBackend::render` callback | Voice pool pull, mixer graph process, atmosphere LPF, spatial render |
| I/O (jobs pool) | Music decode page pulls | `IStreamDecoder::decode_page`, ring buffer push |

No `std::thread` / `std::async` introduced. `engine/jobs/` is the only parallelism mechanism.

## 5. What Phase 11 inherits (handoff)

1. `AudioService` + `InputService` already owned by `Engine::Impl`.
2. Device hotplug events flowing on the main event bus.
3. `audio_mixing.toml` + `input.toml` files; Phase 11's CVar system takes ownership of the *same* files.
4. ImGui rebind panel (editor-only); Phase 11 re-implements in RmlUi.
5. Voice-chat scaffolding: `engine/audio/voice_capture.hpp`, `voice_playback.hpp` interfaces (no networking — Phase 14 wires GNS to these).
6. Accessibility cross-walk self-check (`docs/a11y_phase10_selfcheck.md`) — Phase 11 adds the RmlUi-facing items.

## 6. Cross-platform notes

- **Windows**: WASAPI backend via miniaudio; SDL3's Raw Input path; DualSense trigger rumble through SDL's DualSense driver.
- **Linux**: PulseAudio / PipeWire via miniaudio (PipeWire detected at runtime by miniaudio); SDL3's evdev + HIDAPI path. Real-time priority on the audio thread guarded behind `MA_NO_PTHREAD_REALTIME_PRIORITY_FALLBACK` so containerised CI runners don't crash.
- **Steam Deck**: treated as Linux with `SDL_HINT_JOYSTICK_HIDAPI_STEAMDECK=1` hint; trigger rumble works out of the box via SDL3.

## 7. What did *not* land in Phase 10

- Reflections / reverb probe baking — Phase 12 (needs Jolt meshes).
- Full DSP authoring graph per-voice — deferred indefinitely (out of scope for the identity we're building).
- Voice chat transport — Phase 14 (GNS).
- On-screen virtual keyboard — Phase 16 a11y.

## 8. Exit-gate checklist

- [x] Week rows 060–065 all merged behind the `phase-10` tag
- [x] ADRs 0017–0021 land as Accepted
- [x] ADR 0022 land as the summary (this doc)
- [x] `gw_tests` green with ≥ 40 new audio + input cases
- [x] `ctest --preset dev-win` and `dev-linux` green
- [x] Performance gates §3 green
- [x] `docs/a11y_phase10_selfcheck.md` signed off
- [x] `docs/05` marks Phase 10 *shipped* with today's date
- [x] Git tag `v0.10.0-runtime-1` pushed
