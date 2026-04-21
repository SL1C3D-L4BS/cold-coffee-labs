// tests/unit/input/adaptive_profile_test.cpp — ADR-0021/0022 auto-profile.

#include <doctest/doctest.h>

#include "engine/input/adaptive_profile.hpp"

using namespace gw::input;

TEST_CASE("pick_profile: Quadstick beats generic Adaptive") {
    Device d{};
    d.caps = Cap_Adaptive | Cap_Quadstick;
    auto* p = pick_profile(d);
    REQUIRE(p != nullptr);
    CHECK(p->name == "quadstick");
}

TEST_CASE("pick_profile: Adaptive-only picks XAC profile") {
    Device d{};
    d.caps = Cap_Adaptive;
    auto* p = pick_profile(d);
    REQUIRE(p != nullptr);
    CHECK(p->name == "xbox_adaptive");
}

TEST_CASE("pick_profile: plain gamepad = no profile") {
    Device d{};
    d.caps = Cap_Buttons | Cap_Axes;
    CHECK(pick_profile(d) == nullptr);
}

TEST_CASE("apply_profile: overlays gameplay actions and hold-to-toggle defaults") {
    ActionMap map;
    map.sets.emplace_back("gameplay");
    auto xac = xbox_adaptive_profile();

    apply_profile(map, xac);

    CHECK(map.defaults.hold_to_toggle);
    auto* gameplay = map.find_set("gameplay");
    REQUIRE(gameplay != nullptr);
    CHECK(gameplay->find("fire") != nullptr);
    CHECK(gameplay->find("jump") != nullptr);
}
