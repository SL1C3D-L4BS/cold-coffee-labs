#pragma once

#include "editor/theme/typography.hpp"

struct ImGuiIO;

namespace gw::editor::theme {

/// Applies `TypographyPack` sizes to ImGui's default font atlas. Call once
/// after `ImGui::CreateContext()` and before the first frame / backend Init.
/// When `force_mono_metrics` is true, uses `mono_px_size` from the pack
/// (default atlas remains Proggy until `.gwtex` loading lands).
void load_editor_fonts(ImGuiIO& io, const TypographyPack& typo,
                        bool force_mono_metrics = false) noexcept;

} // namespace gw::editor::theme
