#pragma once
// engine/physics/constraint.hpp — Phase 12 Wave 12C (ADR-0034).
//
// Constraint authoring descriptors. The backend materializes a
// ConstraintHandle from any of these variants. The vehicle path is
// its own descriptor gated behind `GW_PHYSICS_VEHICLE`.

#include "engine/physics/physics_types.hpp"

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include <array>
#include <cstdint>
#include <variant>

namespace gw::physics {

struct FixedConstraintDesc {
    BodyHandle a{};
    BodyHandle b{};
    glm::vec3  anchor_ws{0.0f};
};

struct DistanceConstraintDesc {
    BodyHandle a{};
    BodyHandle b{};
    glm::vec3  anchor_a_ls{0.0f};
    glm::vec3  anchor_b_ls{0.0f};
    float      rest_length_m{1.0f};
    float      stiffness{0.0f};    // 0 => infinitely stiff (position-only)
    float      damping{0.0f};
    float      break_impulse{0.0f}; // 0 => unbreakable
};

struct HingeConstraintDesc {
    BodyHandle a{};
    BodyHandle b{};
    glm::vec3  anchor_ws{0.0f};
    glm::vec3  axis_ws{0.0f, 1.0f, 0.0f};
    float      min_angle_rad{-3.14159265f};
    float      max_angle_rad{ 3.14159265f};
    float      motor_target_rads{0.0f};
    float      motor_max_torque{0.0f};
    float      break_impulse{0.0f};
};

struct SliderConstraintDesc {
    BodyHandle a{};
    BodyHandle b{};
    glm::vec3  anchor_ws{0.0f};
    glm::vec3  axis_ws{1.0f, 0.0f, 0.0f};
    float      min_m{-1.0f};
    float      max_m{ 1.0f};
    float      break_impulse{0.0f};
};

struct ConeConstraintDesc {
    BodyHandle a{};
    BodyHandle b{};
    glm::vec3  anchor_ws{0.0f};
    glm::vec3  twist_axis_ws{0.0f, 1.0f, 0.0f};
    float      half_cone_angle_rad{0.5f};
    float      break_impulse{0.0f};
};

struct SixDOFConstraintDesc {
    BodyHandle a{};
    BodyHandle b{};
    glm::vec3  anchor_ws{0.0f};
    std::array<float, 3> linear_min_m{-0.0f, -0.0f, -0.0f};
    std::array<float, 3> linear_max_m{ 0.0f,  0.0f,  0.0f};
    std::array<float, 3> angular_min_rad{-3.14159265f, -3.14159265f, -3.14159265f};
    std::array<float, 3> angular_max_rad{ 3.14159265f,  3.14159265f,  3.14159265f};
    float      break_impulse{0.0f};
};

using ConstraintDesc = std::variant<
    FixedConstraintDesc,
    DistanceConstraintDesc,
    HingeConstraintDesc,
    SliderConstraintDesc,
    ConeConstraintDesc,
    SixDOFConstraintDesc>;

// -----------------------------------------------------------------------------
// Vehicle — a wheeled-vehicle sub-system. Gated behind `GW_PHYSICS_VEHICLE`.
// -----------------------------------------------------------------------------
struct VehicleWheelDesc {
    glm::vec3 position_ls{0.0f};
    glm::vec3 suspension_axis_ls{0.0f, -1.0f, 0.0f};
    glm::vec3 steering_axis_ls{0.0f, 1.0f, 0.0f};
    float     radius_m{0.3f};
    float     width_m{0.2f};
    float     suspension_min_m{0.0f};
    float     suspension_max_m{0.3f};
    float     max_steer_angle_rad{0.7f};
    float     max_brake_torque{2000.0f};
    float     max_drive_torque{1000.0f};
    bool      driven{true};
    bool      steered{true};
};

struct VehicleDesc {
    BodyHandle chassis{};
    std::span<const VehicleWheelDesc> wheels{};
    float      max_pitch_roll_rad{1.1f};
    float      engine_power_watts{150000.0f};
    float      rear_wheel_drive{true};
};

struct VehicleInput {
    float forward{0.0f};    // -1..+1
    float steer{0.0f};      // -1..+1
    float brake{0.0f};      // 0..1
    float handbrake{0.0f};  // 0..1
};

// ECS component.
struct PhysicsConstraintComponent {
    ConstraintDesc      desc{};
    ConstraintHandle    handle{};
    bool                dirty{true};
};

} // namespace gw::physics
