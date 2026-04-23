#pragma once

namespace gw::gameplay::martyrdom {

/// Reality-warp hooks fed by the render frame graph when RaptureState is
/// active (docs/07 §2.6). Presentation-only. Zero replay-hash impact.
struct BlasphemyState {
    float chromatic_aberration = 0.f;   // 0..1
    float color_grade_shift    = 0.f;   // 0..1
    float post_intensity_bump  = 0.f;   // 0..1

    [[nodiscard]] constexpr bool active() const noexcept {
        return chromatic_aberration > 0.f || color_grade_shift > 0.f || post_intensity_bump > 0.f;
    }
};

} // namespace gw::gameplay::martyrdom
