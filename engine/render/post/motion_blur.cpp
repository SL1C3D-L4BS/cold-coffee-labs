// engine/render/post/motion_blur.cpp — ADR-0079 Wave 17F.

#include "engine/render/post/motion_blur.hpp"

#include <algorithm>
#include <cmath>

namespace gw::render::post {

void motion_blur_tilemax(const std::vector<float>& velocities,
                           std::uint32_t width,
                           std::uint32_t height,
                           std::uint32_t tile_size,
                           std::vector<float>& tile_max) {
    if (tile_size == 0) tile_size = 16;
    const std::uint32_t tw = (width  + tile_size - 1) / tile_size;
    const std::uint32_t th = (height + tile_size - 1) / tile_size;
    tile_max.assign(static_cast<std::size_t>(tw) * th * 2u, 0.0f);

    for (std::uint32_t ty = 0; ty < th; ++ty) {
        for (std::uint32_t tx = 0; tx < tw; ++tx) {
            float best_len_sq = 0.0f;
            float best_vx = 0.0f, best_vy = 0.0f;
            const std::uint32_t y0 = ty * tile_size;
            const std::uint32_t x0 = tx * tile_size;
            const std::uint32_t y1 = std::min(y0 + tile_size, height);
            const std::uint32_t x1 = std::min(x0 + tile_size, width);
            for (std::uint32_t y = y0; y < y1; ++y) {
                for (std::uint32_t x = x0; x < x1; ++x) {
                    const std::size_t i = (static_cast<std::size_t>(y) * width + x) * 2u;
                    const float vx = velocities[i + 0];
                    const float vy = velocities[i + 1];
                    const float len_sq = vx * vx + vy * vy;
                    if (len_sq > best_len_sq) {
                        best_len_sq = len_sq;
                        best_vx = vx;
                        best_vy = vy;
                    }
                }
            }
            const std::size_t o = (static_cast<std::size_t>(ty) * tw + tx) * 2u;
            tile_max[o + 0] = best_vx;
            tile_max[o + 1] = best_vy;
        }
    }
}

void motion_blur_neighbormax(const std::vector<float>& tile_max,
                               std::uint32_t tile_w,
                               std::uint32_t tile_h,
                               std::vector<float>& neighbor_max) {
    neighbor_max.assign(tile_max.size(), 0.0f);
    for (std::uint32_t ty = 0; ty < tile_h; ++ty) {
        for (std::uint32_t tx = 0; tx < tile_w; ++tx) {
            float best_len_sq = 0.0f;
            float best_vx = 0.0f, best_vy = 0.0f;
            for (std::int32_t dy = -1; dy <= 1; ++dy) {
                for (std::int32_t dx = -1; dx <= 1; ++dx) {
                    const std::int32_t xi = static_cast<std::int32_t>(tx) + dx;
                    const std::int32_t yi = static_cast<std::int32_t>(ty) + dy;
                    if (xi < 0 || yi < 0) continue;
                    if (xi >= static_cast<std::int32_t>(tile_w) ||
                        yi >= static_cast<std::int32_t>(tile_h)) continue;
                    const std::size_t i = (static_cast<std::size_t>(yi) * tile_w
                                            + static_cast<std::size_t>(xi)) * 2u;
                    const float vx = tile_max[i + 0];
                    const float vy = tile_max[i + 1];
                    const float len_sq = vx * vx + vy * vy;
                    if (len_sq > best_len_sq) {
                        best_len_sq = len_sq;
                        best_vx = vx;
                        best_vy = vy;
                    }
                }
            }
            const std::size_t o = (static_cast<std::size_t>(ty) * tile_w + tx) * 2u;
            neighbor_max[o + 0] = best_vx;
            neighbor_max[o + 1] = best_vy;
        }
    }
}

} // namespace gw::render::post
