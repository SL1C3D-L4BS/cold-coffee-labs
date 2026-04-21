// engine/net/network_world.cpp — Phase 14 Wave 14A..14L (ADR-0047..0055).

#include "engine/net/network_world.hpp"

#include "engine/net/desync_detector.hpp"
#include "engine/net/lag_comp.hpp"
#include "engine/net/reliability.hpp"
#include "engine/net/session.hpp"
#include "engine/net/snapshot.hpp"
#include "engine/net/voice.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace gw::net {

struct NetworkWorld::Impl {
    NetworkConfig                       cfg{};
    events::CrossSubsystemBus*          xbus{nullptr};
    jobs::Scheduler*                    sched{nullptr};
    bool                                initialized{false};
    BackendKind                         backend{BackendKind::Null};

    std::unique_ptr<ITransport>         transport{};
    ReplicationSystem                   replication{};
    ReliabilityChannel                  reliability{};
    VoicePipeline                       voice{VoiceConfig{}};
    DesyncDetector                      desync{};
    ServerRewind                        rewind{};

    Tick                                tick{0};
    std::uint64_t                       frame_index{0};
    float                               fixed_accum_s{0.0f};
    float                               fixed_dt_s{1.0f / 60.0f};

    events::EventBus<events::PeerConnecting>      ev_peer_connecting{};
    events::EventBus<events::PeerConnected>       ev_peer_connected{};
    events::EventBus<events::PeerDisconnected>    ev_peer_disconnected{};
    events::EventBus<events::PeerTimedOut>        ev_peer_timed_out{};
    events::EventBus<events::PacketDropped>       ev_packet_dropped{};
    events::EventBus<events::SnapshotApplied>     ev_snapshot_applied{};
    events::EventBus<events::DesyncDetected>      ev_desync_detected{};
    events::EventBus<events::VoiceFrameReceived>  ev_voice_received{};

    std::vector<Packet> scratch_in{};
};

NetworkWorld::NetworkWorld() : impl_{std::make_unique<Impl>()} {}
NetworkWorld::~NetworkWorld() { shutdown(); }

bool NetworkWorld::initialize(const NetworkConfig& cfg,
                              events::CrossSubsystemBus* bus,
                              jobs::Scheduler* scheduler) {
    if (impl_->initialized) return false;
    impl_->cfg         = cfg;
    impl_->xbus        = bus;
    impl_->sched       = scheduler;
    impl_->backend     = cfg.backend;
    impl_->fixed_dt_s  = (cfg.tick_hz > 0.0f) ? (1.0f / cfg.tick_hz) : (1.0f / 60.0f);

    // Bring up transport.
    std::unique_ptr<ITransport> raw;
    if (cfg.backend == BackendKind::Gns) {
        raw = make_gns_transport();
        if (!raw) {
            impl_->backend = BackendKind::Null;
            raw = make_null_transport();
        }
    } else {
        raw = make_null_transport();
    }
    if (!raw) return false;

    // Wrap with LinkSim if the profile is non-zero.
    const bool simulate = cfg.linksim_rtt_ms   > 0.0f ||
                          cfg.linksim_loss_pct > 0.0f ||
                          cfg.linksim_jitter_ms > 0.0f;
    if (simulate) {
        LinkProfile lp{};
        lp.latency_mean_ms   = cfg.linksim_rtt_ms * 0.5f;
        lp.latency_jitter_ms = cfg.linksim_jitter_ms;
        lp.loss_pct          = cfg.linksim_loss_pct;
        lp.seed              = cfg.linksim_seed;
        raw = wrap_linksim(std::move(raw), lp);
    }
    impl_->transport = std::move(raw);

    // Replication configuration from net config.
    impl_->replication.set_default_interest_radius_m(cfg.interest_radius_m);
    impl_->replication.set_baseline_ring(cfg.baseline_ring);
    const float per_peer_budget_bytes = (cfg.send_budget_kbps * 1000.0f / 8.0f) /
                                        std::max(1.0f, cfg.tick_hz);
    impl_->replication.set_send_budget_bytes_per_tick(
        static_cast<std::uint32_t>(std::max(64.0f, per_peer_budget_bytes)));
    impl_->replication.set_starvation_frames(cfg.starvation_frames);

    // Voice.
    VoiceConfig vc{};
    vc.bitrate_kbps = cfg.voice_bitrate_kbps;
    vc.fec          = cfg.voice_fec_enable;
    vc.push_to_talk = cfg.voice_push_to_talk;
    vc.max_peers    = cfg.max_peers;
    impl_->voice = VoicePipeline{vc};
    impl_->voice.set_mute_mask(cfg.voice_mute_mask);

    // Desync detector.
    impl_->desync.set_window(cfg.desync_window_ticks);
    impl_->desync.set_auto_dump(cfg.desync_auto_dump);

    // Server rewind ring.
    impl_->rewind.set_capacity(cfg.lagcomp_window_frames);

    impl_->initialized = true;
    return true;
}

