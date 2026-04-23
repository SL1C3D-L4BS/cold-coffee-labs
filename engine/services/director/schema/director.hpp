#pragma once

#include <cstdint>

namespace gw::services::director {

enum class IntensityState : std::uint8_t {
    Relax         = 0,
    BuildUp       = 1,
    SustainedPeak = 2,
    PeakFade      = 3,
};

struct DirectorRequest {
    std::uint64_t  seed                 = 0;
    float          player_health_ratio  = 1.f;
    float          recent_damage_taken  = 0.f;
    float          recent_kps           = 0.f;
    float          intensity_ewma       = 0.f;
    float          normalized_sin       = 0.f;
    float          normalized_grace     = 0.f;
    float          time_in_state_s      = 0.f;
    IntensityState current_state        = IntensityState::Relax;
};

struct DirectorResult {
    IntensityState next_state        = IntensityState::Relax;
    float          spawn_rate_scale  = 1.f;
    float          item_drop_scale   = 1.f;
    float          intensity_target  = 0.f;
};

} // namespace gw::services::director
