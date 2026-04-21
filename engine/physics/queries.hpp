#pragma once
// engine/physics/queries.hpp — Phase 12 Wave 12D (ADR-0035).
//
// Physics query types — raycast, shape sweep, overlap. Single-shot + batched.

#include "engine/physics/physics_types.hpp"
#include "engine/physics/collider.hpp"

#include <glm/vec3.hpp>

#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace gw::physics {

struct QueryFilter {
    LayerMask layer_mask{kLayerMaskAll};
    BodyHandle exclude{};                    // a single body excluded from hits (self)

    // Optional gameplay predicate — evaluated on the hit body's entity.
    // If set and returns `false`, the hit is rejected. Must be deterministic
    // (no global state, no RNG).
    std::function<bool(EntityId)> predicate{};
};

struct RaycastHit {
    BodyHandle body{};
    EntityId   entity{kEntityNone};
    glm::vec3  point_ws{0.0f};
    glm::vec3  normal{0.0f, 1.0f, 0.0f};
    float      fraction{0.0f};               // 0..1 along ray
    std::uint32_t surface_id{0};
};

struct ShapeQueryHit {
    BodyHandle body{};
    EntityId   entity{kEntityNone};
    glm::vec3  point_ws{0.0f};
    glm::vec3  normal{0.0f, 1.0f, 0.0f};
    float      penetration_depth{0.0f};
};

struct RaycastInput {
    glm::vec3 origin_ws{0.0f};
    glm::vec3 direction_ws{0.0f, 0.0f, -1.0f};
    float     max_distance_m{100.0f};
};

struct ShapeSweepInput {
    ShapeHandle shape{};
    glm::vec3   origin_ws{0.0f};
    glm::vec3   direction_ws{0.0f, -1.0f, 0.0f};
    float       max_distance_m{1.0f};
};

struct OverlapInput {
    ShapeHandle shape{};
    glm::vec3   position_ws{0.0f};
};

// Batch surface — the caller provides parallel input + output buffers.
// The backend is free to submit to engine/jobs; results map 1:1 to inputs.
struct RaycastBatch {
    std::span<const RaycastInput>   inputs;
    std::span<RaycastHit>           hits;          // size == inputs.size()
    std::span<std::uint8_t>         hit_flags;     // size == inputs.size(); 0 = miss, 1 = hit
};

} // namespace gw::physics
