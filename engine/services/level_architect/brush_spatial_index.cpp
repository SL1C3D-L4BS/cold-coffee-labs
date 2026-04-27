// engine/services/level_architect/brush_spatial_index.cpp

#include "engine/services/level_architect/brush_spatial_index.hpp"

#include <algorithm>
#include <cmath>

namespace gw::services::level_architect {
namespace {

constexpr float kEpsilonExpand = 1e-3f;

[[nodiscard]] BrushAabb expand_degenerate_axes(BrushAabb b) noexcept {
    auto ax = b.max.x() - b.min.x();
    auto ay = b.max.y() - b.min.y();
    auto az = b.max.z() - b.min.z();
    using gw::math::Vec3f;
    if (ax < kEpsilonExpand) {
        b.min = Vec3f{b.min.x() - kEpsilonExpand, b.min.y(), b.min.z()};
        b.max = Vec3f{b.max.x() + kEpsilonExpand, b.max.y(), b.max.z()};
    }
    if (ay < kEpsilonExpand) {
        b.min = Vec3f{b.min.x(), b.min.y() - kEpsilonExpand, b.min.z()};
        b.max = Vec3f{b.max.x(), b.max.y() + kEpsilonExpand, b.max.z()};
    }
    if (az < kEpsilonExpand) {
        b.min = Vec3f{b.min.x(), b.min.y(), b.min.z() - kEpsilonExpand};
        b.max = Vec3f{b.max.x(), b.max.y(), b.max.z() + kEpsilonExpand};
    }
    return b;
}

[[nodiscard]] std::uint32_t clamp_axis_index(float v, float min_v, float inv_cell, std::uint32_t n) noexcept {
    if (n <= 1u) return 0u;
    const float t = (v - min_v) * inv_cell;
    auto          idx = static_cast<std::int32_t>(std::floor(t));
    if (idx < 0) idx = 0;
    const auto max_i = static_cast<std::int32_t>(n - 1u);
    if (idx > max_i) idx = max_i;
    return static_cast<std::uint32_t>(idx);
}

}  // namespace

BrushSpatialIndex BrushSpatialIndex::build(std::span<const BrushAabb> brushes,
                                           const BrushSpatialIndexConfig& cfg) {
    BrushSpatialIndex out{};
    if (brushes.empty()) {
        out.bounds_ = BrushAabb{};
        out.cell_x_ = out.cell_y_ = out.cell_z_ = 1.f;
        out.nx_ = out.ny_ = out.nz_ = 1u;
        out.buckets_.assign(1, {});
        return out;
    }

    out.bounds_ = expand_degenerate_axes(union_of_brushes(brushes));
    out.nx_     = std::max(1u, cfg.cells_x);
    out.ny_     = std::max(1u, cfg.cells_y);
    out.nz_     = std::max(1u, cfg.cells_z);

    out.cell_x_ = (out.bounds_.max.x() - out.bounds_.min.x()) / static_cast<float>(out.nx_);
    out.cell_y_ = (out.bounds_.max.y() - out.bounds_.min.y()) / static_cast<float>(out.ny_);
    out.cell_z_ = (out.bounds_.max.z() - out.bounds_.min.z()) / static_cast<float>(out.nz_);
    if (out.cell_x_ < kEpsilonExpand) out.cell_x_ = kEpsilonExpand;
    if (out.cell_y_ < kEpsilonExpand) out.cell_y_ = kEpsilonExpand;
    if (out.cell_z_ < kEpsilonExpand) out.cell_z_ = kEpsilonExpand;

    out.buckets_.assign(static_cast<std::size_t>(out.nx_) * out.ny_ * out.nz_, {});

    for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(brushes.size()); ++i) {
        out.rasterize_brush_to_cells(brushes[i], i);
    }
    return out;
}

void BrushSpatialIndex::rasterize_brush_to_cells(const BrushAabb& b, const std::uint32_t brush_index) {
    const float inv_x = 1.f / cell_x_;
    const float inv_y = 1.f / cell_y_;
    const float inv_z = 1.f / cell_z_;

    const std::uint32_t ix0 = clamp_axis_index(b.min.x(), bounds_.min.x(), inv_x, nx_);
    const std::uint32_t iy0 = clamp_axis_index(b.min.y(), bounds_.min.y(), inv_y, ny_);
    const std::uint32_t iz0 = clamp_axis_index(b.min.z(), bounds_.min.z(), inv_z, nz_);
    const std::uint32_t ix1 = clamp_axis_index(b.max.x(), bounds_.min.x(), inv_x, nx_);
    const std::uint32_t iy1 = clamp_axis_index(b.max.y(), bounds_.min.y(), inv_y, ny_);
    const std::uint32_t iz1 = clamp_axis_index(b.max.z(), bounds_.min.z(), inv_z, nz_);

    for (std::uint32_t iz = iz0; iz <= iz1; ++iz) {
        for (std::uint32_t iy = iy0; iy <= iy1; ++iy) {
            for (std::uint32_t ix = ix0; ix <= ix1; ++ix) {
                buckets_[cell_index(ix, iy, iz)].push_back(brush_index);
            }
        }
    }
}

std::vector<std::uint32_t> BrushSpatialIndex::query_overlapping(const BrushAabb& query) const {
    std::vector<std::uint32_t> out;
    if (buckets_.empty()) return out;

    const float inv_x = 1.f / cell_x_;
    const float inv_y = 1.f / cell_y_;
    const float inv_z = 1.f / cell_z_;

    const std::uint32_t ix0 = clamp_axis_index(query.min.x(), bounds_.min.x(), inv_x, nx_);
    const std::uint32_t iy0 = clamp_axis_index(query.min.y(), bounds_.min.y(), inv_y, ny_);
    const std::uint32_t iz0 = clamp_axis_index(query.min.z(), bounds_.min.z(), inv_z, nz_);
    const std::uint32_t ix1 = clamp_axis_index(query.max.x(), bounds_.min.x(), inv_x, nx_);
    const std::uint32_t iy1 = clamp_axis_index(query.max.y(), bounds_.min.y(), inv_y, ny_);
    const std::uint32_t iz1 = clamp_axis_index(query.max.z(), bounds_.min.z(), inv_z, nz_);

    for (std::uint32_t iz = iz0; iz <= iz1; ++iz) {
        for (std::uint32_t iy = iy0; iy <= iy1; ++iy) {
            for (std::uint32_t ix = ix0; ix <= ix1; ++ix) {
                const auto& bucket = buckets_[cell_index(ix, iy, iz)];
                out.insert(out.end(), bucket.begin(), bucket.end());
            }
        }
    }

    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

}  // namespace gw::services::level_architect
