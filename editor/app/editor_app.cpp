// editor/app/editor_app.cpp
// EditorApplication — owns the GLFW window + Vulkan + ImGui frame loop.
// Spec ref: Phase 7 §2, §14, §15.
#include "editor_app.hpp"
#include "editor/bld_api/editor_bld_api.hpp"

#include "editor/panels/outliner_panel.hpp"
#include "editor/panels/inspector_panel.hpp"
#include "editor/panels/viewport_panel.hpp"
#include "editor/panels/asset_browser_panel.hpp"
#include "editor/panels/console_panel.hpp"

#include "engine/render/hal/vulkan_instance.hpp"
#include "engine/render/hal/vulkan_device.hpp"
#include "engine/render/hal/vulkan_swapchain.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <volk.h>

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

namespace gw::editor {

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------
EditorApplication::EditorApplication() {
    init_window();
    init_vulkan();
    init_imgui();
    apply_theme();

    // Panels — order determines draw order.
    auto* vp = new ViewportPanel(*device_);
    vp->set_window(window_);
    panels_.add(std::unique_ptr<ViewportPanel>(vp));
    panels_.add(std::make_unique<OutlinerPanel>());
    panels_.add(std::make_unique<InspectorPanel>());
    panels_.add(std::make_unique<AssetBrowserPanel>());
    panels_.add(std::make_unique<ConsolePanel>());

    // Wire BLD API globals.
    gw::editor::bld_api::g_globals.selection  = &selection_;
    gw::editor::bld_api::g_globals.cmd_stack  = &cmd_stack_;
}

EditorApplication::~EditorApplication() {
    shutdown_imgui();
    shutdown_vulkan();
    shutdown_window();
}

// ---------------------------------------------------------------------------
// Main loop
// ---------------------------------------------------------------------------
void EditorApplication::run() {
    double last_time = glfwGetTime();

    while (!glfwWindowShouldClose(window_) && running_) {
        glfwPollEvents();

        double now = glfwGetTime();
        float  dt  = static_cast<float>(now - last_time);
        last_time  = now;

        // Handle deferred swapchain resize.
        if (swapchain_resize_pending_) {
            // wait_idle + recreate swapchain + viewport resize handled here.
            swapchain_resize_pending_ = false;
        }

        // Handle viewport resize.
        if (auto* vpp = dynamic_cast<ViewportPanel*>(panels_.find("Viewport"))) {
            if (vpp->resize_pending_) {
                // wait_idle before destroying old image.
                // vkDeviceWaitIdle(device_->vk_device());  // wired when device exposes handle
                vpp->apply_resize(vpp->pending_w_, vpp->pending_h_);
            }
        }

        begin_frame();

        EditorContext ctx{
            .selection    = selection_,
            .cmd_stack    = cmd_stack_,
            .world        = nullptr,   // Phase 8
            .asset_db     = nullptr,   // Phase 8
            .delta_time_s = dt,
            .in_pie       = pie_.in_play()
        };

        build_ui();
        panels_.render_all(ctx);

        end_frame();

        pie_.reload_if_changed();
    }
}

// ---------------------------------------------------------------------------
// Init: Window
// ---------------------------------------------------------------------------
void EditorApplication::init_window() {
    if (!glfwInit()) {
        std::fputs("glfwInit failed\n", stderr);
        return;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window_ = glfwCreateWindow(win_w_, win_h_,
        "Greywater Editor  |  Cold Coffee Labs", nullptr, nullptr);
    assert(window_);

    glfwSetWindowUserPointer(window_, this);
    glfwSetKeyCallback(window_, on_key_callback);
    glfwSetScrollCallback(window_, on_scroll_callback);
    glfwSetFramebufferSizeCallback(window_, on_framebuffer_resize);
}

// ---------------------------------------------------------------------------
// Init: Vulkan (stubs — full wiring when VulkanDevice API is complete)
// ---------------------------------------------------------------------------
void EditorApplication::init_vulkan() {
    volkInitialize();

    // Phase 7: HAL objects are constructed here.
    // For now we allocate placeholders; the render path is wired in the
    // EditorScenePass (Phase 7 §6.2) once VulkanDevice exposes create_image().
    // Full device init path mirrors the existing sandbox/main.cpp.
    // TODO: replace with real VulkanInstance/Device/Swapchain construction.
}

// ---------------------------------------------------------------------------
// Init: ImGui
// ---------------------------------------------------------------------------
void EditorApplication::init_imgui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // Disable auto-save — we manage ini explicitly to avoid mid-frame corruption.
    // Spec ref: Phase 7 §14.
    io.IniFilename = nullptr;

    // Load saved layout.
    namespace fs = std::filesystem;
    fs::path ini_path;
#ifdef _WIN32
    const char* appdata = std::getenv("APPDATA");
    ini_path = appdata ? fs::path{appdata} / "GreywaterEditor" / "layout.ini"
                       : fs::path{"layout.ini"};
#else
    const char* home = std::getenv("HOME");
    ini_path = home ? fs::path{home} / ".config" / "greywater_editor" / "layout.ini"
                    : fs::path{"layout.ini"};
#endif

    if (fs::exists(ini_path)) {
        std::ifstream f{ini_path, std::ios::binary | std::ios::ate};
        if (f.good()) {
            auto sz = f.tellg(); f.seekg(0);
            std::string buf(static_cast<size_t>(sz), '\0');
            f.read(buf.data(), sz);
            ImGui::LoadIniSettingsFromMemory(buf.c_str(), buf.size());
            layout_built_ = true;
        }
    }

    // ImGui backend init — GLFW only (Vulkan backend needs real device).
    ImGui_ImplGlfw_InitForVulkan(window_, true);
    // ImGui_ImplVulkan_Init called after VulkanDevice is fully wired.
}

// ---------------------------------------------------------------------------
// Shutdown
// ---------------------------------------------------------------------------
void EditorApplication::shutdown_imgui() {
    // Save layout.
    namespace fs = std::filesystem;
    fs::path ini_dir;
#ifdef _WIN32
    const char* appdata = std::getenv("APPDATA");
    ini_dir = appdata ? fs::path{appdata} / "GreywaterEditor" : fs::path{"."};
#else
    const char* home = std::getenv("HOME");
    ini_dir = home ? fs::path{home} / ".config" / "greywater_editor" : fs::path{"."};
#endif

    std::error_code ec;
    fs::create_directories(ini_dir, ec);
    std::ofstream f{ini_dir / "layout.ini", std::ios::binary};
    if (f.good()) {
        size_t sz = 0;
        const char* data = ImGui::SaveIniSettingsToMemory(&sz);
        f.write(data, static_cast<std::streamsize>(sz));
    }

    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void EditorApplication::shutdown_vulkan() {
    delete swapchain_; swapchain_ = nullptr;
    delete device_;    device_    = nullptr;
    delete instance_;  instance_  = nullptr;
}

void EditorApplication::shutdown_window() {
    glfwDestroyWindow(window_);
    window_ = nullptr;
    glfwTerminate();
}

// ---------------------------------------------------------------------------
// Per-frame
// ---------------------------------------------------------------------------
void EditorApplication::begin_frame() {
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
}

void EditorApplication::build_ui() {
    // Main dockspace over the entire window.
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags host_flags =
        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_MenuBar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.f, 0.f});
    ImGui::Begin("##DockHost", nullptr, host_flags);
    ImGui::PopStyleVar(3);

