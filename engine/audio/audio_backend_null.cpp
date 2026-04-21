// engine/audio/audio_backend_null.cpp — deterministic null backend (ADR-0017).
// Used by tests, CI, and tooling. No OS dependencies.
#include "engine/audio/audio_backend.hpp"

#include <cstring>

namespace gw::audio {

namespace {

class NullAudioBackend final : public IAudioBackend {
public:
    void initialize(const AudioConfig& cfg, RenderCallback cb) override {
        info_.sample_rate   = cfg.sample_rate;
        info_.channels      = cfg.channels;
        info_.frames_period = cfg.frames_period;
        info_.is_realtime   = false;
        info_.name          = "null";
        cb_ = std::move(cb);
        const std::size_t period = cfg.frames_period > 0 ? cfg.frames_period : 512;
        const std::size_t ch     = cfg.channels == 0 ? 2 : cfg.channels;
        buffer_size_ = period * ch;
        buffer_ = std::make_unique<float[]>(buffer_size_);
    }

    void shutdown() noexcept override {
        cb_ = nullptr;
    }

    BackendInfo info() const override { return info_; }

    void pump_manual_frames(std::size_t frames) override {
        if (!cb_ || !buffer_) return;
        const std::size_t period = info_.frames_period > 0 ? info_.frames_period : 512;
        const std::size_t ch     = info_.channels == 0 ? 2 : info_.channels;
        std::size_t remaining = frames;
        while (remaining > 0) {
            const std::size_t chunk = remaining < period ? remaining : period;
            std::memset(buffer_.get(), 0, chunk * ch * sizeof(float));
            cb_(std::span<float>(buffer_.get(), chunk * ch), chunk);
            remaining -= chunk;
        }
    }

    bool was_rerouted() noexcept override { return false; }
    DeviceReroutedEvent consume_reroute() noexcept override { return {}; }

private:
    BackendInfo    info_{};
    RenderCallback cb_{};
    // Reusable interleaved mix buffer. Allocated once at initialize, never
    // re-allocated on the pump loop.
    std::unique_ptr<float[]> buffer_{};
    std::size_t              buffer_size_{0};
};

} // namespace

std::unique_ptr<IAudioBackend> make_null_backend() {
    return std::make_unique<NullAudioBackend>();
}

// Weak fallback when GW_AUDIO_MINIAUDIO is not defined.
#if !defined(GW_AUDIO_MINIAUDIO)
std::unique_ptr<IAudioBackend> make_miniaudio_backend() {
    // Fall back to the null backend if production audio is disabled.
    return make_null_backend();
}
#endif

} // namespace gw::audio
