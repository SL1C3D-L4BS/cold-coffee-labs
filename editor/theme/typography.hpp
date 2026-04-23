#pragma once
// editor/theme/typography.hpp — Part B §12.1 scaffold.

#include <string_view>

namespace gw::editor::theme {

/// Declarative typography pack. Fonts ship as cooked `.gwtex` atlases; the
/// editor has ZERO OS-font dependency. The pack identifies the atlas paths;
/// the shell loads them at boot.
struct TypographyPack {
    std::string_view display_atlas;  // headings, menu titles
    std::string_view body_atlas;     // panel body text
    std::string_view mono_atlas;     // code / hex / paths
    float            display_px_size = 18.f;
    float            body_px_size    = 14.f;
    float            mono_px_size    = 13.f;
};

constexpr TypographyPack TYPO_CORRUPTED_RELIC{
    "assets/editor/fonts/metamorphous_display.gwtex",
    "assets/editor/fonts/metamorphous_body.gwtex",
    "assets/editor/fonts/jetbrains_mono.gwtex",
    20.f, 14.f, 13.f,
};

constexpr TypographyPack TYPO_BREWED_SLATE{
    "assets/editor/fonts/inter_display.gwtex",
    "assets/editor/fonts/inter_body.gwtex",
    "assets/editor/fonts/jetbrains_mono.gwtex",
    18.f, 14.f, 13.f,
};

constexpr TypographyPack TYPO_FIELD_TEST_HC{
    "assets/editor/fonts/atkinson_hyperlegible.gwtex",
    "assets/editor/fonts/atkinson_hyperlegible.gwtex",
    "assets/editor/fonts/atkinson_hyperlegible.gwtex",
    20.f, 16.f, 15.f,
};

} // namespace gw::editor::theme
