# ADR 0017 — Engine Audio Service & Mixer Graph (Wave 10A)

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 10 opening)
- **Deciders:** Cold Coffee Labs engine group (Claude Opus 4.7 drafting; founder final)
- **Supersedes:** —
- **Superseded by:** —
- **Related:**
  - `CLAUDE.md` non-negotiables #5 (RAII, no raw new), #9 (no STL in hot paths), #10 (no std::thread), #11 (OS headers in platform only)
  - `docs/games/greywater/19_AUDIO_DESIGN.md` — voice budget (32 3D + 64 2D), CPU budget (≤ 3 %), bus graph, object-based design
  - ADR-0018 (Steam Audio), ADR-0019 (`.kaudio` streaming), ADR-0022 (Phase 10 cross-cutting)

## 1. Context

Phase 10 opens the Runtime I track with audio + input. Wave 10A (week 060) establishes the mixer foundation. The audio runtime must:

1. Be a **single RAII owner** — `AudioService` is constructed by the engine; its destructor leaves miniaudio cleanly shut down.
2. Produce **zero allocations on the audio callback thread** — all voices pre-allocated at service init.
3. Be **deterministic** — same input clips + same listener trajectory produce identical PCM frames.
4. Be **deferred-backend** — the code path that talks to miniaudio lives behind an `IAudioBackend` interface so CI can run a null backend (no device), and a future backend swap (e.g. WASAPI direct) does not ripple.
5. Keep **OS headers quarantined**: neither `engine/audio/` nor any downstream consumer includes `<windows.h>`, `CoreAudio` headers, or any backend SDK directly. miniaudio's `"miniaudio.c"` TU is a single source file built under `engine/platform/audio/`.
6. Enforce the **bus graph from §5 of the audio bible** as the authoritative topology.

## 2. Decision

### 2.1 Layering

```
engine/audio/
  audio_types.hpp         — enums, handles, config, listener state, stats
  audio_backend.hpp       — IAudioBackend interface
  audio_backend_null.cpp  — deterministic offline backend (CI + tests)
  voice_pool.hpp/.cpp     — lock-free fixed-capacity free-list
  mixer_graph.hpp/.cpp    — Master → {SFX, Music, Dialogue, Ambience} → leaf buses
  atmosphere_filter.hpp/.cpp — biquad LPF cutoff vs density
  emitter.hpp             — AudioEmitter + ListenerState component types
  audio_service.hpp/.cpp  — RAII façade (this ADR's central object)
engine/platform/audio/
  miniaudio_unity.c       — the sole TU that pulls miniaudio amalgamation
  miniaudio_backend.cpp   — IAudioBackend impl calling into miniaudio
```

Downstream code (ECS systems, editor, game) only ever sees `engine/audio/` headers.

### 2.2 Bus graph (mirrors §5 of audio bible, frozen)

```
Master
├── SFX
│   ├── Weapons
│   ├── Environment
│   └── UI
├── Music
│   ├── Combat
│   ├── Exploration
│   └── Space
├── Dialogue
│   ├── NPC
│   └── Radio
└── Ambience
    ├── Wind
    ├── Water
    └── Space
```

`BusId` is a 16-bit enum class; the mixer's 15 leaf + group nodes are stored in a fixed `std::array<BusNode, kBusCount>` inside `MixerGraph`. Bus volumes are `float`; bus mutes are `bool`. The atmosphere LPF node sits between **SFX** and **Master** (SFX only — Dialogue/Music/UI bypass atmospheric filtering per §3 of the bible).

### 2.3 Voice pool

- **32 3D voices** + **64 2D voices**, pre-allocated in `VoicePool` at service construction.
- Acquire/release via a fixed-capacity SPSC free-list (`VoicePool::Slot*` head pointer, `std::atomic` cmpxchg).
- **Steal-oldest** policy on saturation: when all of a class's voices are live, the oldest-started voice is stopped and its slot reissued. Callers observe this via `VoiceHandle::generation` — a stale handle fails `VoicePool::is_live()`.
- `VoiceHandle` is { `u16 index`, `u16 generation` }; 4 bytes, trivially copyable.

### 2.4 AudioConfig

```cpp
struct AudioConfig {
    std::uint32_t sample_rate   = 48'000;
    std::uint32_t frames_period = 512;
    std::uint8_t  channels      = 2;
    std::uint16_t voices_3d     = 32;
    std::uint16_t voices_2d     = 64;
    bool          enable_hrtf   = true;   // Wave 10B
    const char*   device_id     = nullptr; // null = system default
};
```

Determinism gate: when `GW_AUDIO_DETERMINISTIC` is defined (tests, replay), the service pins the null backend and uses a fixed 48 kHz / 2-channel / 512-frame period.

### 2.5 Thread model

- `AudioService::update(dt, listener)` — main thread; submits event commands (play/stop/volume) into a SPSC queue read by the mixer.
- `IAudioBackend::render(frames)` — audio thread; owned by the backend. For miniaudio this is the `ma_device` callback; for `NullAudioBackend` it is pumped manually by tests.
- No `std::thread` / `std::async` anywhere in `engine/audio/`. The audio thread is owned by the platform backend.

### 2.6 Hotplug

`IAudioBackend` reports rerouting via an in-process `Events<DeviceReroutedEvent>` queue. `AudioService::update` drains this queue onto the main-thread event bus; the mixer graph is preserved across reroutes (no state loss).

### 2.7 Decoders

miniaudio ships with WAV/FLAC/MP3 built in. We extend via `engine/audio/decoder_registry.hpp`:

- `WavRawDecoder` — baked f32 (our primary SFX path; part of `.kaudio`)
- `VorbisDecoder` (Wave 10C) — via miniaudio's `miniaudio_libvorbis.c` extra
- `OpusDecoder` (Wave 10C) — via `libopusfile`

## 3. Non-goals

- No reflections/reverb baking — deferred to Wave 10B's Phase-12 continuation (ADR-0018 §4).
- No per-instance DSP graphs — authored effects (reverb preset, filter) live on buses, not voices.
- No Web Audio compatibility.
- No MIDI.

## 4. Consequences

Positive:
- Determinism-first surface: the `NullAudioBackend` can render an N-frame golden buffer in CI without a device.
- Bus graph cannot drift — it is a frozen topology in code with tests that pin it.
- Swap-in seam for Phase 12 (physics-driven reflections) is `MixerGraph::set_reverb_preset(BusId, ReverbPreset)`, not a new API.

Negative:
- Every new bus is a code change (not data-driven). We accept this: the bus graph is a product design decision, not tuning data.
- Voice-pool saturation forces steal-oldest. We document this in the developer-facing audio guide.

## 5. Verification

- `voice_pool_test.cpp` — fill to capacity, observe graceful steal, handle generation check.
- `mixer_graph_test.cpp` — bus topology equality against a golden vector.
- `atmosphere_filter_test.cpp` — LPF cutoff monotonic in distance × density; biquad coefficients stable.
- `audio_service_null_backend_test.cpp` — 10 000 play/stop cycles, zero heap churn after warmup (tracked via an allocator hook).
- Build smoke: `greywater_core` links; `gw_tests` runs all audio cases green.

## 6. Rollback

The audio module is ring-fenced — deleting `engine/audio/` and `engine/platform/audio/` leaves the rest of the engine compiling (only the sandbox demo loses audio).
