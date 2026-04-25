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

#include "editor/a11y/editor_a11y.hpp"
#include "editor/panels/panel_registry.hpp"
#include "editor/render/render_settings.hpp"
#include "editor/selection/selection_context.hpp"
#include "editor/pie/gameplay_host.hpp"
#include "editor/render/imgui_texture_cache.hpp"
#include "editor/shell/recent_projects.hpp"
#include "editor/theme/corruption_pulse.hpp"
#include "editor/undo/command_stack.hpp"
#include "engine/core/config/cvar_registry.hpp"
#include "engine/core/time.hpp"
#include "engine/ecs/entity.hpp"
#include "engine/ecs/world.hpp"

#include <glm/fwd.hpp>
#include "engine/scene/seq/seq_camera.hpp"
#include "engine/scene/seq/sequencer_world.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

struct GLFWwindow;

namespace gw::assets { class AssetDatabase; }
namespace gw::assets::vfs { class VirtualFilesystem; }
namespace gw::editor::render { class EditorScenePass; }
namespace gw::editor::bld { class McpStdioSession; }

namespace gw::editor {

// Opaque backend — all raw Vulkan state lives behind this pimpl so the header
// stays clean of <volk.h> and keeps editor_app.hpp cheap to include.
struct EditorVulkanBackend;

enum class EditorShellPhase : std::uint8_t {
    Authenticate,
    ChooseProject,
    MainEditor,
};

class EditorApplication {
public:
    EditorApplication();
    ~EditorApplication();

    EditorApplication(const EditorApplication&)            = delete;
    EditorApplication& operator=(const EditorApplication&) = delete;

    // Run the main loop until the window is closed.
    void run();

    /// BLD Surface P — same entry as Play menu when `pie_host_context` is wired.
    [[nodiscard]] bool bld_pie_enter();
    void               bld_pie_stop();

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
    /// Binds scene + gbuffer thumbnail descriptors to the Render Targets panel.
    void sync_render_targets_thumbnails();

    // -----------------------------------------------------------------------
    // Docking layout — built once on first launch.
    // -----------------------------------------------------------------------
    void build_docking_layout();
    void apply_theme();
    void reload_imgui_fonts_for_a11y() noexcept;
    void update_clear_colors_from_theme();
    void build_launcher_ui();
    /// After panels render: drain BLD Copilot MCP turns (`GW_BLD_SERVER_EXE`).
    void poll_bld_copilot_mcp();
    /// Set project root, chdir for relative content/ paths, recent list, main editor.
    void open_project(const std::filesystem::path& root);
    /// Export current `.gwscene` and spawn `sandbox_playable` from the same bin dir (Phase 24 loop).
    void launch_detached_headless_runtime();

    /// Align PIE bootstrap (seed + `.play_cvars.toml`) with detached `sandbox_playable`.
    void sync_pie_play_bootstrap_with_scene();
    void clear_pie_play_bootstrap_paths();

    // -----------------------------------------------------------------------
    // Callbacks (GLFW)
    // -----------------------------------------------------------------------
    static void on_key_callback(GLFWwindow* w, int key, int scancode,
                                 int action, int mods);
    static void on_scroll_callback(GLFWwindow* w, double dx, double dy);
    static void on_framebuffer_resize(GLFWwindow* w, int width, int height);
    static void on_drop_paths(GLFWwindow* w, int count, const char** paths);

    /// `EditorContext::pie_pause_toggle` — matches Play menu Pause/Resume / F6.
    static void pie_pause_toggle_static(void* user_data);

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

    // Part B wire-up (pre-ed-a11y-init): accessibility config consumed by
    // apply_theme() and the Window → Accessibility modal.
    gw::editor::a11y::EditorA11yConfig a11y_config_{};
    bool a11y_modal_open_ = false;
    /// Phase 24 week 157 — one-shot photosensitivity acknowledgement (WCAG-style pre-warning).
    bool photosensitivity_warn_ack_ = false;

    // Layout built flag — persisted via ImGui ini.
    bool layout_built_ = false;
    bool layout_pending_reset_ = false;
    /// ImGui multi-viewport (floating windows). Off = consolidate to main GLFW window.
    bool viewports_enabled_ = true;

    // Swapchain resize deferred flag.
    bool swapchain_resize_pending_ = false;

    // Bootstrap shell (private gate + project picker).
    EditorShellPhase shell_phase_{EditorShellPhase::MainEditor};
    gw::editor::theme::CorruptionPulseState pulse_{};
    std::array<float, 4> swapchain_clear_{0.05f, 0.05f, 0.06f, 1.f};
    std::array<float, 4> scene_clear_{0.08f, 0.09f, 0.11f, 1.f};

    char shell_user_[128]{};
    char shell_pass_[256]{};
    char shell_gate_err_[256]{};
    char shell_path_manual_[1024]{};
    std::vector<gw::editor::shell::RecentProject> recent_projects_;
    std::filesystem::path project_root_;
    std::unique_ptr<gw::editor::bld::McpStdioSession> bld_mcp_;
    unsigned                                           mcp_next_jsonrpc_id_ = 2;
    std::optional<std::filesystem::path> pending_folder_drop_;
    std::unique_ptr<gw::editor::render::ImGuiTextureCache> imgui_tex_cache_;
    /// Project-scoped VFS + harness-mode asset DB (no engine HAL device).
    std::unique_ptr<gw::assets::vfs::VirtualFilesystem> editor_asset_vfs_;
    std::unique_ptr<gw::assets::AssetDatabase>          editor_asset_db_;
    float last_dt_sec_ = 1.f / 60.f;

    /// Stable storage for `GameplayContext::play_cvars_toml_abs_utf8` while PIE runs.
    std::string pie_play_cvars_abs_storage_;

    /// Editor-only registry: mirrors `apply_playable_bootstrap` merge for PIE (Phase 24 follow-through).
    /// `optional` so each PIE sync can `emplace()` a fresh registry (`CVarRegistry` is move-only).
    std::optional<gw::config::CVarRegistry> pie_bootstrap_cvars_{};

    // Wave 1C — offscreen scene raster: view/proj written by Viewport each frame;
    // read in the following `begin_frame` to match the debug overlay (one-frame latency).
    glm::mat4 scene_raster_view_{1.f};
    glm::mat4 scene_raster_proj_{1.f};
    std::unique_ptr<render::EditorScenePass> editor_scene_pass_{};
};

}  // namespace gw::editor
