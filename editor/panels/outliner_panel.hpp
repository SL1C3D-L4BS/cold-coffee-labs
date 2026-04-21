#pragma once
// editor/panels/outliner_panel.hpp
// Scene hierarchy tree panel.
// Spec ref: Phase 7 §9.

#include "panel.hpp"
#include "editor/selection/selection_context.hpp"
#include "editor/commands/editor_commands.hpp"

#include <vector>

namespace gw::editor {

class OutlinerPanel final : public IPanel {
public:
    OutlinerPanel() = default;

    void on_imgui_render(EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "Outliner"; }

private:
    void draw_entity_row(EditorContext& ctx, EntityHandle h,
                          const char* label, bool has_children);
    void draw_context_menu(EditorContext& ctx, EntityHandle h);

    char        search_buf_[128]{};
    bool        show_inactive_ = false;
    EntityHandle rename_target_ = kNullEntity;
    char        rename_buf_[64]{};

    // Placeholder entity list (Phase 8 will query the real ECS world).
    // For Phase 7 we maintain a simple flat list of (handle, name) pairs
    // populated by CreateEntityCommand callbacks.
    struct EntityEntry {
        EntityHandle handle;
        char         name[64];
        EntityHandle parent;
    };
    std::vector<EntityEntry> entities_;   // set by friend create/destroy fns

public:
    // Called by CreateEntityCommand on execute/undo to sync the panel.
    void add_entity(EntityHandle h, const char* n, EntityHandle parent = kNullEntity);
    void remove_entity(EntityHandle h);
};

}  // namespace gw::editor
