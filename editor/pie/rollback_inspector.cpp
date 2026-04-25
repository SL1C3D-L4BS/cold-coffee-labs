// editor/pie/rollback_inspector.cpp — Wave 1: deterministic rollback lab UI.

#include "editor/pie/rollback_inspector.hpp"

#include <imgui.h>

#include <algorithm>

namespace gw::editor::pie {

void draw_rollback_inspector(RollbackInspectorState& state) noexcept {
    ImGui::SetNextWindowBgAlpha(0.86f);
    if (ImGui::Begin("Rollback Inspector", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted(
            "Deterministic sim lab — seed/tick/bit-hash mirror the replay harness. "
            "Step controls are presentation-first until the determinism validator "
            "IPC lands; values stay in-session for designers.");
        if (state.authoring_snapshot_bytes > 0) {
            ImGui::Text("Authoring snapshot at Play: %llu B",
                static_cast<unsigned long long>(state.authoring_snapshot_bytes));
        } else {
            ImGui::TextDisabled("Authoring snapshot: — (enter Play to capture)");
        }
        ImGui::Separator();
        ImGui::InputScalar("Universe seed##rb_seed", ImGuiDataType_U64, &state.seed);
        ImGui::InputScalar("Logical tick##rb_tick", ImGuiDataType_U64, &state.tick);
        ImGui::InputScalar("Visited bit-hash##rb_hash", ImGuiDataType_U64, &state.bit_hash);
        ImGui::Checkbox("Paused##rb_pause", &state.paused);
        ImGui::DragInt("Step size##rb_step", &state.step_size, 0.2f, 1, 256);
        ImGui::Separator();
        if (ImGui::Button("Step forward##rb_fwd")) {
            state.tick += static_cast<std::uint64_t>(std::max(1, state.step_size));
        }
        ImGui::SameLine();
        if (ImGui::Button("Step back##rb_back")) {
            const auto dec = static_cast<std::uint64_t>(std::max(1, state.step_size));
            state.tick = state.tick > dec ? state.tick - dec : 0;
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset tick##rb_zero")) {
            state.tick = 0;
        }
    }
    ImGui::End();
}

} // namespace gw::editor::pie
