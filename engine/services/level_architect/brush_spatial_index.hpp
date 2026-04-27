#pragma once
// engine/services/level_architect/brush_spatial_index.hpp
//
// ADR-0123 Phase-A: deterministic **uniform-grid** spatial index over `BrushAabb`.
// This is the documented conservative substitute before classic BSP2 / PVS —
// `query_overlapping` returns every brush whose AABB overlaps the query AABB
// (no false negatives for AABB-vs-AABB tests).

#include "engine/services/level_architect/brush_aabb.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace gw::services::level_architect {

struct BrushSpatialIndexConfig {
    std::uint32_t cells_x{8};
    std::uint32_t cells_y{8};
    std::uint32_t cells_z{8};
};

class BrushSpatialIndex {
public:
    /// Builds a uniform grid covering the union of all brush AABBs (expanded slightly).
    [[nodiscard]] static BrushSpatialIndex build(std::span<const BrushAabb> brushes,
                                                 const BrushSpatialIndexConfig& cfg = {});

    /// Sorted ascending, unique brush indices whose AABB overlaps `query`.
    [[nodiscard]] std::vector<std::uint32_t> query_overlapping(const BrushAabb& query) const;

private:
    BrushSpatialIndex() = default;

    [[nodiscard]] std::size_t cell_index(std::uint32_t ix, std::uint32_t iy, std::uint32_t iz) const noexcept {
        return (static_cast<std::size_t>(iz) * ny_ + iy) * nx_ + ix;
    }

    void rasterize_brush_to_cells(const BrushAabb& b, std::uint32_t brush_index);

    BrushAabb    bounds_{};
    float        cell_x_{1.f};
    float        cell_y_{1.f};
    float        cell_z_{1.f};
    std::uint32_t nx_{1};
    std::uint32_t ny_{1};
    std::uint32_t nz_{1};
    /// Flattened `nx * ny * nz` buckets of brush indices.
    std::vector<std::vector<std::uint32_t>> buckets_{};
};

}  // namespace gw::services::level_architect
