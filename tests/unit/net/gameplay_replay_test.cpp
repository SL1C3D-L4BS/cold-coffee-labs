// tests/unit/net/gameplay_replay_test.cpp — director replay packing (plan Track F3).

#include <doctest/doctest.h>

#include "engine/net/gameplay_replay.hpp"
#include "engine/net/snapshot.hpp"
#include "engine/services/director/director_service.hpp"
#include "gameplay/combat/encounter_director_bridge.hpp"

TEST_CASE("gameplay_replay — DirectorRequest bit round-trip") {
    gw::services::director::DirectorRequest r{};
    r.seed              = 0xDEADBEEFCAFEBABEu;
    r.logical_tick      = 9001;
    r.player_health_ratio = 0.73f;
    r.recent_kps        = 1.2f;
    r.normalized_sin    = 0.55f;
    r.current_state     = gw::services::director::IntensityState::SustainedPeak;

    const auto bytes = gw::net::replay::pack_director_request(r);
    REQUIRE(bytes.size() >= 8u);

    gw::net::BitReader              br(std::span<const std::uint8_t>(bytes.data(), bytes.size()));
    gw::services::director::DirectorRequest out{};
    REQUIRE(gw::net::replay::read_director_request(br, out));

    CHECK(out.seed == r.seed);
    CHECK(out.logical_tick == r.logical_tick);
    CHECK(out.player_health_ratio == doctest::Approx(r.player_health_ratio));
    CHECK(out.recent_kps == doctest::Approx(r.recent_kps));
    CHECK(out.normalized_sin == doctest::Approx(r.normalized_sin));
    CHECK(out.current_state == r.current_state);
}

TEST_CASE("gameplay_replay — director evaluate stable after pack round-trip") {
    gw::services::director::DirectorRequest r{};
    r.seed              = 0xC001D00Du;
    r.logical_tick      = 240;
    r.player_health_ratio = 0.5f;
    r.normalized_sin    = 0.25f;
    r.current_state     = gw::services::director::IntensityState::BuildUp;

    const gw::services::director::DirectorResult before =
        gw::services::director::evaluate(r);

    const auto bytes = gw::net::replay::pack_director_request(r);
    gw::net::BitReader br(std::span<const std::uint8_t>(bytes.data(), bytes.size()));
    gw::services::director::DirectorRequest out{};
    REQUIRE(gw::net::replay::read_director_request(br, out));

    const gw::services::director::DirectorResult after =
        gw::services::director::evaluate(out);

    CHECK(after.spawn_rate_scale == doctest::Approx(before.spawn_rate_scale));
    CHECK(after.next_state == before.next_state);
}

TEST_CASE("gameplay_replay — encounter_director_bridge matches evaluate after pack round-trip") {
    gw::services::director::DirectorRequest r{};
    r.seed                  = 0xBEEF123456789ABCULL;
    r.logical_tick          = 501;
    r.player_health_ratio   = 0.62f;
    r.recent_kps            = 0.9f;
    r.normalized_sin        = 0.31f;
    r.current_state         = gw::services::director::IntensityState::Relax;

    const gw::services::director::DirectorResult via_bridge_before =
        gw::gameplay::combat::evaluate_encounter_director(r);
    const gw::services::director::DirectorResult via_service_before =
        gw::services::director::evaluate(r);
    CHECK(via_bridge_before.spawn_rate_scale ==
          doctest::Approx(via_service_before.spawn_rate_scale));
    CHECK(via_bridge_before.next_state == via_service_before.next_state);

    const auto bytes = gw::net::replay::pack_director_request(r);
    gw::net::BitReader br(std::span<const std::uint8_t>(bytes.data(), bytes.size()));
    gw::services::director::DirectorRequest out{};
    REQUIRE(gw::net::replay::read_director_request(br, out));

    const gw::services::director::DirectorResult via_bridge_after =
        gw::gameplay::combat::evaluate_encounter_director(out);
    CHECK(via_bridge_after.spawn_rate_scale ==
          doctest::Approx(via_bridge_before.spawn_rate_scale));
    CHECK(via_bridge_after.next_state == via_bridge_before.next_state);
}
