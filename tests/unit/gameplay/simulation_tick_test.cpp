// tests/unit/gameplay/simulation_tick_test.cpp — PIE/runtime simulation step harness.

#include <doctest/doctest.h>

#include "engine/ecs/pie_sim_component.hpp"
#include "engine/ecs/world.hpp"
#include "engine/play/playable_paths.hpp"
#include "gameplay/simulation_tick.hpp"

TEST_CASE("simulation_step advances tick counter") {
    gw::gameplay::simulation_reset();
    CHECK(gw::gameplay::simulation_session_tick_count() == 0);
    gw::gameplay::simulation_step(1.0 / 60.0);
    CHECK(gw::gameplay::simulation_session_tick_count() == 1);
    for (int i = 0; i < 99; ++i) {
        gw::gameplay::simulation_step(1.0 / 60.0);
    }
    CHECK(gw::gameplay::simulation_session_tick_count() == 100);
}

TEST_CASE("simulation_reset clears counter") {
    gw::gameplay::simulation_step(0.016);
    gw::gameplay::simulation_reset();
    CHECK(gw::gameplay::simulation_session_tick_count() == 0);
}

TEST_CASE("simulation_bind_ecs_world cleared by simulation_reset") {
    gw::ecs::World w;
    (void)w.create_entity();
    gw::gameplay::simulation_reset();
    gw::gameplay::simulation_bind_ecs_world(&w);
    gw::gameplay::simulation_step(1.0 / 60.0);
    gw::gameplay::simulation_step(1.0 / 60.0);
    CHECK(gw::gameplay::simulation_session_tick_count() == 2);
    gw::gameplay::simulation_reset();
    CHECK(gw::gameplay::simulation_session_tick_count() == 0);
    gw::gameplay::simulation_step(1.0 / 60.0);
    CHECK(gw::gameplay::simulation_session_tick_count() == 1);
}

TEST_CASE("simulation_set_play_bootstrap affects director seed") {
    gw::gameplay::simulation_reset();
    gw::gameplay::simulation_set_play_bootstrap(42, nullptr);
    gw::gameplay::simulation_step(1.0 / 60.0);
    const std::uint64_t s42 = gw::gameplay::simulation_last_director_seed();
    gw::gameplay::simulation_reset();
    gw::gameplay::simulation_set_play_bootstrap(gw::play::kDefaultPlayUniverseSeed, nullptr);
    gw::gameplay::simulation_step(1.0 / 60.0);
    const std::uint64_t s1 = gw::gameplay::simulation_last_director_seed();
    CHECK(s42 != s1);
}

TEST_CASE("simulation_step increments PieSimTickComponent on bound world") {
    gw::gameplay::simulation_reset();
    gw::ecs::World w;
    const auto e = w.create_entity();
    w.add_component(e, gw::ecs::PieSimTickComponent{});
    gw::gameplay::simulation_bind_ecs_world(&w);
    gw::gameplay::simulation_step(1.0 / 60.0);
    gw::gameplay::simulation_step(1.0 / 60.0);
    const auto* tag = w.get_component<gw::ecs::PieSimTickComponent>(e);
    REQUIRE(tag);
    CHECK(tag->accum == 2u);
}
