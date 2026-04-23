#pragma once
// ImVec4 helpers for panel code (ADR-0104).

#include "editor/theme/palette.hpp"
#include "editor/theme/theme_registry.hpp"

#include <imgui.h>

namespace gw::editor::theme {

[[nodiscard]] inline ImVec4 imgui_vec4(const Color32& c,
                                       float alpha_scale = 1.f) noexcept {
    return ImVec4{c.r / 255.f, c.g / 255.f, c.b / 255.f,
                  (c.a / 255.f) * alpha_scale};
}

/// `ImU32` for `ImDrawList` paths — same result as `IM_COL32(r,g,b,a)` for the
/// active channel scaling.
[[nodiscard]] inline ImU32 color32_to_im_u32(const Color32& c,
                                             float alpha_scale = 1.f) noexcept {
    return ImGui::GetColorU32(imgui_vec4(c, alpha_scale));
}

[[nodiscard]] inline ImVec4 active_positive_imgui() noexcept {
    return imgui_vec4(ThemeRegistry::instance().active().palette.positive);
}

[[nodiscard]] inline ImVec4 active_muted_imgui() noexcept {
    return imgui_vec4(ThemeRegistry::instance().active().palette.text_muted);
}

[[nodiscard]] inline ImVec4 active_warning_imgui() noexcept {
    return imgui_vec4(ThemeRegistry::instance().active().palette.warning);
}

[[nodiscard]] inline ImVec4 active_accent_imgui() noexcept {
    return imgui_vec4(ThemeRegistry::instance().active().palette.accent);
}

} // namespace gw::editor::theme
