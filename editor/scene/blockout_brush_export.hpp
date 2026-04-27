#pragma once
// editor/scene/blockout_brush_export.hpp
//
// ADR-0123: authoring blockout boxes → `BrushAabb` feed for `BrushSpatialIndex`
// (deterministic uniform-grid PVS candidate / collision prep).

#include "engine/ecs/world.hpp"
#include "engine/services/level_architect/brush_aabb.hpp"

#include <vector>

namespace gw::editor::scene {

/// Collects axis-aligned bounds for every **box** blockout primitive (shape==0).
/// Rotated boxes use the world-space AABB of the eight corners (matches physics
/// half-extents in `blockout_physics_sync.cpp`).
void append_blockout_brush_aabbs(gw::ecs::World& world,
                                 std::vector<gw::services::level_architect::BrushAabb>& out);

}  // namespace gw::editor::scene
