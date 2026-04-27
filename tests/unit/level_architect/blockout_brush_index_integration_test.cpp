// tests/unit/level_architect/blockout_brush_index_integration_test.cpp
// ADR-0123: authoring blockout → BrushSpatialIndex (graph: brush_spatial_index).

#include <doctest/doctest.h>

#include "editor/scene/authoring_scene_load.hpp"
#include "editor/scene/blockout_brush_export.hpp"
#include "editor/scene/components.hpp"
#include "engine/ecs/world.hpp"
#include "engine/math/vec.hpp"
#include "engine/services/level_architect/brush_spatial_index.hpp"

using gw::editor::scene::BlockoutPrimitiveComponent;
using gw::editor::scene::TransformComponent;
using gw::services::level_architect::BrushAabb;
using gw::services::level_architect::BrushSpatialIndex;

TEST_CASE("blockout brush export → spatial index overlap") {
    gw::ecs::World world;
    gw::editor::scene::ensure_authoring_scene_types_for_load(world);

    const auto e = world.create_entity();
    TransformComponent t{};
    t.position = {0.0, 0.0, 0.0};
    t.scale    = {2.f, 2.f, 2.f};
    BlockoutPrimitiveComponent b{};
    b.shape = 0;
    world.add_component(e, t);
    world.add_component(e, b);

    std::vector<BrushAabb> aabbs;
    gw::editor::scene::append_blockout_brush_aabbs(world, aabbs);
    REQUIRE(aabbs.size() == 1u);

    const auto index = BrushSpatialIndex::build(aabbs);
    BrushAabb      query{};
    query.min = gw::math::Vec3f{-0.05f, -0.05f, -0.05f};
    query.max = gw::math::Vec3f{0.05f, 0.05f, 0.05f};
    const auto hits = index.query_overlapping(query);
    CHECK_FALSE(hits.empty());
}
