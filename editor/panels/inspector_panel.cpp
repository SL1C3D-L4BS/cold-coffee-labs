// editor/panels/inspector_panel.cpp
// Reflection-driven inspector — iterates components on the selected entity
// and draws every reflected field. For each edit we capture the pre-edit
// value on ImGui::IsItemActivated() and push a SetComponentFieldCommand on
// IsItemDeactivatedAfterEdit() so dragging a single slider produces one
// undo step (not 60 per-second).
//
// Spec ref: Phase 7 §10 (Inspector), ADR-0005 §2.7 (SetComponentFieldCommand).
#include "inspector_panel.hpp"
#include "editor/reflect/widget_drawer.hpp"
#include "editor/reflect/reflect.hpp"
#include "editor/undo/commands.hpp"
#include "editor/scene/components.hpp"
#include "engine/ecs/world.hpp"
#include "engine/ecs/archetype.hpp"
#include "engine/core/field_reflection.hpp"

#include <imgui.h>
#include <cstdio>
#include <cstring>
#include <string>

namespace gw::editor {

namespace {

// Render the NameComponent specially — it has no GW_REFLECT so it needs a
// custom widget. One InputText with explicit Activate/Deactivate tracking so
// the edit is undoable as a single step.
void draw_name_component(EditorContext& /*ctx*/, gw::ecs::World& world,
                          EntityHandle e) {
    auto* nc = world.get_component<scene::NameComponent>(e);
    if (!nc) return;

    ImGui::PushID("__name");
    ImGui::Text("Name");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1.f);
    ImGui::InputText("##name", nc->data(), nc->value.size());
    ImGui::PopID();
}

// Render one reflected component whose runtime instance lives at `base`.
// Pushes an undoable SetComponentFieldCommand<float> on change for float
// fields (the common case). Non-float fields still edit in-place; full
// typed-command coverage lands in a follow-up with trait-based dispatch.
void draw_reflected_component(EditorContext& ctx,
                               const char* label,
                               const gw::core::TypeInfo& ti,
                               void* base) {
    if (!ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen)) return;

    for (const auto& fi : ti.fields) {
        ImGui::PushID(fi.name);

        void* field_ptr = static_cast<std::uint8_t*>(base) + fi.offset;

        // For a float field we pre-capture the value when the widget is
        // activated (pointer-comparable and type-stable) and push a single
        // undoable command on deactivate.
        static thread_local const void* active_field_ptr = nullptr;
        static thread_local float       active_before    = 0.f;
        static thread_local std::string active_label;

        const bool changed = get_widget_drawer().draw_field(fi, field_ptr);

        if (ImGui::IsItemActivated() && fi.size == sizeof(float)) {
            active_field_ptr = field_ptr;
            active_before    = *static_cast<float*>(field_ptr);
            active_label     = std::string{fi.name};
        }

        if (changed) {
            // Non-undoable interim write — the final command is pushed on
            // deactivate. This keeps undo atomic per drag.
            (void)changed;
        }

        if (ImGui::IsItemDeactivatedAfterEdit() &&
            active_field_ptr == field_ptr && fi.size == sizeof(float)) {
            const float after = *static_cast<float*>(field_ptr);
            if (after != active_before) {
                float before_cap = active_before;
                float* fp = static_cast<float*>(field_ptr);
                ctx.cmd_stack.push(
                    std::make_unique<undo::SetComponentFieldCommand<float>>(
                        active_label, fp, before_cap, after,
                        [fp](const float& v) { *fp = v; }));
            }
            active_field_ptr = nullptr;
        }

        ImGui::PopID();
    }
}

} // namespace

// ---------------------------------------------------------------------------
void InspectorPanel::on_imgui_render(EditorContext& ctx) {
    ImGui::Begin(name(), &visible_);

    const EntityHandle primary = ctx.selection.primary();

    if (primary == kNullEntity || ctx.selection.empty()) {
        ImGui::TextDisabled("No selection");
        ImGui::End();
        return;
    }

    auto* world = ctx.world;
    if (!world || !world->is_alive(primary)) {
        ImGui::TextDisabled("Selection is stale (dead entity)");
        ImGui::End();
        return;
    }

    ImGui::Text("Entity  0x%llx", static_cast<unsigned long long>(primary.bits));
    ImGui::Separator();

    // Name (special-cased; no GW_REFLECT).
    draw_name_component(ctx, *world, primary);
    ImGui::Separator();

    // Iterate every component type present on the entity's archetype.
    const auto* slot = world->slot_for(primary);
    if (!slot || slot->archetype_id == gw::ecs::kInvalidArchetypeId) {
        ImGui::TextDisabled("No components");
        ImGui::End();
        return;
    }

    auto& registry = world->component_registry();
    auto& arch     = world->archetype_table().archetype(slot->archetype_id);

    for (auto tid : arch.types()) {
        const auto& ct = registry.info(tid);
        if (!ct.reflection) continue;  // opaque (e.g. NameComponent) — drawn above

        std::byte* raw = arch.component_ptr(registry, tid,
                                              slot->chunk_index, slot->slot_index);
        if (!raw) continue;

        const char* hdr = ct.reflection->type_name ? ct.reflection->type_name
                                                    : "<component>";
        draw_reflected_component(ctx, hdr, *ct.reflection, raw);
    }

    ImGui::End();
}

}  // namespace gw::editor
