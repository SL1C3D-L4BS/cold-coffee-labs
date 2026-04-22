// tests/unit/vfx/phase17_ribbons_test.cpp — ADR-0078 Wave 17E.

#include <doctest/doctest.h>

#include "engine/vfx/ribbons/ribbon_state.hpp"

using namespace gw::vfx::ribbons;

TEST_CASE("phase17.ribbons: RibbonVertex frozen at 32 B") {
    CHECK(sizeof(RibbonVertex) == 32);
}

TEST_CASE("phase17.ribbons: append grows until max_length") {
    RibbonDesc d; d.max_length = 4; d.node_period_s = 0.0f; d.lifetime_s = 10.0f;
    RibbonState r{d};
    for (int i = 0; i < 10; ++i) r.append({float(i), 0, 0}, 0.0f);
    CHECK(r.size() <= d.max_length);
    CHECK(r.size() == 4);
}

TEST_CASE("phase17.ribbons: oldest node dropped on overflow (ring)") {
    RibbonDesc d; d.max_length = 2; d.node_period_s = 0.0f; d.lifetime_s = 100.0f;
    RibbonState r{d};
    r.append({1, 0, 0}, 0.0f);
    r.append({2, 0, 0}, 0.0f);
    r.append({3, 0, 0}, 0.0f);
    CHECK(r.size() == 2);
    const auto& vs = r.vertices();
    CHECK(vs.front().position[0] == doctest::Approx(2.0f));
    CHECK(vs.back().position[0]  == doctest::Approx(3.0f));
}

TEST_CASE("phase17.ribbons: advance expires nodes past lifetime") {
    RibbonDesc d; d.max_length = 32; d.node_period_s = 0.0f; d.lifetime_s = 1.0f;
    RibbonState r{d};
    r.append({0, 0, 0}, 0.0f);
    r.advance(2.0f);
    CHECK(r.size() == 0);
}

TEST_CASE("phase17.ribbons: clear zeroes state") {
    RibbonDesc d; d.max_length = 4; d.node_period_s = 0.0f; d.lifetime_s = 10.0f;
    RibbonState r{d};
    r.append({0, 0, 0}, 0.0f);
    r.clear();
    CHECK(r.size() == 0);
    CHECK(r.accumulator() == doctest::Approx(0.0f));
}

TEST_CASE("phase17.ribbons: node_period gates append frequency") {
    RibbonDesc d; d.max_length = 32; d.node_period_s = 1.0f / 60.0f; d.lifetime_s = 10.0f;
    RibbonState r{d};
    r.append({0, 0, 0}, 0.0f);           // first node always lands
    r.append({1, 0, 0}, 0.0001f);         // below period → accumulated, not appended
    CHECK(r.size() <= 2);
}
