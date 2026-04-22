#pragma once
// editor/panels/panel.hpp
// IPanel — the base interface for every docked editor panel.
// Spec ref: Phase 7 §2 — panel.hpp

// Forward declarations to keep this header lightweight.
namespace gw::editor {
    struct EditorContext;
    class  SelectionContext;
}
namespace gw::editor::undo { class CommandStack; }
namespace gw::ecs          { class World; }
namespace gw::assets       { class AssetDatabase; class VirtualFilesystem; }
namespace gw::seq { class SequencerWorld; class CinematicCameraSystem; }

namespace gw::editor {

// ---------------------------------------------------------------------------
// EditorContext — the shared "god object" passed to every panel per frame.
// Panels borrow these references; they never own them.
// ---------------------------------------------------------------------------
struct EditorContext {
    SelectionContext&              selection;
    gw::editor::undo::CommandStack& cmd_stack;
    // Real ECS world pointer since 2026-04-20 Phase-7 Path-A. Never null in a
    // running editor; unit tests that construct a bare EditorContext may pass
    // nullptr.
    gw::ecs::World*            world        = nullptr;
    gw::assets::AssetDatabase* asset_db     = nullptr;
    float                      delta_time_s = 0.f;
    bool                       in_pie       = false;
    /// Phase 18-B — optional; nullptr in headless tests.
    gw::seq::SequencerWorld*          sequencer          = nullptr;
    gw::seq::CinematicCameraSystem*   cinematic          = nullptr;
    /// ImGui::Image handle for the main scene color target (viewport offscreen).
    void*                             scene_color_descriptor = nullptr;
};

// ---------------------------------------------------------------------------
// IPanel
// ---------------------------------------------------------------------------
class IPanel {
public:
    virtual ~IPanel() = default;

    // Called once per frame inside the ImGui frame.
    virtual void on_imgui_render(EditorContext& ctx) = 0;

    // Optional: handle a single GLFW key/mouse event before ImGui sees it.
    virtual void on_event(int /*glfw_key*/, int /*action*/) {}

    // Panel display name — used for ImGui window title and menu toggle.
    [[nodiscard]] virtual const char* name() const = 0;

    // Whether the panel window is currently visible.
    [[nodiscard]] virtual bool visible() const noexcept { return visible_; }
    virtual void set_visible(bool v) noexcept { visible_ = v; }

protected:
    bool visible_ = true;
};

}  // namespace gw::editor
