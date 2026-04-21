#pragma once
// engine/audio/audio_types.hpp — shared primitives for the Phase-10 audio runtime.
//
// This header is the *only* leak between `engine/audio/` public surface and
// downstream code (ECS systems, editor, game). It carries:
//   - integer handles (VoiceHandle, BusId, AudioClipId)
//   - config / stats PODs
//   - listener + emitter state
//   - enums for bus routing, downmix mode, reverb preset, voice class.
//
// It intentionally includes *no* engine math types other than Vec3f/Vec3d to
// keep compile time tight for the thousands of translation units that touch
// audio. World positions are `float64` per CLAUDE.md #17.

#include "engine/math/vec.hpp"

#include <cstdint>

namespace gw::audio {

using std::int16_t;
using std::int32_t;
using std::int64_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;
using std::uint8_t;

// ---------------------------------------------------------------------------
// Handles
// ---------------------------------------------------------------------------

// VoiceHandle: 4-byte trivially copyable identity. The generation counter
// invalidates handles when the same slot is recycled (voice steal or stop).
struct VoiceHandle {
    uint16_t index{0};
    uint16_t generation{0};

    [[nodiscard]] constexpr bool operator==(const VoiceHandle&) const noexcept = default;
    [[nodiscard]] constexpr bool is_null() const noexcept {
        return index == 0 && generation == 0;
    }

    static constexpr VoiceHandle null() noexcept { return {0, 0}; }
};

// AudioClipId: opaque id owned by the asset DB (Phase 6). Zero = none.
struct AudioClipId {
    uint64_t value{0};
    [[nodiscard]] constexpr bool operator==(const AudioClipId&) const noexcept = default;
    [[nodiscard]] constexpr bool is_null() const noexcept { return value == 0; }
};

// ---------------------------------------------------------------------------
// Bus graph — frozen topology from 19_AUDIO_DESIGN §5.
// ---------------------------------------------------------------------------

enum class BusId : uint16_t {
    // Root
    Master = 0,
    // SFX group + leaves
    SFX,
    SFX_Weapons,
    SFX_Environment,
    SFX_UI,
    // Music group + leaves
    Music,
    Music_Combat,
    Music_Exploration,
    Music_Space,
    // Dialogue group + leaves
    Dialogue,
    Dialogue_NPC,
    Dialogue_Radio,
    // Ambience group + leaves
    Ambience,
    Ambience_Wind,
    Ambience_Water,
    Ambience_Space,

    Count
};

static constexpr uint16_t kBusCount = static_cast<uint16_t>(BusId::Count);

// ---------------------------------------------------------------------------
// Voice classes + routing
// ---------------------------------------------------------------------------

enum class VoiceClass : uint8_t {
    NonSpatial2D = 0,  // UI, music, radio, etc. — no HRTF
    Spatial3D    = 1,  // HRTF + attenuation + Doppler
};

// ---------------------------------------------------------------------------
// Downmix / listening mode (GAG Hearing/Intermediate accessibility)
// ---------------------------------------------------------------------------

enum class DownmixMode : uint8_t {
    Stereo = 0,
    Mono   = 1,
    Bypass = 2,
};

// ---------------------------------------------------------------------------
// Reverb presets — preset-based only in Phase 10; probe-baked in Phase 12.
// ---------------------------------------------------------------------------

enum class ReverbPreset : uint8_t {
    None    = 0,
    Cave    = 1,
    Hall    = 2,
    Outdoor = 3,
    Vacuum  = 4,
};

// ---------------------------------------------------------------------------
// Config / stats
// ---------------------------------------------------------------------------

struct AudioConfig {
    uint32_t sample_rate{48'000};
    uint32_t frames_period{512};
    uint8_t  channels{2};
    uint16_t voices_3d{32};
    uint16_t voices_2d{64};
    bool     enable_hrtf{true};
    bool     deterministic{false};   // force null backend + fixed seeds
    const char* device_id{nullptr};  // null = system default
};

struct AudioStats {
    uint32_t active_voices_2d{0};
    uint32_t active_voices_3d{0};
    uint32_t voices_stolen{0};
    uint32_t reroute_count{0};
    uint32_t stream_underruns{0};
    uint64_t frames_rendered{0};
    float    cpu_load_pct{0.0f};
};

// ---------------------------------------------------------------------------
// Listener & emitter state (object-based audio — 19_AUDIO_DESIGN §2)
// ---------------------------------------------------------------------------

// World-space positions are float64 (CLAUDE.md #17).
// The audio thread converts to float relative to the listener each frame.

struct ListenerState {
    math::Vec3d position{0.0, 0.0, 0.0};
    math::Vec3f forward{0.0f, 0.0f, -1.0f};
    math::Vec3f up{0.0f, 1.0f, 0.0f};
    math::Vec3f velocity{0.0f, 0.0f, 0.0f};
    float       atmospheric_density{1.0f};  // 0 = vacuum, 1 = Earth-at-STP
};

struct AttenuationModel {
    float min_distance{1.0f};
    float max_distance{100.0f};
    float rolloff{1.0f};
};

struct DirectivityCone {
    float inner_cos{1.0f};   // cos of inner angle — fully on-axis
    float outer_cos{-1.0f};  // cos of outer angle — fully off-axis
    float outer_gain{1.0f};  // gain off-axis
};

struct EmitterState {
    math::Vec3d      position{0.0, 0.0, 0.0};
    math::Vec3f      forward{0.0f, 0.0f, -1.0f};
    math::Vec3f      velocity{0.0f, 0.0f, 0.0f};
    AttenuationModel attenuation{};
    DirectivityCone  directivity{};
    float            spatial_blend{1.0f};      // 0 = 2D, 1 = fully 3D
    float            occlusion{0.0f};          // 0 = clear, 1 = solid wall
    float            obstruction{0.0f};        // partial block
    BusId            bus{BusId::SFX_Environment};
};

// ---------------------------------------------------------------------------
// Play-args
// ---------------------------------------------------------------------------

struct Play2D {
    BusId bus{BusId::SFX_UI};
    float volume{1.0f};
    float pitch{1.0f};
    bool  looping{false};
};

struct Play3D {
    EmitterState emitter{};
    float        volume{1.0f};
    float        pitch{1.0f};
    bool         looping{false};
};

// ---------------------------------------------------------------------------
// Device reroute events (marshalled onto main-thread event bus)
// ---------------------------------------------------------------------------

struct DeviceReroutedEvent {
    uint32_t new_sample_rate{0};
    uint8_t  new_channels{0};
};

} // namespace gw::audio
