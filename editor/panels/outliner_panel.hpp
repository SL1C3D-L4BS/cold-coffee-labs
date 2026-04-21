#pragma once
// editor/panels/outliner_panel.hpp
// Scene hierarchy tree panel — walks the real ECS world via
// World::visit_roots / visit_descendants (ADR-0004 §2.7).
// Spec ref: Phase 7 §9.

#include "panel.hpp"
#include "editor/selection/selection_context.hpp"
#include "editor/undo/commands.hpp"

namespace gw::editor {

class OutlinerPanel final : public IPanel {
public:
    OutlinerPanel() = default;

    void on_imgui_render(EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "Outliner"; }

private:
    void draw_entity_row(EditorContext& ctx, EntityHandle h);

    char         search_buf_[128]{};
    EntityHandle rename_target_ = kNullEntity;
    char         rename_buf_[64]{};

    // Transient drag-and-drop state — the id of the entity being dragged this
    // frame; consumed by the drop-target handler. Non-null between a
    // BeginDragDropSource and a successful AcceptDragDropPayload.
    EntityHandle drag_source_ = kNullEntity;
};

}  // namespace gw::editor
