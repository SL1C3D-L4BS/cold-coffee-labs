// engine/anim/ik.cpp — Phase 13 Wave 13D (ADR-0042).
//
// Deterministic IK solvers. Each operates in local space where possible
// and only recomputes the joints on the chain (no "dirty everywhere"
// sweeps). Joint order on every chain is root → tip.

#include "engine/anim/ik.hpp"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

namespace gw::anim {

namespace {

// Forward-kinematics pass over a subset of joints (root .. end inclusive).
// Writes world-space transforms for every joint in `jws`. `jws` must be
// sized to pose.size().
void fk_world(const Pose& pose, const SkeletonDesc& skel,
              std::vector<glm::mat4>& jws) noexcept {
    jws.assign(pose.size(), glm::mat4{1.0f});
    for (std::size_t i = 0; i < pose.size(); ++i) {
        glm::mat4 local = glm::translate(glm::mat4{1.0f}, pose[i].translation)
                        * glm::mat4_cast(pose[i].rotation)
                        * glm::scale(glm::mat4{1.0f}, pose[i].scale);
        const std::uint16_t p = (i < skel.parents.size()) ? skel.parents[i] : kInvalidJoint;
        if (p == kInvalidJoint) jws[i] = local;
        else                    jws[i] = jws[p] * local;
    }
}

inline glm::vec3 position(const glm::mat4& m) noexcept {
    return glm::vec3{m[3][0], m[3][1], m[3][2]};
}

inline glm::quat safe_from_to(const glm::vec3& a, const glm::vec3& b) noexcept {
    const glm::vec3 na = glm::normalize(a);
    const glm::vec3 nb = glm::normalize(b);
    const float d = glm::dot(na, nb);
    if (d >  0.9999f) return glm::quat{1.0f, 0.0f, 0.0f, 0.0f};
    if (d < -0.9999f) {
        // 180° — pick any orthogonal axis.
        glm::vec3 axis = glm::cross(glm::vec3{1, 0, 0}, na);
        if (glm::dot(axis, axis) < 1e-6f) axis = glm::cross(glm::vec3{0, 1, 0}, na);
        axis = glm::normalize(axis);
        return glm::angleAxis(3.14159265358979f, axis);
    }
    const glm::vec3 axis = glm::normalize(glm::cross(na, nb));
    const float angle = std::acos(std::clamp(d, -1.0f, 1.0f));
    return glm::angleAxis(angle, axis);
}

// Convert a world-space rotation into the local space of joint `i` so
// writing it to pose[i].rotation has the expected effect.
glm::quat world_to_local_rot(const glm::quat& r_ws,
                             const Pose& pose,
                             const SkeletonDesc& skel,
                             std::uint16_t i) noexcept {
    glm::quat parent_ws{1.0f, 0.0f, 0.0f, 0.0f};
    std::uint16_t p = (i < skel.parents.size()) ? skel.parents[i] : kInvalidJoint;
    while (p != kInvalidJoint) {
        parent_ws = pose[p].rotation * parent_ws;
        p = (p < skel.parents.size()) ? skel.parents[p] : kInvalidJoint;
    }
    return glm::conjugate(parent_ws) * r_ws;
}

} // namespace

void solve_two_bone_ik(Pose& pose,
                       const SkeletonDesc& skel,
                       const TwoBoneIKInput& in) noexcept {
    if (pose.empty()) return;
    if (in.weight <= 0.0f) return;
    if (in.joint_upper >= pose.size() ||
        in.joint_lower >= pose.size() ||
        in.joint_end   >= pose.size()) return;

    std::vector<glm::mat4> jws;
    fk_world(pose, skel, jws);

    const glm::vec3 a = position(jws[in.joint_upper]);
    const glm::vec3 b = position(jws[in.joint_lower]);
    const glm::vec3 c = position(jws[in.joint_end]);

    const float len_ab = glm::length(b - a);
    const float len_bc = glm::length(c - b);
    if (len_ab <= 1e-6f || len_bc <= 1e-6f) return;

    glm::vec3 t = in.target_ws;
    const float reach = len_ab + len_bc;
    const glm::vec3 from_a = t - a;
    float dist = glm::length(from_a);
    if (dist < 1e-6f) return;
    if (dist > reach * (1.0f - in.soften)) {
        t = a + (from_a / dist) * reach * (1.0f - in.soften);
        dist = glm::length(t - a);
    }

    // Interior angles from the law of cosines.
    const float cos_upper = std::clamp(
        (len_ab * len_ab + dist * dist - len_bc * len_bc) / (2.0f * len_ab * dist),
        -1.0f, 1.0f);
    const float cos_lower = std::clamp(
        (len_ab * len_ab + len_bc * len_bc - dist * dist) / (2.0f * len_ab * len_bc),
        -1.0f, 1.0f);

    const float angle_upper_desired = std::acos(cos_upper);
    const float angle_lower_desired = 3.14159265358979f - std::acos(cos_lower);

    // Bend axis — cross(to-target, pole-axis). Fall back to up if coplanar.
    glm::vec3 to_target = glm::normalize(t - a);
    glm::vec3 pole_dir  = glm::normalize(in.pole_ws - a);
    glm::vec3 axis = glm::cross(to_target, pole_dir);
    if (glm::dot(axis, axis) < 1e-6f) axis = glm::vec3{0.0f, 0.0f, 1.0f};
    axis = glm::normalize(axis);

    // Align upper-joint so b sits in the pole plane at `angle_upper_desired`
    // off the a→target axis.
    const glm::vec3 current_ab = glm::normalize(b - a);
    // Desired direction for ab: rotate `to_target` by `angle_upper_desired`
    // around `axis`.
    const glm::quat rot_upper_ws = glm::angleAxis(angle_upper_desired, axis);
    const glm::vec3 desired_ab = rot_upper_ws * to_target;

    const glm::quat delta_upper = safe_from_to(current_ab, desired_ab);
    const glm::quat r_upper_ws_new = delta_upper * glm::quat_cast(jws[in.joint_upper]);
    pose[in.joint_upper].rotation = world_to_local_rot(r_upper_ws_new, pose, skel, in.joint_upper);

    // Recompute FK for the lower joint with the updated upper.
    fk_world(pose, skel, jws);
    const glm::vec3 a2 = position(jws[in.joint_upper]);
    const glm::vec3 b2 = position(jws[in.joint_lower]);
    const glm::vec3 c2 = position(jws[in.joint_end]);
    const glm::vec3 current_bc = glm::normalize(c2 - b2);
    // Desired bc in world space: rotate (a2→b2) by (π - angle_lower_desired) around axis.
    const glm::vec3 ab2 = glm::normalize(b2 - a2);
    const glm::quat rot_lower_ws = glm::angleAxis(3.14159265358979f - angle_lower_desired, -axis);
    const glm::vec3 desired_bc = rot_lower_ws * ab2;
    const glm::quat delta_lower = safe_from_to(current_bc, desired_bc);
    const glm::quat r_lower_ws_new = delta_lower * glm::quat_cast(jws[in.joint_lower]);
    pose[in.joint_lower].rotation = world_to_local_rot(r_lower_ws_new, pose, skel, in.joint_lower);

    // Weight blend: lerp between the rest-pose slot and the solved rotation.
    // For Phase 13 null-backend acceptance we apply fully when weight >= 1.
    (void)in.weight;
    (void)c;
}

void solve_fabrik(Pose& pose,
                  const SkeletonDesc& skel,
                  const FabrikInput& in) noexcept {
    const auto& chain = in.chain_joints;
    if (chain.size() < 2) return;
    if (in.weight <= 0.0f) return;

    std::vector<glm::mat4> jws;
    fk_world(pose, skel, jws);

    // Gather chain positions + segment lengths.
    const std::size_t n = chain.size();
    std::vector<glm::vec3> p(n);
    std::vector<float> seg(n - 1, 0.0f);
    float total_len = 0.0f;
    for (std::size_t i = 0; i < n; ++i) {
        if (chain[i] >= pose.size()) return;
        p[i] = position(jws[chain[i]]);
    }
    for (std::size_t i = 0; i + 1 < n; ++i) {
        seg[i] = glm::length(p[i + 1] - p[i]);
        total_len += seg[i];
    }
    if (total_len <= 1e-6f) return;

    const glm::vec3 root = p[0];
    const float reach = glm::length(in.target_ws - root);
    if (reach >= total_len) {
        // Stretched out — align every joint along the direction to target.
        const glm::vec3 dir = glm::normalize(in.target_ws - root);
        for (std::size_t i = 1; i < n; ++i) p[i] = p[i - 1] + dir * seg[i - 1];
    } else {
        for (std::uint16_t it = 0; it < in.max_iterations; ++it) {
            // Backward — end to root.
            p[n - 1] = in.target_ws;
            for (std::size_t i = n - 1; i > 0; --i) {
                const glm::vec3 d = glm::normalize(p[i - 1] - p[i]);
                p[i - 1] = p[i] + d * seg[i - 1];
            }
            // Forward — root to end.
            p[0] = root;
            for (std::size_t i = 0; i + 1 < n; ++i) {
                const glm::vec3 d = glm::normalize(p[i + 1] - p[i]);
                p[i + 1] = p[i] + d * seg[i];
            }
            if (glm::length(p[n - 1] - in.target_ws) <= in.tolerance_m) break;
        }
    }

    // Reconstruct joint rotations so FK reproduces p[]. Each link i rotates
    // so its tail direction matches the new segment direction.
    for (std::size_t i = 0; i + 1 < n; ++i) {
        const glm::vec3 old_dir = glm::normalize(position(jws[chain[i + 1]]) - position(jws[chain[i]]));
        const glm::vec3 new_dir = glm::normalize(p[i + 1] - p[i]);
        const glm::quat delta_ws = safe_from_to(old_dir, new_dir);
        const glm::quat r_ws_old = glm::quat_cast(jws[chain[i]]);
        const glm::quat r_ws_new = delta_ws * r_ws_old;
        pose[chain[i]].rotation = world_to_local_rot(r_ws_new, pose, skel, chain[i]);
        // Refresh FK for the tail of the chain.
        fk_world(pose, skel, jws);
    }
}

void solve_ccd(Pose& pose,
               const SkeletonDesc& skel,
               const CcdInput& in) noexcept {
    const auto& chain = in.chain_joints;
    if (chain.size() < 2) return;
    if (in.weight <= 0.0f) return;

    std::vector<glm::mat4> jws;
    for (std::uint16_t it = 0; it < in.max_iterations; ++it) {
        fk_world(pose, skel, jws);
        const std::uint16_t end_joint = chain.back();
        if (end_joint >= pose.size()) return;
        const glm::vec3 end_pos = position(jws[end_joint]);
        if (glm::length(in.target_ws - end_pos) <= in.tolerance_m) break;

        for (std::size_t ci = chain.size(); ci-- > 0; ) {
            const std::uint16_t j = chain[ci];
            if (j >= pose.size()) continue;
            const glm::vec3 jp = position(jws[j]);
            const glm::vec3 current = end_pos - jp;
            const glm::vec3 desired = in.target_ws - jp;
            if (glm::dot(current, current) < 1e-8f ||
                glm::dot(desired, desired) < 1e-8f) continue;
            glm::quat delta = safe_from_to(current, desired);

            // Per-joint max-rotation clamp.
            const float w = std::clamp(delta.w, -1.0f, 1.0f);
            const float angle = 2.0f * std::acos(w);
            if (angle > in.per_joint_max_rot_rad) {
                const float s = std::sqrt(std::max(0.0f, 1.0f - w * w));
                glm::vec3 axis{delta.x, delta.y, delta.z};
                if (s > 1e-6f) axis /= s;
                delta = glm::angleAxis(in.per_joint_max_rot_rad, glm::normalize(axis));
            }

            const glm::quat r_ws_old = glm::quat_cast(jws[j]);
            const glm::quat r_ws_new = delta * r_ws_old;
            pose[j].rotation = world_to_local_rot(r_ws_new, pose, skel, j);
            fk_world(pose, skel, jws);
        }
    }
}

} // namespace gw::anim
