// tests/unit/net/cvars_console_test.cpp — Phase 14 Wave 14K.

#include <doctest/doctest.h>

#include "engine/core/config/cvar_registry.hpp"
#include "engine/net/net_cvars.hpp"

using namespace gw;

TEST_CASE("register_network_cvars registers every net.* CVar") {
    config::CVarRegistry r;
    auto nc = net::register_network_cvars(r);
    (void)nc;
    CHECK(r.count() >= 26);
    CHECK(r.get_f32_or("net.tick_hz", 0.0f) == doctest::Approx(60.0f));
    CHECK(r.get_f32_or("net.send.budget_kbps", 0.0f) == doctest::Approx(96.0f));
    CHECK(r.get_i32_or("net.max_peers", 0) == 16);
    CHECK(r.get_bool_or("net.voice.enable", false) == true);
}

TEST_CASE("apply_cvars_to_config bridges CVar state into NetworkConfig") {
    config::CVarRegistry r;
    (void)net::register_network_cvars(r);
    net::NetworkConfig cfg{};
    net::apply_cvars_to_config(r, cfg);
    CHECK(cfg.tick_hz == doctest::Approx(60.0f));
    CHECK(cfg.interest_radius_m == doctest::Approx(64.0f));
    CHECK(cfg.max_peers == 16u);
    CHECK(cfg.voice_enable == true);
    CHECK(cfg.voice_fec_enable == true);
}
