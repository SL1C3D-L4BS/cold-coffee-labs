// engine/render/post/post_world.cpp — ADR-0079 Wave 17F PIMPL.

#include "engine/render/post/post_world.hpp"

#include "engine/core/config/cvar_registry.hpp"

#include <chrono>

namespace gw::render::post {

namespace { using clk = std::chrono::steady_clock; }

struct PostWorld::Impl {
    PostConfig                     cfg{};
    gw::config::CVarRegistry*      cvars{nullptr};
    gw::events::CrossSubsystemBus* bus{nullptr};
    bool                           inited{false};

    Bloom bloom{};
    Taa   taa{};

    PostStats stats{};

    void pull_cvars() {
        if (!cvars) return;
        // Bloom
        cfg.bloom.enabled    = cvars->get_bool_or("post.bloom.enabled", cfg.bloom.enabled);
        cfg.bloom.iterations = static_cast<std::uint32_t>(
            cvars->get_i32_or("post.bloom.iterations", static_cast<std::int32_t>(cfg.bloom.iterations)));
        cfg.bloom.threshold  = cvars->get_f32_or("post.bloom.threshold", cfg.bloom.threshold);
        cfg.bloom.intensity  = cvars->get_f32_or("post.bloom.intensity", cfg.bloom.intensity);
        bloom.configure(cfg.bloom);

        // Tonemap
        cfg.tonemap.curve    = parse_tonemap_curve(
            cvars->get_string_or("post.tonemap.curve", std::string{to_cstring(cfg.tonemap.curve)}));
        cfg.tonemap.exposure = cvars->get_f32_or("post.tonemap.exposure", cfg.tonemap.exposure);

        // TAA
        cfg.taa.enabled      = cvars->get_bool_or("post.taa.enabled", cfg.taa.enabled);
        cfg.taa.clip_mode    = parse_clip_mode(
            cvars->get_string_or("post.taa.clip_mode", std::string{to_cstring(cfg.taa.clip_mode)}));
        cfg.taa.jitter_scale = cvars->get_f32_or("post.taa.jitter_scale", cfg.taa.jitter_scale);
        taa.configure(cfg.taa);

        // MB / DoF / CA / grain
        cfg.motion_blur.enabled = cvars->get_bool_or("post.mb.enabled", cfg.motion_blur.enabled);
        cfg.motion_blur.max_velocity_px = static_cast<std::uint32_t>(
            cvars->get_i32_or("post.mb.max_velocity_px", static_cast<std::int32_t>(cfg.motion_blur.max_velocity_px)));
        cfg.dof.enabled    = cvars->get_bool_or("post.dof.enabled", cfg.dof.enabled);
        cfg.dof.focal_mm   = cvars->get_f32_or("post.dof.focal_mm", cfg.dof.focal_mm);
        cfg.dof.aperture   = cvars->get_f32_or("post.dof.aperture", cfg.dof.aperture);
        cfg.chromatic.strength = cvars->get_f32_or("post.ca.strength", cfg.chromatic.strength);
        cfg.grain.intensity    = cvars->get_f32_or("post.grain.intensity", cfg.grain.intensity);
        cfg.grain.size_px      = cvars->get_f32_or("post.grain.size_px", cfg.grain.size_px);
    }
};

PostWorld::PostWorld() : impl_(std::make_unique<Impl>()) {}
PostWorld::~PostWorld() { shutdown(); }

bool PostWorld::initialize(PostConfig cfg,
                            config::CVarRegistry* cvars,
                            events::CrossSubsystemBus* bus) {
    shutdown();
    impl_->cfg   = std::move(cfg);
    impl_->cvars = cvars;
    impl_->bus   = bus;
    impl_->pull_cvars();
    impl_->bloom.configure(impl_->cfg.bloom);
    impl_->taa.configure(impl_->cfg.taa);
    impl_->inited = true;
    return true;
}

void PostWorld::shutdown() {
    impl_->inited = false;
    impl_->stats = {};
}

bool PostWorld::initialized() const noexcept { return impl_->inited; }

void PostWorld::step(double) {
    if (!impl_->inited) return;
    const auto t0 = clk::now();
    impl_->pull_cvars();
    ++impl_->stats.frame;
    const auto t1 = clk::now();
    impl_->stats.last_post_ms =
        std::chrono::duration<double, std::milli>(t1 - t0).count();
}

void PostWorld::pull_cvars() { impl_->pull_cvars(); }

const PostConfig& PostWorld::config() const noexcept { return impl_->cfg; }
PostStats         PostWorld::stats()  const noexcept { return impl_->stats; }
const Bloom&      PostWorld::bloom() const noexcept  { return impl_->bloom; }
const Taa&        PostWorld::taa()   const noexcept  { return impl_->taa; }

} // namespace gw::render::post
