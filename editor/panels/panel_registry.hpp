#pragma once
// editor/panels/panel_registry.hpp
// PanelRegistry — owns all panels via unique_ptr, renders them in order.
// No raw new/delete. Panel lifetime == EditorApplication lifetime.

#include "panel.hpp"
#include <memory>
#include <vector>

namespace gw::editor {

class PanelRegistry {
public:
    // Register a panel; takes ownership.
    void add(std::unique_ptr<IPanel> panel);

    // Render all visible panels.
    void render_all(EditorContext& ctx);

    // Find panel by name (linear scan — small N).
    [[nodiscard]] IPanel* find(const char* name) noexcept;

    [[nodiscard]] auto& panels() noexcept { return panels_; }

private:
    std::vector<std::unique_ptr<IPanel>> panels_;
};

}  // namespace gw::editor
