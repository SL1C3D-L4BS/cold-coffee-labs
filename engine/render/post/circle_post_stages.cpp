// engine/render/post/circle_post_stages.cpp — Phase 21 W136 (ADR-0108).

#include "engine/render/post/circle_post_stages.hpp"

#include <algorithm>
#include <cmath>

namespace gw::render::post {

namespace {

float clamp01(float v) noexcept {
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

float rec709_luma(const std::array<float, 3>& rgb) noexcept {
    return 0.2126f * rgb[0] + 0.7152f * rgb[1] + 0.0722f * rgb[2];
}

} // namespace

std::array<float, 3> sin_tendril_apply(SinTendrilConfig cfg, std::array<float, 2> uv,
                                       const std::array<float, 3>& rgb) noexcept {
    const float amt = clamp01(cfg.intensity) * (clamp01(cfg.sin_current / 100.0f));
    if (amt <= 0.0f) {
        return rgb;
    }
    const float dx = uv[0] - 0.5f;
    const float dy = uv[1] - 0.5f;
    const float r2 = dx * dx + dy * dy;
    // Radial darkening rising from centre outward — tendril vignette.
    const float tendril = std::exp(-r2 * 4.0f);       // 1 at centre -> ~0 edges
    const float vig     = (1.0f - tendril) * amt;     // edge weight
    const float lum     = rec709_luma(rgb);

    return {
        rgb[0] * (1.0f - vig) + lum * vig * 0.35f,
        rgb[1] * (1.0f - vig) + lum * vig * 0.20f,
        rgb[2] * (1.0f - vig) + lum * vig * 0.15f,
    };
}

std::array<float, 3> ruin_desat_apply(RuinDesatConfig cfg,
                                      const std::array<float, 3>& rgb) noexcept {
    const float amt = clamp01(cfg.intensity) * clamp01(cfg.ruin_strength);
    if (amt <= 0.0f) {
        return rgb;
    }
    const float lum = rec709_luma(rgb);
    // Desaturate toward luma; bias cool.
    return {
        rgb[0] * (1.0f - amt) + lum * amt * 0.95f,
        rgb[1] * (1.0f - amt) + lum * amt * 1.00f,
        rgb[2] * (1.0f - amt) + lum * amt * 1.08f,
    };
}

std::array<float, 3> rapture_whiteout_apply(RaptureWhiteoutConfig cfg,
                                            const std::array<float, 3>& rgb) noexcept {
    const float amt = clamp01(cfg.intensity);
    const float t   = std::max(0.0f, cfg.t_elapsed_sec);
    if (amt <= 0.0f || t <= 0.0f) {
        return rgb;
    }
    // Non-linear response — whiteout approaches 1 as t grows.
    const float k    = 0.6f;
    const float rise = 1.0f - std::exp(-k * t);
    const float w    = amt * rise;
    return {
        rgb[0] * (1.0f - w) + w,
        rgb[1] * (1.0f - w) + w,
        rgb[2] * (1.0f - w) + w,
    };
}

std::array<float, 3> circle_post_apply(CirclePostCompose cfg, std::array<float, 2> uv,
                                       const std::array<float, 3>& rgb) noexcept {
    auto c = sin_tendril_apply(cfg.sin_tendril, uv, rgb);
    c      = ruin_desat_apply(cfg.ruin_desat, c);
    c      = rapture_whiteout_apply(cfg.rapture_whiteout, c);
    return c;
}

CirclePostCVarNames circle_post_cvar_names() noexcept {
    return {};
}

} // namespace gw::render::post