    // Menu bar.
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
                gw_editor_save_scene("content/untitled.gwscene");
            if (ImGui::MenuItem("Open Scene", "Ctrl+O"))
                gw_editor_load_scene("content/untitled.gwscene");
            ImGui::Separator();
            if (ImGui::MenuItem("Quit"))
                running_ = false;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, cmd_stack_.can_undo()))
                cmd_stack_.undo();
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false, cmd_stack_.can_redo()))
                cmd_stack_.redo();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Window")) {
            for (auto& p : panels_.panels()) {
                bool vis = p->visible();
                if (ImGui::MenuItem(p->name(), nullptr, vis))
                    p->set_visible(!vis);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            ImGui::MenuItem("About Greywater Editor");
            ImGui::EndMenu();
        }

        // Dirty flag indicator in menu bar.
        if (cmd_stack_.is_dirty()) {
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20.f);
            ImGui::TextColored({0.9f, 0.6f, 0.2f, 1.f}, "*");
        }

        ImGui::EndMenuBar();
    }

    // Build default docking layout once.
    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspace_id, {0.f, 0.f}, ImGuiDockNodeFlags_None);

    if (!layout_built_ && ImGui::DockBuilderGetNode(dockspace_id) == nullptr) {
        build_docking_layout();
        layout_built_ = true;
    }

    ImGui::End();
}

