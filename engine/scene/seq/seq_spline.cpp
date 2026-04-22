// engine/scene/seq/seq_spline.cpp

#include "engine/scene/seq/seq_spline.hpp"

#include "engine/scene/seq/gwseq_format.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <span>

#define GLM_FORCE_RADIANS
#include <glm/gtc/quaternion.hpp>

namespace gw::seq {
namespace {

using gw::math::Quatd;
using gw::math::Vec3d;

[[nodiscard]] Quatd glm_to_quatd(const GwseqQuat& q) noexcept {
    return Quatd{static_cast<double>(q.w), static_cast<double>(q.x), static_cast<double>(q.y),
                 static_cast<double>(q.z)};
}

[[nodiscard]] Vec3d glm_to_vec3d(const Vec3_f64& v) noexcept {
    return Vec3d{v.x, v.y, v.z};
}

template <typename K>
[[nodiscard]] std::size_t find_segment(const K* frames, std::size_t n, double frame) noexcept {
    if (n == 0u) return 0u;
    if (frame <= static_cast<double>(frames[0].frame)) return 0u;
    if (frame >= static_cast<double>(frames[n - 1].frame)) return n - 1u;
    for (std::size_t i = 0; i + 1u < n; ++i) {
        const double a = static_cast<double>(frames[i].frame);
        const double b = static_cast<double>(frames[i + 1u].frame);
        if (frame >= a && frame < b) return i;
    }
    return n - 2u;
}

[[nodiscard]] Vec3d sample_vec3_span(std::span<const GwseqKeyframe<Vec3_f64>> frames,
                                     double head) noexcept {
    if (frames.empty()) return {};
    const std::size_t n = frames.size();
    const std::size_t i = find_segment(frames.data(), n, head);
    if (i + 1u >= n) return glm_to_vec3d(frames.back().value);

    const auto& k0 = frames[i];
    const auto& k1 = frames[i + 1u];
    const double fa = static_cast<double>(k0.frame);
    const double fb = static_cast<double>(k1.frame);
    if (fb <= fa) return glm_to_vec3d(k0.value);
    const double t = (head - fa) / (fb - fa);
    if (k1.interpolation == GwseqInterpolation::Step || k0.interpolation == GwseqInterpolation::Step) {
        return t < 1.0 ? glm_to_vec3d(k0.value) : glm_to_vec3d(k1.value);
    }
    if (k0.interpolation == GwseqInterpolation::Linear && k1.interpolation == GwseqInterpolation::Linear) {
        const auto a = k0.value;
        const auto b = k1.value;
        return Vec3d{static_cast<double>(a.x) * (1.0 - t) + static_cast<double>(b.x) * t,
                     static_cast<double>(a.y) * (1.0 - t) + static_cast<double>(b.y) * t,
                     static_cast<double>(a.z) * (1.0 - t) + static_cast<double>(b.z) * t};
    }
    return glm_to_vec3d(k0.value);
}

[[nodiscard]] Quatd sample_quat_span(std::span<const GwseqKeyframe<GwseqQuat>> frames,
                                      double head) noexcept {
    if (frames.empty()) return Quatd{1.0, 0.0, 0.0, 0.0};
    const std::size_t n = frames.size();
    const std::size_t i = find_segment(frames.data(), n, head);
    if (i + 1u >= n) return glm_to_quatd(frames.back().value);

    const auto& k0 = frames[i];
    const auto& k1 = frames[i + 1u];
    const double fa  = static_cast<double>(k0.frame);
    const double fb  = static_cast<double>(k1.frame);
    if (fb <= fa) return glm_to_quatd(k0.value);
    const float t = static_cast<float>((head - fa) / (fb - fa));
    if (k1.interpolation == GwseqInterpolation::Step || k0.interpolation == GwseqInterpolation::Step) {
        return t < 1.0f ? glm_to_quatd(k0.value) : glm_to_quatd(k1.value);
    }
    const glm::quat q0(static_cast<float>(k0.value.w), static_cast<float>(k0.value.x),
                       static_cast<float>(k0.value.y), static_cast<float>(k0.value.z));
    const glm::quat q1(static_cast<float>(k1.value.w), static_cast<float>(k1.value.x),
                       static_cast<float>(k1.value.y), static_cast<float>(k1.value.z));
    const glm::quat qs = glm::normalize(glm::slerp(q0, q1, t));
    return Quatd{static_cast<double>(qs.w), static_cast<double>(qs.x), static_cast<double>(qs.y),
                 static_cast<double>(qs.z)};
}

[[nodiscard]] float sample_float_span(std::span<const GwseqKeyframe<float>> frames, double head) noexcept {
    if (frames.empty()) return 60.f;
    const std::size_t n = frames.size();
    const std::size_t i = find_segment(frames.data(), n, head);
    if (i + 1u >= n) return frames.back().value;
    const auto& k0 = frames[i];
    const auto& k1 = frames[i + 1u];
    const double fa = static_cast<double>(k0.frame);
    const double fb = static_cast<double>(k1.frame);
    if (fb <= fa) return k0.value;
    const double t = (head - fa) / (fb - fa);
    if (k1.interpolation == GwseqInterpolation::Step || k0.interpolation == GwseqInterpolation::Step) {
        return t < 1.0 ? k0.value : k1.value;
    }
    return static_cast<float>(static_cast<double>(k0.value) * (1.0 - t) +
                              static_cast<double>(k1.value) * t);
}

[[nodiscard]] Quatd quat_normalize(const Quatd& q) noexcept {
    const double n =
        std::sqrt(q.w() * q.w() + q.x() * q.x() + q.y() * q.y() + q.z() * q.z());
    if (n < 1e-30) return Quatd{1.0, 0.0, 0.0, 0.0};
    return Quatd{q.w() / n, q.x() / n, q.y() / n, q.z() / n};
}

[[nodiscard]] Quatd quat_slerp(Quatd a, Quatd b, double t) noexcept {
    a = quat_normalize(a);
    b = quat_normalize(b);
    double       dot = a.w() * b.w() + a.x() * b.x() + a.y() * b.y() + a.z() * b.z();
    Quatd        b2  = b;
    if (dot < 0.0) {
        b2  = Quatd{-b.w(), -b.x(), -b.y(), -b.z()};
        dot = -dot;
    }
    constexpr double kThresh = 0.9995;
    if (dot > kThresh) {
        const double u = 1.0 - t;
        return quat_normalize(Quatd{a.w() * u + b2.w() * t, a.x() * u + b2.x() * t, a.y() * u + b2.y() * t,
                                    a.z() * u + b2.z() * t});
    }
    dot         = std::min(1.0, std::max(-1.0, dot));
    const double theta0 = std::acos(dot);
    const double s0     = std::sin((1.0 - t) * theta0) / std::sin(theta0);
    const double s1     = std::sin(t * theta0) / std::sin(theta0);
    return quat_normalize(Quatd{a.w() * s0 + b2.w() * s1, a.x() * s0 + b2.x() * s1, a.y() * s0 + b2.y() * s1,
                                a.z() * s0 + b2.z() * s1});
}

[[nodiscard]] Vec3d catmull_rom(const Vec3d& p0, const Vec3d& p1, const Vec3d& p2, const Vec3d& p3,
                               double t) noexcept {
    const double t2 = t * t;
    const double t3 = t2 * t;
    return Vec3d{0.5 * ((2.0 * p1.x()) + (-p0.x() + p2.x()) * t + (2.0 * p0.x() - 5.0 * p1.x() + 4.0 * p2.x() - p3.x()) * t2 +
                        (-p0.x() + 3.0 * p1.x() - 3.0 * p2.x() + p3.x()) * t3),
                 0.5 * ((2.0 * p1.y()) + (-p0.y() + p2.y()) * t + (2.0 * p0.y() - 5.0 * p1.y() + 4.0 * p2.y() - p3.y()) * t2 +
                        (-p0.y() + 3.0 * p1.y() - 3.0 * p2.y() + p3.y()) * t3),
                 0.5 * ((2.0 * p1.z()) + (-p0.z() + p2.z()) * t + (2.0 * p0.z() - 5.0 * p1.z() + 4.0 * p2.z() - p3.z()) * t2 +
                        (-p0.z() + 3.0 * p1.z() - 3.0 * p2.z() + p3.z()) * t3)};
}

}  // namespace

CameraSplineSample CameraSpline::evaluate(const double frame) const noexcept {
    CameraSplineSample out{};
    const auto&        k = knots_;
    if (k.empty()) return out;
    if (k.size() == 1u) {
        out.position = k[0].position;
        out.rotation = quat_normalize(k[0].rotation);
        out.fov_deg  = k[0].fov_deg;
        return out;
    }
    if (frame <= static_cast<double>(k.front().frame)) {
        out.position = k.front().position;
        out.rotation = quat_normalize(k.front().rotation);
        out.fov_deg  = k.front().fov_deg;
        return out;
    }
    if (frame >= static_cast<double>(k.back().frame)) {
        out.position = k.back().position;
        out.rotation = quat_normalize(k.back().rotation);
        out.fov_deg  = k.back().fov_deg;
        return out;
    }

    std::size_t i = 0u;
    for (std::size_t j = 0; j + 1u < k.size(); ++j) {
        if (frame >= static_cast<double>(k[j].frame) && frame < static_cast<double>(k[j + 1u].frame)) {
            i = j;
            break;
        }
    }
    const double f0 = static_cast<double>(k[i].frame);
    const double f1 = static_cast<double>(k[i + 1u].frame);
    if (f1 <= f0) {
        out.position = k[i].position;
        out.rotation = quat_normalize(k[i].rotation);
        out.fov_deg  = k[i].fov_deg;
        return out;
    }
    const double tseg = (frame - f0) / (f1 - f0);

    const auto pick = [&](std::ptrdiff_t idx) -> const SplineKnot& {
        if (idx < 0) return k.front();
        if (static_cast<std::size_t>(idx) >= k.size()) return k.back();
        return k[static_cast<std::size_t>(idx)];
    };
    const Vec3d p0 = pick(static_cast<std::ptrdiff_t>(i) - 1).position;
    const Vec3d p1 = k[i].position;
    const Vec3d p2 = k[i + 1u].position;
    const Vec3d p3 = pick(static_cast<std::ptrdiff_t>(i) + 2).position;
    out.position   = catmull_rom(p0, p1, p2, p3, tseg);

    out.rotation = quat_slerp(k[i].rotation, k[i + 1u].rotation, tseg);
    out.fov_deg  = static_cast<float>(static_cast<double>(k[i].fov_deg) * (1.0 - tseg) +
                                     static_cast<double>(k[i + 1u].fov_deg) * tseg);
    return out;
}

CameraSpline CameraSpline::build_from_tracks(const GwseqTrackView<Vec3_f64>&   position_track,
                                            const GwseqTrackView<GwseqQuat>&  rotation_track,
                                            const GwseqTrackView<float>&      fov_track) noexcept {
    std::vector<std::uint32_t> frames;
    for (const auto& kf : position_track.frames) frames.push_back(kf.frame);
    for (const auto& kf : rotation_track.frames) frames.push_back(kf.frame);
    for (const auto& kf : fov_track.frames) frames.push_back(kf.frame);
    std::sort(frames.begin(), frames.end());
    frames.erase(std::unique(frames.begin(), frames.end()), frames.end());

    CameraSpline spl{};
    for (const std::uint32_t f : frames) {
        const double            fd = static_cast<double>(f);
        SplineKnot kn{};
        kn.frame     = f;
        kn.position  = sample_vec3_span(position_track.frames, fd);
        kn.rotation  = sample_quat_span(rotation_track.frames, fd);
        kn.fov_deg   = sample_float_span(fov_track.frames, fd);
        spl.knots_.push_back(kn);
    }
    return spl;
}

}  // namespace gw::seq
