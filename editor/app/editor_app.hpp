#pragma once
// editor/app/editor_app.hpp
// EditorApplication — owns window, device, swapchain, ImGui, and the frame loop.
// Spec ref: Phase 7 §2, §14, §15.
//
// Phase 7 (2026-04-20): Vulkan + ImGui fully wired directly against volk,
// mirroring sandbox/main.cpp. The engine's render HAL is not used here because
// (a) the HAL device does not yet enable dynamic rendering / sync2, and
// (b) the HAL instance does not take surface extensions — both required by
// the editor. Swapping the editor onto the HAL is a Phase C audit task.

#include "editor/panels/panel_registry.hpp"
#include "editor/render/render_settings.hpp"
#include "editor/selection/selection_context.hpp"
#include "editor/pie/gameplay_host.hpp"
#include "editor/undo/command_stack.hpp"
#include "engine/core/time.hpp"
#include "engine/ecs/entity.hpp"
#include "engine/ecs/world.hpp"
#include "engine/scene/seq/seq_camera.hpp"
#include "engine/scene/seq/sequencer_world.hpp"

#include <memory>

struct GLFWwindow;

namespace gw::editor {

// Opaque backend — all raw Vulkan state lives behind this pimpl so the header
// stays clean of <volk.h> and keeps editor_app.hpp cheap to include.
struct EditorVulkanBackend;

class EditorApplication {
public:
    EditorApplication();
    ~EditorApplication();

    EditorApplication(const EditorApplication&)            = delete;
    EditorApplication& operator=(const EditorApplication&) = delete;

    // Run the main loop until the window is closed.
    void run();

private:
    // -----------------------------------------------------------------------
    // Init / shutdown
    // -----------------------------------------------------------------------
    void init_window();
    void init_vulkan();
    void init_imgui();
    void shutdown_imgui();
    void shutdown_vulkan();
    void shutdown_window();

    // -----------------------------------------------------------------------
    // Per-frame
    // -----------------------------------------------------------------------
    // Returns false when the frame must be skipped (swapchain out-of-date).
    bool begin_frame();
    void build_ui();
    void end_frame();

    // Swapchain lifecycle.
    void recreate_swapchain();

    // Offscreen scene render target — Phase 7 §6.1. Allocated via VMA, sampled
    // by ImGui in the Viewport panel, cleared/rendered by begin_frame().
    void create_scene_rt(uint32_t w, uint32_t h);
    void destroy_scene_rt();

    // -----------------------------------------------------------------------
    // Docking layout — built once on first launch.
    // -----------------------------------------------------------------------
    void build_docking_layout();
    void apply_theme();

    // -----------------------------------------------------------------------
    // Callbacks (GLFW)
    // -----------------------------------------------------------------------
    static void on_key_callback(GLFWwindow* w, int key, int scancode,
                                 int action, int mods);
    static void on_scroll_callback(GLFWwindow* w, double dx, double dy);
    static void on_framebuffer_resize(GLFWwindow* w, int width, int height);

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------
    GLFWwindow* window_  = nullptr;
    int         win_w_   = 1600;
    int         win_h_   = 900;
    bool        running_ = true;

    // Opaque Vulkan backend. Default-constructed empty; populated by
    // init_vulkan() and init_imgui(); torn down in reverse order.
    std::unique_ptr<EditorVulkanBackend> vk_;

    // Editor state.
    SelectionContext              selection_;
    gw::editor::undo::CommandStack cmd_stack_{};
    PanelRegistry                 panels_;
    GameplayHost           pie_;
    GameplayContext        pie_ctx_{};     // lives for the session; world/time filled lazily
    gw::TimeState          pie_time_{};    // handed to gameplay through pie_ctx_.time
    gw::ecs::World         scene_world_;   // authoring world — lives for the editor session

    // Phase 18-B — `.gwseq` playback + cinematic view (editor session objects).
    gw::seq::SequencerWorld            sequencer_world_{};
    gw::seq::CinematicCameraSystem     cinematic_camera_{};
    gw::ecs::Entity                    seq_player_entity_{};

    // Cockpit render settings (Phase 7 gate-E) — shared with the cockpit
    // panels and, once Phase 8 lands, with the frame-graph. Panels borrow a
    // non-owning pointer into this instance.
    gw::editor::render::RenderSettings render_settings_{};

    // Layout built flag — persisted via ImGui ini.
    bool layout_built_ = false;

    // Swapchain resize deferred flag.
    bool swapchain_resize_pending_ = false;
};

}  // namespace gw::editor
