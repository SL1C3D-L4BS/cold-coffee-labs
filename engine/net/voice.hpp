#pragma once
// engine/net/voice.hpp — Phase 14 Wave 14I (ADR-0052).
//
// Voice capture → encode → transport → decode → spatialize pipeline.
// Codec is Opus when `GW_ENABLE_OPUS` is defined; otherwise a PCM
// passthrough (valid for LAN demos and unit tests). Spatialization hooks
// into the existing Steam Audio effects chain.

#include "engine/net/net_types.hpp"
#include "engine/net/transport.hpp"

#include <cstdint>
#include <cstddef>
#include <span>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace gw::net {

struct VoiceConfig {
    std::uint32_t sample_rate_hz{48000};
    std::uint32_t frame_ms{20};
    std::uint32_t bitrate_kbps{24};
    bool          fec{true};
    bool          push_to_talk{true};
    std::uint32_t max_peers{kMaxPeersHard};
};

struct VoiceFrame {
    PeerId                     peer{};
    std::uint32_t              sequence{0};
    std::vector<std::uint8_t>  bytes{};
};

class VoicePipeline {
public:
    explicit VoicePipeline(VoiceConfig cfg);

    // Capture → encode → send. `pcm` is int16 interleaved @ sample_rate_hz.
    void push_local_frame(std::span<const std::int16_t> pcm, ITransport& t, PeerId local);

    // Push an inbound encoded payload. Returns true if accepted
    // (non-duplicate, non-muted).
    bool on_inbound(PeerId peer, std::uint32_t sequence, std::span<const std::uint8_t> bytes);

    // Drain decoded frames for the audio mixer. Clears the ring on drain.
    void drain_decoded(std::vector<VoiceFrame>& out);

    void set_mute_mask(std::uint64_t mask) noexcept { mute_mask_ = mask; }
    [[nodiscard]] std::uint64_t mute_mask() const noexcept { return mute_mask_; }

    [[nodiscard]] std::string_view backend_name() const noexcept { return backend_name_; }

    // Stats.
    [[nodiscard]] std::uint64_t frames_sent()    const noexcept { return frames_sent_; }
    [[nodiscard]] std::uint64_t frames_received() const noexcept { return frames_received_; }
    [[nodiscard]] std::uint64_t frames_dropped() const noexcept { return frames_dropped_; }

private:
    VoiceConfig cfg_;
    std::string_view backend_name_{};
    std::unordered_map<std::uint32_t, std::uint32_t> latest_seen_{}; // peer.id → last seq
    std::vector<VoiceFrame> decoded_ring_{};
    std::uint32_t           out_seq_{0};
    std::uint64_t           mute_mask_{0};
    std::uint64_t           frames_sent_{0};
    std::uint64_t           frames_received_{0};
    std::uint64_t           frames_dropped_{0};
};

} // namespace gw::net
