#pragma once
// engine/physics/character_controller.hpp — Phase 12 Wave 12B (ADR-0033).
//
// Capsule-based character controller. Backend wraps Jolt's CharacterVirtual
// in the physics build; the null backend performs semi-physical integration
// sufficient for deterministic tests (slope classification, coyote-time).

#include "engine/physics/physics_types.hpp"

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstdint>

namespace gw::physics {

struct CharacterDesc {
    glm::dvec3  position_ws{0.0};
    float       capsule_radius_m{0.35f};
    float       capsule_half_height_m{0.8f};    // centre-line half height (axis = Y)
    float       mass_kg{80.0f};

    float       slope_max_angle_deg{50.0f};
    float       step_up_max_m{0.30f};
    float       step_down_max_m{0.40f};
    float       gravity_scale{1.0f};
    float       jump_velocity{5.0f};
    float       coyote_time_s{0.12f};

    ObjectLayer layer{ObjectLayer::Character};
    EntityId    entity{kEntityNone};
};

struct CharacterInput {
    glm::vec3  move_velocity_ws{0.0f};    // horizontal velocity target (m/s)
    bool       jump_pressed{false};
    bool       crouch_held{false};
};

struct CharacterState {
    glm::dvec3 position_ws{0.0};
    glm::quat  rotation_ws{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3  velocity{0.0f};
    glm::vec3  ground_normal{0.0f, 1.0f, 0.0f};
    std::uint32_t ground_surface_id{0};
    bool       on_ground{false};
    bool       in_coyote_window{false};
    bool       crouching{false};
};

} // namespace gw::physics
