#pragma once
// engine/world/streaming/chunk_coord.hpp — Phase 19-B chunk lattice (f64 world space, §4 relevance window).

#include "engine/math/vec.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace gw::world::streaming {

/// Canonical world-space vector for streaming / simulation (f64).
using Vec3_f64 = gw::math::WorldVec3d;

/// Integer lattice cell for spatial streaming; Chebyshev radius matches relevance shells.
struct ChunkCoord {
    std::int64_t x{};
    std::int64_t y{};
    std::int64_t z{};

    [[nodiscard]] constexpr bool operator==(const ChunkCoord& o) const noexcept {
        return x == o.x && y == o.y && z == o.z;
    }
};

[[nodiscard]] constexpr std::size_t chunk_coord_hash(const ChunkCoord& c) noexcept {
    // FNV-1a 64-bit mix — stable across platforms for `gw::core::HashMap` buckets.
    std::uint64_t h = 14695981039346656037ull;
    h ^= static_cast<std::uint64_t>(c.x);
    h *= 1099511628211ull;
    h ^= static_cast<std::uint64_t>(c.y);
    h *= 1099511628211ull;
    h ^= static_cast<std::uint64_t>(c.z);
    h *= 1099511628211ull;
    return static_cast<std::size_t>(h);
}

struct ChunkCoordHash {
    [[nodiscard]] std::size_t operator()(const ChunkCoord& c) const noexcept {
        return chunk_coord_hash(c);
    }
};

inline constexpr double CHUNK_SIZE_METERS = 32.0;

[[nodiscard]] constexpr ChunkCoord world_to_chunk(const Vec3_f64& world_pos) noexcept {
    const auto fx = world_pos.x() / CHUNK_SIZE_METERS;
    const auto fy = world_pos.y() / CHUNK_SIZE_METERS;
    const auto fz = world_pos.z() / CHUNK_SIZE_METERS;
    const auto ix = static_cast<std::int64_t>(std::floor(fx));
    const auto iy = static_cast<std::int64_t>(std::floor(fy));
    const auto iz = static_cast<std::int64_t>(std::floor(fz));
    return ChunkCoord{ix, iy, iz};
}

/// Minimum corner (cell-local origin) of the chunk in absolute world metres.
[[nodiscard]] constexpr Vec3_f64 chunk_origin(const ChunkCoord& c) noexcept {
    return Vec3_f64(static_cast<double>(c.x) * CHUNK_SIZE_METERS,
                    static_cast<double>(c.y) * CHUNK_SIZE_METERS,
                    static_cast<double>(c.z) * CHUNK_SIZE_METERS);
}

[[nodiscard]] constexpr Vec3_f64 chunk_center(const ChunkCoord& c) noexcept {
    const double h = CHUNK_SIZE_METERS * 0.5;
    return chunk_origin(c) + Vec3_f64(h, h, h);
}

[[nodiscard]] constexpr std::int32_t chebyshev_distance(const ChunkCoord& a, const ChunkCoord& b) noexcept {
    const auto ax = static_cast<std::int64_t>(std::llabs(a.x - b.x));
    const auto ay = static_cast<std::int64_t>(std::llabs(a.y - b.y));
    const auto az = static_cast<std::int64_t>(std::llabs(a.z - b.z));
    const auto m  = ax > ay ? ax : ay;
    return static_cast<std::int32_t>((m > az) ? m : az);
}

} // namespace gw::world::streaming

namespace std {

template<>
struct hash<gw::world::streaming::ChunkCoord> {
    [[nodiscard]] std::size_t operator()(const gw::world::streaming::ChunkCoord& c) const noexcept {
        return gw::world::streaming::chunk_coord_hash(c);
    }
};

} // namespace std
