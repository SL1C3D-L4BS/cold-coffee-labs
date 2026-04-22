#pragma once
// Shared minimal .gwseq bytes for the Studio Ready exit gate (Phase 18-C).

#include "engine/scene/seq/gwseq_codec.hpp"

#include <cstdint>
#include <vector>

namespace gw::studio_sandbox {

/// 60 fps, 120 frames, one transform position track (linear 0,0,0 -> 1,1,1).
[[nodiscard]] inline std::vector<std::uint8_t> make_test_gwseq_bytes() {
    std::vector<std::uint8_t> blob;
    gw::seq::GwseqWriter     w(blob);
    (void)w.write_header(60u, 120u);
    (void)w.begin_track(1u, gw::seq::GwseqTrackType::Transform, gw::seq::GwseqTransformPayloadKind::PositionVec3);
    gw::seq::GwseqKeyframe<gw::seq::Vec3_f64> k0{};
    k0.frame         = 0u;
    k0.value         = {0.0, 0.0, 0.0};
    k0.interpolation = gw::seq::GwseqInterpolation::Linear;
    gw::seq::GwseqKeyframe<gw::seq::Vec3_f64> k1 = k0;
    k1.frame         = 119u;
    k1.value         = {1.0, 1.0, 1.0};
    (void)w.write_keyframe(k0);
    (void)w.write_keyframe(k1);
    (void)w.finalise();
    return blob;
}

}  // namespace gw::studio_sandbox
