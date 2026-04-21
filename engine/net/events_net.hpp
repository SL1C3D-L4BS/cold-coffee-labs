#pragma once
// engine/net/events_net.hpp — Phase 14 (ADR-0047..0055).
//
// Networking events travel through the Phase-11 EventBus. Each struct is
// POD; listeners register with `EventBus<T>::subscribe(...)`.

#include "engine/net/net_types.hpp"

#include <cstdint>
#include <string>

namespace gw::events {

struct PeerConnecting {
    gw::net::PeerId peer{};
    std::string     endpoint{};
};

struct PeerConnected {
    gw::net::PeerId peer{};
    std::string     endpoint{};
    std::uint64_t   session_seed{0};
};

struct PeerDisconnected {
    gw::net::PeerId peer{};
    std::uint32_t   reason_code{0};
    std::string     reason_text{};
};

struct PeerTimedOut {
    gw::net::PeerId peer{};
    std::uint32_t   last_rtt_ms{0};
};

struct PeerKicked {
    gw::net::PeerId peer{};
    std::uint32_t   reason_code{0};
    std::string     reason_text{};
};

struct PacketDropped {
    gw::net::PeerId    peer{};
    gw::net::ChannelId channel{0};
    std::uint32_t      sequence{0};
};

struct SnapshotApplied {
    gw::net::PeerId    peer{};
    gw::net::Tick      tick{0};
    gw::net::BaselineId baseline_id{0};
    std::uint64_t      determinism_hash{0};
    std::uint32_t      entity_delta_count{0};
};

struct DesyncDetected {
    gw::net::PeerId peer{};
    gw::net::Tick   tick{0};
    std::uint64_t   server_hash{0};
    std::uint64_t   peer_hash{0};
};

struct VoiceFrameReceived {
    gw::net::PeerId peer{};
    std::uint32_t   sequence{0};
    std::uint32_t   bytes{0};
};

struct SessionCreated {
    gw::net::SessionHandle handle{};
    std::string            id{};
    std::uint64_t          seed{0};
};

struct SessionJoined {
    gw::net::SessionHandle handle{};
    std::string            id{};
    std::uint64_t          seed{0};
};

struct SessionLeft {
    gw::net::SessionHandle handle{};
    std::string            id{};
};

} // namespace gw::events
