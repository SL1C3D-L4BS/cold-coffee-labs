#pragma once
// editor/panels/panel.hpp
// IPanel — the base interface for every docked editor panel.
// Spec ref: Phase 7 §2 — panel.hpp

#include <filesystem>

// glm::mat4 in EditorContext (viewport → offscreen scene pass).
#include <glm/fwd.hpp>

// Forward declarations to keep this header lightweight.
namespace gw::editor::render {
class ImGuiTextureCache;
}

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
    /// Editor Vulkan timestamps: scene pass → post/ImGui (not full shared frame graph).
    float                      framegraph_gpu_ms = 0.f;
    bool                       in_pie       = false;
    /// True while PIE is paused (`in_pie` is still true). Toolbar Pause/Resume label.
    bool                       pie_paused   = false;
    /// Host-only: toggles pause/resume when in PIE (same behavior as Play menu / F6).
    void (*pie_pause_toggle)(void* user_data) = nullptr;
    void*                      pie_pause_user_data = nullptr;
    /// Phase 18-B — optional; nullptr in headless tests.
    gw::seq::SequencerWorld*          sequencer          = nullptr;
    gw::seq::CinematicCameraSystem*   cinematic          = nullptr;
    /// ImGui::Image handle for the main scene color target (viewport offscreen).
    void*                             scene_color_descriptor = nullptr;
    /// Opened project root (content/, assets/ live under here). nullptr in tests / launcher.
    const std::filesystem::path*      project_root         = nullptr;
    /// RGBA8 thumbnails via Vulkan → ImGui (optional).
    gw::editor::render::ImGuiTextureCache* imgui_textures = nullptr;
    /// Filled by the Viewport panel; consumed next frame in `begin_frame` scene
    /// raster (same view/proj as debug overlay, one frame latent vs interaction).
    glm::mat4*            scene_raster_view  = nullptr;
    glm::mat4*            scene_raster_proj  = nullptr;
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
