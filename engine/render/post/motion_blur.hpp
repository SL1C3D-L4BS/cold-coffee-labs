#pragma once
// engine/render/post/motion_blur.hpp — ADR-0079 Wave 17F: McGuire velocity MB.

#include <array>
#include <cstdint>
#include <vector>

namespace gw::render::post {

struct MotionBlurConfig {
    bool          enabled{true};
    std::uint32_t max_velocity_px{32};
    std::uint32_t tile_size{16};
};

// TileMax: per-tile max velocity (length²) reduction. Deterministic.
// velocities: width*height*2 floats (vx, vy).
// Output: tile_w*tile_h*2 floats.
void motion_blur_tilemax(const std::vector<float>& velocities,
                          std::uint32_t width,
                          std::uint32_t height,
                          std::uint32_t tile_size,
                          std::vector<float>& tile_max);

// NeighborMax: 3x3 max over tile_max.
void motion_blur_neighbormax(const std::vector<float>& tile_max,
                              std::uint32_t tile_w,
                              std::uint32_t tile_h,
                              std::vector<float>& neighbor_max);

} // namespace gw::render::post
