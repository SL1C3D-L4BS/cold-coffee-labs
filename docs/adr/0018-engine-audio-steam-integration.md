# ADR 0018 — Steam Audio spatial integration (Wave 10B)

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 10 Wave 10B opening)
- **Deciders:** Cold Coffee Labs engine group
- **Related:** ADR-0017 (audio service), `docs/games/greywater/19_AUDIO_DESIGN.md` §§ 2, 3, 6 (object-based spatial, atmosphere filter, propagation)

## 1. Context

Greywater's audio identity is built on **object-based spatial audio** with HRTF binaural rendering and geometry-aware propagation. Wave 10B adds the Steam Audio layer on top of the Wave-10A mixer foundation.

Steam Audio (v4.8.1, 2026-02-11) is **fully open-source**, C-ABI (`phonon.h`), supports **runtime mesh / material changes** since 4.8.0, and integrates via a custom audio node. Licence permits linking against Greywater's static build; the SOFA HRTF dataset carries its own licence which we vendor separately under `assets/audio/hrtf/default/`.

## 2. Decision

### 2.1 Abstraction

`engine/audio/spatial.hpp` declares `ISpatialBackend`:

```cpp
class ISpatialBackend {
public:
    virtual ~ISpatialBackend() = default;
    virtual void set_listener(const ListenerState&) = 0;
    virtual SpatialHandle acquire(const EmitterState&) = 0;
    virtual void release(SpatialHandle) = 0;
    virtual void update(SpatialHandle, const EmitterState&) = 0;
    // Audio-thread call: in [mono] → out [stereo], deterministic per handle.
    virtual void render(SpatialHandle, std::span<const float> mono_in,
                        std::span<float> stereo_out) = 0;
};
```

Two implementations land in Wave 10B:

1. **`SpatialStubBackend`** — deterministic pan + ITD + distance attenuation, no HRTF. Used in CI, the `NullAudioBackend`, and on systems where Steam Audio cannot initialise. Golden PCM for tests is generated from this backend.
2. **`SpatialSteamBackend`** — production. Owns `IPLContext`, one shared `IPLHRTF` (default SOFA), a pool of 32 `IPLBinauralEffect` + `IPLDirectEffect` + `IPLReverbEffect`, all pre-allocated.

Selection via `AudioConfig::enable_hrtf` + runtime capability probe (Steam Audio DLL presence).

### 2.2 Listener / emitter contract

- `ListenerState` carries position (`Vec3_f64`), forward, up, velocity — fed each frame by `AudioService::update`.
- World-space positions are `float64`; Steam Audio consumes `float`. Conversion happens **once** per voice per frame inside `SpatialSteamBackend::update` using the listener position as the origin (floating-origin-safe).
- `EmitterState` carries the same plus directivity (cardioid cone params), spatialBlend (0 = 2D, 1 = full 3D), distance-attenuation curve, occlusion/obstruction coefficients.

### 2.3 Pipeline per voice (3D path)

```
voice mono PCM → Steam Audio DirectEffect (distance + occlusion + directivity)
              → Steam Audio BinauralEffect (HRTF, ITD, ILD)
              → 2-ch output into Master mixer node
```

Air absorption is **disabled inside Steam Audio** — we author atmospheric filtering ourselves (atmosphere LPF, §3 of the bible) because game design needs authoritative control (vacuum, CO₂ atmosphere, etc.). Doppler is computed engine-side from float64 velocity and applied as a pitch trim before the direct effect (Steam Audio's Doppler uses float positions).

### 2.4 Occlusion

Physics is offline until Phase 12. Wave 10B ships `IOcclusionProvider`:

- `NullOcclusion` — always returns 1.0 (fully audible).
- `GridVoxelOcclusion` — stub backed by the Phase-5 chunk/material grid; a straight ray segment from listener to emitter samples the grid and tallies `transmission * material_coefficient`.

Phase 12 replaces `GridVoxelOcclusion` with a Jolt raycast-based provider without touching `engine/audio/`.

### 2.5 Reflections / reverb

Full probe-baked reflections are **deferred to Phase 12** (when Jolt mesh data is available). In Phase 10 we ship **preset reverb** on each bus via `IPLReverbEffect` parameters: `cave`, `hall`, `outdoor`, `vacuum`. `MixerGraph::set_reverb_preset(BusId, ReverbPreset)` is the authoring surface.

### 2.6 Vacuum path

When `atmospheric_density < 0.001`:

- SFX bus muted (not paused — the state is recoverable on re-entry to an atmosphere)
- Dialogue/Radio buses held at 1.0
- Master gets a contact-audio LPF (2 kHz cutoff, 12 dB/oct) representing solid-body transfer

Implemented as a `VacuumState` field on the mixer plus declarative rules in `audio_mixing.toml`. Not per-voice.

### 2.7 Mono/stereo toggle

Game-Accessibility-Guidelines "Hearing / Intermediate" line item. Implemented as a `MixerGraph::DownmixMode { Stereo, Mono, Bypass }` on the Master path. Default Stereo; user sets Mono in the accessibility panel (Wave 10E).

## 3. Non-goals

- No reflections baking (Phase 12).
- No pathing / occlusion-by-sound-propagation (Phase 12).
- No ambisonics (deferred; HRTF handles the 3D case for our voice budget).

## 4. Verification

- Golden-PCM test: 2-second tone at (+1, 0, 0) relative to listener, compare FFT against checked-in reference `tests/fixtures/audio/golden_tone_right.fft.bin`.
- 32-voice soak: CPU < 3 %, no heap allocs.
- Determinism: same scene seed + same listener trajectory produces identical PCM (bit-identical on the stub backend; ±0.5 dB on Steam Audio where HRTF interpolation is involved).
- Vacuum transition test: SFX voice pre-transition audible; post-transition muted; dialogue voice preserved.

## 5. Consequences

Positive:
- Accessibility (mono toggle), vacuum design, and future reflections all have a single seam.
- CI runs without Steam Audio installed; determinism is stub-backend.

Negative:
- Two codepaths (stub + Steam Audio) must be kept semantically equivalent for the **direct** component. We accept this; the stub is canonical for tests and the Steam Audio path documents its expected PCM envelope per-frame.

## 6. Rollback

Setting `AudioConfig::enable_hrtf = false` forces the stub backend. No schema / file changes involved.
