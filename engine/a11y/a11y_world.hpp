#pragma once
// engine/a11y/a11y_world.hpp — ADR-0071/0072 PIMPL façade.

#include "engine/a11y/a11y_config.hpp"
#include "engine/a11y/a11y_types.hpp"
#include "engine/a11y/events_a11y.hpp"
#include "engine/a11y/screen_reader.hpp"
#include "engine/a11y/selfcheck.hpp"
#include "engine/a11y/subtitles.hpp"

#include <memory>
#include <span>
#include <vector>

namespace gw::config { class CVarRegistry; }
namespace gw::events { class CrossSubsystemBus; }
namespace gw::jobs   { class Scheduler; }

namespace gw::a11y {

class A11yWorld {
public:
    A11yWorld();
    ~A11yWorld();
    A11yWorld(const A11yWorld&)            = delete;
    A11yWorld& operator=(const A11yWorld&) = delete;

    [[nodiscard]] bool initialize(A11yConfig                   cfg,
                                   gw::config::CVarRegistry*    cvars,
                                   gw::events::CrossSubsystemBus* bus = nullptr,
                                   gw::jobs::Scheduler*          scheduler = nullptr);
    void               shutdown();
    [[nodiscard]] bool initialized() const noexcept;

    void step(double dt_seconds, std::int64_t now_unix_ms);

    // Color transform used by the tonemap pass.
    [[nodiscard]] ColorMatrix3 active_color_transform() const noexcept;
    [[nodiscard]] ColorMode    color_mode() const noexcept;
    void set_color_mode(ColorMode m, float strength = 1.0f);

    // Text scale — mirrors `ui.text.scale` via CVar binding.
    [[nodiscard]] float text_scale() const noexcept;
    void  set_text_scale(float s);
    [[nodiscard]] bool  reduce_motion() const noexcept;
    [[nodiscard]] bool  photosensitivity_safe() const noexcept;

    // Subtitles API.
    bool push_subtitle(SubtitleCue cue, std::int64_t now_unix_ms);
    void snapshot_subtitles(std::vector<SubtitleLine>& out,
                              std::int64_t              now_unix_ms) const;
    [[nodiscard]] std::size_t subtitle_active_count() const noexcept;
    [[nodiscard]] std::size_t subtitle_emitted_count() const noexcept;

    // Screen-reader API.
    void update_ax_tree(std::span<const AccessibleNode> nodes);
    void announce(std::string_view utf8, Politeness p = Politeness::Polite);
    void announce_focus(std::uint64_t node_id);
    [[nodiscard]] IScreenReader* screen_reader() noexcept;
    [[nodiscard]] const IScreenReader* screen_reader() const noexcept;

    // Selfcheck.
    [[nodiscard]] SelfCheckReport selfcheck(const SelfCheckContext& ctx) const;

    [[nodiscard]] const A11yConfig& config() const noexcept;
    [[nodiscard]] std::uint64_t     step_count() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gw::a11y
