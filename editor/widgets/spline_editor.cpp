// editor/widgets/spline_editor.cpp — Wave 1: Catmull-Rom / Bezier knot editor.

#include "editor/widgets/spline_editor.hpp"

#include <imgui.h>

#include <algorithm>
#include <string>

namespace gw::editor::widgets {

bool spline_editor(const char* id, SplineEditorState& state) noexcept {
    ImGui::PushID(id);
    bool any_changed = false;

    int type_i = static_cast<int>(state.type);
    if (ImGui::Combo("Spline type", &type_i, "Catmull-Rom\0Bezier\0")) {
        state.type       = static_cast<SplineType>(type_i);
        state.dirty      = true;
        any_changed      = true;
    }

    if (ImGui::Button("Add knot")) {
        SplineKnot k{};
        if (!state.knots.empty()) {
            const auto& last = state.knots.back();
            k.p[0] = last.p[0] + 0.5f;
            k.p[1] = last.p[1];
            k.p[2] = last.p[2];
        }
        state.knots.push_back(k);
        state.selected = static_cast<int>(state.knots.size()) - 1;
        state.dirty    = true;
        any_changed    = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Remove selected") && state.selected >= 0 &&
        state.selected < static_cast<int>(state.knots.size())) {
        state.knots.erase(state.knots.begin() + state.selected);
        state.selected =
            std::min(state.selected, static_cast<int>(state.knots.size()) - 1);
        state.dirty   = true;
        any_changed   = true;
    }

    if (ImGui::BeginChild("knot_list", ImVec2(0, 160), true)) {
        for (int i = 0; i < static_cast<int>(state.knots.size()); ++i) {
            const std::string row = "Knot " + std::to_string(i);
            if (ImGui::Selectable(row.c_str(), state.selected == i)) {
                state.selected = i;
            }
        }
        ImGui::EndChild();
    }

    if (state.selected >= 0 && state.selected < static_cast<int>(state.knots.size())) {
        auto& k = state.knots[static_cast<std::size_t>(state.selected)];
        if (ImGui::DragFloat3("Position", k.p, 0.01f)) {
            state.dirty = true;
            any_changed = true;
        }
        if (state.type == SplineType::Bezier) {
            if (ImGui::DragFloat3("Out tangent", k.t, 0.01f)) {
                state.dirty = true;
                any_changed = true;
            }
        }
    }

    ImGui::PopID();
    return any_changed;
}

} // namespace gw::editor::widgets
