// engine/ai_runtime/director_policy.cpp — Phase 26 scaffold.
//
// The shipping policy is a rule-based state machine whose *threshold* constants
// are tuned offline by the `director_train/` RL pipeline (ADR-0097). This
// scaffold hard-codes sane starting thresholds; the cooked checkpoint overwrites
// them at load time via `model_registry::register_pinned`.

#include "engine/ai_runtime/director_policy.hpp"

namespace gw::ai_runtime {

namespace {

// Rule-based L4D-style transitions. No RNG — fully deterministic given input.
DirectorState step_state_machine(const DirectorPolicyInput& in) noexcept {
    switch (in.current_state) {
    case DirectorState::Relax:
        if (in.combat_intensity_ewma > 0.35f) return DirectorState::BuildUp;
        return DirectorState::Relax;
    case DirectorState::BuildUp:
        if (in.combat_intensity_ewma > 0.75f) return DirectorState::SustainedPeak;
        if (in.time_in_state_sec > 25.f)       return DirectorState::SustainedPeak;
        return DirectorState::BuildUp;
    case DirectorState::SustainedPeak:
        if (in.time_in_state_sec > 12.f)       return DirectorState::PeakFade;
        if (in.player_health_ratio < 0.25f)    return DirectorState::PeakFade;
        return DirectorState::SustainedPeak;
    case DirectorState::PeakFade:
        if (in.combat_intensity_ewma < 0.2f)   return DirectorState::Relax;
        return DirectorState::PeakFade;
    }
    return DirectorState::Relax;
}

} // namespace

DirectorPolicyOutput evaluate_director(const DirectorPolicyInput& input,
                                       std::uint64_t /*seed*/) noexcept {
    (void)input.logical_tick; // wired for replay / MP hash stability; policy is rule-based today.
    DirectorPolicyOutput out{};
    out.next_state = step_state_machine(input);

    switch (out.next_state) {
    case DirectorState::Relax:
        out.spawn_rate_scale = 0.40f;
        out.item_drop_scale  = 1.20f;
        out.intensity_target = 0.10f;
        break;
    case DirectorState::BuildUp:
        out.spawn_rate_scale = 1.00f;
        out.item_drop_scale  = 1.00f;
        out.intensity_target = 0.55f;
        break;
    case DirectorState::SustainedPeak:
        out.spawn_rate_scale = 1.75f;
        out.item_drop_scale  = 0.75f;
        out.intensity_target = 0.95f;
        break;
    case DirectorState::PeakFade:
        out.spawn_rate_scale = 0.80f;
        out.item_drop_scale  = 1.10f;
        out.intensity_target = 0.40f;
        break;
    }

    // Grace-ratio is a mild restraint modifier — high-Grace runs get gentler
    // spawn rates in the Relax state. Presentation-adjacent; still deterministic.
    out.spawn_rate_scale *= (1.f - 0.25f * input.grace_ratio);
    return out;
}

} // namespace gw::ai_runtime
