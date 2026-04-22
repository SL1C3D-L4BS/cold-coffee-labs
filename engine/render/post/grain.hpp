#pragma once
// engine/render/post/grain.hpp — ADR-0079 Wave 17F.

#include <array>
#include <cstdint>

namespace gw::render::post {

struct GrainConfig {
    float         intensity{0.0f};    // 0 = off, 1 = strong
    float         size_px{1.5f};
    std::uint64_t seed{0};
};

// Deterministic film grain noise for a given UV coordinate + frame.
[[nodiscard]] float grain_sample(GrainConfig cfg,
                                   std::array<float, 2> uv,
                                   std::uint64_t frame_index) noexcept;

[[nodiscard]] std::array<float, 3>
apply_grain(GrainConfig cfg, std::array<float, 2> uv,
             const std::array<float, 3>& rgb, std::uint64_t frame_index) noexcept;

} // namespace gw::render::post
