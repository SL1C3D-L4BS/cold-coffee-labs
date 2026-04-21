#pragma once
// editor/app/editor_app.hpp
// EditorApplication — owns window, device, swapchain, ImGui, and the frame loop.
// Spec ref: Phase 7 §2, §14, §15.

#include "editor/panels/panel_registry.hpp"
#include "editor/selection/selection_context.hpp"
#include "editor/pie/gameplay_host.hpp"
#include "engine/core/command.hpp"

// Forward declares.
struct GLFWwindow;

namespace gw::render::hal {
    class VulkanInstance;
    class VulkanDevice;
    class VulkanSwapchain;
}

namespace gw::editor {

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
    void begin_frame();
    void build_ui();
    void end_frame();

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

    // Vulkan resources — owned by the HAL objects.
    gw::render::hal::VulkanInstance*  instance_  = nullptr;
    gw::render::hal::VulkanDevice*    device_    = nullptr;
    gw::render::hal::VulkanSwapchain* swapchain_ = nullptr;

    // Editor state.
    SelectionContext    selection_;
    gw::core::CommandStack cmd_stack_{256};
    PanelRegistry       panels_;
    GameplayHost        pie_;

    // Layout built flag — persisted via ImGui ini.
    bool layout_built_ = false;

    // Swapchain resize deferred flag.
    bool swapchain_resize_pending_ = false;
};

}  // namespace gw::editor
