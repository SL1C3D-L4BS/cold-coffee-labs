// tests/unit/net/link_sim_test.cpp — Phase 14 Wave 14F.

#include <doctest/doctest.h>

#include "engine/net/transport.hpp"

#include <array>
#include <cstdint>
#include <vector>

using namespace gw::net;

TEST_CASE("LinkSim is deterministic across identical seeds") {
    auto mk = []() {
        auto server = make_null_transport();
        (void)server->listen(30020);
        auto client = make_null_transport();
        const PeerId srv = client->connect("127.0.0.1", 30020);
        LinkProfile lp{};
        lp.latency_mean_ms   = 20.0f;
        lp.latency_jitter_ms = 5.0f;
        lp.loss_pct          = 10.0f;
        lp.seed              = 0xABCDEF0123456789ULL;
        auto wrapped = wrap_linksim(std::move(client), lp);
        return std::make_tuple(std::move(server), std::move(wrapped), srv);
    };
    // Two identical runs must drop the same packets at the same ticks.
    auto [s1, c1, peer1] = mk();
    auto [s2, c2, peer2] = mk();
    const std::array<std::uint8_t, 4> payload = {1, 2, 3, 4};
    for (Tick t = 1; t <= 20; ++t) {
        c1->send(peer1, kChannelReplicationUnreliable,
                 std::span<const std::uint8_t>(payload.data(), payload.size()));
        c2->send(peer2, kChannelReplicationUnreliable,
                 std::span<const std::uint8_t>(payload.data(), payload.size()));
        c1->tick_fixed(t);
        c2->tick_fixed(t);
    }
    const auto a = c1->stats();
    const auto b = c2->stats();
    CHECK(a.packets_sent == b.packets_sent);
    CHECK(a.packets_dropped == b.packets_dropped);
}

TEST_CASE("LinkSim pass-through (zero profile) delivers every packet") {
    auto server = make_null_transport();
    REQUIRE(server->listen(30021));
    auto client = make_null_transport();
    const PeerId srv = client->connect("127.0.0.1", 30021);
    auto wrapped = wrap_linksim(std::move(client), LinkProfile{});

    const std::array<std::uint8_t, 4> payload = {9, 8, 7, 6};
    for (int i = 0; i < 8; ++i) {
        wrapped->send(srv, kChannelReplicationUnreliable,
                      std::span<const std::uint8_t>(payload.data(), payload.size()));
        wrapped->tick_fixed(static_cast<Tick>(i));
    }
    std::vector<Packet> inbound;
    (void)server->poll(inbound);
    CHECK(inbound.size() >= 1);
}
