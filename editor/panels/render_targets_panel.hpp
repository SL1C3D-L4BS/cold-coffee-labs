#pragma once
// editor/panels/render_targets_panel.hpp
// Bottom "render targets" strip (Phase 7 gate E / Wave 1C). Slot 0 is the live
// offscreen scene colour. Slots 1–5 are separate RGBA images fed by
// `vkCmdCopyImage` from that scene RT (layout filler until real G-buffer/MRT).
// Excluded to later waves: object-ID pick buffer, MRT in slots 1–5, and
// depth→RGB cockpit preview (requires linearized depth once geometry writes depth).
// The offscreen pass already clears + binds a depth target for a future mesh pass.
// Spec ref: Phase 7 §2 gate-E.

#include "panel.hpp"
#include <imgui.h>

namespace gw::editor {

// One cockpit thumbnail. `mirrors_scene` = reserved G-buffer (or similar) name;
// the GPU image is still a full-frame copy of the scene colour RT (slot 0).
struct RenderTargetSlot {
    const char* name = "";
    bool        mirrors_scene = false;  // false: slot 0; true: slots 1–5
    ImTextureID tex  = 0;               // ImGui texture ID
    bool        live = false;           // descriptor valid + has pixels
};

class RenderTargetsPanel final : public IPanel {
public:
    static constexpr int kSlotCount = 6;

    RenderTargetsPanel() = default;

    void on_imgui_render(EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "Render Targets"; }

    /// Bind one cockpit thumbnail (0 = scene colour; 1–5 = scene-colour mirrors).
    void set_slot(int index, ImTextureID tex, bool live = true) noexcept {
        if (index < 0 || index >= kSlotCount) return;
        slots_[index].tex  = tex;
        slots_[index].live = live && tex != 0;
    }

    void set_scene_texture(ImTextureID tex) noexcept { set_slot(0, tex); }

private:
    RenderTargetSlot slots_[kSlotCount] = {
        {"Scene",     false, 0, false},
        {"Albedo",    true,  0, false},
        {"Normals",   true,  0, false},
        {"Depth",     true,  0, false},
        {"Metal/Rgh", true,  0, false},
        {"SSAO",      true,  0, false},
    };
};

}  // namespace gw::editor
