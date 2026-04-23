// engine/render/post/reality_warp.cpp — Phase 22 W147 (ADR-0108).

#include "engine/render/post/reality_warp.hpp"

#include <chrono>
#include <cmath>

namespace gw::render::post {

namespace {

float clamp01(float v) noexcept {
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

} // namespace

std::array<float, 2> reality_warp_uv(RealityWarpConfig cfg, std::array<float, 2> uv) noexcept {
    const float mw = clamp01(cfg.mirror_weight);
    const float iw = clamp01(cfg.invert_weight);
    const float dp = clamp01(cfg.distortion_pulse);

    float u = uv[0];
    float v = uv[1];

    // Mirror step — blend u → 1-u.
    u = u * (1.0f - mw) + (1.0f - u) * mw;

    // Gravity invert — blend v → 1-v, plus a mild squash.
    v = v * (1.0f - iw) + (1.0f - v) * iw;
    if (iw > 0.0f) {
        // Squash about centre by (1 + 0.05 * iw).
        v = (v - 0.5f) * (1.0f + 0.05f * iw) + 0.5f;
    }

    // Distortion pulse — sinusoidal ripple tied to frame index.
    if (dp > 0.0f) {
        const float phase = static_cast<float>(cfg.frame_index) * 0.05f;
        const float rip   = std::sin(phase + v * 6.2831853f) * 0.005f * dp;
        u += rip;
    }

    return {u, v};
}

double reality_warp_budget_ms(std::uint32_t width, std::uint32_t height,
                              RealityWarpConfig cfg) noexcept {
    constexpr std::uint32_t kTileW = 64;
    constexpr std::uint32_t kTileH = 64;
    constexpr std::uint64_t kIters = 64;
    float acc = 0.f;
    const auto t0 = std::chrono::high_resolution_clock::now();
    for (std::uint64_t it = 0; it < kIters; ++it) {
        cfg.frame_index = it;
        for (std::uint32_t y = 0; y < kTileH; ++y) {
            for (std::uint32_t x = 0; x < kTileW; ++x) {
                std::array<float, 2> uv{static_cast<float>(x) / static_cast<float>(kTileW),
                                         static_cast<float>(y) / static_cast<float>(kTileH)};
                auto w = reality_warp_uv(cfg, uv);
                acc += w[0] + w[1];
            }
        }
    }
    const auto t1 = std::chrono::high_resolution_clock::now();
    (void)acc;
    const double total_ns = static_cast<double>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
    const double tile_pixels = static_cast<double>(kTileW) * static_cast<double>(kTileH)
                             * static_cast<double>(kIters);
    const double ns_per_pixel = total_ns / tile_pixels;
    const double frame_pixels = static_cast<double>(width) * static_cast<double>(height);
    return (ns_per_pixel * frame_pixels / 64.0) / 1'000'000.0;
}

RealityWarpCVarNames reality_warp_cvar_names() noexcept {
    return {};
}

} // namespace gw::render::post
