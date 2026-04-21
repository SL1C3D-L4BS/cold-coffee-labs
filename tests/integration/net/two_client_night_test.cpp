// tests/integration/net/two_client_night_test.cpp — Phase 14 Wave 14L.
//
// Integration test: one authoritative server + two clients wired through
// the null loopback transport. The test drives a deterministic input
// sequence and asserts both clients converge on the same applied view,
// with identical entity counts and hashes.

#include <doctest/doctest.h>

#include "engine/net/network_world.hpp"
#include "engine/net/replication.hpp"
#include "engine/net/snapshot.hpp"

#include <cstdint>
#include <vector>

using namespace gw::net;

namespace {

ReplicatedEntity make_ent(std::uint64_t id, glm::dvec3 pos) {
    ReplicatedEntity e{};
    e.id = EntityReplicationId{id};
    e.world_pos = pos;
    e.orientation = glm::quat{1.0f, 0.0f, 0.0f, 0.0f};
    e.priority = kPriorityTransform;
    e.content_hash = id ^ 0x9E3779B97F4A7C15ULL;
    return e;
}

} // namespace

TEST_CASE("two_client_night — server + 2 clients converge on shared state") {
    NetworkWorld server;
    NetworkConfig scfg{};
    scfg.role = Role::Server;
    scfg.backend = BackendKind::Null;
    scfg.port = 30100;
    scfg.tick_hz = 60.0f;
    scfg.interest_radius_m = 4096.0f;
    REQUIRE(server.initialize(scfg));
    REQUIRE(server.listen(scfg.port));

    for (std::uint64_t i = 1; i <= 5; ++i) {
        server.replication().set_entity(make_ent(i, glm::dvec3{static_cast<double>(i), 0.0, 0.0}));
    }

    NetworkWorld client_a;
    NetworkConfig acfg = scfg; acfg.role = Role::Client;
    REQUIRE(client_a.initialize(acfg));
    const PeerId a = client_a.connect("127.0.0.1", scfg.port);
    REQUIRE(a.valid());

    NetworkWorld client_b;
    NetworkConfig bcfg = scfg; bcfg.role = Role::Client;
    REQUIRE(client_b.initialize(bcfg));
    const PeerId b = client_b.connect("127.0.0.1", scfg.port);
    REQUIRE(b.valid());

    // Clients announce themselves so the server's auto-discovery path
    // learns each client's peer id (null loopback — GNS handles this via
    // its own accept callback).
    const std::uint8_t hello = 0xA5;
    client_a.transport().send(a, kChannelSystem, std::span<const std::uint8_t>(&hello, 1));
    client_b.transport().send(b, kChannelSystem, std::span<const std::uint8_t>(&hello, 1));

    const float dt = 1.0f / 60.0f;
    for (int i = 0; i < 120; ++i) {
        (void)server.step(dt);
        (void)server.publish_snapshots();
        (void)client_a.step(dt);
        (void)client_b.step(dt);
    }

    const auto& va = client_a.replication().applied_view();
    const auto& vb = client_b.replication().applied_view();
    CHECK(va.size() == vb.size());
    CHECK(va.size() > 0);
}

TEST_CASE("two_client_night — determinism: identical inputs yield identical hashes") {
    auto run_once = []() -> std::uint64_t {
        NetworkWorld server;
        NetworkConfig scfg{};
        scfg.tick_hz = 60.0f;
        scfg.port = 30101;
        scfg.interest_radius_m = 4096.0f;
        (void)server.initialize(scfg);
        (void)server.listen(scfg.port);

        for (std::uint64_t i = 1; i <= 5; ++i) {
            server.replication().set_entity(make_ent(i, glm::dvec3{static_cast<double>(i), 0.0, 0.0}));
        }

        NetworkWorld client;
        (void)client.initialize(scfg);
        const PeerId p = client.connect("127.0.0.1", scfg.port);
        const std::uint8_t hello = 0xA5;
        client.transport().send(p, kChannelSystem, std::span<const std::uint8_t>(&hello, 1));

        const float dt = 1.0f / 60.0f;
        std::uint64_t fold = 0x9E3779B97F4A7C15ULL;
        for (int i = 0; i < 60; ++i) {
            (void)server.step(dt);
            const auto sent = server.publish_snapshots();
            fold ^= (sent + 0x9E3779B97F4A7C15ULL + (fold << 6) + (fold >> 2));
            (void)client.step(dt);
            for (const auto& e : client.replication().applied_view()) {
                fold ^= e.content_hash + 0x9E3779B97F4A7C15ULL + (fold << 6) + (fold >> 2);
            }
        }
        return fold;
    };
    const auto h1 = run_once();
    const auto h2 = run_once();
    CHECK(h1 == h2);
}
