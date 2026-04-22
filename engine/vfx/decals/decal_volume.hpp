#pragma once
// engine/vfx/decals/decal_volume.hpp — ADR-0078 Wave 17E.

#include <array>
#include <cstdint>

namespace gw::vfx::decals {

struct DecalId {
    std::uint32_t value{0};
    [[nodiscard]] constexpr bool valid() const noexcept { return value != 0; }
    constexpr bool operator==(const DecalId&) const noexcept = default;
};

struct DecalVolume {
    std::array<float, 3> position{0, 0, 0};
    std::array<float, 3> half_extent{0.5f, 0.5f, 0.5f};
    std::array<float, 4> orientation{0.0f, 0.0f, 0.0f, 1.0f}; // quaternion xyzw
    std::uint32_t        albedo_texture{0};
    std::uint32_t        normal_texture{0};
    float                opacity{1.0f};
    std::uint32_t        sort_key{0};
};

// Test a world-space point against the decal OBB. Deterministic.
[[nodiscard]] bool contains_point(const DecalVolume& v, const std::array<float, 3>& p) noexcept;

// Screen-space cluster binning helper: returns the integer (tile_x, tile_y)
// where the projected decal center falls (reuses Phase 5 cluster schema).
struct TileCoord { std::uint32_t x{0}, y{0}; };
[[nodiscard]] TileCoord project_to_tile(const std::array<float, 3>& world_pos,
                                          std::uint32_t width,
                                          std::uint32_t height,
                                          std::uint32_t tile_size) noexcept;

} // namespace gw::vfx::decals
