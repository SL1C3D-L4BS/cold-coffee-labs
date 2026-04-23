#pragma once
// editor/a11y/editor_a11y.hpp — Part B §14 scaffold (ADR-0108).
//
// Editor-side accessibility toggles. Mirrors the runtime a11y work in
// engine/a11y (Phase 16) but scopes the editor-only presentation surface.

namespace gw::editor::a11y {

struct EditorA11yConfig {
    bool reduce_corruption  = false;  // disables pulse + glitch + jitter
    bool disable_vignette   = false;  // flat background
    bool force_high_contrast = false; // swap to Field Test HC palette
    bool force_mono_font    = false;  // JetBrains Mono everywhere
    bool keyboard_only_nav  = false;  // promote Tab order, hide mouse hints
    bool colour_blind_wong  = false;  // Wong palette overlay
};

/// Persist / load to editor config TOML via engine/core/config.
EditorA11yConfig load_from_config() noexcept;
void             save_to_config(const EditorA11yConfig& cfg) noexcept;

/// Apply the current config to the ThemeRegistry in one shot (flips effect
/// flags + optionally swaps theme / typography).
void apply(const EditorA11yConfig& cfg) noexcept;

} // namespace gw::editor::a11y
