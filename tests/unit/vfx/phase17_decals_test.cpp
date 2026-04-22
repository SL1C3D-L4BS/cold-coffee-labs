// tests/unit/vfx/phase17_decals_test.cpp — ADR-0078 Wave 17E.

#include <doctest/doctest.h>

#include "engine/vfx/decals/decal_renderer.hpp"
#include "engine/vfx/decals/decal_volume.hpp"

using namespace gw::vfx::decals;

static DecalVolume make_volume(float x, float y, float z, float hx, float hy, float hz) {
    DecalVolume v;
    v.position    = {x, y, z};
    v.half_extent = {hx, hy, hz};
    v.orientation = {0, 0, 0, 1};
    return v;
}

TEST_CASE("phase17.decals: contains_point inside unit box") {
    const auto v = make_volume(0, 0, 0, 1, 1, 1);
    CHECK(contains_point(v, {0.5f, 0.5f, 0.5f}));
}

TEST_CASE("phase17.decals: contains_point outside unit box") {
    const auto v = make_volume(0, 0, 0, 1, 1, 1);
    CHECK_FALSE(contains_point(v, {2.0f, 2.0f, 2.0f}));
}

TEST_CASE("phase17.decals: contains_point on boundary is inclusive") {
    const auto v = make_volume(0, 0, 0, 1, 1, 1);
    CHECK(contains_point(v, {1.0f, 0, 0}));
}

TEST_CASE("phase17.decals: project_to_tile is deterministic + in-range") {
    const auto tc1 = project_to_tile({0.5f, -0.25f, 0.0f}, 1920, 1080, 16);
    const auto tc2 = project_to_tile({0.5f, -0.25f, 0.0f}, 1920, 1080, 16);
    CHECK(tc1.x == tc2.x);
    CHECK(tc1.y == tc2.y);
    CHECK(tc1.x < 1920u / 16u);
    CHECK(tc1.y < 1080u / 16u);
}

TEST_CASE("phase17.decals: project_to_tile clamps out-of-range positions") {
    const auto tc = project_to_tile({9999.0f, -9999.0f, 0}, 1920, 1080, 16);
    CHECK(tc.x < 1920u / 16u + 1);
    CHECK(tc.y < 1080u / 16u + 1);
}

TEST_CASE("phase17.decals.binner: insert + find roundtrip") {
    DecalBinner b{1920, 1080, 16};
    const auto v = make_volume(0.25f, 0.25f, 0.0f, 4, 4, 4);
    b.insert(DecalId{1}, v);
    const auto tc = project_to_tile({0.25f, 0.25f, 0.0f}, 1920, 1080, 16);
    const auto* bin = b.find(tc.x, tc.y);
    REQUIRE(bin);
    CHECK(bin->ids.size() == 1);
    CHECK(bin->ids.front().value == 1u);
}

TEST_CASE("phase17.decals.binner: multiple decals in same tile accumulate") {
    DecalBinner b{1920, 1080, 16};
    const auto v = make_volume(0.1f, 0.1f, 0.0f, 2, 2, 2);
    b.insert(DecalId{1}, v);
    b.insert(DecalId{2}, v);
    b.insert(DecalId{3}, v);
    CHECK(b.bin_count() == 1);
    const auto tc = project_to_tile({0.1f, 0.1f, 0.0f}, 1920, 1080, 16);
    REQUIRE(b.find(tc.x, tc.y));
    CHECK(b.find(tc.x, tc.y)->ids.size() == 3);
}

TEST_CASE("phase17.decals.binner: clear empties bins") {
    DecalBinner b{1920, 1080, 16};
    b.insert(DecalId{1}, make_volume(0.0f, 0.0f, 0.0f, 1, 1, 1));
    b.clear();
    CHECK(b.bin_count() == 0);
}
