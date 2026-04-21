#pragma once
// engine/audio/audio_backend.hpp — interface abstraction over the
// platform-specific audio callback loop (ADR-0017 §2.1).
//
// Production backend (miniaudio) lives under engine/platform/audio/ and is
// gated behind `GW_AUDIO_MINIAUDIO`. The null backend compiles everywhere
// and is used by CI, the determinism tests, and headless tools.

#include "engine/audio/audio_types.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <span>

namespace gw::audio {

// The AudioService owns a render callback that the backend invokes each
// buffer. Signature intentionally mirrors miniaudio's `ma_device_proc`.
using RenderCallback = std::function<void(std::span<float> interleaved,
                                          std::size_t frame_count)>;

struct BackendInfo {
    const char*  name{"null"};
    uint32_t     sample_rate{48'000};
    uint8_t      channels{2};
    uint32_t     frames_period{512};
    bool         is_realtime{false};
};

class IAudioBackend {
public:
    virtual ~IAudioBackend() = default;

    virtual void initialize(const AudioConfig& cfg, RenderCallback cb) = 0;
    virtual void shutdown() noexcept = 0;

    virtual BackendInfo info() const = 0;

    // Backends must arrange for the callback to be invoked on a dedicated
    // audio thread. For the null backend, the test harness pumps manually
    // via `pump_manual_frames`; for miniaudio the underlying ma_device owns
    // the thread.
    virtual void pump_manual_frames(std::size_t /*frames*/) {}

    virtual bool was_rerouted() noexcept { return false; }
    virtual DeviceReroutedEvent consume_reroute() noexcept { return {}; }
};

// NullAudioBackend — deterministic, no device. Pump with pump_manual_frames.
[[nodiscard]] std::unique_ptr<IAudioBackend> make_null_backend();

// Forward-declarations for the optional miniaudio backend. Unused when
// GW_AUDIO_MINIAUDIO is not defined — the CMake wiring swaps the factory
// in that case.
[[nodiscard]] std::unique_ptr<IAudioBackend> make_miniaudio_backend();

} // namespace gw::audio