void EditorApplication::build_docking_layout() {
    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->WorkSize);

    ImGuiID left, centre, right, bottom;
    ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.22f, &right,  &centre);
    ImGui::DockBuilderSplitNode(centre,       ImGuiDir_Left,  0.22f, &left,   &centre);
    ImGui::DockBuilderSplitNode(centre,       ImGuiDir_Down,  0.28f, &bottom, &centre);

    ImGui::DockBuilderDockWindow("Outliner",      left);
    ImGui::DockBuilderDockWindow("Viewport",      centre);
    ImGui::DockBuilderDockWindow("Inspector",     right);
    ImGui::DockBuilderDockWindow("Asset Browser", bottom);
    ImGui::DockBuilderDockWindow("Console",       bottom);

    ImGui::DockBuilderFinish(dockspace_id);
}

void EditorApplication::end_frame() {
    ImGui::Render();

    // ImDrawData* draw_data = ImGui::GetDrawData();
    // ImGui_ImplVulkan_RenderDrawData(draw_data, cmd_buf);  // wired once device is live

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

// ---------------------------------------------------------------------------
// Theme — dark sci-fi / Cold Coffee Labs aesthetic.
// Inspired by the attached viewport screenshot: near-black bg, cyan accents,
// amber highlights.
// ---------------------------------------------------------------------------
void EditorApplication::apply_theme() {
    ImGuiStyle& style = ImGui::GetStyle();

    // Rounding.
    style.WindowRounding    = 4.f;
    style.ChildRounding     = 3.f;
    style.FrameRounding     = 3.f;
    style.GrabRounding      = 3.f;
    style.PopupRounding     = 4.f;
    style.ScrollbarRounding = 3.f;
    style.TabRounding       = 4.f;

    // Sizing.
    style.WindowPadding     = {8.f,  8.f};
    style.FramePadding      = {5.f,  3.f};
    style.ItemSpacing       = {6.f,  4.f};
    style.ScrollbarSize     = 12.f;
    style.GrabMinSize       = 8.f;
    style.WindowBorderSize  = 1.f;
    style.FrameBorderSize   = 0.f;
    style.TabBarBorderSize  = 1.f;

    // Palette — 2026 dark studio aesthetic:
    //   BG:      #0D0F14  → very dark near-black with blue tint
    //   Panel:   #151820  → slightly lighter panel
    //   Header:  #1C2030  → section header
    //   Accent:  #4CC9A4  → teal-green (selected/active)
    //   Amber:   #F5A623  → warm amber for warnings / title
    //   Text:    #C8D0E0  → light cool-white
    //   SubText: #6A7080  → muted secondary text
    auto C = [](uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        return ImVec4{r / 255.f, g / 255.f, b / 255.f, a / 255.f};
    };

    ImVec4* col = style.Colors;
    col[ImGuiCol_Text]                  = C(200, 208, 224);
    col[ImGuiCol_TextDisabled]          = C(100, 108, 120);
    col[ImGuiCol_WindowBg]              = C( 13,  15,  20);
    col[ImGuiCol_ChildBg]               = C( 16,  18,  24);
    col[ImGuiCol_PopupBg]               = C( 20,  22,  30, 250);
    col[ImGuiCol_Border]                = C( 35,  40,  55);
    col[ImGuiCol_BorderShadow]          = C(  0,   0,   0,   0);
    col[ImGuiCol_FrameBg]               = C( 22,  26,  36);
    col[ImGuiCol_FrameBgHovered]        = C( 30,  36,  52);
    col[ImGuiCol_FrameBgActive]         = C( 40,  48,  68);
    col[ImGuiCol_TitleBg]               = C( 10,  12,  18);
    col[ImGuiCol_TitleBgActive]         = C( 16,  20,  32);
    col[ImGuiCol_TitleBgCollapsed]      = C( 10,  12,  18, 180);
    col[ImGuiCol_MenuBarBg]             = C( 14,  17,  24);
    col[ImGuiCol_ScrollbarBg]           = C( 12,  14,  20);
    col[ImGuiCol_ScrollbarGrab]         = C( 40,  50,  70);
    col[ImGuiCol_ScrollbarGrabHovered]  = C( 55,  68,  95);
    col[ImGuiCol_ScrollbarGrabActive]   = C( 76, 201, 165);   // teal
    col[ImGuiCol_CheckMark]             = C( 76, 201, 165);
    col[ImGuiCol_SliderGrab]            = C( 76, 201, 165);
    col[ImGuiCol_SliderGrabActive]      = C(120, 230, 200);
    col[ImGuiCol_Button]                = C( 30,  36,  52);
    col[ImGuiCol_ButtonHovered]         = C( 76, 201, 165,  60);
    col[ImGuiCol_ButtonActive]          = C( 76, 201, 165, 180);
    col[ImGuiCol_Header]                = C( 76, 201, 165,  50);
    col[ImGuiCol_HeaderHovered]         = C( 76, 201, 165,  80);
    col[ImGuiCol_HeaderActive]          = C( 76, 201, 165, 180);
    col[ImGuiCol_Separator]             = C( 35,  40,  55);
    col[ImGuiCol_SeparatorHovered]      = C( 76, 201, 165,  80);
    col[ImGuiCol_SeparatorActive]       = C( 76, 201, 165);
    col[ImGuiCol_ResizeGrip]            = C( 76, 201, 165,  30);
    col[ImGuiCol_ResizeGripHovered]     = C( 76, 201, 165,  90);
    col[ImGuiCol_ResizeGripActive]      = C( 76, 201, 165, 220);
    col[ImGuiCol_Tab]                   = C( 20,  24,  34);
    col[ImGuiCol_TabHovered]            = C( 76, 201, 165,  90);
    col[ImGuiCol_TabActive]             = C( 30,  40,  60);
    col[ImGuiCol_TabUnfocused]          = C( 14,  17,  24);
    col[ImGuiCol_TabUnfocusedActive]    = C( 22,  28,  42);
    col[ImGuiCol_DockingPreview]        = C( 76, 201, 165, 100);
    col[ImGuiCol_DockingEmptyBg]        = C( 10,  12,  18);
    col[ImGuiCol_PlotLines]             = C( 76, 201, 165);
    col[ImGuiCol_PlotLinesHovered]      = C(245, 166,  35);   // amber
    col[ImGuiCol_PlotHistogram]         = C( 76, 201, 165);
    col[ImGuiCol_PlotHistogramHovered]  = C(245, 166,  35);
    col[ImGuiCol_TableHeaderBg]         = C( 20,  26,  38);
    col[ImGuiCol_TableBorderStrong]     = C( 40,  46,  62);
    col[ImGuiCol_TableBorderLight]      = C( 28,  34,  48);
    col[ImGuiCol_TableRowBg]            = C(  0,   0,   0,   0);
    col[ImGuiCol_TableRowBgAlt]         = C(255, 255, 255,  8);
    col[ImGuiCol_TextSelectedBg]        = C( 76, 201, 165,  60);
    col[ImGuiCol_DragDropTarget]        = C(245, 166,  35, 200);
    col[ImGuiCol_NavHighlight]          = C( 76, 201, 165);
    col[ImGuiCol_NavWindowingHighlight] = C(245, 166,  35, 200);
    col[ImGuiCol_NavWindowingDimBg]     = C(  0,   0,   0, 100);
    col[ImGuiCol_ModalWindowDimBg]      = C(  0,   0,   0, 140);

    // Multi-viewport: round windows to match OS window decorations on Windows.
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.f;
        col[ImGuiCol_WindowBg].w = 1.f;
    }
}

