#pragma once
// engine/audio/mixer_graph.hpp — frozen Master-SFX-Music-Dialogue-Ambience
// topology from 19_AUDIO_DESIGN §5 (ADR-0017 §2.2).
//
// The mixer graph is *data*: per-bus volume, mute, reverb preset, and the
// parent pointer. The topology itself is compile-time frozen so any drift
// shows up as a unit-test failure.

#include "engine/audio/audio_types.hpp"

#include <array>

namespace gw::audio {

struct BusNode {
    BusId        id{BusId::Master};
    BusId        parent{BusId::Master};     // Master's parent is itself (sentinel).
    float        volume{1.0f};              // linear gain
    bool         muted{false};
    ReverbPreset reverb{ReverbPreset::None};
    float        duck_db{0.0f};             // additive dB from ducking rules (negative = quieter)
};

class MixerGraph {
public:
    MixerGraph();  // initialises the frozen topology.
    ~MixerGraph() = default;

    // Per-bus volume / mute (linear). Ducking is composed on top by
    // apply_duck_db / compute_effective_gain.
    void  set_bus_volume(BusId bus, float volume) noexcept;
    [[nodiscard]] float bus_volume(BusId bus) const noexcept;

    void  set_bus_mute(BusId bus, bool muted) noexcept;
    [[nodiscard]] bool bus_muted(BusId bus) const noexcept;

    void  set_reverb_preset(BusId bus, ReverbPreset preset) noexcept;
    [[nodiscard]] ReverbPreset bus_reverb(BusId bus) const noexcept;

    // Declarative ducking — value is in dB and is accumulated each frame
    // by the mixer system; call reset_ducking() at the start of each frame.
    void apply_duck_db(BusId bus, float db) noexcept;
    void reset_ducking() noexcept;

    // Downmix (GAG stereo/mono toggle)
    void set_downmix_mode(DownmixMode mode) noexcept { downmix_ = mode; }
    [[nodiscard]] DownmixMode downmix_mode() const noexcept { return downmix_; }

    // Vacuum bus state (§2.6 of ADR-0018). Toggles sfx mute + contact LPF.
    void set_vacuum(bool in_vacuum) noexcept { in_vacuum_ = in_vacuum; }
    [[nodiscard]] bool in_vacuum() const noexcept { return in_vacuum_; }

    // Compose the effective gain for a voice routed to `bus`. Walks from
    // `bus` up to Master multiplying bus volume × mute × duck_db,
    // respecting vacuum state (SFX muted in vacuum).
    [[nodiscard]] float compute_effective_gain(BusId bus) const noexcept;

    // Topological equality against the canonical layout; used in tests.
    [[nodiscard]] bool topology_matches_canonical() const noexcept;

    [[nodiscard]] const std::array<BusNode, kBusCount>& nodes() const noexcept { return nodes_; }

private:
    std::array<BusNode, kBusCount> nodes_{};
    DownmixMode downmix_{DownmixMode::Stereo};
    bool        in_vacuum_{false};

    static BusId parent_of(BusId bus) noexcept;
    static bool  is_sfx_subtree(BusId bus) noexcept;
};

} // namespace gw::audio
