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
        t.graph      = GRAPH_CORRUPTED_RELIC;
        t.typography = TYPO_CORRUPTED_RELIC;
        t.effects    = EFFECTS_CORRUPTED_RELIC;
        break;
    case ThemeId::FieldTestHC:
        t.palette    = FIELD_TEST_HC;
        t.graph      = GRAPH_FIELD_TEST_HC;
        t.typography = TYPO_FIELD_TEST_HC;
        t.effects    = EFFECTS_FIELD_TEST;
        break;
    case ThemeId::BrewedSlate:
    default:
        t.palette    = BREWED_SLATE;
        t.graph      = GRAPH_BREWED_SLATE;
        t.typography = TYPO_BREWED_SLATE;
        t.effects    = EFFECTS_BREWED_SLATE;
        break;
    }
    return t;
}

ThemeRegistry::ThemeRegistry() noexcept : stored_theme_id_{ThemeId::BrewedSlate} {
    rebuild_composed_theme();
}

void ThemeRegistry::rebuild_composed_theme() noexcept {
    active_ = theme_for(stored_theme_id_);
    if (wong_semantic_overlay_ && stored_theme_id_ != ThemeId::FieldTestHC) {
        active_.palette = overlay_wong_semantics(active_.palette);
        active_.graph    = overlay_wong_graph_semantics(active_.graph);
    }
}

void ThemeRegistry::set_active(ThemeId id) noexcept {
    stored_theme_id_ = id;
    rebuild_composed_theme();
}

void ThemeRegistry::set_wong_semantic_overlay(bool on) noexcept {
    wong_semantic_overlay_ = on;
    rebuild_composed_theme();
}

void ThemeRegistry::set_reduce_motion(bool on) noexcept { reduce_motion_ = on; }

void ThemeRegistry::override_effect(ThemeEffectFlags flag, bool on) noexcept {
    active_.effects.set(flag, on);
}

} // namespace gw::editor::theme
