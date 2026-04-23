#pragma once
// Maps ThemeRegistry semantic Palette → full ImGuiStyle (ADR-0104).

#include "editor/theme/palette.hpp"

struct ImGuiStyle;

namespace gw::editor::theme {

/// Fills `style.Colors[]` and shared rounding/padding. When `viewports_enable`
/// is true, forces window rounding 0 and full-opaque WindowBg (ImGui viewports).
void apply_palette_to_imgui_style(const Palette& palette, ImGuiStyle& style,
                                  bool viewports_enable) noexcept;

} // namespace gw::editor::theme
