// editor/panels/render_targets_panel.cpp
// Horizontal strip of G-buffer debug thumbnails. Slot 0 is the live scene
// colour RT (bound by EditorApplication); remaining slots render a solid
// "coming in Phase 8" placeholder so the cockpit row still feels populated.
#include "render_targets_panel.hpp"

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
            dl->AddRectFilled(pos,
                               ImVec2{pos.x + thumb.x, pos.y + thumb.y},
                               IM_COL32(18, 26, 42, 255), 4.f);
            dl->AddRect      (pos,
                               ImVec2{pos.x + thumb.x, pos.y + thumb.y},
                               IM_COL32(58, 130, 190, 200), 4.f);
            ImGui::Dummy(thumb);
        }

        ImGui::TextColored(
            slots_[i].live ? ImVec4{0.46f, 0.79f, 0.65f, 1.f}
                             : ImVec4{0.40f, 0.45f, 0.55f, 1.f},
            "%s", slots_[i].label);

        ImGui::PopID();
        ImGui::EndGroup();

        if (i + 1 < kSlotCount) ImGui::SameLine(0.f, 6.f);
    }

    ImGui::End();
}

}  // namespace gw::editor
