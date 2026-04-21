#pragma once
// engine/physics/events_physics.hpp — Phase 12 Wave 12A..F (ADR-0031).
//
// Physics events travel through the Phase-11 EventBus (`engine/core/events/`).
// Each struct is POD; listeners register with `EventBus<T>::subscribe(...)`.

#include "engine/physics/physics_types.hpp"

#include <glm/vec3.hpp>

#include <cstdint>

namespace gw::events {

struct PhysicsContactAdded {
    gw::physics::EntityId a{gw::physics::kEntityNone};
    gw::physics::EntityId b{gw::physics::kEntityNone};
    glm::vec3             point_ws{0.0f};
    glm::vec3             normal{0.0f, 1.0f, 0.0f};
    float                 impulse{0.0f};
};

struct PhysicsContactPersisted {
    gw::physics::EntityId a{gw::physics::kEntityNone};
    gw::physics::EntityId b{gw::physics::kEntityNone};
    glm::vec3             point_ws{0.0f};
    glm::vec3             normal{0.0f, 1.0f, 0.0f};
};

struct PhysicsContactRemoved {
    gw::physics::EntityId a{gw::physics::kEntityNone};
    gw::physics::EntityId b{gw::physics::kEntityNone};
};

struct TriggerEntered {
    gw::physics::EntityId trigger{gw::physics::kEntityNone};
    gw::physics::EntityId other  {gw::physics::kEntityNone};
};

struct TriggerExited {
    gw::physics::EntityId trigger{gw::physics::kEntityNone};
    gw::physics::EntityId other  {gw::physics::kEntityNone};
};

struct CharacterGrounded {
    gw::physics::EntityId character{gw::physics::kEntityNone};
    glm::vec3             normal{0.0f, 1.0f, 0.0f};
    std::uint32_t         surface_id{0};
};

struct ConstraintBroken {
    gw::physics::EntityId a{gw::physics::kEntityNone};
    gw::physics::EntityId b{gw::physics::kEntityNone};
    float                 impulse{0.0f};
};

} // namespace gw::events
