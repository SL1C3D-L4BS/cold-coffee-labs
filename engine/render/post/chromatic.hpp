#pragma once
// engine/render/post/chromatic.hpp — ADR-0079 Wave 17F.

#include <array>

namespace gw::render::post {

struct ChromaticConfig {
    float strength{0.0f};   // fraction of half-image offset
};

// Applies a radial red/blue channel offset to an (R,G,B) triple at UV (0..1).
[[nodiscard]] std::array<float, 3>
chromatic_aberration(ChromaticConfig cfg,
                      std::array<float, 2> uv,
                      const std::array<float, 3>& rgb_center,
                      const std::array<float, 3>& rgb_red,   // sampled at uv_red
                      const std::array<float, 3>& rgb_blue)  // sampled at uv_blue
    noexcept;

[[nodiscard]] std::array<float, 2>
chromatic_uv_offset(ChromaticConfig cfg, std::array<float, 2> uv, int channel) noexcept;

} // namespace gw::render::post
