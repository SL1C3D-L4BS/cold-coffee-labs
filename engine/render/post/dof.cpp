// engine/render/post/dof.cpp — ADR-0079 Wave 17F.

#include "engine/render/post/dof.hpp"

#include <algorithm>
#include <cmath>

namespace gw::render::post {

float coc_radius_px(DofConfig cfg, float depth_m, std::uint32_t image_h) noexcept {
    if (cfg.focal_mm <= 0.0f || cfg.aperture <= 0.0f || cfg.focus_distance_m <= 0.0f) return 0.0f;
    if (depth_m <= 0.0f) return 0.0f;
    const float f_m        = cfg.focal_mm * 1e-3f;
    const float focus_m    = cfg.focus_distance_m;
    const float aperture_m = f_m / cfg.aperture;
    const float denom      = std::max(1e-6f, depth_m * (focus_m - f_m));
    const float coc_m      = std::abs((f_m * (focus_m - depth_m)) / denom * aperture_m);
    const float sensor_h_m = cfg.sensor_mm * 1e-3f;
    const float px_per_m   = static_cast<float>(image_h) / std::max(1e-6f, sensor_h_m);
    return coc_m * px_per_m;
}

CocBlend coc_blend(DofConfig cfg, float depth_m, std::uint32_t image_h) noexcept {
    CocBlend out{};
    out.coc_px = coc_radius_px(cfg, depth_m, image_h);
    const bool is_near = depth_m < cfg.focus_distance_m;
    const float t = std::clamp(out.coc_px / 16.0f, 0.0f, 1.0f);
    out.near_w = is_near ? t : 0.0f;
    out.far_w  = is_near ? 0.0f : t;
    return out;
}

} // namespace gw::render::post
