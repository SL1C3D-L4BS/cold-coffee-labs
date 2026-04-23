#pragma once

#include "editor/theme/typography.hpp"

struct ImGuiIO;

namespace gw::editor::theme {

/// Applies `TypographyPack` sizes to ImGui's default font atlas. Call once
/// after `ImGui::CreateContext()` and before the first frame / backend Init.
void load_editor_fonts(ImGuiIO& io, const TypographyPack& typo) noexcept;

} // namespace gw::editor::theme
