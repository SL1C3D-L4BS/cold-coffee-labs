// editor/theme/theme_registry.cpp — Part B §12.1 scaffold.

#include "editor/theme/theme_registry.hpp"

namespace gw::editor::theme {

ThemeRegistry& ThemeRegistry::instance() noexcept {
    static ThemeRegistry inst{};
    return inst;
}

Theme theme_for(ThemeId id) noexcept {
    Theme t{};
    t.id = id;
    switch (id) {
    case ThemeId::CorruptedRelic:
        t.palette    = CORRUPTED_RELIC;
        t.typography = TYPO_CORRUPTED_RELIC;
        t.effects    = EFFECTS_CORRUPTED_RELIC;
        break;
    case ThemeId::FieldTestHC:
        t.palette    = FIELD_TEST_HC;
        t.typography = TYPO_FIELD_TEST_HC;
        t.effects    = EFFECTS_FIELD_TEST;
        break;
    case ThemeId::BrewedSlate:
    default:
        t.palette    = BREWED_SLATE;
        t.typography = TYPO_BREWED_SLATE;
        t.effects    = EFFECTS_BREWED_SLATE;
        break;
    }
    return t;
}

void ThemeRegistry::set_active(ThemeId id) noexcept {
    active_ = theme_for(id);
}

void ThemeRegistry::override_effect(ThemeEffectFlags flag, bool on) noexcept {
    active_.effects.set(flag, on);
}

} // namespace gw::editor::theme
