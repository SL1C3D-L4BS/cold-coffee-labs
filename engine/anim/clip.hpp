#pragma once
// engine/anim/clip.hpp — Phase 13 Wave 13B (ADR-0040 .kanim format).
//
// A Clip is a deterministic sampler over keyframed joint tracks. The null
// backend (always on) stores uncompressed keyframes and samples via
// linear interpolation between the two bracketing frames. The Ozz/ACL
// backend (living-* preset) replaces the sampler with the ACL uniform
// decoder but presents an identical public API.

#include "engine/anim/anim_types.hpp"
#include "engine/anim/pose.hpp"

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace gw::anim {

// -----------------------------------------------------------------------------
// Raw keyframe tracks for the null backend. Keyframes are globally sampled
// at `duration_s / (key_count - 1)` spacing so `sample()` is O(1).
// -----------------------------------------------------------------------------
struct ClipTrack {
    std::uint16_t             joint_index{0};
    std::vector<glm::vec3>    translations;   // size == key_count
    std::vector<glm::quat>    rotations;      // size == key_count
    std::vector<glm::vec3>    scales;         // size == key_count
};

// -----------------------------------------------------------------------------
// Clip description.
// -----------------------------------------------------------------------------
struct ClipDesc {
    std::string               name{};
    std::uint32_t             key_count{1};      // >= 1
    float                     duration_s{0.0f};
    bool                      looping{true};
    SkeletonHandle            skeleton{};         // skeleton the clip targets
    std::uint64_t             target_skeleton_hash{0}; // authoritative match
    std::vector<ClipTrack>    tracks{};
    // Optional root-motion track (world-space delta per frame). If empty,
    // root motion is implicit in the joint-0 translation track.
    std::vector<glm::vec3>    root_motion_translation{};
    std::vector<glm::quat>    root_motion_rotation{};
};

// -----------------------------------------------------------------------------
// Sampler output — ADR-0039 §4 contract.
// -----------------------------------------------------------------------------
struct ClipSample {
    Pose           pose{};              // local-space TRS, joint_count wide
    glm::vec3      root_motion_dp{0.0f};// delta translation since last sample
    glm::quat      root_motion_dq{1.0f, 0.0f, 0.0f, 0.0f};
};

// -----------------------------------------------------------------------------
// Clip content hash — determinism-critical. Folds name, key count,
// duration, looping flag, and a byte-for-byte hash of every track over all
// keyframes. Used by the .kanim cook pipeline to short-circuit recompile.
// -----------------------------------------------------------------------------
[[nodiscard]] std::uint64_t clip_content_hash(const ClipDesc& desc) noexcept;

} // namespace gw::anim
