// editor/theme/imgui_style_from_palette.cpp

#include "editor/theme/imgui_style_from_palette.hpp"

#include <imgui.h>

namespace gw::editor::theme {
namespace {

[[nodiscard]] ImVec4 C(const Color32& c, float a_scale = 1.f) noexcept {
    return ImVec4{c.r / 255.f, c.g / 255.f, c.b / 255.f,
                  (c.a / 255.f) * a_scale};
}

[[nodiscard]] ImVec4 lerp(const ImVec4& a, const ImVec4& b, float t) noexcept {
    return ImVec4{a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t,
                  a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t};
}

[[nodiscard]] ImVec4 mul_alpha(ImVec4 v, float m) noexcept {
    v.w *= m;
    return v;
}

} // namespace

void apply_palette_to_imgui_style(const Palette& palette, ImGuiStyle& style,
                                  bool viewports_enable) noexcept {
    style.WindowRounding    = 4.f;
    style.ChildRounding     = 3.f;
    style.FrameRounding     = 3.f;
    style.GrabRounding      = 3.f;
    style.PopupRounding     = 4.f;
    style.ScrollbarRounding = 3.f;
    style.TabRounding       = 4.f;

    style.WindowPadding     = {8.f, 8.f};
    style.FramePadding      = {5.f, 3.f};
    style.ItemSpacing       = {8.f, 6.f};
    style.ScrollbarSize     = 12.f;
    style.GrabMinSize       = 8.f;
    style.WindowBorderSize  = 1.f;
    style.FrameBorderSize   = 0.f;
    style.TabBarBorderSize  = 1.f;

    if (viewports_enable)
        style.WindowRounding = 0.f;

    ImGui::StyleColorsDark(&style);

    const ImVec4 text          = C(palette.text);
    const ImVec4 text_dis      = C(palette.text_muted);
    const ImVec4 bg            = C(palette.background);
    const ImVec4 panel         = C(palette.panel);
    const ImVec4 surf1        = C(palette.surface_1);
    const ImVec4 surf2        = C(palette.surface_2);
    const ImVec4 accent        = C(palette.accent);
    const ImVec4 accent_alt    = C(palette.accent_secondary);
    const ImVec4 accent_hi     = C(palette.accent_strong);
    const ImVec4 info_c        = C(palette.info);
    const ImVec4 link_c        = C(palette.link);
    const ImVec4 focus_c       = C(palette.focus);
    const ImVec4 bad           = C(palette.destructive);
    const ImVec4 good          = C(palette.positive);
    const ImVec4 warn          = C(palette.warning);
    const ImVec4 sep           = C(palette.separator);
    const ImVec4 sel           = C(palette.selection);

    const ImVec4 frame_base    = lerp(bg, panel, 0.55f);
    const ImVec4 frame_hov     = lerp(panel, link_c, 0.22f);
    const ImVec4 frame_act     = lerp(panel, info_c, 0.32f);
    const ImVec4 title         = lerp(bg, surf1, 0.5f);
    const ImVec4 title_act     = lerp(surf1, accent, 0.18f);
    const ImVec4 scroll_track  = lerp(bg, panel, 0.4f);
    const ImVec4 scroll_grab   = lerp(sep, accent, 0.35f);
    const ImVec4 scroll_grab_h = lerp(scroll_grab, accent_hi, 0.5f);
    const ImVec4 scroll_grab_a = accent_hi;

    const ImVec4 table_alt =
        mul_alpha(lerp(C(palette.surface_1), text, 0.06f), 0.45f);

    ImVec4* col = style.Colors;
    col[ImGuiCol_Text]                  = text;
    col[ImGuiCol_TextDisabled]          = text_dis;
    col[ImGuiCol_WindowBg]              = bg;
    col[ImGuiCol_ChildBg]               = panel;
    col[ImGuiCol_PopupBg]               = mul_alpha(surf2, viewports_enable ? 1.f : 0.98f);
    col[ImGuiCol_Border]                = sep;
    col[ImGuiCol_BorderShadow]          = ImVec4{0, 0, 0, 0};
    col[ImGuiCol_FrameBg]               = frame_base;
    col[ImGuiCol_FrameBgHovered]        = frame_hov;
    col[ImGuiCol_FrameBgActive]         = frame_act;
    col[ImGuiCol_TitleBg]               = title;
    col[ImGuiCol_TitleBgActive]         = title_act;
    col[ImGuiCol_TitleBgCollapsed] =
        mul_alpha(lerp(C(palette.surface_2), C(palette.surface_3), 0.45f), 0.88f);
    col[ImGuiCol_MenuBarBg]             = surf1;
    col[ImGuiCol_ScrollbarBg]           = scroll_track;
    col[ImGuiCol_ScrollbarGrab]         = scroll_grab;
    col[ImGuiCol_ScrollbarGrabHovered]  = scroll_grab_h;
    col[ImGuiCol_ScrollbarGrabActive]   = scroll_grab_a;
    col[ImGuiCol_CheckMark]             = good;
    col[ImGuiCol_SliderGrab]            = info_c;
    col[ImGuiCol_SliderGrabActive]      = link_c;
    col[ImGuiCol_Button]                = frame_base;
    col[ImGuiCol_ButtonHovered]         = mul_alpha(accent, 0.42f);
    col[ImGuiCol_ButtonActive]          = mul_alpha(accent_hi, 0.72f);
    col[ImGuiCol_Header]                = mul_alpha(accent, 0.32f);
    col[ImGuiCol_HeaderHovered]         = mul_alpha(accent, 0.5f);
    col[ImGuiCol_HeaderActive]          = mul_alpha(accent_hi, 0.75f);
    col[ImGuiCol_Separator]             = sep;
    col[ImGuiCol_SeparatorHovered]      = mul_alpha(bad, 0.45f);
    col[ImGuiCol_SeparatorActive]       = mul_alpha(accent_hi, 0.85f);
    col[ImGuiCol_ResizeGrip]            = mul_alpha(info_c, 0.28f);
    col[ImGuiCol_ResizeGripHovered]     = lerp(mul_alpha(info_c, 0.55f),
                                               mul_alpha(link_c, 0.5f), 0.4f);
    col[ImGuiCol_ResizeGripActive]      = mul_alpha(accent_hi, 0.85f);
    col[ImGuiCol_Tab]                   = lerp(bg, panel, 0.65f);
    col[ImGuiCol_TabHovered]            = lerp(mul_alpha(accent, 0.5f),
                                               mul_alpha(accent_alt, 0.42f), 0.35f);
    col[ImGuiCol_TabActive]             = lerp(panel, accent, 0.22f);
    col[ImGuiCol_TabUnfocused]          = lerp(bg, panel, 0.5f);
    col[ImGuiCol_TabUnfocusedActive]    = lerp(panel, accent, 0.12f);
    col[ImGuiCol_DockingPreview]        = mul_alpha(focus_c, 0.55f);
    col[ImGuiCol_DockingEmptyBg]        = bg;
    col[ImGuiCol_PlotLines]             = info_c;
    col[ImGuiCol_PlotLinesHovered]      = mul_alpha(bad, 0.92f);
    col[ImGuiCol_PlotHistogram]         = good;
    col[ImGuiCol_PlotHistogramHovered]  = mul_alpha(bad, 0.75f);
    col[ImGuiCol_TableHeaderBg]         = lerp(surf1, accent, 0.06f);
    col[ImGuiCol_TableBorderStrong]     = lerp(sep, accent, 0.12f);
    col[ImGuiCol_TableBorderLight]      = mul_alpha(sep, 0.65f);
    col[ImGuiCol_TableRowBg]            = ImVec4{0, 0, 0, 0};
    col[ImGuiCol_TableRowBgAlt]        = table_alt;
    col[ImGuiCol_TextSelectedBg]        = sel;
    col[ImGuiCol_DragDropTarget]        = mul_alpha(warn, 0.85f);
    col[ImGuiCol_NavHighlight]          = mul_alpha(focus_c, 0.95f);
    col[ImGuiCol_NavWindowingHighlight] = mul_alpha(focus_c, 0.88f);
    col[ImGuiCol_NavWindowingDimBg]     = ImVec4{0, 0, 0, 0.45f};
    col[ImGuiCol_ModalWindowDimBg]      = ImVec4{0, 0, 0, 0.55f};

    if (viewports_enable)
        col[ImGuiCol_WindowBg].w = 1.f;
}

} // namespace gw::editor::theme
