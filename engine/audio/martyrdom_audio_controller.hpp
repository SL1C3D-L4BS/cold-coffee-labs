#pragma once
// engine/audio/martyrdom_audio_controller.hpp — Phase 21 W139 (ADR-0108).
//
// Three-stem Martyrdom audio bed:
//   - Sin choir      (rises with SinComponent::current)
//   - Rapture thunder (plays while RaptureState::active)
//   - Ruin drone      (plays during RuinState)
// BPM crossfade between stems keeps the layered bed musical. The controller
// publishes a DialogueLineEvent via the narrative dialogue graph when
// SinComponent crosses the 30 / 60 / 90 thresholds, when Rapture ignites,
// and when Ruin begins.
//
// Deterministic: all inputs are value types and all decisions are a pure
// function of prior state + frame inputs.

#include "engine/narrative/narrative_types.hpp"

#include <array>
#include <cstdint>

namespace gw::audio {

struct MartyrdomAudioInput {
    float sin_current{0.0f};        // 0..100
    float prev_sin_current{0.0f};
    bool  rapture_active{false};
    bool  prev_rapture_active{false};
    float ruin_intensity{0.0f};     // 0..1
    float prev_ruin_intensity{0.0f};
    float dt_sec{0.0f};
    float target_bpm{110.0f};
};

struct MartyrdomAudioState {
    float sin_choir_gain{0.0f};     // 0..1 smoothed toward target
    float rapture_thunder_gain{0.0f};
    float ruin_drone_gain{0.0f};
    float bpm_crossfade{0.0f};      // 0..1 cross-mix between layers
};

enum class DialogueTrigger : std::uint8_t {
    None              = 0,
    SinThreshold30    = 1,
    SinThreshold60    = 2,
    SinThreshold90    = 3,
    RaptureOnset      = 4,
    RuinTickOver      = 5,
};

struct MartyrdomAudioOutput {
    MartyrdomAudioState     state{};
    DialogueTrigger         trigger{DialogueTrigger::None};
    gw::narrative::Speaker  trigger_speaker{gw::narrative::Speaker::None};
};

/// Deterministic single step — advances gains, emits at most one trigger
/// per call (the highest-priority crossing wins).
[[nodiscard]] MartyrdomAudioOutput step_martyrdom_audio(const MartyrdomAudioState& prev,
                                                        const MartyrdomAudioInput& in) noexcept;

/// Convenience — returns the three CVar/bus names for mix routing.
struct MartyrdomAudioBusNames {
    const char* sin_choir{"audio.martyrdom.sin_choir"};
    const char* rapture_thunder{"audio.martyrdom.rapture_thunder"};
    const char* ruin_drone{"audio.martyrdom.ruin_drone"};
};

[[nodiscard]] MartyrdomAudioBusNames martyrdom_bus_names() noexcept;

} // namespace gw::audio
