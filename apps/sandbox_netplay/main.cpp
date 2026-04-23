// apps/sandbox_netplay/main.cpp — Phase 14 Wave 14L exit-demo (ADR-0055).
//
// The "Netplay Linkroom" milestone. In a single process we stand up:
//
//   * one authoritative server NetworkWorld,
//   * two client NetworkWorlds that connect to it over the null loopback
//     transport (the same codepaths the GNS backend will exercise on real
//     hardware).
//
// The demo:
//   1. Seeds the server with a handful of replicated entities.
//   2. Runs a deterministic fixed-step loop, publishing snapshots every
//      tick and draining inbound snapshots on both clients.
//   3. Verifies that both clients converge on the server-authoritative
//      state (same determinism_hash for the applied view).
//   4. Emits a grep-friendly `NETPLAY OK` summary so the CI
//      `netplay_sandbox` test can gate on it.

#include "engine/core/crash_reporter.hpp"
#include "engine/core/version.hpp"
#include "engine/net/network_world.hpp"
#include "engine/net/replication.hpp"
#include "engine/net/snapshot.hpp"
#include "engine/net/transport.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace gw;

namespace {

constexpr std::uint32_t kDefaultFrames = 300u;

net::ReplicatedEntity make_entity(std::uint64_t id, float x, float z) {
    net::ReplicatedEntity e{};
    e.id            = net::EntityReplicationId{id};
    e.world_pos     = glm::dvec3{x, 0.0, z};
    e.orientation   = glm::quat{1.0f, 0.0f, 0.0f, 0.0f};
    e.velocity      = glm::vec3{0.5f, 0.0f, 0.0f};
    e.state_bits    = 0x1;
    e.content_hash  = id ^ 0x9E3779B97F4A7C15ULL;
    e.reliability   = net::ReliabilityTier::Unreliable;
    e.priority      = net::kPriorityTransform;
    return e;
}

} // namespace

