#pragma once
// editor/panels/render_targets_panel.hpp
// Bottom "render targets" strip from the cockpit mock-up. Presents a grid
// of debug thumbnails (scene, albedo, normals, depth, …). Until the
// full G-buffer lands in Phase 8 we wire the one live RT (the scene colour
// attachment) and placeholder the remaining slots.
// Spec ref: Phase 7 §2 gate-E.

#include "panel.hpp"
#include <cstdint>
#include <imgui.h>

namespace gw::editor {

// Opaque ImTextureID for the debug RT slots. The backend (EditorApplication)
// binds the scene RT into slot 0; later gates fill the rest.
struct RenderTargetSlot {
    const char*  label = "";
    ImTextureID  tex   = 0;  // ImGui texture handle
    bool         live  = false;
};

class RenderTargetsPanel final : public IPanel {
public:
    RenderTargetsPanel() = default;

    void on_imgui_render(EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "Render Targets"; }

    // Called by EditorApplication whenever the scene RT is (re)bound.
    void set_scene_texture(ImTextureID tex) noexcept {
        slots_[0].tex  = tex;
        slots_[0].live = tex != 0;
    }

private:
    static constexpr int kSlotCount = 6;
    RenderTargetSlot slots_[kSlotCount] = {
        {"Scene",     0, false},
        {"Albedo",    0, false},
        {"Normals",   0, false},
        {"Depth",     0, false},
        {"Metal/Rgh", 0, false},
        {"SSAO",      0, false},
    };
};

}  // namespace gw::editor
