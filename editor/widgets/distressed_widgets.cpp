// editor/widgets/distressed_widgets.cpp

#include "editor/widgets/distressed_widgets.hpp"

#include "editor/theme/distressed_draw.hpp"
#include "editor/theme/theme_registry.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>

namespace gw::editor::widgets {
namespace {

[[nodiscard]] bool distressed_enabled() noexcept {
    return gw::editor::theme::ThemeRegistry::instance()
        .active()
        .effects.has(gw::editor::theme::EF_Distressed);
}

[[nodiscard]] gw::editor::theme::DistressedParams distressed_params() noexcept {
    gw::editor::theme::DistressedParams p{};
    const auto& pal =
        gw::editor::theme::ThemeRegistry::instance().active().palette;
    p.accent_col = IM_COL32(pal.accent.r, pal.accent.g, pal.accent.b, pal.accent.a);
    return p;
}

[[nodiscard]] bool glitch_enabled() noexcept {
    return gw::editor::theme::ThemeRegistry::instance()
        .active()
        .effects.has(gw::editor::theme::EF_GlitchHover);
}

} // namespace

bool DistressedButton(const char* label) noexcept {
    return DistressedButton(label, 0.f, 0.f);
}

bool DistressedButton(const char* label, float width, float height) noexcept {
    const bool d = distressed_enabled();
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    const bool   clicked =
        width > 0.f && height > 0.f
            ? ImGui::Button(label, ImVec2{width, height})
            : ImGui::Button(label);
    const ImVec2 p1 = ImGui::GetItemRectMax();

    if (d) {
        gw::editor::theme::draw_jagged_border(p0.x - 2.f, p0.y - 2.f, p1.x + 2.f,
                                              p1.y + 2.f, distressed_params());
        gw::editor::theme::draw_pixel_jitter_hover(
            p0.x, p0.y, p1.x, p1.y,
            static_cast<std::uint32_t>(ImGui::GetItemID()), distressed_params());
    }
    return clicked;
}

bool DistressedCheckbox(const char* label, bool* v) noexcept {
    const bool d = distressed_enabled();
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    const bool   ch = ImGui::Checkbox(label, v);
    const ImVec2 p1 = ImGui::GetItemRectMax();
    if (d) {
        gw::editor::theme::draw_pixel_jitter_hover(
            p0.x, p0.y, p1.x, p1.y,
            static_cast<std::uint32_t>(ImGui::GetItemID()), distressed_params());
    }
    return ch;
}

bool DistressedSliderFloat(const char* label, float* v, float v_min,
                           float v_max) noexcept {
    const bool d = distressed_enabled();
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    const bool   s = ImGui::SliderFloat(label, v, v_min, v_max);
    const ImVec2 p1 = ImGui::GetItemRectMax();
    if (d) {
        gw::editor::theme::draw_pixel_jitter_hover(
            p0.x, p0.y, p1.x, p1.y,
            static_cast<std::uint32_t>(ImGui::GetItemID()), distressed_params());
    }
    return s;
}

void GlitchText(const char* label) noexcept {
    const ImVec2 p = ImGui::GetCursorScreenPos();
    if (glitch_enabled()) {
        const float ox =
            std::sin(static_cast<float>(ImGui::GetFrameCount()) * 0.31f) * 1.25f;
        ImGui::SetCursorScreenPos(ImVec2{p.x + ox, p.y});
    }
    ImGui::TextUnformatted(label);
}

void BlasphemousProgressBar(float fraction_0_1, std::uint32_t color_abgr) noexcept {
    fraction_0_1 = std::clamp(fraction_0_1, 0.f, 1.f);
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float  w = ImGui::CalcItemWidth();
    const float  h = ImGui::GetFrameHeight() * 0.65f;
    ImDrawList*  dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p, ImVec2{p.x + w, p.y + h}, IM_COL32(12, 12, 18, 255), 3.f);
    dl->AddRectFilled(p, ImVec2{p.x + w * fraction_0_1, p.y + h},
                      color_abgr, 3.f);
    dl->AddRect(p, ImVec2{p.x + w, p.y + h}, IM_COL32(255, 255, 255, 60), 3.f);
    ImGui::Dummy(ImVec2{w, h});
}

bool AssetPreviewTile(const char* id, const char* display_name, void* tex,
                      float size_px) noexcept {
    ImGui::PushID(id);
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const ImVec2 sz{size_px, size_px};
    if (tex)
        ImGui::Image(tex, sz);
    else
        ImGui::Dummy(sz);
    const bool click = ImGui::IsItemClicked();
    ImGui::SetCursorScreenPos(ImVec2{p.x, p.y + size_px + 4.f});
    ImGui::TextUnformatted(display_name);
    ImGui::PopID();
    return click;
}

} // namespace gw::editor::widgets
