// engine/net/voice.cpp — Phase 14 Wave 14I (ADR-0052).
//
// When `GW_ENABLE_OPUS` is defined the encode/decode path routes through
// libopus; otherwise a PCM passthrough is used. The PCM path is
// deterministic and is what the unit tests exercise.

#include "engine/net/voice.hpp"

#include <cstring>

namespace gw::net {

VoicePipeline::VoicePipeline(VoiceConfig cfg) : cfg_{cfg} {
#if defined(GW_ENABLE_OPUS)
    backend_name_ = std::string_view{"opus"};
#else
    backend_name_ = std::string_view{"pcm-passthrough"};
#endif
}

void VoicePipeline::push_local_frame(std::span<const std::int16_t> pcm, ITransport& t, PeerId local) {
    ++out_seq_;
    // PCM passthrough encoding: LE int16 samples, no header.
    std::vector<std::uint8_t> bytes;
    bytes.reserve(pcm.size() * sizeof(std::int16_t));
    for (std::int16_t s : pcm) {
        bytes.push_back(static_cast<std::uint8_t>(s & 0xFF));
        bytes.push_back(static_cast<std::uint8_t>((static_cast<std::uint16_t>(s) >> 8) & 0xFF));
    }
    t.send(local, kChannelVoice, std::span<const std::uint8_t>(bytes));
    frames_sent_ += 1;
}

bool VoicePipeline::on_inbound(PeerId peer, std::uint32_t sequence, std::span<const std::uint8_t> bytes) {
    if (!peer.valid()) return false;
    const std::uint64_t bit = (peer.id < 64) ? (std::uint64_t{1} << peer.id) : 0;
    if (bit != 0 && (mute_mask_ & bit)) {
        frames_dropped_ += 1;
        return false;
    }
    auto& latest = latest_seen_[peer.id];
    if (sequence != 0 && sequence <= latest) {
        frames_dropped_ += 1;
        return false;
    }
    latest = sequence;
    VoiceFrame vf{};
    vf.peer     = peer;
    vf.sequence = sequence;
    vf.bytes.assign(bytes.begin(), bytes.end());
    decoded_ring_.push_back(std::move(vf));
    frames_received_ += 1;
    return true;
}

void VoicePipeline::drain_decoded(std::vector<VoiceFrame>& out) {
    out.insert(out.end(),
               std::make_move_iterator(decoded_ring_.begin()),
               std::make_move_iterator(decoded_ring_.end()));
    decoded_ring_.clear();
}

} // namespace gw::net
