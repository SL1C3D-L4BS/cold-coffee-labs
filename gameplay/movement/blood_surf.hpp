#pragma once
// gameplay/movement/blood_surf.hpp — Phase 22 W147
//
// Blood-surf: friction = 0.02 on slopes > 15° or on blood-decal cells
// (docs/07 §3). Pure scalar kernel — no ECS — so the movement test can
// sweep the slope/decal parameter space without a decal ring buffer.

#include "gameplay/movement/player_controller.hpp"

namespace gw::gameplay::movement {

struct BloodSurfQuery {
    float  slope_degrees   = 0.f;
    bool   on_blood_decal  = false;
};

struct BloodSurfResult {
    bool   surf_active   = false;
    float  effective_friction = 0.f;
    Vec3   post_friction_velocity{};
};

/// Deterministic: returns whether blood-surf is active this tick and the new
/// velocity after applying surf friction.
[[nodiscard]] BloodSurfResult apply_blood_surf(Vec3                velocity_in,
                                               const BloodSurfQuery& q,
                                               const PlayerTuning&  tuning,
                                               float                dt_sec) noexcept;

} // namespace gw::gameplay::movement
