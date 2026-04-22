// engine/render/post/post_cvars.cpp — ADR-0079/0080.

#include "engine/render/post/post_cvars.hpp"

namespace gw::render::post {

namespace {
constexpr std::uint32_t kP = config::kCVarPersist;
constexpr std::uint32_t kU = config::kCVarUserFacing;
} // namespace

PostCVars register_post_cvars(config::CVarRegistry& r) {
    PostCVars c{};
    c.bloom_enabled = r.register_bool({
        "post.bloom.enabled", true, kP | kU, {}, {}, false,
        "Enable dual-Kawase bloom (ADR-0079).",
    });
    c.bloom_iterations = r.register_i32({
        "post.bloom.iterations", 5, kP, 0, 6, true,
        "Dual-Kawase down/up iterations (0..6).",
    });
    c.bloom_threshold = r.register_f32({
        "post.bloom.threshold", 1.0f, kP | kU, 0.0f, 16.0f, true,
        "Bloom bright-pass threshold (luminance).",
    });
    c.bloom_intensity = r.register_f32({
        "post.bloom.intensity", 1.0f, kP | kU, 0.0f, 4.0f, true,
        "Bloom composite scale.",
    });

    c.tonemap_curve = r.register_string({
        "post.tonemap.curve", std::string{"pbr_neutral"}, kP | kU, {}, {}, false,
        "pbr_neutral | aces | linear (ADR-0080).",
    });
    c.tonemap_exposure = r.register_f32({
        "post.tonemap.exposure", 1.0f, kP | kU, 0.0f, 16.0f, true,
        "Exposure multiplier applied before tonemap.",
    });

    c.taa_enabled = r.register_bool({
        "post.taa.enabled", true, kP | kU, {}, {}, false,
        "Enable temporal anti-aliasing.",
    });
    c.taa_clip_mode = r.register_string({
        "post.taa.clip_mode", std::string{"kdop"}, kP | kU, {}, {}, false,
        "kdop | aabb | variance (ADR-0079).",
    });
    c.taa_jitter_scale = r.register_f32({
        "post.taa.jitter_scale", 1.0f, kP, 0.0f, 4.0f, true,
        "Sub-pixel jitter magnitude.",
    });

    c.mb_enabled = r.register_bool({
        "post.mb.enabled", true, kP | kU, {}, {}, false,
        "Enable velocity-buffer motion blur (McGuire).",
    });
    c.mb_max_velocity_px = r.register_i32({
        "post.mb.max_velocity_px", 32, kP, 0, 256, true,
        "Max velocity magnitude for McGuire reconstruction (px).",
    });

    c.dof_enabled = r.register_bool({
        "post.dof.enabled", false, kP | kU, {}, {}, false,
        "Enable depth-of-field (CoC).",
    });
    c.dof_focal_mm = r.register_f32({
        "post.dof.focal_mm", 50.0f, kP | kU, 8.0f, 500.0f, true,
        "Lens focal length in millimeters.",
    });
    c.dof_aperture = r.register_f32({
        "post.dof.aperture", 2.8f, kP | kU, 1.0f, 22.0f, true,
        "Aperture f-stop.",
    });

    c.ca_strength = r.register_f32({
        "post.ca.strength", 0.0f, kP | kU, 0.0f, 0.05f, true,
        "Chromatic aberration strength.",
    });
    c.grain_intensity = r.register_f32({
        "post.grain.intensity", 0.0f, kP | kU, 0.0f, 0.5f, true,
        "Film grain intensity.",
    });
    c.grain_size_px = r.register_f32({
        "post.grain.size_px", 1.5f, kP, 0.5f, 8.0f, true,
        "Film grain size in pixels.",
    });
    return c;
}

} // namespace gw::render::post
