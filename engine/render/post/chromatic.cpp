// engine/render/post/chromatic.cpp — ADR-0079 Wave 17F.

#include "engine/render/post/chromatic.hpp"

namespace gw::render::post {

std::array<float, 2>
chromatic_uv_offset(ChromaticConfig cfg, std::array<float, 2> uv, int channel) noexcept {
    const float sign = channel == 0 ? -1.0f : channel == 2 ? 1.0f : 0.0f;
    const float dx = (uv[0] - 0.5f) * cfg.strength * sign;
    const float dy = (uv[1] - 0.5f) * cfg.strength * sign;
    return {uv[0] + dx, uv[1] + dy};
}

std::array<float, 3>
chromatic_aberration(ChromaticConfig,
                      std::array<float, 2>,
                      const std::array<float, 3>& rgb_center,
                      const std::array<float, 3>& rgb_red,
                      const std::array<float, 3>& rgb_blue) noexcept {
    return {rgb_red[0], rgb_center[1], rgb_blue[2]};
}

} // namespace gw::render::post
