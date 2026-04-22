#pragma once
// engine/render/post/dof.hpp — ADR-0079 Wave 17F: CoC DoF.

#include <cstdint>

namespace gw::render::post {

struct DofConfig {
    bool  enabled{true};
    float focal_mm{50.0f};
    float aperture{2.8f};           // f-stop
    float focus_distance_m{5.0f};
    float sensor_mm{36.0f};          // full-frame
};

// Circle-of-confusion radius (pixels) at a given scene depth.
// Thin-lens model:
//   CoC = |(f * (d_focus - d)) / (d * (d_focus - f)) * (f / N)|  → meters
//   pixels = CoC_m * (pixel_density_per_m on sensor)
[[nodiscard]] float coc_radius_px(DofConfig cfg,
                                    float scene_depth_m,
                                    std::uint32_t image_height_px) noexcept;

// Near/far blend weight for half-res CoC composite. Returns {near, far, coc_px}.
struct CocBlend { float near_w{0}, far_w{0}, coc_px{0}; };
[[nodiscard]] CocBlend coc_blend(DofConfig cfg,
                                   float scene_depth_m,
                                   std::uint32_t image_height_px) noexcept;

} // namespace gw::render::post
