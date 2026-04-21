#pragma once
// engine/audio/audio_service.hpp — RAII façade owning the Phase-10 audio
// runtime (ADR-0017 §2.4, §2.5).
//
// Single instance per engine. Lifetime is RAII: constructor initialises all
// submodules and the backend; destructor shuts them down cleanly. No
// two-phase init/shutdown (CLAUDE.md #6).

#include "engine/audio/audio_backend.hpp"
#include "engine/audio/audio_types.hpp"
#include "engine/audio/decoder_registry.hpp"
#include "engine/audio/mixer_graph.hpp"
#include "engine/audio/music_streamer.hpp"
#include "engine/audio/propagation.hpp"
#include "engine/audio/spatial.hpp"
#include "engine/audio/voice_pool.hpp"

#include <memory>
#include <vector>

namespace gw::audio {

class AudioService {
public:
    // Construct with the default production backend (miniaudio if
    // GW_AUDIO_MINIAUDIO is defined, else null). Tests typically pass a
    // custom backend via the second constructor.
    explicit AudioService(const AudioConfig& cfg);

    // Test / tooling constructor: inject a custom backend + spatial backend.
    AudioService(const AudioConfig& cfg,
                 std::unique_ptr<IAudioBackend> backend,
                 std::unique_ptr<ISpatialBackend> spatial);

    ~AudioService();

    AudioService(const AudioService&) = delete;
    AudioService& operator=(const AudioService&) = delete;

    // -----------------------------------------------------------------
    // Playback API
    // -----------------------------------------------------------------
    [[nodiscard]] VoiceHandle play_2d(AudioClipId clip, const Play2D& p);
    [[nodiscard]] VoiceHandle play_3d(AudioClipId clip, const Play3D& p);
    void stop(VoiceHandle);

    // Voice emitter updates (3D voice re-position each frame).
    void set_emitter(VoiceHandle, const EmitterState&);

    // -----------------------------------------------------------------
    // Bus + mixer controls
    // -----------------------------------------------------------------
    void set_bus_volume(BusId bus, float volume) noexcept;
    void set_bus_mute(BusId bus, bool muted) noexcept;
    void set_reverb_preset(BusId bus, ReverbPreset preset) noexcept;
    void set_downmix_mode(DownmixMode mode) noexcept;
    void set_vacuum(bool in_vacuum) noexcept;

    // Duck a bus by the given dB for the rest of the frame. Cleared on update.
    void duck_bus(BusId bus, float db) noexcept;

    // -----------------------------------------------------------------
    // Music streaming
    // -----------------------------------------------------------------
    // Starts or replaces the currently-playing music stream. Returns true
    // on success; false if the stream could not be opened.
    bool play_music_stream(std::unique_ptr<IStreamDecoder> decoder,
                           const MusicStreamConfig& cfg = {});

    // Crossfade the music bus to a new stream over `duration_s`.
    bool crossfade_music(std::unique_ptr<IStreamDecoder> decoder, float duration_s);

    void stop_music();

    // -----------------------------------------------------------------
    // Occlusion provider injection
    // -----------------------------------------------------------------
    void set_occlusion_provider(std::unique_ptr<IOcclusionProvider>);

    // -----------------------------------------------------------------
    // Listener & per-frame update (main thread)
    // -----------------------------------------------------------------
    void update(double dt_seconds, const ListenerState&);

    // -----------------------------------------------------------------
    // For CI/tests only — pump the null backend forward.
    // -----------------------------------------------------------------
    void pump_frames(std::size_t frame_count);

    // -----------------------------------------------------------------
    // Inspection
    // -----------------------------------------------------------------
    [[nodiscard]] AudioStats       stats() const noexcept;
    [[nodiscard]] const MixerGraph& mixer() const noexcept;
    [[nodiscard]] MixerGraph&       mixer_mut() noexcept;
    [[nodiscard]] const AudioConfig& config() const noexcept { return cfg_; }
    [[nodiscard]] ISpatialBackend*   spatial() noexcept { return spatial_.get(); }
    [[nodiscard]] const VoicePool&   voices() const noexcept { return *pool_; }

private:
    void audio_render_callback(std::span<float> interleaved, std::size_t frame_count);

    AudioConfig                          cfg_{};
    std::unique_ptr<IAudioBackend>       backend_;
    std::unique_ptr<ISpatialBackend>     spatial_;
    std::unique_ptr<VoicePool>           pool_;
    std::unique_ptr<MixerGraph>          mixer_;
    std::unique_ptr<DecoderRegistry>     decoders_;
    std::unique_ptr<IOcclusionProvider>  occlusion_;
    std::unique_ptr<MusicStreamer>       music_;
    std::unique_ptr<MusicStreamer>       music_xfade_in_;
    std::unique_ptr<MusicStreamer>       music_xfade_out_;
    float                                xfade_duration_s_{0.0f};
    float                                xfade_elapsed_s_{0.0f};

    ListenerState                        listener_{};
    uint64_t                             frame_counter_{0};
    AudioStats                           stats_{};

    // Per-voice spatial handle mapping.
    struct SpatialBind {
        VoiceHandle   voice{};
        SpatialHandle spatial{};
        bool          active{false};
    };
    std::vector<SpatialBind>             spatial_binds_;
};

} // namespace gw::audio
