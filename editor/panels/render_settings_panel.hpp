#pragma once
// editor/panels/render_settings_panel.hpp
// "Render Settings" cockpit panel — the large left-column card in the
// editor mock-up. Hosts tone-map sliders, SSAO parameters, exposure bounds
// and the live luminance histogram read-out.
// Spec ref: Phase 7 §2 gate-E.

#include "panel.hpp"

namespace gw::editor::render { struct RenderSettings; }

namespace gw::editor {

class RenderSettingsPanel final : public IPanel {
public:
    explicit RenderSettingsPanel(gw::editor::render::RenderSettings* settings) noexcept
        : settings_(settings) {}

    void on_imgui_render(EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "Render Settings"; }

private:
    gw::editor::render::RenderSettings* settings_ = nullptr;
};

}  // namespace gw::editor
