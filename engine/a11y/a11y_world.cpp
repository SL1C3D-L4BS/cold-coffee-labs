// engine/a11y/a11y_world.cpp — ADR-0071/0072.

#include "engine/a11y/a11y_world.hpp"

#include "engine/a11y/color_matrices.hpp"
#include "engine/core/config/cvar_registry.hpp"

#include <algorithm>
#include <utility>

namespace gw::a11y {

struct A11yWorld::Impl {
    A11yConfig                    cfg{};
    gw::config::CVarRegistry*     cvars{nullptr};
    gw::events::CrossSubsystemBus* bus{nullptr};
    gw::jobs::Scheduler*          sched{nullptr};

    SubtitleQueue                 queue{};
    std::unique_ptr<IScreenReader> reader{};

    bool                          inited{false};
    std::uint64_t                 steps{0};
};

A11yWorld::A11yWorld() : impl_(std::make_unique<Impl>()) {}
A11yWorld::~A11yWorld() { shutdown(); }

bool A11yWorld::initialize(A11yConfig                   cfg,
                             gw::config::CVarRegistry*    cvars,
                             gw::events::CrossSubsystemBus* bus,
                             gw::jobs::Scheduler*          scheduler) {
    shutdown();
    impl_->cfg   = std::move(cfg);
    impl_->cvars = cvars;
    impl_->bus   = bus;
    impl_->sched = scheduler;

    if (impl_->cvars) {
        const auto mode_str = impl_->cvars->get_string_or("a11y.color_mode",
                                    std::string{to_string(impl_->cfg.color_mode)});
        impl_->cfg.color_mode             = parse_color_mode(mode_str);
        impl_->cfg.color_strength         = impl_->cvars->get_f32_or("a11y.color_strength",       impl_->cfg.color_strength);
        impl_->cfg.text_scale             = impl_->cvars->get_f32_or("a11y.text.scale",           impl_->cfg.text_scale);
        impl_->cfg.contrast_boost         = impl_->cvars->get_f32_or("a11y.contrast.boost",       impl_->cfg.contrast_boost);
        impl_->cfg.reduce_motion          = impl_->cvars->get_bool_or("a11y.reduce_motion",       impl_->cfg.reduce_motion);
        impl_->cfg.photosensitivity_safe  = impl_->cvars->get_bool_or("a11y.photosensitivity_safe", impl_->cfg.photosensitivity_safe);
        impl_->cfg.subtitles_enabled      = impl_->cvars->get_bool_or("a11y.subtitle.enabled",    impl_->cfg.subtitles_enabled);
        impl_->cfg.subtitle_scale         = impl_->cvars->get_f32_or("a11y.subtitle.scale",       impl_->cfg.subtitle_scale);
        impl_->cfg.subtitle_bg_opacity    = impl_->cvars->get_f32_or("a11y.subtitle.bg_opacity",  impl_->cfg.subtitle_bg_opacity);
        impl_->cfg.subtitle_speaker_color = impl_->cvars->get_bool_or("a11y.subtitle.speaker_color", impl_->cfg.subtitle_speaker_color);
        impl_->cfg.subtitle_max_lines     = impl_->cvars->get_i32_or("a11y.subtitle.max_lines",   impl_->cfg.subtitle_max_lines);
        impl_->cfg.screen_reader_enabled  = impl_->cvars->get_bool_or("a11y.screen_reader.enabled", impl_->cfg.screen_reader_enabled);
        impl_->cfg.screen_reader_verbosity = impl_->cvars->get_string_or("a11y.screen_reader.verbosity", impl_->cfg.screen_reader_verbosity);
        impl_->cfg.focus_show_ring        = impl_->cvars->get_bool_or("a11y.focus.show_ring",     impl_->cfg.focus_show_ring);
        impl_->cfg.focus_ring_width_px    = impl_->cvars->get_f32_or("a11y.focus.ring_width_px", impl_->cfg.focus_ring_width_px);
        impl_->cfg.wcag_version           = impl_->cvars->get_string_or("a11y.wcag.version",      impl_->cfg.wcag_version);
    }

    impl_->cfg.text_scale        = std::clamp(impl_->cfg.text_scale,        0.5f, 2.5f);
    impl_->cfg.color_strength    = std::clamp(impl_->cfg.color_strength,    0.0f, 1.0f);
    impl_->cfg.contrast_boost    = std::clamp(impl_->cfg.contrast_boost,    0.0f, 1.0f);
    impl_->cfg.subtitle_scale    = std::clamp(impl_->cfg.subtitle_scale,    0.5f, 2.0f);
    impl_->cfg.subtitle_bg_opacity = std::clamp(impl_->cfg.subtitle_bg_opacity, 0.0f, 1.0f);
    if (impl_->cfg.subtitle_max_lines < 1) impl_->cfg.subtitle_max_lines = 1;
    if (impl_->cfg.subtitle_max_lines > 6) impl_->cfg.subtitle_max_lines = 6;
    impl_->cfg.focus_ring_width_px = std::clamp(impl_->cfg.focus_ring_width_px, 1.0f, 8.0f);

    impl_->queue.set_max_lines(impl_->cfg.subtitle_max_lines);
    impl_->reader = make_screen_reader_aggregated(impl_->cfg.screen_reader_enabled);

    impl_->inited = true;
    return true;
}

void A11yWorld::shutdown() {
    impl_->queue.clear();
    impl_->reader.reset();
    impl_->inited = false;
    impl_->steps  = 0;
}

bool A11yWorld::initialized() const noexcept { return impl_->inited; }

void A11yWorld::step(double /*dt*/, std::int64_t now_unix_ms) {
    if (!impl_->inited) return;
    ++impl_->steps;
    impl_->queue.tick(now_unix_ms);
}

ColorMatrix3 A11yWorld::active_color_transform() const noexcept {
    return color_matrix_for(impl_->cfg.color_mode, impl_->cfg.color_strength);
}

ColorMode A11yWorld::color_mode() const noexcept { return impl_->cfg.color_mode; }

void A11yWorld::set_color_mode(ColorMode m, float strength) {
    impl_->cfg.color_mode     = m;
    impl_->cfg.color_strength = std::clamp(strength, 0.0f, 1.0f);
}

float A11yWorld::text_scale() const noexcept { return impl_->cfg.text_scale; }
void  A11yWorld::set_text_scale(float s)     { impl_->cfg.text_scale = std::clamp(s, 0.5f, 2.5f); }
bool  A11yWorld::reduce_motion() const noexcept { return impl_->cfg.reduce_motion; }
bool  A11yWorld::photosensitivity_safe() const noexcept { return impl_->cfg.photosensitivity_safe; }

bool A11yWorld::push_subtitle(SubtitleCue cue, std::int64_t now_ms) {
    if (!impl_->inited || !impl_->cfg.subtitles_enabled) return false;
    return impl_->queue.push(std::move(cue), now_ms);
}

void A11yWorld::snapshot_subtitles(std::vector<SubtitleLine>& out, std::int64_t now_ms) const {
    impl_->queue.snapshot(out, now_ms);
}

std::size_t A11yWorld::subtitle_active_count() const noexcept { return impl_->queue.active_count(); }
std::size_t A11yWorld::subtitle_emitted_count() const noexcept { return impl_->queue.emitted_count(); }

void A11yWorld::update_ax_tree(std::span<const AccessibleNode> nodes) {
    if (impl_->reader) impl_->reader->update_tree(nodes);
}
void A11yWorld::announce(std::string_view utf8, Politeness p) {
    if (impl_->reader && impl_->cfg.screen_reader_enabled) impl_->reader->announce_text(utf8, p);
}
void A11yWorld::announce_focus(std::uint64_t node_id) {
    if (impl_->reader && impl_->cfg.screen_reader_enabled) impl_->reader->announce_focus(node_id);
}
IScreenReader*       A11yWorld::screen_reader()       noexcept { return impl_->reader.get(); }
const IScreenReader* A11yWorld::screen_reader() const noexcept { return impl_->reader.get(); }

SelfCheckReport A11yWorld::selfcheck(const SelfCheckContext& ctx_in) const {
    SelfCheckContext ctx = ctx_in;
    ctx.cfg = impl_->cfg;
    return run_selfcheck(ctx);
}

const A11yConfig& A11yWorld::config() const noexcept { return impl_->cfg; }
std::uint64_t     A11yWorld::step_count() const noexcept { return impl_->steps; }

} // namespace gw::a11y
