#pragma once
// engine/anim/events_animation.hpp — Phase 13 Wave 13A..F (ADR-0039, §5).
//
// Animation events travel through the Phase-11 EventBus (`engine/core/events/`).
// Each struct is POD; listeners register with `EventBus<T>::subscribe(...)`.

#include "engine/anim/anim_types.hpp"

#include <glm/vec3.hpp>

#include <cstdint>
#include <string>

namespace gw::events {

struct AnimationClipPlay {
    gw::anim::EntityId      entity{gw::anim::kEntityNone};
    gw::anim::InstanceHandle instance{};
    gw::anim::ClipHandle    clip{};
    std::uint64_t           clip_content_hash{0};
    float                   blend_in_s{0.0f};
    float                   speed{1.0f};
};

struct AnimationClipStop {
    gw::anim::EntityId       entity{gw::anim::kEntityNone};
    gw::anim::InstanceHandle instance{};
    gw::anim::ClipHandle     clip{};
    float                    blend_out_s{0.0f};
    bool                     completed{false};
};

struct AnimationStateEntered {
    gw::anim::EntityId       entity{gw::anim::kEntityNone};
    gw::anim::InstanceHandle instance{};
    std::string              state_name{};
};

struct AnimationStateExited {
    gw::anim::EntityId       entity{gw::anim::kEntityNone};
    gw::anim::InstanceHandle instance{};
    std::string              state_name{};
};

struct AnimationEvent {
    gw::anim::EntityId       entity{gw::anim::kEntityNone};
    gw::anim::InstanceHandle instance{};
    std::string              name{};              // e.g. "footstep.left"
    float                    clip_time_s{0.0f};
    float                    intensity{1.0f};
};

struct AnimationBlendCompleted {
    gw::anim::EntityId       entity{gw::anim::kEntityNone};
    gw::anim::InstanceHandle instance{};
    std::uint16_t            from_state{0};
    std::uint16_t            to_state{0};
};

struct AnimationIKApplied {
    gw::anim::EntityId       entity{gw::anim::kEntityNone};
    gw::anim::InstanceHandle instance{};
    std::uint16_t            chain_length{0};
    float                    residual_m{0.0f};
    std::uint16_t            iterations_used{0};
};

struct AnimationRagdollActivated {
    gw::anim::EntityId       entity{gw::anim::kEntityNone};
    gw::anim::InstanceHandle instance{};
    gw::anim::RagdollHandle  ragdoll{};
    glm::vec3                hit_point_ws{0.0f};
    glm::vec3                impulse_ns{0.0f};
};

} // namespace gw::events
