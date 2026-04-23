// gameplay/movement/blood_surf.cpp — Phase 22 W147

#include "gameplay/movement/blood_surf.hpp"

namespace gw::gameplay::movement {

BloodSurfResult apply_blood_surf(Vec3                velocity_in,
                                 const BloodSurfQuery& q,
                                 const PlayerTuning&  tuning,
                                 float                dt_sec) noexcept {
    BloodSurfResult out{};
    const bool slope_gate = q.slope_degrees > 15.f;
    const bool decal_gate = q.on_blood_decal;
    out.surf_active = slope_gate || decal_gate;
    out.effective_friction = out.surf_active ? tuning.surf_friction
                                             : tuning.slide_friction;
    const float decay = 1.f - out.effective_friction * dt_sec;
    const float safe_decay = decay > 0.f ? decay : 0.f;
    out.post_friction_velocity.x = velocity_in.x * safe_decay;
    out.post_friction_velocity.y = velocity_in.y; // vertical preserved
    out.post_friction_velocity.z = velocity_in.z * safe_decay;
    return out;
}

} // namespace gw::gameplay::movement
