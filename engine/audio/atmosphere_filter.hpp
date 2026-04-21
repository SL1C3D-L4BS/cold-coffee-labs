#pragma once
// engine/audio/atmosphere_filter.hpp — density-driven low-pass filter for
// the SFX bus (19_AUDIO_DESIGN §3.1).
//
// Cutoff model:
//   fc = fc_base * exp(-distance * density * k)
//
// where `density` is the atmospheric density at the listener (0 = vacuum,
// 1 = Earth-STP), `distance` is the emitter-to-listener distance, and `k`
// is a per-medium absorption coefficient.
//
// Biquad: single-pole / zero RBJ low-pass (cookbook). Stable across the
// usable cutoff range at our 48 kHz sample rate.
//
// Not a DSP module per se — this is a small self-contained math helper
// with one runtime-updated state vector.

#include <cmath>
#include <cstdint>
#include <cstddef>

namespace gw::audio {

struct AtmosphereFilterConfig {
    float sample_rate{48'000.0f};
    float base_cutoff_hz{20'000.0f};   // fully open at density=0
    float min_cutoff_hz{250.0f};       // maximum occlusion floor
    float absorption_k{0.005f};        // air absorption coefficient / metre
};

// Fast cutoff-from-density + distance calculator; no DSP state.
[[nodiscard]] inline float atmosphere_cutoff_hz(const AtmosphereFilterConfig& cfg,
                                                float density, float distance_m) noexcept {
    if (density < 0.0f) density = 0.0f;
    if (distance_m < 0.0f) distance_m = 0.0f;
    // When density is zero (vacuum) we keep the filter fully open — vacuum
    // rendering is handled by the mixer state machine (§2.6 of ADR-0018).
    const float exponent = -distance_m * density * cfg.absorption_k;
    float fc = cfg.base_cutoff_hz * std::exp(exponent);
    if (fc < cfg.min_cutoff_hz) fc = cfg.min_cutoff_hz;
    if (fc > cfg.base_cutoff_hz) fc = cfg.base_cutoff_hz;
    return fc;
}

// RBJ biquad low-pass coefficient pack.
struct BiquadLPF {
    // Transfer function: H(z) = (b0 + b1 z^-1 + b2 z^-2) / (1 + a1 z^-1 + a2 z^-2)
    float b0{1.0f};
    float b1{0.0f};
    float b2{0.0f};
    float a1{0.0f};
    float a2{0.0f};

    // Per-channel state (z1, z2). Two-channel plenty for stereo SFX bus.
    float z1_l{0.0f};
    float z2_l{0.0f};
    float z1_r{0.0f};
    float z2_r{0.0f};

    void reset() noexcept { z1_l = z2_l = z1_r = z2_r = 0.0f; }

    // Compute RBJ LPF coefficients. `q` at 0.7071 is a Butterworth response.
    void design(float sample_rate, float cutoff_hz, float q = 0.7071f) noexcept {
        if (sample_rate <= 0.0f || cutoff_hz <= 0.0f || q <= 0.0f) {
            // Degenerate — pass-through.
            b0 = 1.0f; b1 = 0.0f; b2 = 0.0f;
            a1 = 0.0f; a2 = 0.0f;
            return;
        }
        // Guard Nyquist
        const float nyq = 0.5f * sample_rate;
        if (cutoff_hz >= nyq) {
            b0 = 1.0f; b1 = 0.0f; b2 = 0.0f;
            a1 = 0.0f; a2 = 0.0f;
            return;
        }
        const float two_pi = 6.28318530717958647692f;
        const float w0    = two_pi * cutoff_hz / sample_rate;
        const float cos_w = std::cos(w0);
        const float sin_w = std::sin(w0);
        const float alpha = sin_w / (2.0f * q);

        const float a0 = 1.0f + alpha;
        b0 = (1.0f - cos_w) * 0.5f / a0;
        b1 = (1.0f - cos_w) / a0;
        b2 = (1.0f - cos_w) * 0.5f / a0;
        a1 = -2.0f * cos_w / a0;
        a2 = (1.0f - alpha) / a0;
    }

    // Process in-place, interleaved L/R stereo. Transposed Direct Form II.
    void process_stereo(float* samples, std::size_t frame_count) noexcept {
        for (std::size_t i = 0; i < frame_count; ++i) {
            const float l = samples[2 * i + 0];
            const float r = samples[2 * i + 1];

            const float yl = b0 * l + z1_l;
            z1_l = b1 * l - a1 * yl + z2_l;
            z2_l = b2 * l - a2 * yl;

            const float yr = b0 * r + z1_r;
            z1_r = b1 * r - a1 * yr + z2_r;
            z2_r = b2 * r - a2 * yr;

            samples[2 * i + 0] = yl;
            samples[2 * i + 1] = yr;
        }
    }
};

} // namespace gw::audio
