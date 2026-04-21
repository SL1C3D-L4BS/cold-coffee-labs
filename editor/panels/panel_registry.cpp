// editor/panels/panel_registry.cpp
#include "panel_registry.hpp"

#include <cstring>

namespace gw::editor {

void PanelRegistry::add(std::unique_ptr<IPanel> panel) {
    panels_.push_back(std::move(panel));
}

void PanelRegistry::render_all(EditorContext& ctx) {
    for (auto& p : panels_) {
        if (p && p->visible())
            p->on_imgui_render(ctx);
    }
}

IPanel* PanelRegistry::find(const char* name) noexcept {
    for (auto& p : panels_) {
        if (p && std::strcmp(p->name(), name) == 0)
            return p.get();
    }
    return nullptr;
}

}  // namespace gw::editor
