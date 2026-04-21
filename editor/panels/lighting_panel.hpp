#pragma once
// editor/panels/lighting_panel.hpp
// "Lighting" cockpit card — ambient colour + a per-index dynamic-light
// editor (mirrors the "light N" selector in the mock-up's bottom-left
// quadrant).
// Spec ref: Phase 7 §2 gate-E.

#include "panel.hpp"

namespace gw::editor::render { struct RenderSettings; }

namespace gw::editor {

class LightingPanel final : public IPanel {
public:
    explicit LightingPanel(gw::editor::render::RenderSettings* settings) noexcept
        : settings_(settings) {}

    void on_imgui_render(EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "Lighting"; }

private:
    gw::editor::render::RenderSettings* settings_ = nullptr;
};

}  // namespace gw::editor
