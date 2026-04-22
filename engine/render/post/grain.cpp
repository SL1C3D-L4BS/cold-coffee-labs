// engine/render/post/grain.cpp — ADR-0079 Wave 17F.

#include "engine/render/post/grain.hpp"

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

} // namespace

float grain_sample(GrainConfig cfg, std::array<float, 2> uv, std::uint64_t frame) noexcept {
    const float sz = std::max(0.5f, cfg.size_px);
    const auto px = static_cast<std::uint32_t>(std::max(0.0f, uv[0]) * 4096.0f / sz);
    const auto py = static_cast<std::uint32_t>(std::max(0.0f, uv[1]) * 4096.0f / sz);
    const auto pz = static_cast<std::uint32_t>((cfg.seed ^ frame) & 0xFFFFFFFFu);
    const auto h  = hash3(px, py, pz);
    return (static_cast<float>(h) / 4294967295.0f) * 2.0f - 1.0f; // [-1, 1]
}

std::array<float, 3>
apply_grain(GrainConfig cfg, std::array<float, 2> uv,
             const std::array<float, 3>& rgb, std::uint64_t frame) noexcept {
    if (cfg.intensity <= 0.0f) return rgb;
    const float g = grain_sample(cfg, uv, frame) * cfg.intensity;
    return {rgb[0] + g, rgb[1] + g, rgb[2] + g};
}

} // namespace gw::render::post
