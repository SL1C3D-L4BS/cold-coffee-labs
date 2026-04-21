// editor/panels/inspector_panel.cpp
// Reflection-driven inspector panel — iterates GW_REFLECT fields for each
// component on the selected entity and dispatches to WidgetDrawer.
// Spec ref: Phase 7 §10.
#include "inspector_panel.hpp"
#include "editor/reflect/widget_drawer.hpp"
#include "editor/reflect/reflect.hpp"
#include "editor/commands/editor_commands.hpp"

#include <imgui.h>

namespace gw::editor {

// ---------------------------------------------------------------------------
// Phase 7 example component for demonstration / test.
// In production, these live in the gameplay module.
// ---------------------------------------------------------------------------
struct ExampleTransformComponent {
    float x = 0.f, y = 0.f, z = 0.f;
    float yaw = 0.f, pitch = 0.f, roll = 0.f;
    float scale = 1.f;

    GW_REFLECT(ExampleTransformComponent, x, y, z, yaw, pitch, roll, scale)
};

// ---------------------------------------------------------------------------
void InspectorPanel::on_imgui_render(EditorContext& ctx) {
    ImGui::Begin(name(), &visible_);

    EntityHandle primary = ctx.selection.primary();

    if (primary == kNullEntity || ctx.selection.empty()) {
        ImGui::TextDisabled("No selection");
        ImGui::End();
        return;
    }

    // Header: entity name + handle.
    ImGui::Text("Entity  0x%llx", static_cast<unsigned long long>(primary));
    ImGui::Separator();

    // Phase 7 demonstration: show a fake transform component.
    // Phase 8 will iterate the actual ECS component types via the world.
    static ExampleTransformComponent demo_transform;
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        const reflect::TypeInfo& ti =
            describe(static_cast<ExampleTransformComponent*>(nullptr));

        for (const auto& fi : ti.fields) {
            ImGui::PushID(fi.name);
            void* ptr = reinterpret_cast<uint8_t*>(&demo_transform) + fi.offset;
            bool changed = get_widget_drawer().draw_field(fi, ptr);
            if (changed) {
                // Phase 7: push a float SetComponentFieldCommand for Ctrl+Z.
                float val = *static_cast<float*>(ptr);
                ctx.cmd_stack.execute_and_push(
                    std::make_unique<commands::SetComponentFieldCommand<float>>(
                        fi.name,
                        val,   // before == after since widget already wrote it
                        val,
                        [ptr]() -> float { return *static_cast<float*>(ptr); },
                        [ptr](const float& v){ *static_cast<float*>(ptr) = v; }
                    )
                );
            }
            ImGui::PopID();
        }
    }

    ImGui::End();
}

}  // namespace gw::editor
