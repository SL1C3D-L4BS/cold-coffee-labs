#pragma once
// engine/net/network_world.hpp — Phase 14 Wave 14A (ADR-0047).
//
// PIMPL facade that orchestrates transport, replication, reliability,
// voice, session, lockstep, and the netcode harness. No third-party
// headers (GameNetworkingSockets, Opus, Steam Audio, Steamworks, EOS)
// are reachable through this file. Mirrors the PhysicsWorld/AnimationWorld
// pattern (ADR-0031, ADR-0042).

#include "engine/net/events_net.hpp"
#include "engine/net/net_config.hpp"
#include "engine/net/net_types.hpp"
#include "engine/net/replication.hpp"
#include "engine/net/transport.hpp"

#include "engine/core/events/event_bus.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace gw::events { class CrossSubsystemBus; }
namespace gw::jobs   { class Scheduler; }

namespace gw::net {

class ReliabilityChannel;
class VoicePipeline;
class DesyncDetector;
class ServerRewind;
class LockstepSession;
class ISessionProvider;

class NetworkWorld {
public:
    NetworkWorld();
    ~NetworkWorld();

    NetworkWorld(const NetworkWorld&)            = delete;
    NetworkWorld& operator=(const NetworkWorld&) = delete;
    NetworkWorld(NetworkWorld&&)                 = delete;
    NetworkWorld& operator=(NetworkWorld&&)      = delete;

    // Lifecycle ---------------------------------------------------------------
    [[nodiscard]] bool initialize(const NetworkConfig& cfg,
                                  events::CrossSubsystemBus* bus = nullptr,
                                  jobs::Scheduler* scheduler = nullptr);
    void shutdown();

    [[nodiscard]] bool initialized() const noexcept;
    [[nodiscard]] BackendKind backend() const noexcept;
    [[nodiscard]] std::string_view backend_name() const noexcept;
    [[nodiscard]] const NetworkConfig& config() const noexcept;

    // Connection --------------------------------------------------------------
    [[nodiscard]] bool listen(std::uint16_t port);
    [[nodiscard]] PeerId connect(std::string_view host, std::uint16_t port);
    void disconnect(PeerId peer) noexcept;
    [[nodiscard]] PeerState peer_state(PeerId) const noexcept;
    [[nodiscard]] std::uint32_t peer_count() const noexcept;

    // Simulation --------------------------------------------------------------
    [[nodiscard]] std::uint32_t step(float delta_time_s);
    void step_fixed();
    [[nodiscard]] Tick tick() const noexcept;
    [[nodiscard]] std::uint64_t frame_index() const noexcept;

    // Sub-facet accessors -----------------------------------------------------
    [[nodiscard]] ReplicationSystem&       replication()       noexcept;
    [[nodiscard]] const ReplicationSystem& replication() const noexcept;
    [[nodiscard]] ReliabilityChannel&      reliability()       noexcept;
    [[nodiscard]] VoicePipeline&           voice()             noexcept;
    [[nodiscard]] DesyncDetector&          desync_detector()   noexcept;
    [[nodiscard]] ServerRewind&            server_rewind()     noexcept;
    [[nodiscard]] ITransport&              transport()         noexcept;

    // Convenience send helpers ------------------------------------------------
    void send_reliable(PeerId peer, std::span<const std::uint8_t> bytes);
    void send_unreliable(PeerId peer, std::span<const std::uint8_t> bytes);

    // Replication publishing: build + send snapshots for every known peer.
    std::uint32_t publish_snapshots();

    // Stats -------------------------------------------------------------------
    [[nodiscard]] TransportStats transport_stats() const noexcept;

    // Event buses (owned) -----------------------------------------------------
    [[nodiscard]] events::EventBus<events::PeerConnecting>&   bus_peer_connecting()   noexcept;
    [[nodiscard]] events::EventBus<events::PeerConnected>&    bus_peer_connected()    noexcept;
    [[nodiscard]] events::EventBus<events::PeerDisconnected>& bus_peer_disconnected() noexcept;
    [[nodiscard]] events::EventBus<events::PeerTimedOut>&     bus_peer_timed_out()    noexcept;
    [[nodiscard]] events::EventBus<events::PacketDropped>&    bus_packet_dropped()    noexcept;
    [[nodiscard]] events::EventBus<events::SnapshotApplied>&  bus_snapshot_applied()  noexcept;
    [[nodiscard]] events::EventBus<events::DesyncDetected>&   bus_desync_detected()   noexcept;
    [[nodiscard]] events::EventBus<events::VoiceFrameReceived>& bus_voice_received()  noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gw::net
