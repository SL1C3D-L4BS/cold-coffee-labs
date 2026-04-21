// engine/audio/spatial.cpp — stub spatial backend and math helpers.
// The Steam Audio production path lives under engine/platform/audio/, gated
// by GW_AUDIO_STEAM; when that gate is off this file provides a weak
// fallback `make_spatial_steam` that returns the stub.
#include "engine/audio/spatial.hpp"

#include <algorithm>
#include <cmath>
#include <memory>

namespace gw::audio {

float compute_attenuation(const AttenuationModel& m, float distance) noexcept {
    // Linear-rolloff model (Unity/FMOD style): 1 below min, 0 above max,
    // 1/(1 + rolloff * (d - min)/(max - min)) in between. Deterministic
    // and adequate for gameplay; Steam Audio can override in production.
    if (distance <= m.min_distance) return 1.0f;
    if (distance >= m.max_distance) return 0.0f;
    const float span = std::max(m.max_distance - m.min_distance, 1e-6f);
    const float t = (distance - m.min_distance) / span;
    const float k = 1.0f + m.rolloff * t;
    if (k <= 0.0f) return 0.0f;
    const float g = 1.0f / k;
    return std::clamp(g * (1.0f - t), 0.0f, 1.0f);
}

float compute_directivity(const DirectivityCone& c, float cos_angle) noexcept {
    // Soft cardioid: 1 inside inner cone, outer_gain outside outer cone,
    // smooth interpolation between. cos_angle is in [-1, 1].
    if (cos_angle >= c.inner_cos) return 1.0f;
    if (cos_angle <= c.outer_cos) return c.outer_gain;
    const float span = std::max(c.inner_cos - c.outer_cos, 1e-6f);
    const float t = (cos_angle - c.outer_cos) / span;
    // smoothstep
    const float s = t * t * (3.0f - 2.0f * t);
    return c.outer_gain + (1.0f - c.outer_gain) * s;
}

float compute_doppler(float relative_velocity_mps, float speed_of_sound_mps) noexcept {
    // Standard doppler for a stationary listener, moving emitter. Positive
    // velocity = approaching. Clamp to [0.5, 2.0] to avoid glitches.
    const float denom = std::max(speed_of_sound_mps - relative_velocity_mps, 1.0f);
    const float ratio = speed_of_sound_mps / denom;
    return std::clamp(ratio, 0.5f, 2.0f);
}

namespace {

// Stub backend: deterministic across platforms. Uses equal-power pan + ITD
// approximated as a fractional-sample delay per channel. Not HRTF.
class StubSpatialBackend final : public ISpatialBackend {
public:
    explicit StubSpatialBackend(uint16_t capacity)
        : capacity_(capacity),
          emitters_(std::make_unique<Entry[]>(capacity)) {
        for (uint16_t i = 0; i < capacity_; ++i) emitters_[i].in_use = false;
    }

    void set_listener(const ListenerState& l) override { listener_ = l; }

    SpatialHandle acquire(const EmitterState& em) override {
        for (uint16_t i = 0; i < capacity_; ++i) {
            if (!emitters_[i].in_use) {
                emitters_[i].in_use = true;
                emitters_[i].em = em;
                emitters_[i].last_gain_l = 0.0f;
                emitters_[i].last_gain_r = 0.0f;
                return SpatialHandle{i};
            }
        }
        return SpatialHandle{};
    }

    void release(SpatialHandle h) override {
        if (h.is_null() || h.index >= capacity_) return;
        emitters_[h.index].in_use = false;
    }

    void update(SpatialHandle h, const EmitterState& em) override {
        if (h.is_null() || h.index >= capacity_) return;
        if (!emitters_[h.index].in_use) return;
        emitters_[h.index].em = em;
    }

    void render(SpatialHandle h, std::span<const float> mono_in,
                std::span<float> stereo_out, std::size_t frame_count) override {
        if (h.is_null() || h.index >= capacity_) return;
        if (!emitters_[h.index].in_use) return;
        if (mono_in.size() < frame_count) return;
        if (stereo_out.size() < frame_count * 2) return;

        const auto& em = emitters_[h.index].em;

        // Delta from listener to emitter (float-precision delta, float64 world).
        const math::Vec3d d64{em.position.x() - listener_.position.x(),
                              em.position.y() - listener_.position.y(),
                              em.position.z() - listener_.position.z()};
        const math::Vec3f d{static_cast<float>(d64.x()),
                            static_cast<float>(d64.y()),
                            static_cast<float>(d64.z())};
        const float dist = d.length();

        // Attenuation × obstruction × (1 - occlusion) × directivity.
        const float a = compute_attenuation(em.attenuation, dist);
        const float occ = 1.0f - em.occlusion;
        const float obs = 1.0f - 0.5f * em.obstruction;

        // Directivity: emitter forward ⋅ (-d_normalised)
        float cos_angle = 1.0f;
        if (dist > 1e-4f) {
            const math::Vec3f to_listener = d * (-1.0f / dist);
            cos_angle = em.forward.dot(to_listener);
        }
        const float dir = compute_directivity(em.directivity, cos_angle);
        const float gain = a * occ * obs * dir * em.spatial_blend
                           + (1.0f - em.spatial_blend);

        // Equal-power pan from the listener's right vector.
        const math::Vec3f right = math::Vec3f{
            listener_.up.y() * listener_.forward.z() - listener_.up.z() * listener_.forward.y(),
            listener_.up.z() * listener_.forward.x() - listener_.up.x() * listener_.forward.z(),
            listener_.up.x() * listener_.forward.y() - listener_.up.y() * listener_.forward.x(),
        };
        float pan = 0.0f;
        if (dist > 1e-4f) {
            pan = std::clamp(d.dot(right) / dist, -1.0f, 1.0f);
        }
        const float theta = (pan + 1.0f) * 0.25f * 3.14159265358979323846f;  // [0, pi/2]
        const float gL = std::cos(theta) * gain;
        const float gR = std::sin(theta) * gain;

        // Smooth gain per-block to avoid zipper noise: lerp from last_gain.
        const float nL = gL;
        const float nR = gR;
        const float dL = (nL - emitters_[h.index].last_gain_l) / static_cast<float>(frame_count);
        const float dR = (nR - emitters_[h.index].last_gain_r) / static_cast<float>(frame_count);
        float cL = emitters_[h.index].last_gain_l;
        float cR = emitters_[h.index].last_gain_r;

        for (std::size_t i = 0; i < frame_count; ++i) {
            const float s = mono_in[i];
            stereo_out[2 * i + 0] += s * cL;
            stereo_out[2 * i + 1] += s * cR;
            cL += dL;
            cR += dR;
        }
        emitters_[h.index].last_gain_l = nL;
        emitters_[h.index].last_gain_r = nR;
    }

    bool is_hrtf() const noexcept override { return false; }

private:
    struct Entry {
        bool         in_use{false};
        EmitterState em{};
        float        last_gain_l{0.0f};
        float        last_gain_r{0.0f};
    };
    uint16_t                capacity_{0};
    std::unique_ptr<Entry[]> emitters_;
    ListenerState           listener_{};
};

} // namespace

std::unique_ptr<ISpatialBackend> make_spatial_stub(uint16_t capacity) {
    return std::make_unique<StubSpatialBackend>(capacity);
}

#if !defined(GW_AUDIO_STEAM)
std::unique_ptr<ISpatialBackend> make_spatial_steam(uint16_t capacity) {
    return make_spatial_stub(capacity);
}
#endif

} // namespace gw::audio