void NetworkWorld::shutdown() {
    if (!impl_ || !impl_->initialized) return;
    impl_->transport.reset();
    impl_->replication = ReplicationSystem{};
    impl_->voice = VoicePipeline{VoiceConfig{}};
    impl_->desync.clear();
    impl_->rewind.clear();
    impl_->tick = 0;
    impl_->frame_index = 0;
    impl_->fixed_accum_s = 0.0f;
    impl_->initialized = false;
}

bool NetworkWorld::initialized() const noexcept { return impl_ && impl_->initialized; }
BackendKind NetworkWorld::backend() const noexcept { return impl_ ? impl_->backend : BackendKind::Null; }
std::string_view NetworkWorld::backend_name() const noexcept {
    if (!impl_ || !impl_->transport) return std::string_view{"uninitialized"};
    return impl_->transport->backend_name();
}
const NetworkConfig& NetworkWorld::config() const noexcept {
    return impl_->cfg;
}

bool NetworkWorld::listen(std::uint16_t port) {
    if (!initialized()) return false;
    return impl_->transport->listen(port);
}
PeerId NetworkWorld::connect(std::string_view host, std::uint16_t port) {
    if (!initialized()) return kPeerNone;
    const PeerId p = impl_->transport->connect(host, port);
    if (p.valid()) {
        impl_->replication.add_peer(p, glm::dvec3{0.0, 0.0, 0.0});
        events::PeerConnecting e{};
        e.peer = p;
        e.endpoint = std::string{host};
        impl_->ev_peer_connecting.publish(e);
    }
    return p;
}
void NetworkWorld::disconnect(PeerId peer) noexcept {
    if (!initialized() || !peer.valid()) return;
    impl_->transport->disconnect(peer);
    impl_->replication.remove_peer(peer);
    events::PeerDisconnected e{};
    e.peer = peer;
    impl_->ev_peer_disconnected.publish(e);
}
PeerState NetworkWorld::peer_state(PeerId peer) const noexcept {
    if (!initialized()) return PeerState::Disconnected;
    return impl_->transport->peer_state(peer);
}
std::uint32_t NetworkWorld::peer_count() const noexcept {
    if (!initialized()) return 0;
    return impl_->transport->stats().peers_connected;
}

std::uint32_t NetworkWorld::step(float delta_time_s) {
    if (!initialized()) return 0;
    impl_->fixed_accum_s += delta_time_s;
    std::uint32_t steps = 0;
    const std::uint32_t max_steps = impl_->cfg.max_substeps;
    while (impl_->fixed_accum_s >= impl_->fixed_dt_s && steps < max_steps) {
        step_fixed();
        impl_->fixed_accum_s -= impl_->fixed_dt_s;
        ++steps;
    }
    if (impl_->fixed_accum_s > impl_->fixed_dt_s * static_cast<float>(max_steps)) {
        impl_->fixed_accum_s = 0.0f; // catch-up clamp
    }
    return steps;
}

void NetworkWorld::step_fixed() {
    if (!initialized()) return;
    impl_->transport->tick_fixed(impl_->tick);

    impl_->scratch_in.clear();
    (void)impl_->transport->poll(impl_->scratch_in);
    for (const auto& pkt : impl_->scratch_in) {
        // Auto-discover peers: any inbound packet from an unknown peer
        // registers that peer with the replication system. This is the
        // analogue of a GNS `AcceptConnection` callback for the null
        // loopback transport.
        if (pkt.peer.valid() && !impl_->replication.peer_state(pkt.peer)) {
            impl_->replication.add_peer(pkt.peer, glm::dvec3{0.0, 0.0, 0.0});
            events::PeerConnected ev{};
            ev.peer = pkt.peer;
            ev.session_seed = impl_->cfg.world_seed;
            impl_->ev_peer_connected.publish(ev);
        }
        switch (pkt.channel) {
        case kChannelVoice: {
            if (impl_->voice.on_inbound(pkt.peer, pkt.sequence,
                                        std::span<const std::uint8_t>(pkt.bytes))) {
                events::VoiceFrameReceived vf{};
                vf.peer = pkt.peer;
                vf.sequence = pkt.sequence;
                vf.bytes = static_cast<std::uint32_t>(pkt.bytes.size());
                impl_->ev_voice_received.publish(vf);
            }
            break;
        }
        case kChannelReplicationUnreliable:
        case kChannelReplicationSequenced:
        case kChannelReplicationReliable: {
            Snapshot s{};
            if (deserialize_snapshot(std::span<const std::uint8_t>(pkt.bytes), s)) {
                if (impl_->replication.apply_snapshot(s)) {
                    events::SnapshotApplied ev{};
                    ev.peer = pkt.peer;
                    ev.tick = s.tick;
                    ev.baseline_id = s.baseline_id;
                    ev.determinism_hash = s.determinism_hash;
                    ev.entity_delta_count = static_cast<std::uint32_t>(s.deltas.size());
                    impl_->ev_snapshot_applied.publish(ev);
                }
            }
            break;
        }
        default:
            break;
        }
    }

    ++impl_->tick;
    ++impl_->frame_index;
}

