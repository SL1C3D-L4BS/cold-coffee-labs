#pragma once
// engine/anim/pose.hpp — Phase 13 Wave 13A (ADR-0039).
//
// A Pose is a dense local-space TRS array sized to a skeleton's joint
// count. Poses are the canonical exchange value between clip sampler, blend
// tree, IK solver, morph driver, and physics ragdoll bridge.

#include "engine/anim/anim_types.hpp"

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace gw::anim {

// -----------------------------------------------------------------------------
// Per-joint local TRS. Runtime uses a flat SoA layout on hot paths; the
// public struct is AoS so a callee can pass a single Pose{} to the IK /
// morph APIs without manual splicing.
// -----------------------------------------------------------------------------
struct JointTransform {
    glm::vec3 translation{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
};

// -----------------------------------------------------------------------------
// Pose — owning value type. `resize` is the only allocating API; all hot
// paths write into a pre-sized pose.
// -----------------------------------------------------------------------------
class Pose {
public:
    Pose() = default;
    explicit Pose(std::size_t joint_count) { joints_.resize(joint_count); }

    void resize(std::size_t joint_count) { joints_.resize(joint_count); }
    void clear() noexcept { joints_.clear(); }

    [[nodiscard]] std::size_t size() const noexcept { return joints_.size(); }
    [[nodiscard]] bool empty() const noexcept { return joints_.empty(); }

    [[nodiscard]] JointTransform&       operator[](std::size_t i)       noexcept { return joints_[i]; }
    [[nodiscard]] const JointTransform& operator[](std::size_t i) const noexcept { return joints_[i]; }

    [[nodiscard]] std::span<JointTransform>       joints()       noexcept { return joints_; }
    [[nodiscard]] std::span<const JointTransform> joints() const noexcept { return joints_; }

    // Reset to identity TRS on every joint (scale = 1, rot = identity).
    void reset_identity() noexcept;

private:
    std::vector<JointTransform> joints_{};
};

// -----------------------------------------------------------------------------
// Blend / copy helpers — deterministic across platforms (no reductions
// whose order depends on thread count).
// -----------------------------------------------------------------------------

// Copy the rest-pose TRS into `out`. Sizes must match.
void pose_set_from_rest(Pose& out, std::span<const struct JointRest> rest) noexcept;

// Deterministic LERP + NLERP blend: out = mix(a, b, w). Sizes must match.
void pose_lerp(Pose& out, const Pose& a, const Pose& b, float w) noexcept;

// Additive blend: out = a + w * (b - rest). Useful for overlay layers.
void pose_add(Pose& out, const Pose& a, const Pose& b,
              std::span<const struct JointRest> rest, float w) noexcept;

// Per-joint mask blend: out = mix(a, b, w * mask[i]). `mask.size()` must
// equal the pose size; values are clamped to [0, 1].
void pose_mask_lerp(Pose& out, const Pose& a, const Pose& b, float w,
                    std::span<const float> mask) noexcept;

// Canonical FNV-1a-64 over (quantized translation + canonical quaternion +
// quantized scale) — matches ADR-0039 §6 pose_hash contract. Mirrors the
// physics canonicalization rules.
struct PoseHashOptions {
    float translation_epsilon_m{1e-5f};
    float scale_epsilon{1e-5f};
    bool  include_scale{true};
};
[[nodiscard]] std::uint64_t pose_hash(const Pose& pose,
                                      const PoseHashOptions& opt = {}) noexcept;

} // namespace gw::anim
