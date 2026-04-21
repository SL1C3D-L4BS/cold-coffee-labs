#pragma once
// editor/panels/stats_panel.hpp
// Scene / stats cockpit panel — top-left card from the editor mock-up.
// Shows live FPS, frame-time history, entity counters, and the
// debug-draw toggle grid (grid / wireframe / gizmos / bounds / …).
// Spec ref: Phase 7 §2 gate-E (cockpit polish).

#include "panel.hpp"

namespace gw::editor::render { struct RenderSettings; }

namespace gw::editor {

class StatsPanel final : public IPanel {
public:
    explicit StatsPanel(gw::editor::render::RenderSettings* settings) noexcept
        : settings_(settings) {}

    void on_imgui_render(EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "Scene"; }

private:
    gw::editor::render::RenderSettings* settings_ = nullptr;
};

}  // namespace gw::editor
