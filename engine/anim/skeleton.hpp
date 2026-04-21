#pragma once
// engine/anim/skeleton.hpp — Phase 13 Wave 13A (ADR-0039).
//
// A Skeleton is a flat joint list in topological order (parent before any
// child). The root joint's parent index is kInvalidJoint. Each joint has a
// rest-pose local TRS and a stable content hash (fnv1a64 of joint names +
// parents) folded into the SkeletonDesc so .kanim clips and runtime
// skeletons can be matched.

#include "engine/anim/anim_types.hpp"

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace gw::anim {

inline constexpr std::uint16_t kInvalidJoint = 0xFFFFu;

// -----------------------------------------------------------------------------
// Rest-pose TRS for a single joint.
// -----------------------------------------------------------------------------
struct JointRest {
    glm::vec3 translation{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
};

// -----------------------------------------------------------------------------
// Skeleton description. `parent[i]` must be < i (or kInvalidJoint for the
// root). `joint_names` is indexed identically. Both arrays are required to
// be the same length.
// -----------------------------------------------------------------------------
struct SkeletonDesc {
    std::string              name{};
    std::vector<std::string> joint_names{};
    std::vector<std::uint16_t> parents{};
    std::vector<JointRest>   rest_pose{};
    // Authoring content hash (fnv1a64 of names+parents). 0 means "compute
    // on create". This is what .kanim clips match against.
    std::uint64_t            content_hash{0};
};

// -----------------------------------------------------------------------------
// Inspection — returned by AnimationWorld::skeleton_info().
// -----------------------------------------------------------------------------
struct SkeletonInfo {
    std::uint32_t joint_count{0};
    std::uint64_t content_hash{0};
};

// Free function — FNV-1a-64 of the skeleton topology (joint names + parent
// indices + order). Determinism-critical; used by ACL-compressed clips to
// match a decoder against the skeleton it was compressed for.
[[nodiscard]] std::uint64_t skeleton_content_hash(const SkeletonDesc& desc) noexcept;

} // namespace gw::anim
