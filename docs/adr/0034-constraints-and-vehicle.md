# ADR 0034 — Constraint catalogue + vehicle sub-system

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 12 Wave 12C)
- **Deciders:** Cold Coffee Labs engine + gameplay group
- **Relates:** ADR-0031 (facade), ADR-0032 (ECS bridge), ADR-0037 (determinism)

## Context

Jolt exposes a rich constraint catalogue (Fixed, Distance, Hinge, Slider, Cone, SixDOF, Path, Pulley, RackAndPinion, Gear, Point, Swing/Twist). The Phase-12 minimum surface is six primitives that cover 95% of gameplay needs: doors, pulleys, hinges, sliders, ragdoll joints, and ropes. The vehicle sub-system lands as a separately-gated sub-module so pure-walking games never pull vehicle cost.

## Decision

### 1. Constraint catalogue (Phase 12 minimum)

```cpp
namespace gw::physics {

struct FixedConstraintDesc {
    BodyHandle a{}, b{};
    glm::vec3  point_ws{0};              // world-space attach
    glm::quat  rotation_ws{1, 0, 0, 0};
};

struct DistanceConstraintDesc {
    BodyHandle a{}, b{};
    glm::vec3  point_a_ls{0};            // local-space attach
    glm::vec3  point_b_ls{0};
    float      rest_length_m{1.0f};
    float      stiffness_hz{0.0f};       // 0 = rigid
    float      damping_ratio{0.0f};
};

struct HingeConstraintDesc {
    BodyHandle a{}, b{};
    glm::vec3  point_ws{0};
    glm::vec3  axis_ws{0, 1, 0};
    float      limit_lo_rad{-3.1415927f};
    float      limit_hi_rad{+3.1415927f};
    float      motor_target_velocity_rps{0.0f};
    float      motor_max_torque_nm{0.0f};
};

struct SliderConstraintDesc {
    BodyHandle a{}, b{};
    glm::vec3  point_ws{0};
    glm::vec3  axis_ws{0, 0, 1};
    float      limit_lo_m{-1.0f};
    float      limit_hi_m{+1.0f};
    float      motor_target_velocity_mps{0.0f};
    float      motor_max_force_n{0.0f};
};

struct ConeConstraintDesc {
    BodyHandle a{}, b{};
    glm::vec3  point_ws{0};
    glm::vec3  twist_axis_ws{0, 0, 1};
    float      half_angle_rad{0.5f};     // ragdoll-joint swing limit
};

struct SixDOFConstraintDesc {
    BodyHandle a{}, b{};
    glm::vec3  point_ws{0};
    glm::quat  rotation_ws{1, 0, 0, 0};
    glm::vec3  linear_lo{0.0f}, linear_hi{0.0f};     // zero = locked axis
    glm::vec3  angular_lo_rad{0.0f}, angular_hi_rad{0.0f};
};

using ConstraintDesc = std::variant<
    FixedConstraintDesc,
    DistanceConstraintDesc,
    HingeConstraintDesc,
    SliderConstraintDesc,
    ConeConstraintDesc,
    SixDOFConstraintDesc>;

} // namespace gw::physics
```

The `PhysicsWorld` API is:

```cpp
ConstraintHandle add_constraint(const ConstraintDesc&, float break_impulse = 0.0f);
void             destroy_constraint(ConstraintHandle);
bool             get_constraint_broken(ConstraintHandle) const;
```

### 2. Break-force event

If `break_impulse > 0.0` and the resolved contact impulse exceeds that value during a substep, the constraint is destroyed and `events::ConstraintBroken{a, b, impulse}` is published on the `InFrameQueue<ConstraintBroken>`. The constraint handle is invalidated; subsequent `get_constraint_broken` returns `true`.

Unit test `hinge_breaks_on_impulse` asserts the event fires exactly once and the handle is invalid afterwards.

### 3. Vehicle — wheeled reference car

Gated behind `GW_PHYSICS_VEHICLE` (default **ON** under `physics-*`, **OFF** under `dev-*`). When off, the null backend provides stubs that return failure. When on, Jolt's `VehicleCollisionTester` + `WheeledVehicleController` back the API.

```cpp
struct WheelDesc {
    glm::vec3 attach_point_ls{0};
    glm::vec3 steering_axis_ls{0, 1, 0};
    glm::vec3 suspension_up_ls{0, 1, 0};
    glm::vec3 suspension_forward_ls{0, 0, 1};
    float     radius_m{0.30f};
    float     width_m{0.20f};
    float     suspension_min_m{0.0f};
    float     suspension_max_m{0.5f};
    float     suspension_stiffness_hz{1.5f};
    float     suspension_damping_ratio{0.5f};
    float     inertia_kgm2{0.9f};
    bool      drivable{true};
    bool      steerable{false};
};

struct VehicleDesc {
    BodyHandle chassis{};
    std::array<WheelDesc, 8> wheels{};
    std::uint8_t wheel_count{4};
    float max_engine_torque_nm{500.0f};
    float max_brake_torque_nm{1500.0f};
    float max_handbrake_torque_nm{4000.0f};
};

struct VehicleInput {
    float throttle{0};     // [-1, 1]
    float brake{0};        // [0, 1]
    float handbrake{0};    // [0, 1]
    float steer{0};        // [-1, 1]
};

class PhysicsWorld {
    VehicleHandle create_vehicle(const VehicleDesc&);
    void          destroy_vehicle(VehicleHandle);
    void          set_vehicle_input(VehicleHandle, const VehicleInput&);
    bool          wheel_on_ground(VehicleHandle, std::uint8_t wheel) const;
};
```

Unit test `vehicle_wheel_contact_classifier` drops a 4-wheel chassis onto a ground box → all four wheels report grounded after settle.

### 4. Soft-limit + motor events

Hinge + slider motors have `motor_target_velocity_*`. When the error exceeds `motor_max_*`, Jolt publishes a one-shot `ConstraintMotorSaturated{a, b}` on the `InFrameQueue<ConstraintBroken>` (same queue, different kind tag). Null backend: no-op.

## Consequences

- Ragdolls compose from SixDOF + Cone + Hinge primitives — Phase 13 rigs them from authoring data.
- Break-force events are the hook for Phase 22 destruction (walls snap).
- Vehicles are gated, so the `physics_sandbox` exit-demo must guard the vehicle scene behind `GW_PHYSICS_VEHICLE`.

## Alternatives considered

- **Exposing every Jolt constraint type.** Rejected — the six-primitive set covers Phases 12–22; we add more when content needs them.
- **Vehicle via custom rigid-body suspension.** Rejected — Jolt's wheeled vehicle is the stable reference; DIY re-invents a wheel.
- **Making vehicles always-on.** Rejected — Phase 13's *Living Scene* demo has no vehicles; the gate keeps that path cheap.
