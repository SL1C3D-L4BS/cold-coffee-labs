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

[[nodiscard]] inline const Palette& active_palette() noexcept {
    return ThemeRegistry::instance().active().palette;
}

/// Elevation ramp: 1 = menu/toolbar step, 2 = popups, 3 = peak chrome.
[[nodiscard]] inline ImVec4 active_surface_imgui(int level) noexcept {
    const Palette& p = active_palette();
    switch (level) {
    case 1: return imgui_vec4(p.surface_1);
    case 2: return imgui_vec4(p.surface_2);
    case 3: return imgui_vec4(p.surface_3);
    default: return imgui_vec4(p.panel);
    }
}

[[nodiscard]] inline ImVec4 active_positive_imgui() noexcept {
    return imgui_vec4(active_palette().positive);
}

[[nodiscard]] inline ImVec4 active_muted_imgui() noexcept {
    return imgui_vec4(active_palette().text_muted);
}

[[nodiscard]] inline ImVec4 active_warning_imgui() noexcept {
    return imgui_vec4(active_palette().warning);
}

[[nodiscard]] inline ImVec4 active_accent_imgui() noexcept {
    return imgui_vec4(active_palette().accent);
}

[[nodiscard]] inline ImVec4 active_accent_secondary_imgui() noexcept {
    return imgui_vec4(active_palette().accent_secondary);
}

[[nodiscard]] inline ImVec4 active_accent_strong_imgui() noexcept {
    return imgui_vec4(active_palette().accent_strong);
}

[[nodiscard]] inline ImVec4 active_destructive_imgui() noexcept {
    return imgui_vec4(active_palette().destructive);
}

[[nodiscard]] inline ImVec4 active_info_imgui() noexcept {
    return imgui_vec4(active_palette().info);
}

[[nodiscard]] inline ImVec4 active_link_imgui() noexcept {
    return imgui_vec4(active_palette().link);
}

[[nodiscard]] inline ImVec4 active_focus_imgui() noexcept {
    return imgui_vec4(active_palette().focus);
}

[[nodiscard]] inline ImVec4 active_on_accent_imgui() noexcept {
    return imgui_vec4(active_palette().on_accent);
}

} // namespace gw::editor::theme
