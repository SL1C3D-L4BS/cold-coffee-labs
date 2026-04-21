// tests/unit/net/reliability_test.cpp — Phase 14 Wave 14D.

#include <doctest/doctest.h>

#include "engine/net/reliability.hpp"
#include "engine/net/transport.hpp"

#include <array>
#include <vector>

using namespace gw::net;

TEST_CASE("ReliabilityChannel monotonically increases sequence per-channel") {
    auto server = make_null_transport();
    REQUIRE(server->listen(30030));
    auto client = make_null_transport();
    const PeerId srv = client->connect("127.0.0.1", 30030);
    ReliabilityChannel rc;
    const std::array<std::uint8_t, 4> bytes{1, 2, 3, 4};
    const auto s1 = rc.send(*client, srv, kChannelReplicationReliable, ReliabilityTier::Reliable,
                            std::span<const std::uint8_t>(bytes), 1);
    const auto s2 = rc.send(*client, srv, kChannelReplicationReliable, ReliabilityTier::Reliable,
                            std::span<const std::uint8_t>(bytes), 2);
    CHECK(s2 == s1 + 1);
    CHECK(rc.stats().reliable_sent == 2);
}

TEST_CASE("Reliable retransmits unacked messages after RTT elapses") {
    auto server = make_null_transport();
    REQUIRE(server->listen(30031));
    auto client = make_null_transport();
    const PeerId srv = client->connect("127.0.0.1", 30031);
    ReliabilityChannel rc;
    const std::array<std::uint8_t, 3> bytes{9, 8, 7};
    const auto seq = rc.send(*client, srv, kChannelReplicationReliable, ReliabilityTier::Reliable,
                             std::span<const std::uint8_t>(bytes), 1);
    CHECK(rc.pending_reliable() == 1);
    // Advance ticks well beyond the RTT; retransmission should fire.
    for (Tick t = 2; t < 100; ++t) rc.tick(*client, srv, t, /*rtt_ticks=*/4, /*retx_mult=*/1.0f);
    CHECK(rc.stats().reliable_retransmitted >= 1);
    rc.on_ack(seq);
    rc.tick(*client, srv, 200, 4, 1.0f);
    CHECK(rc.pending_reliable() == 0);
    CHECK(rc.stats().reliable_acked == 1);
}

TEST_CASE("Unreliable tier drops old sequences") {
    ReliabilityChannel rc;
    CHECK(rc.on_inbound(kChannelReplicationUnreliable, ReliabilityTier::Unreliable, 5));
    CHECK_FALSE(rc.on_inbound(kChannelReplicationUnreliable, ReliabilityTier::Unreliable, 3));
    CHECK(rc.stats().unreliable_dropped_old == 1);
    CHECK(rc.on_inbound(kChannelReplicationUnreliable, ReliabilityTier::Unreliable, 9));
}

TEST_CASE("Sequenced tier drops out-of-order but latest-wins") {
    ReliabilityChannel rc;
    CHECK(rc.on_inbound(kChannelReplicationSequenced, ReliabilityTier::Sequenced, 1));
    CHECK(rc.on_inbound(kChannelReplicationSequenced, ReliabilityTier::Sequenced, 2));
    CHECK_FALSE(rc.on_inbound(kChannelReplicationSequenced, ReliabilityTier::Sequenced, 2));
    CHECK(rc.stats().sequenced_dropped_old == 1);
    CHECK(rc.latest_seen_sequence(kChannelReplicationSequenced) == 2);
}
