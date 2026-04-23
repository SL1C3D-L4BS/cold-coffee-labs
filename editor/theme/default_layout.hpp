#pragma once
// editor/theme/default_layout.hpp — Part B §12.1 scaffold.
//
// Declarative dockspace layouts, one per theme. Serialised as ImGui `.ini`
// fragments shipped with the editor; `View > Reset Layout` applies them.

#include "editor/theme/theme_registry.hpp"

#include <string_view>

namespace gw::editor::theme {

/// Returns the path to the cooked `.ini` dockspace layout for the theme.
/// Corrupted Relic uses a golden-ratio asymmetric layout; Brewed Slate and
/// Field Test HC use a symmetric grid.
[[nodiscard]] std::string_view default_layout_path(ThemeId id) noexcept;

} // namespace gw::editor::theme
