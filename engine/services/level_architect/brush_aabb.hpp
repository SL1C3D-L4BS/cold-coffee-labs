#pragma once
// engine/services/level_architect/brush_aabb.hpp — Tier-0 brush bounds (ADR-0123 feed).
//
// Full BSP/vis lives on the ADR-0123 track; this TU is the deterministic
// spatial primitive every cook/collision path can consume without a second
// world model.

#include "engine/math/vec.hpp"

#include <span>

namespace gw::services::level_architect {

struct BrushAabb {
    gw::math::Vec3f min{};
    gw::math::Vec3f max{};
};

/// Axis-aligned union of brush volumes. Empty input yields a degenerate box at origin.
[[nodiscard]] BrushAabb union_of_brushes(std::span<const BrushAabb> brushes) noexcept;

}  // namespace gw::services::level_architect
