#pragma once
// gameplay/movement/grapple.hpp — Phase 22 W147
//
// Deterministic grapple hook. Tap = projectile + tether + pull; hold on a
// small-class enemy = reel-in that triggers Glory Kill on impact.
//
// Pure scalar; the actual ray-cast against the GPTM collision mesh happens
// at a higher layer and is passed in as `target_point`.

#include "gameplay/movement/player_controller.hpp"

#include <cstdint>

namespace gw::gameplay::movement {

enum class GrappleMode : std::uint8_t {
    Idle     = 0,
    Flying   = 1, ///< tether extending
    Anchored = 2, ///< hooked on geometry; pulling player
    Reeling  = 3, ///< hooked on small enemy; pulling enemy in
    Broken   = 4, ///< overstretched or interrupted
};

struct GrappleState {
    GrappleMode mode          = GrappleMode::Idle;
    Vec3        anchor_point{}; ///< latched geometry/enemy point
    Vec3        origin_point{}; ///< where the tether started
    float       tether_length = 0.f;
    float       time_in_mode  = 0.f;
    bool        anchor_is_enemy = false;
};

struct GrappleInput {
    bool  tap        = false;   ///< single-press pull-to-point
    bool  hold       = false;   ///< held reel on enemy
    bool  hit        = false;   ///< ray-cast hit something within range
    bool  hit_enemy  = false;   ///< hit a small-class enemy
    Vec3  target_point{};
    float distance   = 0.f;
};

struct GrappleStepResult {
    Vec3  applied_velocity_delta{};
    bool  trigger_glory_kill = false;
    bool  broken_this_tick   = false;
};

[[nodiscard]] GrappleStepResult step_grapple(GrappleState&        state,
                                             const PlayerMotion&  motion,
                                             const GrappleInput&  input,
                                             const PlayerTuning&  tuning,
                                             float                dt_sec) noexcept;

} // namespace gw::gameplay::movement
