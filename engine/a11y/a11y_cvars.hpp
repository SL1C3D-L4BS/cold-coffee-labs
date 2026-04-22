#pragma once
// engine/a11y/a11y_cvars.hpp — ADR-0071 §2 (16 a11y.* CVars).

#include "engine/core/config/cvar_registry.hpp"

namespace gw::a11y {

struct A11yCVars {
    config::CVarRef<std::string>  color_mode{};
    config::CVarRef<float>        color_strength{};
    config::CVarRef<float>        text_scale{};
    config::CVarRef<float>        contrast_boost{};
    config::CVarRef<bool>         reduce_motion{};
    config::CVarRef<bool>         photosensitivity_safe{};
    config::CVarRef<bool>         subtitle_enabled{};
    config::CVarRef<float>        subtitle_scale{};
    config::CVarRef<float>        subtitle_bg_opacity{};
    config::CVarRef<bool>         subtitle_speaker_color{};
    config::CVarRef<std::int32_t> subtitle_max_lines{};
    config::CVarRef<bool>         screen_reader_enabled{};
    config::CVarRef<std::string>  screen_reader_verbosity{};
    config::CVarRef<bool>         focus_show_ring{};
    config::CVarRef<float>        focus_ring_width_px{};
    config::CVarRef<std::string>  wcag_version{};
};

[[nodiscard]] A11yCVars register_a11y_cvars(config::CVarRegistry& r);

} // namespace gw::a11y
