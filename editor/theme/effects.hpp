#pragma once
// editor/theme/effects.hpp — Part B §12.1 scaffold.

#include <cstdint>

namespace gw::editor::theme {

/// Bit-flags for optional horror effects. Each flag toggles a presentation-only
/// layer; none of these ever affect cooked content or authoritative state.
enum ThemeEffectFlags : std::uint32_t {
    EF_None             = 0,
    EF_Distressed       = 1u << 0, // jagged borders + pixel jitter on hover
    EF_Vignette         = 1u << 1, // corruption pulse overlay
    EF_GlitchHover      = 1u << 2, // glitch-text overlay on destructive labels
    EF_CornerCracks     = 1u << 3, // crack separators between sections
    EF_AsymmetricLayout = 1u << 4, // golden-ratio default dockspace
};

struct ThemeEffects {
    std::uint32_t flags = EF_None;

    [[nodiscard]] constexpr bool has(ThemeEffectFlags f) const noexcept {
        return (flags & f) != 0;
    }
    constexpr void set(ThemeEffectFlags f, bool on) noexcept {
        flags = on ? (flags | f) : (flags & ~std::uint32_t{f});
    }
};

constexpr ThemeEffects EFFECTS_CORRUPTED_RELIC{
    EF_Distressed | EF_Vignette | EF_GlitchHover | EF_CornerCracks | EF_AsymmetricLayout
};
constexpr ThemeEffects EFFECTS_BREWED_SLATE{EF_None};
constexpr ThemeEffects EFFECTS_FIELD_TEST{EF_None};

} // namespace gw::editor::theme
