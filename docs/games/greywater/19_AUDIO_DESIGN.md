# 19 — Greywater Audio Design

**Status:** Canonical (game-side)
**Last revised:** 2026-04-19

---

## 1. Core audio architecture

Greywater audio is a **fully object-based spatial audio engine** built on miniaudio (mixer/device) + Steam Audio (spatial). Every sound in the world is positioned in 3D and subject to:

- Distance attenuation
- Doppler effect
- Atmospheric filtering (density-driven low-pass)
- Occlusion and obstruction by geometry
- Sound propagation paths (ray-based)

Audio is **environment-aware** — the same gunshot sounds different on a thick-atmosphere planet, in a vacuum, or through a wall.

## 2. Spatial audio

### 2.1 3D positioning

```cpp
struct AudioEmitter {
    EntityID          entity;
    Vec3_f64          position;
    Vec3_f64          velocity;
    AudioClipHandle   clip;
    f32               volume;
    f32               pitch;
    f32               max_distance_m;
    AttenuationModel  attenuation;
};

void SpatialAudioSystem::update(Vec3_f64 listener_pos, Vec3_f64 listener_vel) {
    for (auto& e : emitters_) {
        Vec3_f64 rel_pos = e.position - listener_pos;
        f32 dist = f32(length(rel_pos));

        f32 atten = compute_attenuation(dist, e.max_distance_m, e.attenuation);
        Vec3_f64 rel_vel = e.velocity - listener_vel;
        f32 doppler = compute_doppler(rel_pos, rel_vel);

        e.source->set_volume(e.volume * atten);
        e.source->set_pitch(e.pitch * doppler);
        e.source->set_position(Vec3(rel_pos));   // converted to float for the audio API
    }
}
```

### 2.2 Sound propagation (Steam Audio)

Steam Audio performs ray-based acoustic simulation for sound paths between emitter and listener. Paths are subject to:

- **Occlusion** — completely blocked by geometry.
- **Obstruction** — partially blocked.
- **Reflection** — bouncing off walls.
- **Diffraction** — bending around edges (Steam Audio's higher tier; enable where budget permits).

## 3. Atmospheric filtering

### 3.1 Density-driven low-pass

The atmospheric layer the listener is in (see `15_ATMOSPHERE_AND_SPACE.md §3`) drives a low-pass filter on all audio sources beyond a short radius:

```cpp
f32 compute_lowpass_cutoff(f32 dist_m, f32 atmospheric_density) noexcept {
    constexpr f32 base = 20'000.0f;
    f32 attenuation = dist_m * atmospheric_density * k_frequency_attenuation_rate;
    return std::max(500.0f, base * std::exp(-attenuation));
}
```

### 3.2 Layer effects

| Layer         | Filter              | Perceptual effect                             |
| ------------- | ------------------- | --------------------------------------------- |
| Troposphere   | Low-pass (mild)     | Normal sound                                  |
| Stratosphere  | Low-pass (moderate) | Muffled; high frequencies lost                |
| Mesosphere    | Low-pass (severe)   | Deep, rumbling sounds only                    |
| Thermosphere  | Low-pass (extreme)  | Almost silent                                 |
| Exosphere     | No sound            | Vacuum — only direct-contact audio (suit, radio) |

The transition between layers is a smooth interpolation. In vacuum, only first-person radio, suit breathing, and contact-transmitted sound (hull impacts) are audible.

### 3.3 Re-entry audio

During atmospheric entry, plasma, structural stress, and wind all contribute to a layered audio profile:

```cpp
void ReEntryAudioSystem::update(const ReEntryState& s) {
    plasma_source_->set_volume(s.heating / k_max_heating);
    plasma_source_->set_pitch(0.8f + s.heating * 0.4f);

    f32 stress_vol = (s.structural_integrity < 0.3f) ? 1.0f
                   : (s.structural_integrity < 0.6f) ? 0.5f
                   : 0.0f;
    stress_source_->set_volume(stress_vol);

    wind_source_->set_volume(s.atmospheric_density * s.velocity * 0.1f);
}
```

## 4. Occlusion and obstruction

```cpp
f32 compute_occlusion(const SoundPath& path) {
    f32 occ = 1.0f;
    for (auto mat : path.materials_hit) {
        occ *= material_occlusion_factor(mat);
    }
    return occ;
}

// Apply: low-pass cutoff reduced, volume attenuated
```

Material properties are attached to chunk voxels and structural pieces at cook time.

## 5. Audio buses

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

### Dynamic mixing

```cpp
void AudioMixerSystem::update(const GameState& s) {
    // Combat ducking: lower music when SFX intensity rises.
    f32 intensity = compute_combat_intensity(s);
    music_bus_->set_volume(1.0f - intensity * 0.5f);

    // Space silence: master volume reduced in vacuum; radio bus full.
    if (s.player_in_vacuum) {
        master_bus_->set_volume(0.1f);
        radio_bus_->set_volume(1.0f);
    }
}
```

## 6. Implementation stack

| Layer           | Choice                                      |
| --------------- | ------------------------------------------- |
| Device / mixer  | **miniaudio** — MIT, single-header, portable |
| Spatial         | **Steam Audio SDK** — HRTF, ray-based occlusion, reflection baking (free) |
| Codecs          | Opus (voice, streaming), Ogg Vorbis (music), WAV (source SFX) |
| Runtime format  | Custom `.kaudio` cooked format (compressed, seek-optimized) |
| Voice chat      | Opus over GameNetworkingSockets, spatialized via Steam Audio (see `17_MULTIPLAYER.md §5`) |

## 7. Performance (RX 580 & average CPU)

| Metric                           | Target                   |
| -------------------------------- | ------------------------ |
| Total audio CPU usage            | < 3 % of main thread     |
| Simultaneous 3D voices           | 32 (with full spatialization) |
| Simultaneous 2D voices           | 64                       |
| Steam Audio occlusion budget     | < 1 ms per frame         |
| Voice chat round-trip (LAN)      | < 150 ms                 |

---

*Audio is environment. The atmosphere filters the sound. The geometry occludes it. The vacuum silences it. Greywater listens before it speaks.*
