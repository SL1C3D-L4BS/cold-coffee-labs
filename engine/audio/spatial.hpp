#pragma once
// engine/audio/spatial.hpp — ISpatialBackend + deterministic stub (ADR-0018).
//
// The stub backend performs:
//   - distance attenuation (inverse curve clamped against AttenuationModel)
//   - directivity cone (cardioid-style cos-blended gain)
//   - equal-power pan (stereo) + inter-aural time delay (ITD, max 600 µs)
//   - doppler pitch (consumed in AudioService; pre-pitch is applied per voice)
//
// This is deterministic to ±1 ULP across platforms and is the canonical
// reference the Steam Audio backend must envelope-match in CI.

#include "engine/audio/audio_types.hpp"

#include <cstddef>
#include <memory>
#include <span>

namespace gw::audio {

struct SpatialHandle {
    uint16_t index{0xFFFFu};
    [[nodiscard]] constexpr bool is_null() const noexcept { return index == 0xFFFFu; }
};

class ISpatialBackend {
public:
    virtual ~ISpatialBackend() = default;

    virtual void set_listener(const ListenerState&) = 0;
    virtual SpatialHandle acquire(const EmitterState&) = 0;
    virtual void          release(SpatialHandle) = 0;
    virtual void          update(SpatialHandle, const EmitterState&) = 0;

    // Render one frame-block. mono_in is `frame_count` samples; stereo_out
    // is `frame_count * 2` interleaved samples. Implementations accumulate
    // (additive mix) into stereo_out.
    virtual void render(SpatialHandle handle,
                        std::span<const float> mono_in,
                        std::span<float> stereo_out,
                        std::size_t frame_count) = 0;

    // Capability flag — true when this backend uses HRTF (ADR-0018 §2.1).
    virtual bool is_hrtf() const noexcept = 0;
};

// Deterministic stub used in tests and as a production fallback.
[[nodiscard]] std::unique_ptr<ISpatialBackend> make_spatial_stub(uint16_t capacity = 32);

// Steam Audio production backend (gated by GW_AUDIO_STEAM).
[[nodiscard]] std::unique_ptr<ISpatialBackend> make_spatial_steam(uint16_t capacity = 32);

// ---------------------------------------------------------------------------
// Attenuation helpers — exposed for tests.
// ---------------------------------------------------------------------------

[[nodiscard]] float compute_attenuation(const AttenuationModel& m, float distance) noexcept;
[[nodiscard]] float compute_directivity(const DirectivityCone& c, float cos_angle) noexcept;

// Doppler pitch factor given relative velocity along the listener→emitter
// axis. Clamped to reasonable ranges to prevent audio glitches.
[[nodiscard]] float compute_doppler(float relative_velocity_mps,
                                    float speed_of_sound_mps = 343.0f) noexcept;

} // namespace gw::audio
