// editor/panels/render_targets_panel.cpp
// Horizontal strip of G-buffer debug thumbnails. Slot 0 is the live scene
// colour RT (bound by EditorApplication); remaining slots render a solid
// "coming in Phase 8" placeholder so the cockpit row still feels populated.
#include "render_targets_panel.hpp"

#include "editor/theme/palette_imgui.hpp"

#include <imgui.h>
#include <algorithm>

namespace gw::editor {

void RenderTargetsPanel::on_imgui_render(EditorContext& /*ctx*/) {
    if (!visible_) return;
    ImGui::Begin(name(), &visible_);

    const float avail_w  = ImGui::GetContentRegionAvail().x;
    const float thumb_w  = std::max(64.f, (avail_w - (kSlotCount - 1) * 6.f) / kSlotCount);
    const float thumb_h  = thumb_w * 9.f / 16.f;
    const ImVec2 thumb{thumb_w, thumb_h};

    for (int i = 0; i < kSlotCount; ++i) {
        ImGui::BeginGroup();
        ImGui::PushID(i);

        if (slots_[i].live && slots_[i].tex != 0) {
            ImGui::Image(slots_[i].tex, thumb);
        } else {
            // Placeholder — translucent blue card with the slot label.
            const ImVec2 pos = ImGui::GetCursorScreenPos();
            ImDrawList* dl   = ImGui::GetWindowDrawList();
            const auto& pal =
                gw::editor::theme::ThemeRegistry::instance().active().palette;
            dl->AddRectFilled(pos,
                               ImVec2{pos.x + thumb.x, pos.y + thumb.y},
                               IM_COL32(pal.panel.r, pal.panel.g, pal.panel.b, 255),
                               4.f);
            dl->AddRect      (pos,
                               ImVec2{pos.x + thumb.x, pos.y + thumb.y},
                               IM_COL32(pal.accent.r, pal.accent.g, pal.accent.b, 200),
                               4.f);
            ImGui::Dummy(thumb);
        }

        ImGui::TextColored(slots_[i].live
                               ? gw::editor::theme::active_positive_imgui()
                               : gw::editor::theme::active_muted_imgui(),
                           "%s", slots_[i].label);

        ImGui::PopID();
        ImGui::EndGroup();

        if (i + 1 < kSlotCount) ImGui::SameLine(0.f, 6.f);
    }

    ImGui::End();
}

}  // namespace gw::editor
