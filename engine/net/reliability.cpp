// engine/net/reliability.cpp — Phase 14 Wave 14D (ADR-0049).

#include "engine/net/reliability.hpp"

#include <algorithm>

namespace gw::net {

std::uint32_t ReliabilityChannel::send(ITransport& t, PeerId peer, ChannelId channel,
                                       ReliabilityTier tier,
                                       std::span<const std::uint8_t> bytes, Tick now) {
    std::uint32_t& seq = out_seq_[channel];
    seq += 1;
    const std::uint32_t mine = seq;
    t.send(peer, channel, bytes);
    if (tier == ReliabilityTier::Reliable) {
        reliable_channel_ = channel;
        PendingReliable pr{};
        pr.sequence       = mine;
        pr.sent_tick      = now;
        pr.next_retx_tick = now + 8; // bootstrap; updated on tick()
        pr.bytes.assign(bytes.begin(), bytes.end());
        pending_.push_back(std::move(pr));
        stats_.reliable_sent += 1;
    }
    return mine;
}

bool ReliabilityChannel::on_inbound(ChannelId channel, ReliabilityTier tier, std::uint32_t sequence) {
    auto& latest = latest_seen_[channel];
    if (tier == ReliabilityTier::Reliable) {
        if (sequence <= latest) return false; // duplicate.
        latest = sequence;
        return true;
    }
    if (tier == ReliabilityTier::Unreliable) {
        if (sequence <= latest) {
            stats_.unreliable_dropped_old += 1;
            return false;
        }
        latest = sequence;
        return true;
    }
    // Sequenced.
    if (sequence <= latest) {
        stats_.sequenced_dropped_old += 1;
        return false;
    }
    latest = sequence;
    return true;
}

void ReliabilityChannel::tick(ITransport& t, PeerId peer, Tick now, std::uint32_t rtt_ticks, float retx_mult) {
    if (rtt_ticks == 0) rtt_ticks = 4;
    const std::uint32_t retx_interval = static_cast<std::uint32_t>(
        std::max(1.0f, static_cast<float>(rtt_ticks) * retx_mult));
    for (auto& pr : pending_) {
        if (pr.acked) continue;
        if (pr.sequence <= highest_acked_) {
            pr.acked = true;
            stats_.reliable_acked += 1;
            continue;
        }
        if (now >= pr.next_retx_tick) {
            t.send(peer, reliable_channel_, std::span<const std::uint8_t>(pr.bytes));
            pr.next_retx_tick = now + retx_interval;
            stats_.reliable_retransmitted += 1;
        }
    }
    // GC fully-acked prefix.
    while (!pending_.empty() && pending_.front().acked) {
        pending_.pop_front();
    }
}

void ReliabilityChannel::on_ack(std::uint32_t highest_acked_sequence) {
    if (highest_acked_sequence > highest_acked_) highest_acked_ = highest_acked_sequence;
}

} // namespace gw::net
