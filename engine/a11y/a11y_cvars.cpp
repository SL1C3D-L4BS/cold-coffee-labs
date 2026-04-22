// engine/a11y/a11y_cvars.cpp

#include "engine/a11y/a11y_cvars.hpp"

namespace gw::a11y {

namespace {
constexpr std::uint32_t kP = config::kCVarPersist;
constexpr std::uint32_t kU = config::kCVarUserFacing;
} // namespace

A11yCVars register_a11y_cvars(config::CVarRegistry& r) {
    A11yCVars c{};
    c.color_mode = r.register_string({
        "a11y.color_mode", std::string{"none"}, kP | kU, {}, {}, false,
        "Color-vision simulation mode: none | protan | deutan | tritan | hc.",
    });
    c.color_strength = r.register_f32({
        "a11y.color_strength", 1.0f, kP | kU, 0.0f, 1.0f, true,
        "Strength of the color-vision simulation (0=identity,1=full).",
    });
    c.text_scale = r.register_f32({
        "a11y.text.scale", 1.0f, kP | kU, 0.5f, 2.5f, true,
        "Global UI text scale (WCAG 2.2 §1.4.4).",
    });
    c.contrast_boost = r.register_f32({
        "a11y.contrast.boost", 0.0f, kP | kU, 0.0f, 1.0f, true,
        "Post-tonemap contrast boost.",
    });
    c.reduce_motion = r.register_bool({
        "a11y.reduce_motion", false, kP | kU, {}, {}, false,
        "Clamp parallax / flashing / fade effects (WCAG 2.2 §2.3.3).",
    });
    c.photosensitivity_safe = r.register_bool({
        "a11y.photosensitivity_safe", false, kP | kU, {}, {}, false,
        "Enforce ≤3Hz flash rate (WCAG 2.2 §2.3.1 three-flash rule).",
    });
    c.subtitle_enabled = r.register_bool({
        "a11y.subtitle.enabled", true, kP | kU, {}, {}, false,
        "Show subtitles + captions.",
    });
    c.subtitle_scale = r.register_f32({
        "a11y.subtitle.scale", 1.0f, kP | kU, 0.5f, 2.0f, true,
        "Subtitle text scale.",
    });
    c.subtitle_bg_opacity = r.register_f32({
        "a11y.subtitle.bg_opacity", 0.7f, kP | kU, 0.0f, 1.0f, true,
        "Subtitle background opacity.",
    });
    c.subtitle_speaker_color = r.register_bool({
        "a11y.subtitle.speaker_color", true, kP | kU, {}, {}, false,
        "Per-speaker color tint for subtitles.",
    });
    c.subtitle_max_lines = r.register_i32({
        "a11y.subtitle.max_lines", 3, kP | kU, 1, 6, true,
        "Maximum concurrent subtitle lines.",
    });
    c.screen_reader_enabled = r.register_bool({
        "a11y.screen_reader.enabled", false, kP | kU, {}, {}, false,
        "Route UI tree to AccessKit-C / UIA / AT-SPI.",
    });
    c.screen_reader_verbosity = r.register_string({
        "a11y.screen_reader.verbosity", std::string{"terse"}, kP | kU, {}, {}, false,
        "Announcement verbosity: terse | normal | verbose.",
    });
    c.focus_show_ring = r.register_bool({
        "a11y.focus.show_ring", true, kP | kU, {}, {}, false,
        "Draw the keyboard-focus ring (WCAG 2.2 §2.4.7, §2.4.11).",
    });
    c.focus_ring_width_px = r.register_f32({
        "a11y.focus.ring_width_px", 2.0f, kP | kU, 1.0f, 8.0f, true,
        "Focus-ring stroke width, virtual pixels.",
    });
    c.wcag_version = r.register_string({
        "a11y.wcag.version", std::string{"2.2"}, config::kCVarReadOnly, {}, {}, false,
        "Targeted WCAG conformance version (read-only).",
    });
    return c;
}

} // namespace gw::a11y
