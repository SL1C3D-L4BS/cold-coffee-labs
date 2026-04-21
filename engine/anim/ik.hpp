#pragma once
// engine/anim/ik.hpp — Phase 13 Wave 13D (ADR-0042).
//
// Deterministic inverse-kinematics solvers operating directly on gw::anim::Pose.
// Each solver:
//   * Runs in local-space then emits rotations that reproduce a requested
//     world-space effector position.
//   * Clamps iteration count — zero unbounded loops.
//   * Is bit-stable across platforms (no std::async / thread-pool, no
//     reductions over thread count; IEEE-754 operations only).

#include "engine/anim/anim_types.hpp"
#include "engine/anim/pose.hpp"
#include "engine/anim/skeleton.hpp"

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstdint>
#include <span>
#include <vector>

namespace gw::anim {

// -----------------------------------------------------------------------------
// TwoBoneIK — analytical solver for a 3-joint chain: upper/lower/end.
// -----------------------------------------------------------------------------
struct TwoBoneIKInput {
    std::uint16_t joint_upper{0};
    std::uint16_t joint_lower{0};
    std::uint16_t joint_end{0};

    glm::vec3     target_ws{0.0f};
    glm::vec3     pole_ws{0.0f, 0.0f, 1.0f};
    float         weight{1.0f};
    float         soften{0.0f};  // 0..1 — softens the knee near full extension
};

void solve_two_bone_ik(Pose& pose,
                       const SkeletonDesc& skel,
                       const TwoBoneIKInput& in) noexcept;

// -----------------------------------------------------------------------------
// FABRIK — N-link chain solver. `chain_joints` is ordered root-to-tip.
// -----------------------------------------------------------------------------
struct FabrikInput {
    std::span<const std::uint16_t> chain_joints{};
    glm::vec3                      target_ws{0.0f};
    float                          tolerance_m{1e-4f};
    std::uint16_t                  max_iterations{16};
    float                          weight{1.0f};
};

void solve_fabrik(Pose& pose,
                  const SkeletonDesc& skel,
                  const FabrikInput& in) noexcept;

// -----------------------------------------------------------------------------
// Cyclic Coordinate Descent. Fallback for chains where FABRIK diverges.
// -----------------------------------------------------------------------------
struct CcdInput {
    std::span<const std::uint16_t> chain_joints{}; // root-to-tip
    glm::vec3                      target_ws{0.0f};
    float                          tolerance_m{1e-4f};
    std::uint16_t                  max_iterations{16};
    float                          weight{1.0f};
    float                          per_joint_max_rot_rad{3.14159265f};
};

void solve_ccd(Pose& pose,
               const SkeletonDesc& skel,
               const CcdInput& in) noexcept;

} // namespace gw::anim
