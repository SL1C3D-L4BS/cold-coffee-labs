// engine/vfx/decals/decal_volume.cpp — ADR-0078 Wave 17E.

#include "engine/vfx/decals/decal_volume.hpp"

#include <algorithm>
#include <cmath>

namespace gw::vfx::decals {

namespace {

inline std::array<float, 3> rotate_by_quat(const std::array<float, 4>& q,
                                             const std::array<float, 3>& v) noexcept {
    // v' = v + 2 * cross(q.xyz, cross(q.xyz, v) + q.w * v)
    const float qx = q[0], qy = q[1], qz = q[2], qw = q[3];
    const float ix =  qw * v[0] + qy * v[2] - qz * v[1];
    const float iy =  qw * v[1] + qz * v[0] - qx * v[2];
    const float iz =  qw * v[2] + qx * v[1] - qy * v[0];
    const float iw = -qx * v[0] - qy * v[1] - qz * v[2];
    return {
        ix * qw + iw * -qx + iy * -qz - iz * -qy,
        iy * qw + iw * -qy + iz * -qx - ix * -qz,
        iz * qw + iw * -qz + ix * -qy - iy * -qx,
    };
}

} // namespace

bool contains_point(const DecalVolume& v, const std::array<float, 3>& p) noexcept {
    // Translate to decal local space.
    const std::array<float, 3> d{
        p[0] - v.position[0],
        p[1] - v.position[1],
        p[2] - v.position[2],
    };
    // Inverse-rotate (conjugate quaternion).
    const std::array<float, 4> conj{-v.orientation[0], -v.orientation[1], -v.orientation[2], v.orientation[3]};
    const auto local = rotate_by_quat(conj, d);
    return std::abs(local[0]) <= v.half_extent[0] &&
           std::abs(local[1]) <= v.half_extent[1] &&
           std::abs(local[2]) <= v.half_extent[2];
}

TileCoord project_to_tile(const std::array<float, 3>& world_pos,
                            std::uint32_t width,
                            std::uint32_t height,
                            std::uint32_t tile_size) noexcept {
    // Deterministic mock projection: NDC = tanh(position). Good enough for
    // tile-binning unit tests (real path plugs in camera matrix).
    const float nx = std::tanh(world_pos[0]);
    const float ny = std::tanh(world_pos[1]);
    const float sx = (nx * 0.5f + 0.5f) * static_cast<float>(width);
    const float sy = (ny * 0.5f + 0.5f) * static_cast<float>(height);
    const auto ts = tile_size == 0 ? 16u : tile_size;
    const std::uint32_t tx = std::min(
        static_cast<std::uint32_t>(std::max(0.0f, sx)) / ts,
        (width  + ts - 1) / ts - 1);
    const std::uint32_t ty = std::min(
        static_cast<std::uint32_t>(std::max(0.0f, sy)) / ts,
        (height + ts - 1) / ts - 1);
    return {tx, ty};
}

} // namespace gw::vfx::decals
