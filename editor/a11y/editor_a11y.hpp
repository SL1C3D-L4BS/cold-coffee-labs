#pragma once
// editor/a11y/editor_a11y.hpp — editor accessibility (ADR-0108, Wave 1C).
//
// Editor-side accessibility toggles. Mirrors the runtime a11y work in
// engine/a11y but scopes the editor-only presentation surface.
// TOML round-trip is unit-tested; production path is `editor_data_root()/editor_a11y.toml`.

#include <filesystem>

namespace gw::editor::a11y {

struct EditorA11yConfig {
    bool reduce_corruption  = false;  // disables pulse + glitch + jitter
    bool disable_vignette   = false;  // flat background
    bool force_high_contrast = false; // swap to Field Test HC palette
    bool force_mono_font    = false;  // JetBrains Mono everywhere
    bool keyboard_only_nav  = false;  // promote Tab order, hide mouse hints
    bool colour_blind_wong  = false;  // Wong palette overlay
    bool reduce_motion      = false;  // vignette pulse, glitch offset, border jitter
};

/// Load / save from a specific path (e.g. unit tests use a temp file; production
/// should call `load_from_config` / `save_to_config`).
/// Parser accepts optional `[editor_a11y]` section headers, `#` / `;` comments,
/// and `key = value` lines (bool: true/false/1/0/yes/no/on/off).
[[nodiscard]] EditorA11yConfig load_from_path(const std::filesystem::path& path) noexcept;
void                           save_to_path(
                                const EditorA11yConfig&    cfg, const std::filesystem::path& path) noexcept;

/// Persist / load from `editor_data_root()/editor_a11y.toml`.
EditorA11yConfig load_from_config() noexcept;
void             save_to_config(const EditorA11yConfig& cfg) noexcept;

/// Apply the current config to the ThemeRegistry in one shot (flips effect
/// flags + optionally swaps theme / typography).
void apply(const EditorA11yConfig& cfg) noexcept;

} // namespace gw::editor::a11y
