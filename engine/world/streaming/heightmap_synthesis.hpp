#pragma once
// Synchronous 32×32 height + biome lattice (same kernel as time-sliced chunk jobs).

#include "engine/world/streaming/chunk_data.hpp"
#include "engine/world/universe/hec.hpp"

namespace gw::world::streaming {

void fill_heightmap_row_range(HeightmapChunk& hc, const gw::universe::UniverseSeed& chunk_seed, int y_begin, int y_end) noexcept;

inline void fill_heightmap_chunk_full(HeightmapChunk& hc, const gw::universe::UniverseSeed& chunk_seed) noexcept {
    fill_heightmap_row_range(hc, chunk_seed, 0, static_cast<int>(kHeightmapResolution));
}

} // namespace gw::world::streaming
