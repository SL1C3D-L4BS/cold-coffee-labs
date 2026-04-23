// engine/audio/martyrdom_audio_controller.cpp — Phase 21 W139 (ADR-0108).

#include "engine/audio/martyrdom_audio_controller.hpp"

#include <algorithm>
#include <cmath>

namespace gw::audio {

namespace {

float clamp01(float v) noexcept {
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

float smooth_toward(float current, float target, float dt_sec, float tau_sec) noexcept {
    if (tau_sec <= 0.0f) {
        return target;
    }
    const float alpha = 1.0f - std::exp(-dt_sec / tau_sec);
    return current + (target - current) * alpha;
}

bool crossed_up(float prev, float cur, float threshold) noexcept {
    return prev < threshold && cur >= threshold;
}

} // namespace

MartyrdomAudioOutput step_martyrdom_audio(const MartyrdomAudioState& prev,
                                          const MartyrdomAudioInput& in) noexcept {
    const float dt = std::max(0.0f, in.dt_sec);

    // Targets. Sin choir rises monotonically with Sin; saturates at Sin=100.
    const float sin_target     = clamp01(in.sin_current / 100.0f);
    const float thunder_target = in.rapture_active ? 1.0f : 0.0f;
    const float drone_target   = clamp01(in.ruin_intensity);

    MartyrdomAudioState next{};
    next.sin_choir_gain       = smooth_toward(prev.sin_choir_gain, sin_target, dt, 0.8f);
    next.rapture_thunder_gain = smooth_toward(prev.rapture_thunder_gain, thunder_target, dt, 0.25f);
    next.ruin_drone_gain      = smooth_toward(prev.ruin_drone_gain, drone_target, dt, 1.2f);

    // Crossfade — 0 = Sin-dominant, 1 = Rapture/Ruin dominant.
    const float dominance = std::max(next.rapture_thunder_gain, next.ruin_drone_gain);
    next.bpm_crossfade    = smooth_toward(prev.bpm_crossfade, dominance, dt, 0.5f);

    MartyrdomAudioOutput out{};
    out.state = next;

    // Trigger priority: Rapture onset > Ruin onset > Sin 90 > 60 > 30.
    if (in.rapture_active && !in.prev_rapture_active) {
        out.trigger         = DialogueTrigger::RaptureOnset;
        out.trigger_speaker = gw::narrative::Speaker::Malakor;
    } else if (in.ruin_intensity > 0.0f && in.prev_ruin_intensity <= 0.0f) {
        out.trigger         = DialogueTrigger::RuinTickOver;
        out.trigger_speaker = gw::narrative::Speaker::Niccolo;
    } else if (crossed_up(in.prev_sin_current, in.sin_current, 90.0f)) {
        out.trigger         = DialogueTrigger::SinThreshold90;
        out.trigger_speaker = gw::narrative::Speaker::Malakor;
    } else if (crossed_up(in.prev_sin_current, in.sin_current, 60.0f)) {
        out.trigger         = DialogueTrigger::SinThreshold60;
        out.trigger_speaker = gw::narrative::Speaker::Niccolo;
    } else if (crossed_up(in.prev_sin_current, in.sin_current, 30.0f)) {
        out.trigger         = DialogueTrigger::SinThreshold30;
        out.trigger_speaker = gw::narrative::Speaker::Malakor;
    }

    return out;
}

MartyrdomAudioBusNames martyrdom_bus_names() noexcept {
    return {};
}

} // namespace gw::audio
