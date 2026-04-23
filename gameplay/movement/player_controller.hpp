#pragma once
// gameplay/movement/player_controller.hpp — Phase 22 W146
//
// Deterministic swept-AABB player controller for *Sacrilege*. Runs at fixed
// timestep against the GPTM collision mesh. No Jolt character controller —
// movement values are load-bearing gameplay and must reproduce exactly for
// speedrun splits and replays. See docs/07 §3.
//
// Three movement primitives are exposed as pure kernels, each pureley
// algebraic and independently testable:
//
//   1. apply_bunny_hop     — velocity boost on jump + airstrafe redirect
//   2. apply_slide         — momentum-preserving crouch slide
//   3. apply_wall_kick     — wall-normal impulse; one use per ground-reset
//
// The swept-AABB step is `step_player`, which composes the three primitives
// into a single fixed-dt advance.

#include <cstdint>

namespace gw::gameplay::movement {

struct Vec3 { float x = 0.f, y = 0.f, z = 0.f; };

/// Tuneables mirror the CVar block in docs/07 §3.
struct PlayerTuning {
    float bhop_multiplier      = 1.10f;
    float slide_friction       = 0.15f;
    float wall_kick_horizontal = 4.0f;
    float wall_kick_vertical   = 5.0f;
    float grapple_speed        = 15.0f;
    float grapple_range        = 20.0f;
    float surf_friction        = 0.02f;
    float air_control          = 0.75f;
    float gravity              = 25.0f;
    float base_move_speed      = 7.5f;
    float max_air_speed        = 30.f;
};

/// Per-tick input bundle. Axes are normalized unit vectors in [-1,1]²; the
/// controller does not assume any mouse or gamepad binding — the platform
/// layer hands us this POD.
struct PlayerInput {
    float  move_axis_x    = 0.f; // strafe
    float  move_axis_y    = 0.f; // forward
    bool   jump_pressed   = false;
    bool   crouch_held    = false;
    bool   grapple_tapped = false;
    bool   grapple_held   = false;
};

/// Persistent kinematic state. POD — rollback-safe.
struct PlayerMotion {
    Vec3  position{};
    Vec3  velocity{};
    bool  grounded            = false;
    bool  sliding             = false;
    bool  wall_kick_used      = false; ///< resets on ground contact
    bool  blood_surf          = false; ///< set by environment query
    float slope_degrees       = 0.f;   ///< current ground slope
    Vec3  wall_normal{};               ///< last touched wall normal (zero if none)
    float wall_contact_age    = 999.f; ///< seconds since wall touch
};

/// Environment sample returned by the collision mesh query.
struct EnvironmentSample {
    bool  ground_hit        = false;
    float ground_height_y   = 0.f;
    float ground_slope_deg  = 0.f;
    bool  wall_hit          = false;
    Vec3  wall_normal{};
    bool  on_blood_decal    = false;
};

/// Minimal swept-AABB result — `t_hit` is the fraction of dt at which we'd
/// collide with `plane_normal`. Used by the test rig to compose a tiny
/// scene without pulling the whole GPTM mesh into a unit test.
struct SweptAabbResult {
    bool  hit          = false;
    float t_hit        = 1.f;
    Vec3  plane_normal{};
};

// ---------------------------------------------------------------------------
// Movement primitives — pure functions over (tuning, input, state).
// ---------------------------------------------------------------------------

[[nodiscard]] Vec3 apply_bunny_hop(const PlayerMotion&  motion,
                                   const PlayerInput&   input,
                                   const PlayerTuning&  tuning,
                                   float                dt_sec) noexcept;

[[nodiscard]] Vec3 apply_slide(const PlayerMotion&  motion,
                               const PlayerInput&   input,
                               const PlayerTuning&  tuning,
                               float                dt_sec) noexcept;

[[nodiscard]] Vec3 apply_wall_kick(const PlayerMotion&  motion,
                                   const PlayerInput&   input,
                                   const PlayerTuning&  tuning) noexcept;

/// Scalar swept-AABB check against a single axis-aligned plane. Used as the
/// test oracle; the real controller folds an iterator of these.
[[nodiscard]] SweptAabbResult swept_aabb_vs_plane(Vec3  origin,
                                                  Vec3  delta,
                                                  Vec3  plane_point,
                                                  Vec3  plane_normal) noexcept;

/// One fixed-dt advance. Composes bhop/slide/wall-kick/gravity with the
/// env sample, clamps by swept-AABB, and writes back into `motion`.
void step_player(PlayerMotion&             motion,
                 const PlayerInput&        input,
                 const EnvironmentSample&  env,
                 const PlayerTuning&       tuning,
                 float                     dt_sec) noexcept;

// ---------------------------------------------------------------------------
// Mirror-Step (Act II unlockable) — docs/07 §2.6 reality-warp hook + voice.
// Fires when Sin ≥ 80 and the player inputs a slide-dash; returns true on
// activation so the caller can queue the VoiceDirector mirror-step line.
// ---------------------------------------------------------------------------
[[nodiscard]] bool try_mirror_step(float       sin_value,
                                   bool        slide_dash_input,
                                   bool        has_unlocked,
                                   bool        in_ruin) noexcept;

} // namespace gw::gameplay::movement
