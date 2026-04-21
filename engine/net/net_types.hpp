#pragma once
// engine/net/net_types.hpp — Phase 14 Wave 14A (ADR-0047).
//
// Opaque handles + scalar types used by the networking facade. No
// GameNetworkingSockets, Opus, or Steam Audio headers leak through this
// file (non-negotiable #3 + ADR-0047 §2.1).

#include <compare>
#include <cstdint>

namespace gw::net {

// -----------------------------------------------------------------------------
// Primitive wire types.
// -----------------------------------------------------------------------------
using Tick             = std::uint32_t;
using ChannelId        = std::uint8_t;
using BaselineId       = std::uint32_t;
using EntityId         = std::uint64_t; // matches gw::physics::EntityId
inline constexpr EntityId kEntityNone = 0;

inline constexpr ChannelId kChannelSystem              = 0;
inline constexpr ChannelId kChannelReplicationReliable = 1;
inline constexpr ChannelId kChannelReplicationUnreliable = 2;
inline constexpr ChannelId kChannelReplicationSequenced = 3;
inline constexpr ChannelId kChannelVoice               = 4;
inline constexpr ChannelId kChannelRpcReliable         = 5;
inline constexpr ChannelId kChannelMax                 = 32;

// -----------------------------------------------------------------------------
// Opaque handles. An id == 0 handle is null.
// -----------------------------------------------------------------------------
struct PeerId {
    std::uint32_t id{0};
    [[nodiscard]] constexpr bool valid() const noexcept { return id != 0; }
    [[nodiscard]] constexpr auto operator<=>(const PeerId&) const = default;
};

inline constexpr PeerId kPeerNone{0};
inline constexpr PeerId kPeerServer{1}; // reserved slot for server-local routing

struct SessionHandle {
    std::uint64_t value{0};
    [[nodiscard]] constexpr bool valid() const noexcept { return value != 0; }
    [[nodiscard]] constexpr auto operator<=>(const SessionHandle&) const = default;
};

struct EntityReplicationId {
    std::uint64_t value{0};
    [[nodiscard]] constexpr bool valid() const noexcept { return value != 0; }
    [[nodiscard]] constexpr auto operator<=>(const EntityReplicationId&) const = default;

    [[nodiscard]] constexpr EntityId entity() const noexcept {
        return static_cast<EntityId>(value >> 16);
    }
    [[nodiscard]] constexpr std::uint16_t revision() const noexcept {
        return static_cast<std::uint16_t>(value & 0xFFFFu);
    }
};

[[nodiscard]] constexpr EntityReplicationId make_replication_id(EntityId e, std::uint16_t rev) noexcept {
    return EntityReplicationId{(static_cast<std::uint64_t>(e) << 16) | static_cast<std::uint64_t>(rev)};
}

// -----------------------------------------------------------------------------
// Reliability + priority tiers (ADR-0049).
// -----------------------------------------------------------------------------
enum class ReliabilityTier : std::uint8_t {
    Reliable   = 0,
    Unreliable = 1,
    Sequenced  = 2,
};

using PriorityHint = std::uint8_t;
inline constexpr PriorityHint kPriorityCosmetic    = 0;
inline constexpr PriorityHint kPriorityAnimation   = 64;
inline constexpr PriorityHint kPriorityTransform   = 128;
inline constexpr PriorityHint kPriorityAuthoritative = 255;

// -----------------------------------------------------------------------------
// Role / backend.
// -----------------------------------------------------------------------------
enum class Role : std::uint8_t {
    Standalone = 0,
    Server     = 1,
    Client     = 2,
    Linkroom   = 3, // server + clients in one process (sandbox_netplay)
};

enum class BackendKind : std::uint8_t {
    Null = 0,
    Gns  = 1,
};

enum class PeerState : std::uint8_t {
    Disconnected = 0,
    Connecting   = 1,
    Connected    = 2,
    Disconnecting = 3,
    TimedOut     = 4,
    Kicked       = 5,
};

// -----------------------------------------------------------------------------
// Transport statistics (ADR-0047 §2.1).
// -----------------------------------------------------------------------------
struct TransportStats {
    std::uint64_t packets_sent{0};
    std::uint64_t packets_received{0};
    std::uint64_t bytes_sent{0};
    std::uint64_t bytes_received{0};
    std::uint64_t packets_dropped{0};     // by local congestion or LinkSim
    std::uint64_t packets_duplicated{0};  // received duplicates
    std::uint64_t ack_roundtrips{0};      // counted on reliable channel
    float         rtt_ms_mean{0.0f};
    float         rtt_ms_jitter{0.0f};
    std::uint32_t peers_connected{0};
};

// -----------------------------------------------------------------------------
// Limits. Powers of two so fixed buffers don't need modulo.
// -----------------------------------------------------------------------------
inline constexpr std::uint32_t kMaxPeersHard    = 64;
inline constexpr std::uint32_t kPacketBufferMax = 2048; // MTU safety margin
inline constexpr std::uint32_t kBaselineRingMax = 32;

} // namespace gw::net
