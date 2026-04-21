#pragma once
// engine/physics/ecs/physics_components.hpp — Phase 12 Wave 12A (ADR-0032).
//
// Aggregator header so gameplay code gets the whole ECS component catalogue
// in one pull. The systems themselves live next to this file.

#include "engine/physics/character_controller.hpp"
#include "engine/physics/collider.hpp"
#include "engine/physics/constraint.hpp"
#include "engine/physics/rigid_body.hpp"

namespace gw::physics {

// Trigger component — a sensor body paired with the per-entity trigger lifetime.
struct TriggerComponent {
    BodyHandle handle{};
    bool       dirty{true};
};

// Character component wraps a handle; state is pulled from the world each frame.
struct CharacterComponent {
    CharacterDesc   desc{};
    CharacterHandle handle{};
    CharacterState  last_state{};    // populated by CharacterSystem each tick
    bool            dirty{true};
};

} // namespace gw::physics
