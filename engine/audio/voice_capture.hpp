#pragma once
// engine/audio/voice_capture.hpp — voice-chat capture seam (ADR-0022 §5).
//
// Phase 10 ships only the interface. Phase 14 will implement a libopus-
// backed concrete type. Kept under `engine/audio/` so Phase 14's Networking
// wave can wire it up without carving out a new module.

#include <cstddef>
#include <cstdint>
#include <span>

namespace gw::audio {

struct VoiceEncodedFrame {
    std::span<const uint8_t> bytes;
    uint64_t                 frame_index{0};
    uint32_t                 sample_rate{48'000};
};

class IVoiceCapture {
public:
    virtual ~IVoiceCapture() = default;
    // Encode one 20 ms frame of 48 kHz mono PCM.
    virtual VoiceEncodedFrame encode_frame(std::span<const float> pcm_20ms) = 0;
    virtual void              set_bitrate_kbps(uint32_t) = 0;
};

} // namespace gw::audio
