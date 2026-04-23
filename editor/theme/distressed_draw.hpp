#pragma once
// editor/theme/distressed_draw.hpp — Part B §12.1 scaffold.
//
// Custom ImDrawList helpers that implement the Corrupted Relic aesthetic
// without introducing a custom UI framework. Pure ImGui primitives.

#include <cstdint>

namespace gw::editor::theme {

struct DistressedParams {
    float         jitter_amplitude_px = 1.5f;
    float         crack_thickness_px  = 1.0f;
    std::uint8_t  notch_count         = 3;
    /// `ImU32` colour in the same packing as `IM_COL32(r,g,b,a)`.
    std::uint32_t accent_col          = 0xFFC9A63Bu;
};

/// Draw a jagged rectangular border with irregular notches. Called from
/// DistressedButton / DistressedSlider when `EF_Distressed` is set.
/// The implementation uses `ImGui::GetWindowDrawList()` internally; declared
/// here as a shim so the header is self-contained.
void draw_jagged_border(float x0, float y0, float x1, float y1,
                        const DistressedParams& params) noexcept;

/// Pixel jitter overlay for hovered widgets. Deterministic per-widget using
/// the stable ImGui ID so hovering the same element always jitters the same
/// way (no visual noise between frames on idle).
void draw_pixel_jitter_hover(float x0, float y0, float x1, float y1,
                             std::uint32_t imgui_id,
                             const DistressedParams& params) noexcept;

/// Crack separator — an irregular polyline between two panes.
void draw_crack_separator(float x0, float y0, float x1, float y1,
                          const DistressedParams& params) noexcept;

} // namespace gw::editor::theme
