// tests/unit/level_architect/brush_aabb_test.cpp

#include <doctest/doctest.h>

#include "engine/math/vec.hpp"
#include "engine/services/level_architect/brush_aabb.hpp"

#include <array>

TEST_CASE("brush_aabb — union_of_brushes merges extents") {
    using gw::math::Vec3f;
    using gw::services::level_architect::BrushAabb;
    using gw::services::level_architect::union_of_brushes;

    const std::array<BrushAabb, 2> boxes{{
        BrushAabb{Vec3f{0.f, 0.f, 0.f}, Vec3f{1.f, 1.f, 1.f}},
        BrushAabb{Vec3f{1.f, 0.f, 0.f}, Vec3f{2.f, 2.f, 2.f}},
    }};
    const BrushAabb u = union_of_brushes(boxes);
    CHECK(u.min.x() == doctest::Approx(0.f));
    CHECK(u.min.y() == doctest::Approx(0.f));
    CHECK(u.min.z() == doctest::Approx(0.f));
    CHECK(u.max.x() == doctest::Approx(2.f));
    CHECK(u.max.y() == doctest::Approx(2.f));
    CHECK(u.max.z() == doctest::Approx(2.f));
}
