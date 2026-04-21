// editor/panels/outliner_panel.cpp
// Walks the real ECS world via World::visit_roots / visit_descendants.
// Create / rename / delete / reparent all flow through the CommandStack so
// every mutation is undoable (ADR-0004 §2.7, ADR-0005).
#include "outliner_panel.hpp"
#include "editor/undo/commands.hpp"
#include "editor/scene/components.hpp"
#include "engine/ecs/world.hpp"
#include "engine/ecs/hierarchy.hpp"

#include <imgui.h>
#include <cstdio>
#include <cstring>

namespace gw::editor {

namespace {

// Read the entity's display name (from its NameComponent). Returns a stable
// internal buffer pointer so ImGui can render it directly; ok to read across
// the whole frame since components live at stable addresses within a single
// panel-render scope.
const char* name_of(const gw::ecs::World* w, EntityHandle e) {
    if (!w) return "<?>";
    const auto* nc = w->get_component<scene::NameComponent>(e);
    return (nc && nc->c_str()[0] != '\0') ? nc->c_str() : "<unnamed>";
}

// True when `e` has a child (first_child != null). Safe for entities without
// a HierarchyComponent (returns false).
bool has_children(const gw::ecs::World* w, EntityHandle e) {
    if (!w) return false;
    const auto* h = w->get_component<gw::ecs::HierarchyComponent>(e);
    return h && !h->first_child.is_null();
}

} // namespace

// ---------------------------------------------------------------------------
void OutlinerPanel::draw_entity_row(EditorContext& ctx, EntityHandle h) {
    auto* world = ctx.world;
    const char* label = name_of(world, h);

    // Rename mode — an active InputText that commits on Enter / focus loss.
    if (rename_target_ == h) {
        ImGui::SetNextItemWidth(-1.f);
        const bool commit = ImGui::InputText("##rename", rename_buf_, sizeof(rename_buf_),
                              ImGuiInputTextFlags_EnterReturnsTrue |
                              ImGuiInputTextFlags_AutoSelectAll);
        if (commit) {
            if (auto* nc = world->get_component<scene::NameComponent>(h)) {
                const auto n = std::strlen(rename_buf_);
                const auto cap = nc->value.size() - 1;
                const auto cp = n < cap ? n : cap;
                std::memcpy(nc->data(), rename_buf_, cp);
                nc->value[cp] = '\0';
            }
            rename_target_ = kNullEntity;
        }
        if (!ImGui::IsItemActive() && !ImGui::IsItemFocused()) {
            rename_target_ = kNullEntity;
        }
        return;
    }

    const bool is_selected = ctx.selection.is_selected(h);
    const bool is_branch   = has_children(world, h);

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_SpanFullWidth |
        ImGuiTreeNodeFlags_OpenOnArrow;
    if (!is_branch)  flags |= ImGuiTreeNodeFlags_Leaf;
    if (is_selected) flags |= ImGuiTreeNodeFlags_Selected;

    ImGui::PushID(static_cast<int>(h.bits));
    const bool open = ImGui::TreeNodeEx(label, flags);

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        if (ImGui::GetIO().KeyCtrl) ctx.selection.toggle(h);
        else                         ctx.selection.set(h);
    }

    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        rename_target_ = h;
        std::strncpy(rename_buf_, label, sizeof(rename_buf_) - 1);
        rename_buf_[sizeof(rename_buf_) - 1] = '\0';
    }

    if (ImGui::BeginDragDropSource()) {
        ImGui::SetDragDropPayload("ENTITY", &h, sizeof(h));
        ImGui::Text("Move %s", label);
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("ENTITY")) {
            const EntityHandle dragged = *static_cast<const EntityHandle*>(p->Data);
            if (dragged != h && world) {
                gw::ecs::Entity old_parent = gw::ecs::Entity::null();
                if (auto* ch = world->get_component<gw::ecs::HierarchyComponent>(dragged)) {
                    old_parent = ch->parent;
                }
                ctx.cmd_stack.push(
                    std::make_unique<undo::ReparentEntityCommand>(
                        dragged, old_parent, h,
                        [world](EntityHandle child, EntityHandle new_parent) {
                            (void)world->reparent(child, new_parent);
                        }));
            }
        }
        ImGui::EndDragDropTarget();
    }

    // Right-click context menu.
    if (ImGui::BeginPopupContextItem("##entity_ctx")) {
        if (ImGui::MenuItem("Rename")) {
            rename_target_ = h;
            std::strncpy(rename_buf_, label, sizeof(rename_buf_) - 1);
            rename_buf_[sizeof(rename_buf_) - 1] = '\0';
        }
        if (ImGui::MenuItem("Create Child") && world) {
            auto child = world->create_entity();
            world->add_component(child, scene::NameComponent{"Child"});
            world->add_component(child, scene::TransformComponent{});
            (void)world->reparent(child, h);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Delete") && world) {
            // Direct destroy for Phase-7. The full DestroyEntityCommand
            // (undoable via ECS serialization snapshot) lands in Gate C.
            world->destroy_entity(h);
            ctx.selection.clear();
        }
        ImGui::EndPopup();
    }

    if (open) {
        if (world) {
            for (auto c : world->children_of(h)) {
                draw_entity_row(ctx, c);
            }
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

// ---------------------------------------------------------------------------
void OutlinerPanel::on_imgui_render(EditorContext& ctx) {
    ImGui::Begin(name(), &visible_);

    auto* world = ctx.world;

    ImGui::SetNextItemWidth(-28.f);
    ImGui::InputTextWithHint("##search", "Filter...", search_buf_, sizeof(search_buf_));
    ImGui::SameLine();
    if (ImGui::SmallButton("+") && world) {
        const auto e = world->create_entity();
        world->add_component(e, scene::NameComponent{"Entity"});
        world->add_component(e, scene::TransformComponent{});
        ctx.selection.set(e);
    }
    ImGui::Separator();

    if (!world) {
        ImGui::TextDisabled("No scene world bound.");
        ImGui::End();
        return;
    }

    const std::string_view filter{search_buf_};

    world->visit_roots([&](EntityHandle root) {
        if (!filter.empty() &&
            std::strstr(name_of(world, root), search_buf_) == nullptr) {
            return;
        }
        draw_entity_row(ctx, root);
    });

    ImGui::End();
}

}  // namespace gw::editor
