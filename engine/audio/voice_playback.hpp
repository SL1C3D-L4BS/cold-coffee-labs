#pragma once
// engine/audio/voice_playback.hpp — voice-chat playback seam (ADR-0022 §5).
// Decodes Opus frames back into an ma_sound slot on the Dialogue bus. The
// Steam Audio spatial pipeline then applies identically to any other 3D
// source.

#include "engine/audio/audio_types.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace gw::audio {

class IVoicePlayback {
public:
    virtual ~IVoicePlayback() = default;
    virtual VoiceHandle open(const EmitterState& em) = 0;
    virtual void        close(VoiceHandle) = 0;
    virtual void        push_encoded_frame(VoiceHandle, std::span<const uint8_t>) = 0;
};

} // namespace gw::audio
