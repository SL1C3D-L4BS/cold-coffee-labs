// engine/anim/pose.cpp — Phase 13 Wave 13A (ADR-0039).

#include "engine/anim/pose.hpp"
#include "engine/anim/skeleton.hpp"

#include "engine/physics/determinism_hash.hpp"

#include <glm/common.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>

namespace gw::anim {

void Pose::reset_identity() noexcept {
    for (auto& j : joints_) {
        j.translation = glm::vec3{0.0f};
        j.rotation    = glm::quat{1.0f, 0.0f, 0.0f, 0.0f};
        j.scale       = glm::vec3{1.0f};
    }
}

void pose_set_from_rest(Pose& out, std::span<const JointRest> rest) noexcept {
    out.resize(rest.size());
    for (std::size_t i = 0; i < rest.size(); ++i) {
        out[i].translation = rest[i].translation;
        out[i].rotation    = rest[i].rotation;
        out[i].scale       = rest[i].scale;
    }
}

namespace {

inline glm::quat canonical(glm::quat q) noexcept {
    if (q.w < 0.0f) { q.w = -q.w; q.x = -q.x; q.y = -q.y; q.z = -q.z; }
    return q;
}

// Short-arc NLERP: negate b if dot < 0 before mixing, then normalize.
inline glm::quat nlerp_short(glm::quat a, glm::quat b, float w) noexcept {
    const float d = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    if (d < 0.0f) { b.x = -b.x; b.y = -b.y; b.z = -b.z; b.w = -b.w; }
    glm::quat q{
        a.w + (b.w - a.w) * w,
        a.x + (b.x - a.x) * w,
        a.y + (b.y - a.y) * w,
        a.z + (b.z - a.z) * w,
    };
    const float len2 = q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z;
    if (len2 > 0.0f) {
        const float inv = 1.0f / std::sqrt(len2);
        q.w *= inv; q.x *= inv; q.y *= inv; q.z *= inv;
    }
    return q;
}

} // namespace

void pose_lerp(Pose& out, const Pose& a, const Pose& b, float w) noexcept {
    const std::size_t n = std::min(a.size(), b.size());
    out.resize(n);
    w = std::clamp(w, 0.0f, 1.0f);
    for (std::size_t i = 0; i < n; ++i) {
        out[i].translation = a[i].translation + (b[i].translation - a[i].translation) * w;
        out[i].rotation    = nlerp_short(a[i].rotation, b[i].rotation, w);
        out[i].scale       = a[i].scale + (b[i].scale - a[i].scale) * w;
    }
}

void pose_add(Pose& out, const Pose& a, const Pose& b,
              std::span<const JointRest> rest, float w) noexcept {
    const std::size_t n = std::min({a.size(), b.size(), rest.size()});
    out.resize(n);
    for (std::size_t i = 0; i < n; ++i) {
        const glm::vec3 delta_t = b[i].translation - rest[i].translation;
        out[i].translation = a[i].translation + delta_t * w;

        // Rotation delta: delta = b * rest^-1, applied to a with weight.
        const glm::quat rest_inv = glm::conjugate(rest[i].rotation);
        glm::quat delta_q = b[i].rotation * rest_inv;
        // Scale the delta angle by w via NLERP from identity.
        glm::quat ident{1.0f, 0.0f, 0.0f, 0.0f};
        delta_q = nlerp_short(ident, delta_q, w);
        out[i].rotation = delta_q * a[i].rotation;

        const glm::vec3 rest_s = rest[i].scale;
        const glm::vec3 delta_s = b[i].scale - rest_s;
        out[i].scale = a[i].scale + delta_s * w;
    }
}

void pose_mask_lerp(Pose& out, const Pose& a, const Pose& b, float w,
                    std::span<const float> mask) noexcept {
    const std::size_t n = std::min({a.size(), b.size(), mask.size()});
    out.resize(n);
    for (std::size_t i = 0; i < n; ++i) {
        const float wi = std::clamp(mask[i] * w, 0.0f, 1.0f);
        out[i].translation = a[i].translation + (b[i].translation - a[i].translation) * wi;
        out[i].rotation    = nlerp_short(a[i].rotation, b[i].rotation, wi);
        out[i].scale       = a[i].scale + (b[i].scale - a[i].scale) * wi;
    }
}

std::uint64_t pose_hash(const Pose& pose, const PoseHashOptions& opt) noexcept {
    std::uint64_t h = gw::physics::kFnvOffset64;

    auto fold_u32 = [&](std::uint32_t v) {
        std::uint8_t bytes[4];
        std::memcpy(bytes, &v, sizeof(v));
        h = gw::physics::fnv1a64_combine(h, {bytes, 4});
    };
    auto fold_u64 = [&](std::uint64_t v) {
        std::uint8_t bytes[8];
        std::memcpy(bytes, &v, sizeof(v));
        h = gw::physics::fnv1a64_combine(h, {bytes, 8});
    };
    auto fold_f32 = [&](float v) {
        if (v == 0.0f) v = 0.0f;
        if (std::isnan(v)) { fold_u32(0x7FC00000u); return; }
        fold_u32(std::bit_cast<std::uint32_t>(v));
    };
    auto fold_quant = [&](float v, float eps) {
        const float q = (std::fabs(v) < eps) ? 0.0f : v;
        fold_f32(q);
    };

    fold_f32(opt.translation_epsilon_m);
    fold_f32(opt.scale_epsilon);
    h ^= opt.include_scale ? 0x1ULL : 0x0ULL;

    fold_u64(static_cast<std::uint64_t>(pose.size()));

    for (std::size_t i = 0; i < pose.size(); ++i) {
        const auto& j = pose[i];
        fold_quant(j.translation.x, opt.translation_epsilon_m);
        fold_quant(j.translation.y, opt.translation_epsilon_m);
        fold_quant(j.translation.z, opt.translation_epsilon_m);

        const glm::quat q = canonical(j.rotation);
        fold_f32(q.w); fold_f32(q.x); fold_f32(q.y); fold_f32(q.z);

        if (opt.include_scale) {
            fold_quant(j.scale.x, opt.scale_epsilon);
            fold_quant(j.scale.y, opt.scale_epsilon);
            fold_quant(j.scale.z, opt.scale_epsilon);
        }
    }

    return h;
}

} // namespace gw::anim
