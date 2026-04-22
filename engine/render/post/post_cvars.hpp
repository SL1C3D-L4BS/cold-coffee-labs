#pragma once
// engine/render/post/post_cvars.hpp — ADR-0079/0080.

#include "engine/core/config/cvar_registry.hpp"

namespace gw::render::post {

struct PostCVars {
    config::CVarRef<bool>         bloom_enabled{};
    config::CVarRef<std::int32_t> bloom_iterations{};
    config::CVarRef<float>        bloom_threshold{};
    config::CVarRef<float>        bloom_intensity{};

    config::CVarRef<std::string>  tonemap_curve{};
    config::CVarRef<float>        tonemap_exposure{};

    config::CVarRef<bool>         taa_enabled{};
    config::CVarRef<std::string>  taa_clip_mode{};
    config::CVarRef<float>        taa_jitter_scale{};

    config::CVarRef<bool>         mb_enabled{};
    config::CVarRef<std::int32_t> mb_max_velocity_px{};

    config::CVarRef<bool>         dof_enabled{};
    config::CVarRef<float>        dof_focal_mm{};
    config::CVarRef<float>        dof_aperture{};

    config::CVarRef<float>        ca_strength{};
    config::CVarRef<float>        grain_intensity{};
    config::CVarRef<float>        grain_size_px{};
};

[[nodiscard]] PostCVars register_post_cvars(config::CVarRegistry& r);

} // namespace gw::render::post
