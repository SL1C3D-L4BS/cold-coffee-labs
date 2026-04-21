#pragma once
// engine/net/reliability.hpp — Phase 14 Wave 14D (ADR-0049).
//
// Per-peer-per-channel reliability state. Reliable channel: 32-bit
// sequence + 32-bit ack bitmap + retransmit ring. Unreliable: latest-wins.
// Sequenced: latest-wins with out-of-order drop.
//
// The reliability layer is a thin shim above `ITransport`; it owns the
// ack bookkeeping and the retransmit queue but not the underlying socket.

#include "engine/net/net_types.hpp"
#include "engine/net/transport.hpp"

#include <cstdint>
#include <cstddef>
#include <deque>
#include <span>
#include <unordered_map>
#include <vector>

namespace gw::net {

struct ReliabilityStats {
    std::uint64_t reliable_sent{0};
    std::uint64_t reliable_acked{0};
    std::uint64_t reliable_retransmitted{0};
    std::uint64_t unreliable_dropped_old{0};
    std::uint64_t sequenced_dropped_old{0};
};

struct PendingReliable {
    std::uint32_t sequence{0};
    Tick          sent_tick{0};
    Tick          next_retx_tick{0};
    std::vector<std::uint8_t> bytes{};
    bool          acked{false};
};

class ReliabilityChannel {
public:
    // Send on this channel. Returns the sequence used. The logical
    // reliability tier is chosen by the caller per-channel (typically
    // kChannelReplicationReliable == Reliable, kChannelReplicationUnreliable
    // == Unreliable, kChannelReplicationSequenced == Sequenced).
    std::uint32_t send(ITransport& t, PeerId peer, ChannelId channel,
                       ReliabilityTier tier,
                       std::span<const std::uint8_t> bytes, Tick now);

    // Demux an inbound packet by tier. Returns true if payload should be
    // delivered upstream (new, not duplicate / not stale).
    bool on_inbound(ChannelId channel, ReliabilityTier tier, std::uint32_t sequence);

    // Tick: retransmit reliable messages whose timer expired, given the
    // estimated RTT in ticks.
    void tick(ITransport& t, PeerId peer, Tick now, std::uint32_t rtt_ticks, float retx_mult);

    // Called when the remote side informs us of the latest acked sequence.
    void on_ack(std::uint32_t highest_acked_sequence);

    [[nodiscard]] const ReliabilityStats& stats() const noexcept { return stats_; }
    [[nodiscard]] std::size_t pending_reliable() const noexcept { return pending_.size(); }
    [[nodiscard]] std::uint32_t latest_seen_sequence(ChannelId ch) const noexcept {
        auto it = latest_seen_.find(ch);
        return (it == latest_seen_.end()) ? 0u : it->second;
    }
    [[nodiscard]] std::uint32_t highest_acked() const noexcept { return highest_acked_; }

private:
    std::unordered_map<ChannelId, std::uint32_t> out_seq_{};
    std::unordered_map<ChannelId, std::uint32_t> latest_seen_{};
    std::deque<PendingReliable>                  pending_{};
    ChannelId                                    reliable_channel_{kChannelReplicationReliable};
    std::uint32_t                                highest_acked_{0};
    ReliabilityStats                             stats_{};
};

} // namespace gw::net
