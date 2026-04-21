#pragma once
// editor/panels/inspector_panel.hpp
// Reflection-driven component editor panel.
// Spec ref: Phase 7 §10.

#include "panel.hpp"

namespace gw::editor {

class InspectorPanel final : public IPanel {
public:
    InspectorPanel() = default;

    void on_imgui_render(EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "Inspector"; }
};

}  // namespace gw::editor
