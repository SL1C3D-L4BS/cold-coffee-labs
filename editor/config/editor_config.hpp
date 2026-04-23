#pragma once

#include "editor/theme/theme_registry.hpp"

namespace gw::editor::config {

/// Returns persisted theme or `fallback` when missing / unreadable.
[[nodiscard]] gw::editor::theme::ThemeId load_saved_theme(
    gw::editor::theme::ThemeId fallback) noexcept;

void save_theme(gw::editor::theme::ThemeId id) noexcept;

} // namespace gw::editor::config
