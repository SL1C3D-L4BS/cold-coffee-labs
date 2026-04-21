// engine/audio/audio_service.cpp — see header for contract.
#include "engine/audio/audio_service.hpp"

#include <algorithm>
#include <cstring>
#include <cmath>

namespace gw::audio {

namespace {

constexpr std::size_t kMaxSpatialBinds = 256;  // 32 active at any one time; oversubscribed for release lag

// Resolve the frame_period-sized mix buffer used by the render callback.
// It is allocated once at service construction (see AudioService ctor).

float bus_effective_gain(const MixerGraph& m, BusId bus) noexcept {
    return m.compute_effective_gain(bus);
}

} // namespace

AudioService::AudioService(const AudioConfig& cfg)
    : AudioService(cfg,
                   cfg.deterministic ? make_null_backend() : make_miniaudio_backend(),
                   make_spatial_stub(cfg.voices_3d)) {}

AudioService::AudioService(const AudioConfig& cfg,
                           std::unique_ptr<IAudioBackend> backend,
                           std::unique_ptr<ISpatialBackend> spatial)
    : cfg_(cfg),
      backend_(std::move(backend)),
      spatial_(std::move(spatial)),
      pool_(std::make_unique<VoicePool>(VoicePool::Config{cfg.voices_2d, cfg.voices_3d})),
      mixer_(std::make_unique<MixerGraph>()),
      decoders_(std::make_unique<DecoderRegistry>()),
      occlusion_(make_null_occlusion()) {
    spatial_binds_.reserve(kMaxSpatialBinds);
    if (backend_) {
        backend_->initialize(cfg_, [this](std::span<float> buf, std::size_t n) {
            audio_render_callback(buf, n);
        });
    }
}

AudioService::~AudioService() {
    if (backend_) backend_->shutdown();
}

VoiceHandle AudioService::play_2d(AudioClipId clip, const Play2D& p) {
    return pool_->acquire_2d(p.bus, clip, p, frame_counter_);
}

VoiceHandle AudioService::play_3d(AudioClipId clip, const Play3D& p) {
    auto h = pool_->acquire_3d(clip, p, frame_counter_);
    if (h.is_null()) return h;
    if (spatial_) {
        auto sh = spatial_->acquire(p.emitter);
        if (!sh.is_null()) {
            if (spatial_binds_.size() < kMaxSpatialBinds) {
                spatial_binds_.push_back(SpatialBind{h, sh, true});
            } else {
                for (auto& b : spatial_binds_) {
                    if (!b.active) { b = SpatialBind{h, sh, true}; break; }
                }
            }
        }
    }
    return h;
}

void AudioService::stop(VoiceHandle h) {
    if (!pool_->is_live(h)) return;
    for (auto& b : spatial_binds_) {
        if (b.active && b.voice == h) {
            if (spatial_) spatial_->release(b.spatial);
            b.active = false;
        }
    }
    pool_->release(h);
}

void AudioService::set_emitter(VoiceHandle h, const EmitterState& em) {
    auto* slot = pool_->get_mut(h);
    if (!slot || slot->klass != VoiceClass::Spatial3D) return;
    slot->emitter = em;
    for (auto& b : spatial_binds_) {
        if (b.active && b.voice == h && spatial_) {
            spatial_->update(b.spatial, em);
            break;
        }
    }
}

void AudioService::set_bus_volume(BusId bus, float volume) noexcept {
    mixer_->set_bus_volume(bus, volume);
}
void AudioService::set_bus_mute(BusId bus, bool muted) noexcept {
    mixer_->set_bus_mute(bus, muted);
}
void AudioService::set_reverb_preset(BusId bus, ReverbPreset preset) noexcept {
    mixer_->set_reverb_preset(bus, preset);
}
void AudioService::set_downmix_mode(DownmixMode mode) noexcept {
    mixer_->set_downmix_mode(mode);
}
void AudioService::set_vacuum(bool in_vacuum) noexcept {
    mixer_->set_vacuum(in_vacuum);
    mixer_->set_reverb_preset(BusId::Master, in_vacuum ? ReverbPreset::Vacuum : ReverbPreset::None);
}
void AudioService::duck_bus(BusId bus, float db) noexcept {
    mixer_->apply_duck_db(bus, db);
}

bool AudioService::play_music_stream(std::unique_ptr<IStreamDecoder> decoder,
                                     const MusicStreamConfig& cfg) {
    if (!decoder) return false;
    music_ = std::make_unique<MusicStreamer>(std::move(decoder), cfg);
    music_->fill();
    return true;
}

bool AudioService::crossfade_music(std::unique_ptr<IStreamDecoder> decoder, float duration_s) {
    if (!decoder) return false;
    music_xfade_out_ = std::move(music_);
    MusicStreamConfig cfg{};
    music_xfade_in_ = std::make_unique<MusicStreamer>(std::move(decoder), cfg);
    xfade_duration_s_ = std::max(duration_s, 0.001f);
    xfade_elapsed_s_ = 0.0f;
    if (music_xfade_in_) music_xfade_in_->fill();
    return true;
}

void AudioService::stop_music() {
    music_.reset();
    music_xfade_in_.reset();
    music_xfade_out_.reset();
    xfade_duration_s_ = 0.0f;
    xfade_elapsed_s_ = 0.0f;
}

void AudioService::set_occlusion_provider(std::unique_ptr<IOcclusionProvider> p) {
    occlusion_ = p ? std::move(p) : make_null_occlusion();
}

void AudioService::update(double dt_seconds, const ListenerState& l) {
    listener_ = l;
    mixer_->set_vacuum(l.atmospheric_density < 0.001f);
    mixer_->reset_ducking();
    if (spatial_) spatial_->set_listener(l);

    // Drive occlusion updates for every active 3D voice.
    pool_->for_each_active_mut([&](VoiceSlotInfo& slot) {
        if (slot.klass != VoiceClass::Spatial3D) return;
        const auto r = occlusion_->query({l.position, slot.emitter.position});
        slot.emitter.occlusion = r.occlusion;
        slot.emitter.obstruction = r.obstruction;
        for (auto& b : spatial_binds_) {
            if (b.active && b.voice == slot.handle && spatial_) {
                spatial_->update(b.spatial, slot.emitter);
                break;
            }
        }
    });

    // Crossfade advance.
    if (music_xfade_in_) {
        xfade_elapsed_s_ += static_cast<float>(dt_seconds);
        if (xfade_elapsed_s_ >= xfade_duration_s_) {
            music_ = std::move(music_xfade_in_);
            music_xfade_out_.reset();
        }
    }

    // Refill music streams from main thread each update.
    if (music_) music_->fill();
    if (music_xfade_in_) music_xfade_in_->fill();
    if (music_xfade_out_) music_xfade_out_->fill();

    ++frame_counter_;

    // Consume device reroute events.
    if (backend_ && backend_->was_rerouted()) {
        ++stats_.reroute_count;
        (void) backend_->consume_reroute();
    }
}

void AudioService::pump_frames(std::size_t frame_count) {
    if (backend_) backend_->pump_manual_frames(frame_count);
}

AudioStats AudioService::stats() const noexcept {
    AudioStats s = stats_;
    s.active_voices_2d = pool_->active_2d();
    s.active_voices_3d = pool_->active_3d();
    s.voices_stolen    = pool_->stolen_count();
    s.frames_rendered  = frame_counter_;
    if (music_) s.stream_underruns = music_->stats().underruns;
    return s;
}

const MixerGraph& AudioService::mixer() const noexcept { return *mixer_; }
MixerGraph&       AudioService::mixer_mut() noexcept   { return *mixer_; }

void AudioService::audio_render_callback(std::span<float> interleaved, std::size_t frame_count) {
    // interleaved is already zeroed by the null backend (and by miniaudio
    // when ma_format_f32 is the output). We accumulate into it.
    // For the null backend this is effectively a determinism-hook: we
    // mix into the buffer here and tests observe the PCM.
    if (interleaved.size() < frame_count * 2) return;

    // Mix music into the buffer first (stereo drain).
    if (music_) {
        const float bus_gain = bus_effective_gain(*mixer_, BusId::Music);
        if (bus_gain > 0.0f) {
            // Temporary scratch per frame isn't allocated in the hot path —
            // we pull in-place into the interleaved buffer via a small
            // stack scratch (frame_count ≤ frames_period is guaranteed).
            constexpr std::size_t kScratchMax = 2048;
            float scratch[kScratchMax * 2];
            const std::size_t n = std::min<std::size_t>(frame_count, kScratchMax);
            music_->pull(std::span<float>(scratch, n * 2), n);
            for (std::size_t i = 0; i < n * 2; ++i) interleaved[i] += scratch[i] * bus_gain;
        }
    }
    // Crossfade pair
    if (music_xfade_in_ && music_xfade_out_ && xfade_duration_s_ > 0.0f) {
        const float t_in  = std::clamp(xfade_elapsed_s_ / xfade_duration_s_, 0.0f, 1.0f);
        const float t_out = 1.0f - t_in;
        const float bus_gain = bus_effective_gain(*mixer_, BusId::Music);
        constexpr std::size_t kScratchMax = 2048;
        float scratch[kScratchMax * 2];
        const std::size_t n = std::min<std::size_t>(frame_count, kScratchMax);
        // In
        music_xfade_in_->pull(std::span<float>(scratch, n * 2), n);
        for (std::size_t i = 0; i < n * 2; ++i) interleaved[i] += scratch[i] * bus_gain * t_in;
        // Out
        music_xfade_out_->pull(std::span<float>(scratch, n * 2), n);
        for (std::size_t i = 0; i < n * 2; ++i) interleaved[i] += scratch[i] * bus_gain * t_out;
    }

    // 3D voices — route through the spatial backend (accumulated stereo).
    // Each voice currently plays a placeholder 440 Hz sine proportional to
    // the bus's effective gain. The real clip-sample-pull path lands when
    // the asset DB is wired to audio in Wave 10C integration; for now this
    // gives us deterministic golden-PCM.
    for (auto& b : spatial_binds_) {
        if (!b.active) continue;
        const auto* slot = pool_->get(b.voice);
        if (!slot) { b.active = false; continue; }
        const float bus_gain = bus_effective_gain(*mixer_, slot->bus);
        if (bus_gain <= 0.0f) continue;
        constexpr std::size_t kScratchMax = 2048;
        float mono[kScratchMax];
        const std::size_t n = std::min<std::size_t>(frame_count, kScratchMax);
        const double freq = 440.0;
        for (std::size_t i = 0; i < n; ++i) {
            const double t = static_cast<double>(frame_counter_ * frame_count + i)
                             / static_cast<double>(cfg_.sample_rate);
            mono[i] = static_cast<float>(std::sin(2.0 * 3.14159265358979323846 * freq * t))
                      * slot->play3d.volume * bus_gain;
        }
        if (spatial_) {
            spatial_->render(b.spatial,
                             std::span<const float>(mono, n),
                             std::span<float>(interleaved.data(), n * 2),
                             n);
        }
    }

    // 2D voices — directly mixed; placeholder sine at bus gain.
    pool_->for_each_active([&](const VoiceSlotInfo& slot) {
        if (slot.klass != VoiceClass::NonSpatial2D) return;
        const float bus_gain = bus_effective_gain(*mixer_, slot.bus);
        if (bus_gain <= 0.0f) return;
        constexpr std::size_t kScratchMax = 2048;
        float scratch[kScratchMax];
        const std::size_t n = std::min<std::size_t>(frame_count, kScratchMax);
        const double freq = 660.0;
        for (std::size_t i = 0; i < n; ++i) {
            const double t = static_cast<double>(frame_counter_ * frame_count + i)
                             / static_cast<double>(cfg_.sample_rate);
            scratch[i] = static_cast<float>(std::sin(2.0 * 3.14159265358979323846 * freq * t))
                         * slot.play2d.volume * bus_gain * 0.5f;
        }
        for (std::size_t i = 0; i < n; ++i) {
            interleaved[2 * i + 0] += scratch[i];
            interleaved[2 * i + 1] += scratch[i];
        }
    });

    // Master mute / downmix.
    if (mixer_->bus_muted(BusId::Master)) {
        std::memset(interleaved.data(), 0, interleaved.size() * sizeof(float));
        return;
    }
    if (mixer_->downmix_mode() == DownmixMode::Mono) {
        for (std::size_t i = 0; i < frame_count; ++i) {
            const float avg = 0.5f * (interleaved[2 * i + 0] + interleaved[2 * i + 1]);
            interleaved[2 * i + 0] = avg;
            interleaved[2 * i + 1] = avg;
        }
    }
}

} // namespace gw::audio
