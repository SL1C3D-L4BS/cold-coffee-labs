// editor/panels/outliner_panel.cpp
#include "outliner_panel.hpp"
#include "editor/commands/editor_commands.hpp"

#include <imgui.h>
#include <algorithm>
#include <cstring>

namespace gw::editor {

// ---------------------------------------------------------------------------
void OutlinerPanel::add_entity(EntityHandle h, const char* n, EntityHandle parent) {
    EntityEntry e{};
    e.handle = h;
    e.parent = parent;
    std::strncpy(e.name, n ? n : "Entity", sizeof(e.name) - 1);
    entities_.push_back(e);
}

void OutlinerPanel::remove_entity(EntityHandle h) {
    entities_.erase(
        std::remove_if(entities_.begin(), entities_.end(),
            [h](const EntityEntry& e){ return e.handle == h; }),
        entities_.end());
}

// ---------------------------------------------------------------------------
void OutlinerPanel::draw_entity_row(EditorContext& ctx, EntityHandle h,
                                     const char* label, bool /*has_children*/)
{
    bool selected = ctx.selection.is_selected(h);

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_Leaf |
        ImGuiTreeNodeFlags_SpanFullWidth |
        ImGuiTreeNodeFlags_OpenOnArrow;
    if (selected) flags |= ImGuiTreeNodeFlags_Selected;

    // Rename mode.
    if (rename_target_ == h) {
        ImGui::SetNextItemWidth(-1.f);
        if (ImGui::InputText("##rename", rename_buf_, sizeof(rename_buf_),
                              ImGuiInputTextFlags_EnterReturnsTrue |
                              ImGuiInputTextFlags_AutoSelectAll)) {
            // Apply rename — update local cache.
            for (auto& e : entities_) {
                if (e.handle == h)
                    std::strncpy(e.name, rename_buf_, sizeof(e.name) - 1);
            }
            rename_target_ = kNullEntity;
        }
        if (!ImGui::IsItemActive() && !ImGui::IsItemFocused())
            rename_target_ = kNullEntity;
        return;
    }

    ImGui::PushID(static_cast<int>(h.bits));
    bool open = ImGui::TreeNodeEx(label, flags);

    // Selection on click.
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        if (ImGui::GetIO().KeyCtrl)
            ctx.selection.toggle(h);
        else
            ctx.selection.set(h);
    }

    // Double-click → rename.
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        rename_target_ = h;
        std::strncpy(rename_buf_, label, sizeof(rename_buf_) - 1);
    }

    // Drag source.
    if (ImGui::BeginDragDropSource()) {
        ImGui::SetDragDropPayload("ENTITY", &h, sizeof(h));
        ImGui::Text("Move %s", label);
        ImGui::EndDragDropSource();
    }

    // Drop target.
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("ENTITY")) {
            EntityHandle dragged = *static_cast<const EntityHandle*>(p->Data);
            ctx.cmd_stack.execute_and_push(
                std::make_unique<commands::ReparentEntityCommand>(
                    dragged, kNullEntity, h,
                    [](EntityHandle /*c*/, EntityHandle /*np*/){}));
        }
        ImGui::EndDragDropTarget();
    }

    draw_context_menu(ctx, h);

    if (open) ImGui::TreePop();
    ImGui::PopID();
}

void OutlinerPanel::draw_context_menu(EditorContext& ctx, EntityHandle h) {
    if (!ImGui::BeginPopupContextItem("##entity_ctx")) return;

    if (ImGui::MenuItem("Rename")) {
        rename_target_ = h;
        for (const auto& e : entities_)
            if (e.handle == h) std::strncpy(rename_buf_, e.name, sizeof(rename_buf_)-1);
    }
    if (ImGui::MenuItem("Duplicate")) {
        // Phase 8: real deep-clone via ECS serialization. For now, mint a
        // distinct fake handle so the UI round-trips.
        static uint64_t counter = 1000;
        ++counter;
        add_entity(gw::ecs::Entity{counter}, "Duplicate", kNullEntity);
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Create Child")) {
        static uint64_t child_ctr = 2000;
        add_entity(gw::ecs::Entity{++child_ctr}, "Child Entity", h);
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Delete")) {
        ctx.cmd_stack.execute_and_push(
            std::make_unique<commands::CreateEntityCommand>(
                "deleted",
                [](const char*) -> EntityHandle { return kNullEntity; },
                [this](EntityHandle hd){ remove_entity(hd); }));
        ctx.selection.clear();
    }

    ImGui::EndPopup();
}

// ---------------------------------------------------------------------------
void OutlinerPanel::on_imgui_render(EditorContext& ctx) {
    ImGui::Begin(name(), &visible_);

    // Search bar.
    ImGui::SetNextItemWidth(-1.f);
    ImGui::InputTextWithHint("##search", "Filter...", search_buf_, sizeof(search_buf_));
    ImGui::SameLine();
    if (ImGui::SmallButton("+")) {
        // Create Entity action. Until EditorContext carries a real World
        // pointer, mint a unique fake handle so the UI round-trips.
        static uint64_t new_entity_id = 100;
        EntityHandle h{++new_entity_id};
        add_entity(h, "New Entity", kNullEntity);
        ctx.selection.set(h);
    }
    ImGui::Separator();

    std::string_view filter{search_buf_};

    // Render root entities (those with no parent).
    for (const auto& e : entities_) {
        if (e.parent != kNullEntity) continue;
        if (!filter.empty() &&
            std::strstr(e.name, search_buf_) == nullptr) continue;
        draw_entity_row(ctx, e.handle, e.name, false);
    }

    ImGui::End();
}

}  // namespace gw::editor