// ---------------------------------------------------------------------------
// GLFW callbacks
// ---------------------------------------------------------------------------
void EditorApplication::on_key_callback(GLFWwindow* w, int key,
                                          int /*scancode*/, int action, int mods)
{
    auto* app = static_cast<EditorApplication*>(glfwGetWindowUserPointer(w));
    if (!app) return;

    // Ctrl+Z / Ctrl+Y — undo/redo.
    if (action == GLFW_PRESS && (mods & GLFW_MOD_CONTROL)) {
        if (key == GLFW_KEY_Z) { app->cmd_stack_.undo(); return; }
        if (key == GLFW_KEY_Y) { app->cmd_stack_.redo(); return; }
        if (key == GLFW_KEY_S) { gw_editor_save_scene("content/untitled.gwscene"); return; }
    }

    // Forward to panels.
    for (auto& p : app->panels_.panels())
        p->on_event(key, action);
}

void EditorApplication::on_scroll_callback(GLFWwindow* w,
                                             double /*dx*/, double dy) {
    auto* app = static_cast<EditorApplication*>(glfwGetWindowUserPointer(w));
    if (!app) return;
    if (auto* vp = dynamic_cast<ViewportPanel*>(app->panels_.find("Viewport")))
        vp->inject_scroll(static_cast<float>(dy));
}

void EditorApplication::on_framebuffer_resize(GLFWwindow* w,
                                               int /*width*/, int /*height*/) {
    auto* app = static_cast<EditorApplication*>(glfwGetWindowUserPointer(w));
    if (app) app->swapchain_resize_pending_ = true;
}

}  // namespace gw::editor
