// editor/a11y/editor_a11y.cpp — Part B §14 scaffold.

#include "editor/a11y/editor_a11y.hpp"

#include "editor/theme/theme_registry.hpp"

namespace gw::editor::a11y {

EditorA11yConfig load_from_config() noexcept {
    // Phase 21: read from engine/core/config TOML. Scaffold returns defaults.
    return EditorA11yConfig{};
}

void save_to_config(const EditorA11yConfig& /*cfg*/) noexcept {
    // Phase 21: write back to editor config TOML.
}

void apply(const EditorA11yConfig& cfg) noexcept {
    auto& theme = gw::editor::theme::ThemeRegistry::instance();
    if (cfg.force_high_contrast) {
        theme.set_active(gw::editor::theme::ThemeId::FieldTestHC);
    }
    if (cfg.reduce_corruption) {
        theme.override_effect(gw::editor::theme::EF_Distressed,  false);
        theme.override_effect(gw::editor::theme::EF_GlitchHover, false);
        theme.override_effect(gw::editor::theme::EF_CornerCracks,false);
    }
    if (cfg.disable_vignette) {
        theme.override_effect(gw::editor::theme::EF_Vignette, false);
    }
}

} // namespace gw::editor::a11y
