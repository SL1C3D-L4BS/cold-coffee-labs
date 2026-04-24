// tests/unit/render/render_visibility_policy_test.cpp — GPU visibility policy stub (plan Track B1).

#include <doctest/doctest.h>

#include "engine/render/visibility/render_visibility_policy.hpp"

TEST_CASE("render_visibility — passthrough policy draws") {
    gw::render::visibility::PassthroughVisibilityPolicy pol;
    const auto d = pol.evaluate(gw::render::visibility::VisibilityQuery{});
    CHECK(d.draw);
    CHECK(d.lod_level == 0);
}
