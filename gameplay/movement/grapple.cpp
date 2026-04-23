// gameplay/movement/grapple.cpp — Phase 22 W147

#include "gameplay/movement/grapple.hpp"

#include <cmath>

namespace gw::gameplay::movement {

namespace {

constexpr float kFloatEps = 1.0e-6f;

[[nodiscard]] Vec3  sub(Vec3 a, Vec3 b) noexcept { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
[[nodiscard]] Vec3  scale(Vec3 a, float k) noexcept { return {a.x * k, a.y * k, a.z * k}; }
[[nodiscard]] float dot(Vec3 a, Vec3 b) noexcept { return a.x * b.x + a.y * b.y + a.z * b.z; }
[[nodiscard]] float length(Vec3 a) noexcept { return std::sqrt(dot(a, a)); }
[[nodiscard]] Vec3  norm(Vec3 a) noexcept {
    const float l = length(a);
    return l > kFloatEps ? scale(a, 1.f / l) : Vec3{};
}

} // namespace

GrappleStepResult step_grapple(GrappleState&       state,
                               const PlayerMotion& motion,
                               const GrappleInput& input,
                               const PlayerTuning& tuning,
                               float               dt_sec) noexcept {
    GrappleStepResult result{};
    state.time_in_mode += dt_sec;

    switch (state.mode) {
        case GrappleMode::Idle: {
            if (!input.hit || input.distance > tuning.grapple_range) {
                return result;
            }
            if (input.tap || input.hold) {
                state.mode            = input.hit_enemy && input.hold ? GrappleMode::Reeling
                                                                      : GrappleMode::Flying;
                state.origin_point    = motion.position;
                state.anchor_point    = input.target_point;
                state.anchor_is_enemy = input.hit_enemy;
                state.tether_length   = length(sub(state.anchor_point, motion.position));
                state.time_in_mode    = 0.f;
            }
            return result;
        }
        case GrappleMode::Flying: {
            const Vec3 toward = sub(state.anchor_point, motion.position);
            const float d = length(toward);
            if (d < 1.5f) {
                state.mode = GrappleMode::Anchored;
            } else if (d > tuning.grapple_range * 1.5f) {
                state.mode            = GrappleMode::Broken;
                result.broken_this_tick = true;
            } else {
                result.applied_velocity_delta = scale(norm(toward), tuning.grapple_speed);
            }
            return result;
        }
        case GrappleMode::Anchored: {
            const Vec3 toward = sub(state.anchor_point, motion.position);
            const float d = length(toward);
            if (d < 0.5f || state.time_in_mode > 1.5f) {
                state.mode = GrappleMode::Idle;
                return result;
            }
            result.applied_velocity_delta = scale(norm(toward), tuning.grapple_speed);
            return result;
        }
        case GrappleMode::Reeling: {
            const Vec3 toward = sub(state.anchor_point, motion.position);
            const float d = length(toward);
            if (d < 2.0f) {
                state.mode                = GrappleMode::Idle;
                result.trigger_glory_kill = true;
            } else if (!input.hold || d > tuning.grapple_range) {
                state.mode            = GrappleMode::Broken;
                result.broken_this_tick = true;
            }
            return result;
        }
        case GrappleMode::Broken: {
            if (state.time_in_mode > 0.25f) {
                state.mode = GrappleMode::Idle;
            }
            return result;
        }
    }
    return result;
}

} // namespace gw::gameplay::movement