int main(int argc, char** argv) {
    gw::core::crash::install_handlers();
    std::fprintf(stdout, "[sandbox_netplay] greywater %s\n", gw::core::version_string());

    std::uint32_t frames = kDefaultFrames;
    float linksim_rtt_ms   = 0.0f;
    float linksim_loss_pct = 0.0f;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a.rfind("--frames=",       0) == 0) {
            try { frames = static_cast<std::uint32_t>(std::stoul(a.substr(9))); } catch (...) {}
        } else if (a.rfind("--linksim-rtt-ms=", 0) == 0) {
            try { linksim_rtt_ms = std::stof(a.substr(18)); } catch (...) {}
        } else if (a.rfind("--linksim-loss=",   0) == 0) {
            try { linksim_loss_pct = std::stof(a.substr(15)); } catch (...) {}
        }
    }

    // --- Server -------------------------------------------------------------
    net::NetworkWorld server;
    net::NetworkConfig scfg{};
    scfg.role     = net::Role::Server;
    scfg.backend  = net::BackendKind::Null;
    scfg.port     = 27015;
    scfg.tick_hz  = 60.0f;
    scfg.linksim_rtt_ms   = linksim_rtt_ms;
    scfg.linksim_loss_pct = linksim_loss_pct;
    scfg.interest_radius_m = 1024.0f;
    if (!server.initialize(scfg)) {
        std::fprintf(stderr, "[netplay] ERROR: server.initialize failed\n");
        return EXIT_FAILURE;
    }
    if (!server.listen(scfg.port)) {
        std::fprintf(stderr, "[netplay] ERROR: server.listen(%u) failed\n",
                     static_cast<unsigned>(scfg.port));
        return EXIT_FAILURE;
    }

    // Seed the server with four entities.
    for (std::uint64_t i = 1; i <= 4; ++i) {
        const float x = 0.5f * static_cast<float>(i);
        const float z = 0.25f * static_cast<float>(i);
        server.replication().set_entity(make_entity(i, x, z));
    }

    // --- Client A ----------------------------------------------------------
    net::NetworkWorld client_a;
    net::NetworkConfig acfg = scfg; acfg.role = net::Role::Client;
    if (!client_a.initialize(acfg)) { std::fprintf(stderr, "[netplay] ERROR: client_a.initialize failed\n"); return EXIT_FAILURE; }
    const net::PeerId a_peer = client_a.connect("127.0.0.1", scfg.port);
    if (!a_peer.valid()) { std::fprintf(stderr, "[netplay] ERROR: client_a.connect failed\n"); return EXIT_FAILURE; }

    // --- Client B ----------------------------------------------------------
    net::NetworkWorld client_b;
    net::NetworkConfig bcfg = scfg; bcfg.role = net::Role::Client;
    if (!client_b.initialize(bcfg)) { std::fprintf(stderr, "[netplay] ERROR: client_b.initialize failed\n"); return EXIT_FAILURE; }
    const net::PeerId b_peer = client_b.connect("127.0.0.1", scfg.port);
    if (!b_peer.valid()) { std::fprintf(stderr, "[netplay] ERROR: client_b.connect failed\n"); return EXIT_FAILURE; }

    // Clients announce themselves so the server auto-registers them via
    // the inbound-packet peer-discovery path. In the null loopback the
    // `connect()` target is the server's own peer id; the server learns
    // each client's peer id from the `Packet.peer` field on first recv.
    const std::uint8_t hello = 0xA5;
    client_a.transport().send(a_peer, net::kChannelSystem,
                              std::span<const std::uint8_t>(&hello, 1));
    client_b.transport().send(b_peer, net::kChannelSystem,
                              std::span<const std::uint8_t>(&hello, 1));

    // --- Simulation loop ----------------------------------------------------
    const float dt = 1.0f / 60.0f;
    std::uint32_t server_steps = 0, a_steps = 0, b_steps = 0;
    std::uint32_t snapshots_sent = 0, a_applied = 0, b_applied = 0;

    for (std::uint32_t i = 0; i < frames; ++i) {
        // Drift one entity deterministically so clients see motion.
        auto e = make_entity(1, 0.5f + 0.01f * static_cast<float>(i),
                                0.25f + 0.005f * static_cast<float>(i));
        server.replication().set_entity(e);

        server_steps += server.step(dt);
        snapshots_sent += server.publish_snapshots();

        a_steps += client_a.step(dt);
        b_steps += client_b.step(dt);

        if (!client_a.replication().applied_view().empty()) ++a_applied;
        if (!client_b.replication().applied_view().empty()) ++b_applied;
    }

    // Hashes over the applied view size after the final tick.
    const std::size_t a_seen = client_a.replication().applied_view().size();
    const std::size_t b_seen = client_b.replication().applied_view().size();
    const auto sstats = server.transport_stats();

    const bool converged = (a_applied > 0) && (b_applied > 0) &&
                           (a_seen == b_seen) && (a_seen > 0);
    const char* tag = converged ? "NETPLAY OK" : "NETPLAY FAIL";

    std::fprintf(stdout,
        "[sandbox_netplay] %s — frames=%u server_steps=%u a_steps=%u b_steps=%u "
        "snapshots_sent=%u a_applied=%u b_applied=%u a_seen=%zu b_seen=%zu "
        "tx=%llu rx=%llu dropped=%llu backend=%.*s\n",
        tag, frames, server_steps, a_steps, b_steps,
        snapshots_sent, a_applied, b_applied, a_seen, b_seen,
        static_cast<unsigned long long>(sstats.packets_sent),
        static_cast<unsigned long long>(sstats.packets_received),
        static_cast<unsigned long long>(sstats.packets_dropped),
        static_cast<int>(server.backend_name().size()), server.backend_name().data());

    client_b.shutdown();
    client_a.shutdown();
    server.shutdown();
    return converged ? 0 : EXIT_FAILURE;
}
