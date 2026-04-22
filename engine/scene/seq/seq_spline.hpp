#pragma once
// engine/scene/seq/seq_spline.hpp
// Catmull-Rom + slerp camera spline — Phase 18-B.

#include "engine/math/quat.hpp"
#include "engine/math/vec.hpp"
#include "engine/scene/seq/gwseq_codec.hpp"

#include <cstdint>
#include <vector>

namespace gw::seq {

/// World-space point + orientation + lens for one spline support.
struct SplineKnot {
    std::uint32_t   frame   = 0;
    gw::math::Vec3d position{};
    gw::math::Quatd rotation{};
    float           fov_deg = 60.f;
};

/// Interpolated camera sample on the spline.
struct CameraSplineSample {
    gw::math::Vec3d position{};
    gw::math::Quatd rotation{};
    float           fov_deg = 60.f;
};

/// Evaluatable camera path built from .gwseq transform + float (FOV) tracks.
class CameraSpline {
public:
    CameraSpline() noexcept = default;

    /// Monotonic by `frame` (builder enforces order).
    [[nodiscard]] const std::vector<SplineKnot>& knots() const noexcept { return knots_; }

    /// Replaces knot data (used by tests and editors).
    void set_knots(std::vector<SplineKnot> k) { knots_ = std::move(k); }

    /// Interpolated camera at fractional frame time.
    [[nodiscard]] CameraSplineSample evaluate(double frame) const noexcept;

    /// Merges position, rotation, and FOV keyframes into one knot stream, then
    /// sorts by frame. At each support frame, channels without an exact key
    /// are sampled with the same rules as `SequencerSystem` (linear / slerp
    /// between surrounding keys).
    [[nodiscard]] static CameraSpline build_from_tracks(
        const GwseqTrackView<Vec3_f64>&     position_track,
        const GwseqTrackView<GwseqQuat>&   rotation_track,
        const GwseqTrackView<float>&        fov_track) noexcept;

private:
    std::vector<SplineKnot> knots_{};
};

}  // namespace gw::seq