Tick NetworkWorld::tick() const noexcept { return impl_ ? impl_->tick : 0; }
std::uint64_t NetworkWorld::frame_index() const noexcept { return impl_ ? impl_->frame_index : 0; }

ReplicationSystem& NetworkWorld::replication() noexcept { return impl_->replication; }
const ReplicationSystem& NetworkWorld::replication() const noexcept { return impl_->replication; }
ReliabilityChannel& NetworkWorld::reliability() noexcept { return impl_->reliability; }
VoicePipeline&      NetworkWorld::voice()       noexcept { return impl_->voice; }
DesyncDetector&     NetworkWorld::desync_detector() noexcept { return impl_->desync; }
ServerRewind&       NetworkWorld::server_rewind()  noexcept { return impl_->rewind; }
ITransport&         NetworkWorld::transport()      noexcept { return *impl_->transport; }

void NetworkWorld::send_reliable(PeerId peer, std::span<const std::uint8_t> bytes) {
    if (!initialized() || !peer.valid()) return;
    impl_->reliability.send(*impl_->transport, peer, kChannelReplicationReliable,
                             ReliabilityTier::Reliable, bytes, impl_->tick);
}
void NetworkWorld::send_unreliable(PeerId peer, std::span<const std::uint8_t> bytes) {
    if (!initialized() || !peer.valid()) return;
    impl_->reliability.send(*impl_->transport, peer, kChannelReplicationUnreliable,
                             ReliabilityTier::Unreliable, bytes, impl_->tick);
}

std::uint32_t NetworkWorld::publish_snapshots() {
    if (!initialized()) return 0;
    std::uint32_t sent = 0;
    // Walk every tracked peer in the replication system; schedule a
    // snapshot build per peer. Deterministic iteration via sorted keys.
    std::vector<PeerId> peers;
    peers.reserve(16);
    impl_->replication.list_peers(peers);
    for (const PeerId& peer : peers) {
        Snapshot s = impl_->replication.build_snapshot_for(peer, impl_->tick);
        if (s.deltas.empty()) continue;
        const auto bytes = serialize_snapshot(s);
        impl_->reliability.send(*impl_->transport, peer, kChannelReplicationUnreliable,
                                 ReliabilityTier::Unreliable,
                                 std::span<const std::uint8_t>(bytes), impl_->tick);
        ++sent;
    }
    return sent;
}

TransportStats NetworkWorld::transport_stats() const noexcept {
    if (!initialized()) return TransportStats{};
    return impl_->transport->stats();
}

events::EventBus<events::PeerConnecting>&   NetworkWorld::bus_peer_connecting()   noexcept { return impl_->ev_peer_connecting; }
events::EventBus<events::PeerConnected>&    NetworkWorld::bus_peer_connected()    noexcept { return impl_->ev_peer_connected; }
events::EventBus<events::PeerDisconnected>& NetworkWorld::bus_peer_disconnected() noexcept { return impl_->ev_peer_disconnected; }
events::EventBus<events::PeerTimedOut>&     NetworkWorld::bus_peer_timed_out()    noexcept { return impl_->ev_peer_timed_out; }
events::EventBus<events::PacketDropped>&    NetworkWorld::bus_packet_dropped()    noexcept { return impl_->ev_packet_dropped; }
events::EventBus<events::SnapshotApplied>&  NetworkWorld::bus_snapshot_applied()  noexcept { return impl_->ev_snapshot_applied; }
events::EventBus<events::DesyncDetected>&   NetworkWorld::bus_desync_detected()   noexcept { return impl_->ev_desync_detected; }
events::EventBus<events::VoiceFrameReceived>& NetworkWorld::bus_voice_received()  noexcept { return impl_->ev_voice_received; }

} // namespace gw::net
