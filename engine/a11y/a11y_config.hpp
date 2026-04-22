#pragma once
// engine/a11y/a11y_config.hpp — ADR-0071.

#include "engine/a11y/a11y_types.hpp"

#include <cstdint>
#include <string>

namespace gw::a11y {

struct A11yConfig {
    ColorMode    color_mode{ColorMode::None};
    float        color_strength{1.0f};
    float        text_scale{1.0f};
    float        contrast_boost{0.0f};
    bool         reduce_motion{false};
    bool         photosensitivity_safe{false};

    bool         subtitles_enabled{true};
    float        subtitle_scale{1.0f};
    float        subtitle_bg_opacity{0.7f};
    bool         subtitle_speaker_color{true};
    std::int32_t subtitle_max_lines{3};

    bool         screen_reader_enabled{false};
    std::string  screen_reader_verbosity{"terse"};

    bool         focus_show_ring{true};
    float        focus_ring_width_px{2.0f};

    std::string  wcag_version{"2.2"};
};

} // namespace gw::a11y
