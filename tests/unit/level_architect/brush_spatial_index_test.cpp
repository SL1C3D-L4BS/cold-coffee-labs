// tests/unit/level_architect/brush_spatial_index_test.cpp — ADR-0123 Phase-A grid index.

#include <doctest/doctest.h>

#include "engine/services/level_architect/brush_spatial_index.hpp"

#include "engine/math/vec.hpp"

using gw::math::Vec3f;
using gw::services::level_architect::BrushAabb;
using gw::services::level_architect::BrushSpatialIndex;
using gw::services::level_architect::BrushSpatialIndexConfig;

TEST_CASE("BrushSpatialIndex — empty build yields empty query") {
    const std::vector<BrushAabb> empty{};
    const auto                   idx = BrushSpatialIndex::build(empty);
    const BrushAabb              q{Vec3f{0.f, 0.f, 0.f}, Vec3f{1.f, 1.f, 1.f}};
    CHECK(idx.query_overlapping(q).empty());
}

TEST_CASE("BrushSpatialIndex — finds overlapping brush deterministically") {
    const BrushAabb b0{Vec3f{0.f, 0.f, 0.f}, Vec3f{1.f, 1.f, 1.f}};
    const BrushAabb b1{Vec3f{10.f, 10.f, 10.f}, Vec3f{11.f, 11.f, 11.f}};
    const BrushAabb brushes[2] = {b0, b1};

    BrushSpatialIndexConfig cfg{};
    cfg.cells_x = cfg.cells_y = cfg.cells_z = 4u;
    const auto                idx = BrushSpatialIndex::build(brushes, cfg);

    const BrushAabb query{Vec3f{0.5f, 0.5f, 0.5f}, Vec3f{0.6f, 0.6f, 0.6f}};
    const auto      hits = idx.query_overlapping(query);
    REQUIRE(hits.size() == 1);
    CHECK(hits[0] == 0u);

    const BrushAabb query2{Vec3f{10.2f, 10.2f, 10.2f}, Vec3f{10.8f, 10.8f, 10.8f}};
    const auto      hits2 = idx.query_overlapping(query2);
    REQUIRE(hits2.size() == 1);
    CHECK(hits2[0] == 1u);
}

TEST_CASE("BrushSpatialIndex — sorted order stable across queries") {
    const BrushAabb b0{Vec3f{0.f, 0.f, 0.f}, Vec3f{2.f, 0.5f, 0.5f}};
    const BrushAabb b1{Vec3f{0.5f, 0.f, 0.f}, Vec3f{1.5f, 1.f, 1.f}};
    const BrushAabb brushes[2] = {b0, b1};
    const auto      idx = BrushSpatialIndex::build(brushes, {4u, 4u, 4u});
    const BrushAabb q{Vec3f{0.4f, 0.4f, 0.4f}, Vec3f{0.6f, 0.6f, 0.6f}};
    const auto      a = idx.query_overlapping(q);
    const auto      b = idx.query_overlapping(q);
    CHECK(a == b);
    for (std::size_t i = 1; i < a.size(); ++i) {
        CHECK(a[i - 1] < a[i]);
    }
}
