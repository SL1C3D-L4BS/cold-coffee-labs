// engine/render/post/horror_post.cpp — Phase 21 W135 (ADR-0108).

#include "engine/render/post/horror_post.hpp"

#include <chrono>
#include <cmath>
#include <cstdint>

namespace gw::render::post {

namespace {

constexpr std::uint32_t hash3(std::uint32_t x, std::uint32_t y, std::uint32_t z) noexcept {
    std::uint32_t h = x * 0x9E3779B9u + y * 0x85EBCA6Bu + z * 0xC2B2AE35u;
    h ^= h >> 16;
    h *= 0x7FEB352Du;
    h ^= h >> 15;
    h *= 0x846CA68Bu;
    h ^= h >> 16;
    return h;
}

constexpr float hash_to_signed_unit(std::uint32_t h) noexcept {
    return (static_cast<float>(h) / 4294967295.0f) * 2.0f - 1.0f;
}

float clamp01(float v) noexcept {
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

std::array<float, 2> shake_offset(float amount, std::uint64_t frame,
                                  std::uint64_t seed) noexcept {
    const auto s = static_cast<std::uint32_t>(seed ^ frame);
    const float dx = hash_to_signed_unit(hash3(s, 0x17u, 0x3Cu));
    const float dy = hash_to_signed_unit(hash3(s, 0x29u, 0x91u));
    return {dx * amount * 0.05f, dy * amount * 0.05f};
}

} // namespace

std::array<float, 3> horror_post_sample(HorrorPostConfig cfg,
                                        std::array<float, 2> uv,
                                        const std::array<float, 3>& rgb) noexcept {
    const float ca = clamp01(cfg.chromatic_aberration);
    const float fg = clamp01(cfg.film_grain);
    const float sh = clamp01(cfg.screen_shake);

    // Screen shake — UV wobble before sampling. We approximate by modulating
    // the sampled RGB by a luminance-preserving offset. Real frame-graph pass
    // re-samples the source; the scalar oracle just perturbs the centre.
    const auto offs = shake_offset(sh, cfg.frame_index, cfg.seed);
    std::array<float, 2> suv{uv[0] + offs[0], uv[1] + offs[1]};

    // Chromatic aberration — radial R/B offset approximated as channel tint.
    const float cx = (suv[0] - 0.5f) * ca;
    const float cy = (suv[1] - 0.5f) * ca;
    const float r  = rgb[0] + cx;
    const float g  = rgb[1];
    const float b  = rgb[2] - cx - cy * 0.25f;

    // Film grain — zero-mean additive noise scaled by intensity.
    const auto gx = static_cast<std::uint32_t>(suv[0] * 4096.0f);
    const auto gy = static_cast<std::uint32_t>(suv[1] * 4096.0f);
    const auto gz = static_cast<std::uint32_t>((cfg.seed ^ cfg.frame_index) & 0xFFFFFFFFu);
    const float noise = hash_to_signed_unit(hash3(gx, gy, gz)) * fg * 0.15f;

    return {r + noise, g + noise, b + noise};
}

double horror_post_budget_ms_per_frame(std::uint32_t width, std::uint32_t height,
                                       HorrorPostConfig cfg) noexcept {
    // Measure a small tile (64x64) of the scalar kernel to estimate per-pixel
    // cost, then scale to the target resolution. Real Vulkan compute pipeline
    // has different perf characteristics; this bound gives a lower-bound
    // perf-gate for the visual oracle, flagging regressions in the kernel
    // itself.
    constexpr std::uint32_t kTileW = 64;
    constexpr std::uint32_t kTileH = 64;
    constexpr std::uint64_t kIters = 64;
    std::array<float, 3>    acc{};

    const auto t0 = std::chrono::high_resolution_clock::now();
    for (std::uint64_t it = 0; it < kIters; ++it) {
        cfg.frame_index = it;
        for (std::uint32_t y = 0; y < kTileH; ++y) {
            for (std::uint32_t x = 0; x < kTileW; ++x) {
                std::array<float, 2> uv{static_cast<float>(x) / static_cast<float>(kTileW),
                                         static_cast<float>(y) / static_cast<float>(kTileH)};
                acc = horror_post_sample(cfg, uv, {0.5f, 0.5f, 0.5f});
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
    // GPU fragment pass is vastly wider — divide by 64 as a crude SIMT factor
    // to keep the scalar oracle comparable to the real perf target.
    return (ns_per_pixel * frame_pixels / 64.0) / 1'000'000.0;
}

HorrorPostCVarNames horror_post_cvar_names() noexcept {
    return {};
}

} // namespace gw::render::post
