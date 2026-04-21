// tests/unit/net/transport_null_test.cpp — Phase 14 Wave 14A.

#include <doctest/doctest.h>

#include "engine/net/transport.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

using namespace gw::net;

TEST_CASE("null transport can listen and accept a loopback connection") {
    auto server = make_null_transport();
    REQUIRE(server);
    REQUIRE(server->listen(30010));
    CHECK(std::string{server->backend_name()} == "null");

    auto client = make_null_transport();
    REQUIRE(client);
    const PeerId target = client->connect("127.0.0.1", 30010);
    REQUIRE(target.valid());
    CHECK(client->peer_state(target) == PeerState::Connected);
}

TEST_CASE("null transport delivers packets round-trip") {
    auto server = make_null_transport();
    REQUIRE(server->listen(30011));
    auto client = make_null_transport();
    const PeerId srv = client->connect("127.0.0.1", 30011);
    REQUIRE(srv.valid());

    const std::array<std::uint8_t, 5> payload = {0xDE, 0xAD, 0xBE, 0xEF, 0x42};
    client->send(srv, kChannelReplicationUnreliable,
                 std::span<const std::uint8_t>(payload.data(), payload.size()));

    std::vector<Packet> inbound;
    const auto n = server->poll(inbound);
    CHECK(n == 1);
    REQUIRE(inbound.size() == 1);
    CHECK(inbound.front().channel == kChannelReplicationUnreliable);
    CHECK(inbound.front().bytes.size() == payload.size());
    for (std::size_t i = 0; i < payload.size(); ++i) {
        CHECK(inbound.front().bytes[i] == payload[i]);
    }
}

TEST_CASE("null transport rejects packets larger than kPacketBufferMax") {
    auto server = make_null_transport();
    REQUIRE(server->listen(30012));
    auto client = make_null_transport();
    const PeerId srv = client->connect("127.0.0.1", 30012);
    REQUIRE(srv.valid());

    std::vector<std::uint8_t> huge(kPacketBufferMax + 10, 0);
    client->send(srv, kChannelReplicationUnreliable,
                 std::span<const std::uint8_t>(huge.data(), huge.size()));

    std::vector<Packet> inbound;
    const auto n = server->poll(inbound);
    CHECK(n == 0); // dropped at the transport.
}

TEST_CASE("null transport connect fails on unknown host") {
    auto client = make_null_transport();
    const PeerId p = client->connect("203.0.113.1", 30013);
    CHECK_FALSE(p.valid());
}
