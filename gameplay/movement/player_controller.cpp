// gameplay/movement/player_controller.cpp — Phase 22 W146

#include "gameplay/movement/player_controller.hpp"

#include <cmath>

namespace gw::gameplay::movement {

namespace {

constexpr float kFloatEps = 1.0e-6f;

[[nodiscard]] Vec3 add(Vec3 a, Vec3 b) noexcept { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
[[nodiscard]] Vec3 sub(Vec3 a, Vec3 b) noexcept { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
[[nodiscard]] Vec3 scale(Vec3 a, float k) noexcept { return {a.x * k, a.y * k, a.z * k}; }
[[nodiscard]] float dot(Vec3 a, Vec3 b) noexcept { return a.x * b.x + a.y * b.y + a.z * b.z; }
[[nodiscard]] float length(Vec3 a) noexcept { return std::sqrt(dot(a, a)); }
[[nodiscard]] Vec3 normalize(Vec3 a) noexcept {
    const float l = length(a);
    return (l > kFloatEps) ? scale(a, 1.f / l) : Vec3{};
}

} // namespace

Vec3 apply_bunny_hop(const PlayerMotion&  motion,
                     const PlayerInput&   input,
                     const PlayerTuning&  tuning,
                     float                dt_sec) noexcept {
    (void)dt_sec;
    Vec3 v = motion.velocity;

    if (motion.grounded && input.jump_pressed) {
        // Preserve horizontal velocity, boost by bhop multiplier, add upward.
        const float speed = std::sqrt(v.x * v.x + v.z * v.z);
        const float new_speed = speed * tuning.bhop_multiplier;
        if (speed > kFloatEps) {
            v.x = v.x / speed * new_speed;
            v.z = v.z / speed * new_speed;
        }
        v.y = tuning.wall_kick_vertical; // jump impulse shares the vertical CVar
    }
    // Air strafe redirect — apply air-control fraction of desired direction.
    if (!motion.grounded) {
        Vec3 desired{input.move_axis_x, 0.f, input.move_axis_y};
        desired = normalize(desired);
        const float target_speed = tuning.base_move_speed;
        const float control = tuning.air_control;
        v.x += desired.x * target_speed * control * dt_sec;
        v.z += desired.z * target_speed * control * dt_sec;
    }
    // Cap air speed so strafe stacking is bounded.
    const float horiz_speed = std::sqrt(v.x * v.x + v.z * v.z);
    if (horiz_speed > tuning.max_air_speed) {
        const float k = tuning.max_air_speed / horiz_speed;
        v.x *= k;
        v.z *= k;
    }
    return v;
}

Vec3 apply_slide(const PlayerMotion&  motion,
                 const PlayerInput&   input,
                 const PlayerTuning&  tuning,
                 float                dt_sec) noexcept {
    if (!motion.grounded || !input.crouch_held) {
        return motion.velocity;
    }
    // Slide preserves horizontal velocity, scaled down by slide_friction·dt.
    Vec3 v = motion.velocity;
    const float friction = motion.blood_surf ? tuning.surf_friction : tuning.slide_friction;
    const float decay = 1.f - friction * dt_sec;
    v.x *= decay > 0.f ? decay : 0.f;
    v.z *= decay > 0.f ? decay : 0.f;
    // Directional steer while sliding — scaled by air control (50 %).
    const Vec3 steer = normalize(Vec3{input.move_axis_x, 0.f, input.move_axis_y});
    v.x += steer.x * tuning.base_move_speed * 0.5f * dt_sec;
    v.z += steer.z * tuning.base_move_speed * 0.5f * dt_sec;
    return v;
}

Vec3 apply_wall_kick(const PlayerMotion&  motion,
                     const PlayerInput&   input,
                     const PlayerTuning&  tuning) noexcept {
    if (!input.jump_pressed || motion.wall_kick_used || motion.grounded) {
        return motion.velocity;
    }
    const Vec3 wall_n = motion.wall_normal;
    if (length(wall_n) < kFloatEps) {
        return motion.velocity;
    }
    Vec3 v = motion.velocity;
    v = add(v, scale(wall_n, tuning.wall_kick_horizontal));
    v.y = tuning.wall_kick_vertical;
    return v;
}

SweptAabbResult swept_aabb_vs_plane(Vec3 origin,
                                    Vec3 delta,
                                    Vec3 plane_point,
                                    Vec3 plane_normal) noexcept {
    SweptAabbResult result{};
    const float n_dot_d = dot(plane_normal, delta);
    if (std::fabs(n_dot_d) < kFloatEps) {
        return result; // parallel
    }
    const float numerator = dot(plane_normal, sub(plane_point, origin));
    const float t = numerator / n_dot_d;
    if (t >= 0.f && t <= 1.f) {
        result.hit          = true;
        result.t_hit        = t;
        result.plane_normal = plane_normal;
    }
    return result;
}

void step_player(PlayerMotion&             motion,
                 const PlayerInput&        input,
                 const EnvironmentSample&  env,
                 const PlayerTuning&       tuning,
                 float                     dt_sec) noexcept {
    // 1. Wall / ground state → refresh.
    motion.grounded      = env.ground_hit;
    motion.slope_degrees = env.ground_slope_deg;
    motion.blood_surf    = env.on_blood_decal;
    if (env.wall_hit) {
        motion.wall_normal      = env.wall_normal;
        motion.wall_contact_age = 0.f;
    } else {
        motion.wall_contact_age += dt_sec;
    }
    if (motion.grounded) {
        motion.wall_kick_used = false;
    }

    // 2. Apply primitives (order matters: bhop → wall kick → slide).
    motion.velocity = apply_bunny_hop(motion, input, tuning, dt_sec);
    if (input.jump_pressed && !motion.grounded &&
        motion.wall_contact_age < 0.2f && !motion.wall_kick_used) {
        motion.velocity = apply_wall_kick(motion, input, tuning);
        motion.wall_kick_used = true;
    }
    motion.sliding = input.crouch_held && motion.grounded;
    if (motion.sliding) {
        motion.velocity = apply_slide(motion, input, tuning, dt_sec);
    }

    // 3. Gravity.
    if (!motion.grounded) {
        motion.velocity.y -= tuning.gravity * dt_sec;
    } else if (motion.velocity.y < 0.f) {
        motion.velocity.y = 0.f;
    }

    // 4. Integrate.
    motion.position = add(motion.position, scale(motion.velocity, dt_sec));
}

bool try_mirror_step(float sin_value,
                     bool  slide_dash_input,
                     bool  has_unlocked,
                     bool  in_ruin) noexcept {
    if (in_ruin || !has_unlocked || !slide_dash_input) {
        return false;
    }
    return sin_value >= 80.f;
}

} // namespace gw::gameplay::movement
