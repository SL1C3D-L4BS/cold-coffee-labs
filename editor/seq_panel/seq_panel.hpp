#pragma once
// editor/seq_panel/seq_panel.hpp — ImGui sequencer timeline (Phase 18-B).

#include "editor/panels/panel.hpp"

#include "engine/assets/asset_handle.hpp"

#include <cstdint>

namespace gw::editor::seq {

class SeqPanel final : public IPanel {
public:
    SeqPanel();

    void on_imgui_render(EditorContext& ctx) override;

    [[nodiscard]] const char* name() const override { return "Sequencer"; }

    void set_active_sequence(gw::assets::AssetHandle h) noexcept { active_seq_ = h; }

private:
    void                      render_toolbar(EditorContext& ctx);
    void                      render_timeline(EditorContext& ctx);
    void                      render_camera_preview(EditorContext& ctx) const;

    gw::assets::AssetHandle   active_seq_{};
    float                     timeline_zoom_  = 4.f;  // pixels per frame
    std::uint32_t             selected_track_   = 0u;
    int                       selected_key_idx_ = -1;
    bool                      dragging_key_   = false;
    float                     drag_start_x_   = 0.f;
    std::uint32_t             drag_key_frame0_  = 0u;
};

}  // namespace gw::editor::seq
