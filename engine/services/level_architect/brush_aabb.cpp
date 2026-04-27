// engine/services/level_architect/brush_aabb.cpp
#include "engine/services/level_architect/brush_aabb.hpp"

#include <algorithm>

namespace gw::services::level_architect {

BrushAabb union_of_brushes(std::span<const BrushAabb> brushes) noexcept {
    if (brushes.empty()) {
        return BrushAabb{};
    }
    float min_x = brushes[0].min.x();
    float min_y = brushes[0].min.y();
    float min_z = brushes[0].min.z();
    float max_x = brushes[0].max.x();
    float max_y = brushes[0].max.y();
    float max_z = brushes[0].max.z();
    for (std::size_t i = 1; i < brushes.size(); ++i) {
        const auto& b = brushes[i];
        min_x = std::min(min_x, b.min.x());
        min_y = std::min(min_y, b.min.y());
        min_z = std::min(min_z, b.min.z());
        max_x = std::max(max_x, b.max.x());
        max_y = std::max(max_y, b.max.y());
        max_z = std::max(max_z, b.max.z());
    }
    return BrushAabb{gw::math::Vec3f{min_x, min_y, min_z}, gw::math::Vec3f{max_x, max_y, max_z}};
}

}  // namespace gw::services::level_architect
