#pragma once
// engine/physics/collider.hpp — Phase 12 Wave 12A (ADR-0031).
//
// Shape descriptors — POD-ish authoring structs. The backend creates an
// opaque ShapeHandle from one of these. glm vectors are the wire format;
// Jolt conversion happens inside `impl/jolt_bridge.cpp`.

#include "engine/physics/physics_types.hpp"

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstdint>
#include <variant>
#include <span>

namespace gw::physics {

struct BoxShapeDesc {
    glm::vec3 half_extents_m{0.5f, 0.5f, 0.5f};
    float     convex_radius_m{0.02f};        // small rounding for numerical robustness
};

struct SphereShapeDesc {
    float radius_m{0.5f};
};

struct CapsuleShapeDesc {
    float radius_m{0.25f};
    float half_height_m{0.5f};               // axis = Y
};

struct CylinderShapeDesc {
    float radius_m{0.25f};
    float half_height_m{0.5f};
    float convex_radius_m{0.02f};
};

// Convex hull — up to 256 points. Backend runs QuickHull + decimates.
struct ConvexHullShapeDesc {
    std::span<const glm::vec3> points{};
    float convex_radius_m{0.02f};
};

// Triangle mesh — immutable static geometry. Intended for `ObjectLayer::Static`.
struct TriangleMeshShapeDesc {
    std::span<const glm::vec3>     vertices{};
    std::span<const std::uint32_t> indices{};   // triplet per triangle
};

// Compound — child shapes at local offsets. Useful for tools like the
// character's sensor capsule clipped to a weapon hitbox.
struct CompoundChild {
    ShapeHandle shape{};
    glm::vec3   local_offset{0.0f};
    glm::quat   local_rotation{1.0f, 0.0f, 0.0f, 0.0f};
};

struct CompoundShapeDesc {
    std::span<const CompoundChild> children{};
};

using ShapeDesc = std::variant<
    BoxShapeDesc,
    SphereShapeDesc,
    CapsuleShapeDesc,
    CylinderShapeDesc,
    ConvexHullShapeDesc,
    TriangleMeshShapeDesc,
    CompoundShapeDesc>;

} // namespace gw::physics
