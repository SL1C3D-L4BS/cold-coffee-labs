#pragma once
// editor/widgets/distressed_widgets.hpp — Part B §12.3 scaffold.
//
// Wrappers around ImGui primitives that add the Corrupted Relic distressed
// aesthetic when `ThemeEffectFlags::EF_Distressed` is active, and fall back
// to vanilla ImGui otherwise.

#include <cstdint>

namespace gw::editor::widgets {

bool DistressedButton(const char* label) noexcept;
bool DistressedButton(const char* label, float width, float height) noexcept;

bool DistressedCheckbox(const char* label, bool* v) noexcept;

bool DistressedSliderFloat(const char* label, float* v,
                           float v_min, float v_max) noexcept;

/// Hover-glitch overlay for destructive labels. Presentation only.
void GlitchText(const char* label) noexcept;

/// Runic-ring arc progress bar. Used for Sin / Mantra / Grace previews
/// in PIE debug overlays.
void BlasphemousProgressBar(float fraction_0_1,
                            std::uint32_t color_abgr = 0xFF'3BA6C9u) noexcept;

/// Drag-source asset tile with thumbnail + corruption badge + hover glyph.
/// Returns true if the tile was clicked this frame.
bool AssetPreviewTile(const char* id, const char* display_name,
                      void* texture_id, float size_px) noexcept;

} // namespace gw::editor::widgets
