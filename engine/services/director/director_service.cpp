// engine/services/director/director_service.cpp — Phase 27 scaffold.

#include "engine/services/director/director_service.hpp"
#include "engine/ai_runtime/director_policy.hpp"

namespace gw::services::director {

namespace {
ai_runtime::DirectorState to_gw(IntensityState s) noexcept {
    switch (s) {
    case IntensityState::Relax:         return ai_runtime::DirectorState::Relax;
    case IntensityState::BuildUp:       return ai_runtime::DirectorState::BuildUp;
    case IntensityState::SustainedPeak: return ai_runtime::DirectorState::SustainedPeak;
    case IntensityState::PeakFade:      return ai_runtime::DirectorState::PeakFade;
    }
    return ai_runtime::DirectorState::Relax;
}
IntensityState from_gw(ai_runtime::DirectorState s) noexcept {
    switch (s) {
    case ai_runtime::DirectorState::Relax:         return IntensityState::Relax;
    case ai_runtime::DirectorState::BuildUp:       return IntensityState::BuildUp;
    case ai_runtime::DirectorState::SustainedPeak: return IntensityState::SustainedPeak;
    case ai_runtime::DirectorState::PeakFade:      return IntensityState::PeakFade;
    }
    return IntensityState::Relax;
}
} // namespace

DirectorResult evaluate(const DirectorRequest& req) noexcept {
    ai_runtime::DirectorPolicyInput in{};
    in.player_health_ratio   = req.player_health_ratio;
    in.recent_damage_taken   = req.recent_damage_taken;
    in.recent_kills_per_sec  = req.recent_kps;
    in.combat_intensity_ewma = req.intensity_ewma;
    in.sin_ratio             = req.normalized_sin;
    in.grace_ratio           = req.normalized_grace;
    in.time_in_state_sec     = req.time_in_state_s;
    in.current_state         = to_gw(req.current_state);

    const auto out = ai_runtime::evaluate_director(in, req.seed);

    DirectorResult r{};
    r.next_state       = from_gw(out.next_state);
    r.spawn_rate_scale = out.spawn_rate_scale;
    r.item_drop_scale  = out.item_drop_scale;
    r.intensity_target = out.intensity_target;
    return r;
}

} // namespace gw::services::director
