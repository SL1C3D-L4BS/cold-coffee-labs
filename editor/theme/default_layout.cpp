// editor/theme/default_layout.cpp — Part B §12.1 scaffold.

#include "editor/theme/default_layout.hpp"

namespace gw::editor::theme {

std::string_view default_layout_path(ThemeId id) noexcept {
    switch (id) {
    case ThemeId::CorruptedRelic:
        return "assets/editor/layouts/corrupted_relic.ini";
    case ThemeId::FieldTestHC:
        return "assets/editor/layouts/field_test_hc.ini";
    case ThemeId::BrewedSlate:
    default:
        return "assets/editor/layouts/brewed_slate.ini";
    }
}

} // namespace gw::editor::theme
