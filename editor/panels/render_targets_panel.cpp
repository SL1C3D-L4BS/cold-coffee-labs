// editor/panels/render_targets_panel.cpp
// Horizontal strip: slot 0 = offscreen scene RT; slots 1–5 = `vkCmdCopy` of
// that RT (stand-ins until the frame graph exposes real G-buffer / MRT).

#include "render_targets_panel.hpp"

#include "editor/theme/palette_imgui.hpp"

#include <imgui.h>

namespace {

[[nodiscard]] const char* slot_tooltip(int slot) noexcept {
    // clang-format off
    static constexpr const char* const k[6] = {
        "Slot 0: live offscreen scene colour (RGBA8). The editor clears it each frame, optionally draws, and "
        "the viewport samples this image. A depth target is already attached and cleared; a mesh or debug pass "
        "can write it in a later editor wave.",
        "Reserved: base colour / albedo. The image is a full-frame `vkCmdCopy` of the scene colour target "
        "until the renderer provides a true MRT albedo here.",
        "Reserved: per-pixel normals. Currently a copy of the scene colour image. Real G-buffer in a follow-on wave.",
        "Reserved: depth view (e.g. linearized). Currently a copy of the scene colour image. A depth preview "
        "requires a post pass or readback after geometry populates the depth image.",
        "Reserved: metalness and roughness (or packed material data). Currently a scene-colour copy.",
        "Reserved: ambient occlusion. Currently a scene-colour copy until a screen-space AO pass is wired in.",
    };
    // clang-format on
    if (slot < 0 || slot >= 6) return k[0];
    return k[slot];
}

}  // namespace

namespace gw::editor {

void RenderTargetsPanel::on_imgui_render(EditorContext& /*ctx*/) {
    if (!visible_) return;
    ImGui::Begin(name(), &visible_);

    const float  avail_w = ImGui::GetContentRegionAvail().x;
    const float  thumb_w = std::max(64.f, (avail_w - (kSlotCount - 1) * 6.f) / kSlotCount);
    const float  thumb_h = thumb_w * 9.f / 16.f;
    const ImVec2 thumb{thumb_w, thumb_h};

    const ImVec4  img_border = ImGui::GetStyleColorVec4(ImGuiCol_Border);
    const ImVec4  tint       = {1.f, 1.f, 1.f, 1.f};
    using gw::editor::theme::color32_to_im_u32;

    for (int i = 0; i < kSlotCount; ++i) {
        ImGui::BeginGroup();
        ImGui::PushID(i);

        if (slots_[i].live && slots_[i].tex != 0) {
            ImGui::Image(slots_[i].tex, thumb, ImVec2{0.f, 0.f}, ImVec2{1.f, 1.f},
                         tint, img_border);
        } else {
            const ImVec2 pos = ImGui::GetCursorScreenPos();
            ImDrawList*  dl  = ImGui::GetWindowDrawList();
            const auto&  pal = gw::editor::theme::ThemeRegistry::instance().active().palette;
            dl->AddRectFilled(pos,
                              ImVec2{pos.x + thumb.x, pos.y + thumb.y},
                              color32_to_im_u32(pal.panel),
                              4.f);
            dl->AddRect     (pos,
                            ImVec2{pos.x + thumb.x, pos.y + thumb.y},
                            color32_to_im_u32(pal.accent, 200.f / 255.f),
                            4.f);
            ImGui::Dummy(thumb);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", slot_tooltip(i));
        }

        {
            const ImVec4 name_col = slots_[i].live
                ? gw::editor::theme::active_positive_imgui()
                : gw::editor::theme::active_muted_imgui();
            ImGui::TextColored(name_col, "%s", slots_[i].name);
            if (slots_[i].mirrors_scene) {
                ImGui::SameLine(0.f, 0.f);
                ImGui::TextColored(gw::editor::theme::active_muted_imgui(), "*");
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", slot_tooltip(i));
        }

        ImGui::PopID();
        ImGui::EndGroup();

        if (i + 1 < kSlotCount) {
            ImGui::SameLine(0.f, 6.f);
        }
    }

    ImGui::Separator();
    ImGui::TextDisabled("Legend");

    constexpr const char* k_m = "Asterisk (*) = reserved G-buffer (or similar) name; the image is a full-frame copy "
        "of the offscreen scene colour (slot 0), not a separate MRT. ";
    constexpr const char* k_e = "This pass does not include: multi-render-target exports into these thumbnails, a "
        "GPU object-ID pick buffer, or a depth buffer as greyscale / RGBA (that needs a post pass or readback after "
        "geometry writes the depth target). The offscreen pass already clears and binds a depth target for a future "
        "editor mesh or debug path.";

    const float wrap_w = ImGui::GetContentRegionAvail().x;
    if (wrap_w > 4.f) {
        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + wrap_w);
    } else {
        ImGui::PushTextWrapPos(0.f);
    }
    ImGui::TextDisabled("%s", k_m);
    ImGui::TextDisabled("%s", k_e);
    ImGui::PopTextWrapPos();

    ImGui::End();
}

}  // namespace gw::editor
