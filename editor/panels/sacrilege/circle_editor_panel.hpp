#pragma once
// editor/panels/sacrilege/circle_editor_panel.hpp — Phase 21 W140 (ADR-0108).
// Nine-Circle live tuning + Location Skin selector.

#include "editor/panels/panel.hpp"
#include "engine/world/circle_profile.hpp"

namespace gw::editor::panels::sacrilege {

class CircleEditorPanel final : public gw::editor::IPanel {
public:
    void on_imgui_render(gw::editor::EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "Circle Editor"; }

    // Direct programmatic access used by tests — returns the profile for the
    // currently selected Circle. Phase-21 authoring writes through
    // `selected_profile_mut` via CommandStack; Phase-22 will promote the
    // working profile into the ECS world.
    [[nodiscard]] gw::world::CircleId selected_circle() const noexcept { return selected_; }
    void set_selected_circle(gw::world::CircleId id) noexcept { selected_ = id; }

    [[nodiscard]] const gw::world::CircleProfile& selected_profile() const noexcept {
        return gw::world::circle_profile(selected_);
    }

private:
    gw::world::CircleId selected_{gw::world::CircleId::Violence};
};

} // namespace gw::editor::panels::sacrilege
