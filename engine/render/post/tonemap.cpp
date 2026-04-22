// engine/render/post/tonemap.cpp — ADR-0080 Wave 17F.

#include "engine/render/post/tonemap.hpp"

#include <algorithm>
#include <cmath>

namespace gw::render::post {

namespace {
inline float step_positive(float x) noexcept { return x < 0.0f ? 0.0f : x; }
} // namespace

TonemapCurve parse_tonemap_curve(const std::string& s) noexcept {
    if (s == "aces")        return TonemapCurve::ACES;
    if (s == "linear")      return TonemapCurve::Linear;
    return TonemapCurve::PbrNeutral;
}

const char* to_cstring(TonemapCurve c) noexcept {
    switch (c) {
        case TonemapCurve::Linear:     return "linear";
        case TonemapCurve::ACES:       return "aces";
        case TonemapCurve::PbrNeutral:
        default:                        return "pbr_neutral";
    }
}

TaaClipMode parse_clip_mode(const std::string& s) noexcept {
    if (s == "aabb")      return TaaClipMode::Aabb;
    if (s == "variance")  return TaaClipMode::Variance;
    return TaaClipMode::KDop;
}

const char* to_cstring(TaaClipMode m) noexcept {
    switch (m) {
        case TaaClipMode::Aabb:     return "aabb";
        case TaaClipMode::Variance: return "variance";
        case TaaClipMode::KDop:
        default:                     return "kdop";
    }
}

// ---- PBR Neutral (Khronos spec v1.0) ------------------------------------

std::array<float, 3> tonemap_pbr_neutral(const std::array<float, 3>& rgb) noexcept {
    constexpr float start_compression = 0.8f - 0.04f;
    constexpr float desaturation      = 0.15f;

    const float x = std::min({rgb[0], rgb[1], rgb[2]});
    const float offset = x < 0.08f
                          ? x - 6.25f * x * x
                          : 0.04f;
    float r = rgb[0] - offset;
    float g = rgb[1] - offset;
    float b = rgb[2] - offset;

    const float peak = std::max({r, g, b});
    if (peak < start_compression) {
        return {r, g, b};
    }
    const float d = 1.0f - start_compression;
    const float new_peak = 1.0f - d * d / (peak + d - start_compression);
    const float scale = new_peak / peak;
    r *= scale; g *= scale; b *= scale;

    const float g_factor = 1.0f - 1.0f / (desaturation * (peak - new_peak) + 1.0f);
    const auto lerp = [](float a, float b_, float t) noexcept { return a + (b_ - a) * t; };
    return {
        lerp(r, new_peak, g_factor),
        lerp(g, new_peak, g_factor),
        lerp(b, new_peak, g_factor),
    };
}

std::array<float, 3> tonemap_pbr_neutral_inverse(const std::array<float, 3>& rgb) noexcept {
    // Inverse for the low-x regime assuming a uniform (achromatic) input.
    // For x <= 0.08 the forward curve reduces to y = 6.25 * x * x (since the
    // per-channel offset subtracts x - 6.25*x^2); so the inverse is x =
    // sqrt(y / 6.25). Above the start-compression knee we invert the Khronos
    // rational compression y = 1 - d^2 / (x + d - s) by solving for x.
    constexpr float start_compression = 0.76f;
    constexpr float d                  = 0.24f;   // 1 - start_compression
    auto invert_channel = [](float y) noexcept {
        if (y <= 0.04f) {
            const float ratio = y / 6.25f;
            return ratio > 0.0f ? std::sqrt(ratio) : 0.0f;
        }
        if (y <= start_compression) return y + 0.04f;
        const float denom = 1.0f - y;
        if (denom <= 1e-6f) return 10.0f;
        return (d * d) / denom + start_compression - d;
    };
    return {invert_channel(rgb[0]), invert_channel(rgb[1]), invert_channel(rgb[2])};
}

// ---- ACES Narkowicz approximation ---------------------------------------

std::array<float, 3> tonemap_aces(const std::array<float, 3>& rgb) noexcept {
    // Narkowicz 2015 fit: (x*(a*x+b)) / (x*(c*x+d)+e)
    constexpr float a = 2.51f;
    constexpr float b = 0.03f;
    constexpr float c = 2.43f;
    constexpr float d = 0.59f;
    constexpr float e = 0.14f;
    auto f = [&](float x) noexcept {
        const float xp = step_positive(x);
        return std::clamp((xp * (a * xp + b)) / (xp * (c * xp + d) + e), 0.0f, 1.0f);
    };
    return {f(rgb[0]), f(rgb[1]), f(rgb[2])};
}

std::array<float, 3> apply_tonemap(TonemapConfig cfg,
                                     const std::array<float, 3>& rgb) noexcept {
    const std::array<float, 3> scaled{
        rgb[0] * cfg.exposure,
        rgb[1] * cfg.exposure,
        rgb[2] * cfg.exposure,
    };
    switch (cfg.curve) {
        case TonemapCurve::Linear:     return {
            std::clamp(scaled[0], 0.0f, 1.0f),
            std::clamp(scaled[1], 0.0f, 1.0f),
            std::clamp(scaled[2], 0.0f, 1.0f)
        };
        case TonemapCurve::ACES:       return tonemap_aces(scaled);
        case TonemapCurve::PbrNeutral:
        default:                        return tonemap_pbr_neutral(scaled);
    }
}

std::array<float, 3> srgb_encode(const std::array<float, 3>& linear) noexcept {
    auto f = [](float v) noexcept {
        if (v <= 0.0031308f) return 12.92f * v;
        return 1.055f * std::pow(std::max(v, 0.0f), 1.0f / 2.4f) - 0.055f;
    };
    return {f(linear[0]), f(linear[1]), f(linear[2])};
}

} // namespace gw::render::post
