#pragma once

#include <cstdint>

namespace gw::ai_runtime {

/// Hybrid AI Director: L4D-style state machine with bounded-RL parameter tuning.
/// See ADR-0097 and docs/06 §3.32.

enum class DirectorState : std::uint8_t {
    BuildUp        = 0,
    SustainedPeak  = 1,
    PeakFade       = 2,
    Relax          = 3,
};

struct DirectorPolicyInput {
    std::uint64_t logical_tick   = 0;
    float  player_health_ratio   = 1.f;
    float  recent_damage_taken   = 0.f;
    float  recent_kills_per_sec  = 0.f;
    float  combat_intensity_ewma = 0.f;
    float  sin_ratio             = 0.f;      // 0..1 normalized Sin
    float  grace_ratio           = 0.f;      // 0..1 normalized Grace
    float  time_in_state_sec     = 0.f;
    DirectorState current_state  = DirectorState::Relax;
};

struct DirectorPolicyOutput {
    DirectorState next_state          = DirectorState::Relax;
    float         spawn_rate_scale    = 1.f;
    float         item_drop_scale     = 1.f;
    float         intensity_target    = 0.f;
};

/// Pure function of `(input, seed)`. Rollback-safe. CPU-only.
/// Budget: ≤ 0.10 ms/frame on RX 580 (ADR-0097).
/// `seed` ties the random parameter-tuning branch to the world seed.
[[nodiscard]] DirectorPolicyOutput evaluate_director(const DirectorPolicyInput& input,
                                                     std::uint64_t              seed) noexcept;

inline constexpr float DIRECTOR_BUDGET_MS = 0.10f;

} // namespace gw::ai_runtime
