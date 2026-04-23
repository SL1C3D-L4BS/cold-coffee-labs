#pragma once
// editor/theme/theme_registry.hpp — Part B §12.1 scaffold (ADR-0104).
//
// Three-profile runtime-switchable theme. Stored as an `int` in the editor
// config TOML; swappable via `View > Theme`.

#include "editor/theme/effects.hpp"
#include "editor/theme/palette.hpp"
#include "editor/theme/typography.hpp"

#include <cstdint>

namespace gw::editor::theme {

enum class ThemeId : std::uint8_t {
    BrewedSlate    = 0,   // a11y default
    CorruptedRelic = 1,   // horror aesthetic
    FieldTestHC    = 2,   // WCAG AA high-contrast
};

struct Theme {
    ThemeId        id             = ThemeId::BrewedSlate;
    Palette        palette{};
    GraphTheme     graph{};
    TypographyPack typography{};
    ThemeEffects   effects{};
};

class ThemeRegistry {
public:
    [[nodiscard]] static ThemeRegistry& instance() noexcept;

    [[nodiscard]] ThemeId active_id() const noexcept { return active_.id; }
    [[nodiscard]] const Theme& active() const noexcept { return active_; }

    /// Swap the active theme; emits a `ThemeChangedEvent` consumed by the
    /// panel registry so every panel re-reads palette colours next frame.
    void set_active(ThemeId id) noexcept;

    /// Per-user opt-in toggle for individual effects without switching theme.
    void override_effect(ThemeEffectFlags flag, bool on) noexcept;

private:
    ThemeRegistry() noexcept { set_active(ThemeId::BrewedSlate); }

    Theme active_{};
};

[[nodiscard]] Theme theme_for(ThemeId id) noexcept;

} // namespace gw::editor::theme
