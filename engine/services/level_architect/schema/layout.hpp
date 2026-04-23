#pragma once

#include <array>
#include <cstdint>

namespace gw::services::level_architect {

struct LayoutTileId { std::uint32_t value = 0; };

struct LayoutRequest {
    std::uint64_t seed           = 0;
    std::uint32_t profile_index  = 0;   // per-IP Circle / Chapter / Zone id
    std::uint32_t chunk_x        = 0;
    std::uint32_t chunk_y        = 0;
    std::uint32_t chunk_z        = 0;
    std::uint32_t tile_budget    = 1024;
};

struct LayoutTile {
    LayoutTileId tile{};
    std::array<float, 3> center{0.f, 0.f, 0.f};
    std::uint32_t        type   = 0;   // consumer-defined
    std::uint32_t        rot_oct = 0;  // 0..3 (90° steps)
};

struct LayoutResult {
    std::uint32_t tile_count = 0;
    const LayoutTile* tiles  = nullptr;   // borrowed; lives in caller arena
};

} // namespace gw::services::level_architect
