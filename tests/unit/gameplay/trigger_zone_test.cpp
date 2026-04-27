// tests/unit/gameplay/trigger_zone_test.cpp
// Pickup respawn state uses `unsigned` frame counts (see trigger_zone.hpp).

#include <doctest/doctest.h>

#include "gameplay/interaction/trigger_zone.hpp"

TEST_CASE("trigger_zone — contains_point inside/outside") {
    using gw::gameplay::interaction::AxisAlignedZone;
    using gw::gameplay::interaction::contains_point;

    const AxisAlignedZone box{.min_x = 0.f,
                               .min_y = 0.f,
                               .min_z = 0.f,
                               .max_x = 1.f,
                               .max_y = 1.f,
                               .max_z = 1.f};
    CHECK(contains_point(box, 0.5f, 0.5f, 0.5f));
    CHECK_FALSE(contains_point(box, 2.f, 0.f, 0.f));
}

TEST_CASE("trigger_zone — zones_overlap mirrors AABB predicate") {
    using gw::gameplay::interaction::AxisAlignedZone;
    using gw::gameplay::interaction::zones_overlap;

    const AxisAlignedZone a{.min_x = 0.f, .min_y = 0.f, .min_z = 0.f, .max_x = 1.f, .max_y = 1.f, .max_z = 1.f};
    const AxisAlignedZone b{.min_x = 0.5f, .min_y = 0.f, .min_z = 0.f, .max_x = 1.5f, .max_y = 1.f, .max_z = 1.f};
    CHECK(zones_overlap(a, b));
    const AxisAlignedZone far{.min_x = 5.f, .min_y = 0.f, .min_z = 0.f, .max_x = 6.f, .max_y = 1.f, .max_z = 1.f};
    CHECK_FALSE(zones_overlap(a, far));
}

TEST_CASE("trigger_zone — door slide advances toward fully open") {
    using gw::gameplay::interaction::advance_door_towards_open;
    using gw::gameplay::interaction::DoorSlideState;

    DoorSlideState st{};
    CHECK(advance_door_towards_open(st, 0.5f, 2.f) == doctest::Approx(1.f));
    CHECK(st.open01 == doctest::Approx(1.f));
}

TEST_CASE("trigger_zone — pickup respawn countdown") {
    using gw::gameplay::interaction::pickup_is_available;
    using gw::gameplay::interaction::PickupRespawnState;
    using gw::gameplay::interaction::tick_pickup_respawn;

    PickupRespawnState s{.frames_until_spawn = 2u};
    CHECK_FALSE(pickup_is_available(s));
    tick_pickup_respawn(s);
    CHECK_FALSE(pickup_is_available(s));
    tick_pickup_respawn(s);
    CHECK(pickup_is_available(s));
}
