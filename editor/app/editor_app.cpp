// editor/app/editor_app.cpp
// EditorApplication — GLFW window + Vulkan 1.2/1.3 backend + ImGui docking
// frame loop. Full real wiring; no stubs.
//
// Spec ref: Phase 7 §2, §14, §15.
//
// Architecture notes
// ------------------
// The editor creates its own VkInstance / VkDevice / VkSwapchainKHR directly
// against volk rather than reusing engine/render/hal. The HAL device does not
// yet request the dynamic-rendering + sync2 extensions that ImGui's Vulkan
// backend requires, and the HAL instance does not wire surface extensions.
// Once the HAL is extended (Phase C audit follow-up), this file can shrink to
// the Vulkan*/HAL pattern used by future engine modules.
//
// Presentation model: one semaphore per frame-in-flight for acquire, one
// semaphore per swapchain image for submit->present. Fences gate the host.
// Dynamic rendering (VK_KHR_dynamic_rendering) drives the single swapchain
// color attachment per frame. ImGui draw data is rendered into that attachment.
//
// volk + VMA must appear before `editor_app.hpp`: that header (via imgui_texture_cache.hpp, etc.)
// would otherwise install `using Vk* =` forward typedefs and clash with vulkan_core.h.
#include <volk.h>
#include <vk_mem_alloc.h>

#include "editor_app.hpp"

#include "editor/bld_api/editor_bld_api.hpp"

#include "editor/panels/outliner_panel.hpp"
#include "editor/panels/inspector_panel.hpp"
#include "editor/panels/viewport_panel.hpp"
#include "editor/panels/asset_browser_panel.hpp"
#include "editor/panels/console_panel.hpp"
#include "editor/panels/stats_panel.hpp"
#include "editor/panels/render_settings_panel.hpp"
#include "editor/panels/lighting_panel.hpp"
#include "editor/panels/render_targets_panel.hpp"
#include "editor/vscript/vscript_panel.hpp"
#include "editor/agent_panel/agent_panel.hpp"
#include "editor/bld_bridge/mcp_stdio_client.hpp"
#include "editor/seq_panel/seq_panel.hpp"
#include "editor/bld_api/editor_bld_api.hpp"
#include "editor/scene/components.hpp"
#include "engine/ecs/hierarchy.hpp"
#include "engine/scene/seq/seq_cut.hpp"
#include "engine/scene/scene_file.hpp"
#include "engine/core/log.hpp"
#include "engine/platform/process.hpp"
#include "engine/play/playable_paths.hpp"
#include "engine/play/play_bootstrap_cvars.hpp"
#include "engine/core/config/standard_cvars.hpp"
#include "engine/scene/seq/sequencer_world.hpp"
#include "engine/assets/asset_db.hpp"
#include "engine/assets/vfs/virtual_fs.hpp"

// Phase 21 wire-up: Sacrilege-specific authoring panels (pack.yml: pre-ed-sacrilege-panels).
#include "editor/panels/sacrilege/act_state_panel.hpp"
#include "editor/panels/sacrilege/ai_director_sandbox_panel.hpp"
#include "editor/panels/sacrilege/circle_editor_panel.hpp"
#include "editor/panels/sacrilege/dialogue_graph_panel.hpp"
#include "editor/panels/sacrilege/editor_copilot_panel.hpp"
#include "editor/panels/sacrilege/encounter_editor_panel.hpp"
#include "editor/panels/sacrilege/material_forge_panel.hpp"
#include "editor/panels/sacrilege/pcg_node_graph_panel.hpp"
#include "editor/panels/sacrilege/shader_forge_panel.hpp"
#include "editor/panels/sacrilege/sin_signature_panel.hpp"
#include "editor/panels/sacrilege/sacrilege_library_panel.hpp"
#include "editor/panels/sacrilege/sacrilege_readiness_panel.hpp"
#include "editor/panels/sacrilege/panel_manifest.hpp"

// Part C §23 wire-up: audit/tooling panels (pack.yml: pre-ed-audit-panels).
#include "editor/panels/audit/asset_deps_panel.hpp"
#include "editor/panels/audit/bake_panel.hpp"
#include "editor/panels/audit/heatmap_panel.hpp"
#include "editor/panels/audit/localization_panel.hpp"
#include "editor/panels/audit/profiler_panel.hpp"
#include "editor/panels/audit/shader_hotreload_panel.hpp"
#include "editor/panels/audit/shader_permutations_panel.hpp"

// Part B wire-up: module manifest, theme registry, a11y (pack.yml:
// pre-ed-module-manifest, pre-ed-theme-menu, pre-ed-a11y-init).
#include "editor/render/editor_scene_pass.hpp"
#include "engine/render/hal/vulkan_device.hpp"
#include "editor/config/editor_config.hpp"
#include "editor/modules/modules_builtin.hpp"
#include "editor/shell/file_dialog.hpp"
#include "editor/shell/franchise_roots.hpp"
#include "editor/shell/layout_state.hpp"
#include "editor/shell/private_gate.hpp"
#include "editor/shell/recent_projects.hpp"
#include "editor/theme/editor_fonts.hpp"
#include "editor/theme/imgui_style_from_palette.hpp"
#include "editor/theme/theme_registry.hpp"
#include "editor/theme/palette_imgui.hpp"
#include "editor/a11y/editor_a11y.hpp"
#include "editor/scene/transform_system.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <ImGuizmo.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#define GLM_FORCE_RADIANS
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

static void touch_play_cvars_stub_file(const std::filesystem::path& abs) {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::create_directories(abs.parent_path(), ec);
    std::ofstream f(abs, std::ios::binary);
    if (f) {
        f << "# Editor play-from-here CVar snapshot (merged into `pie_bootstrap_cvars_` on "
             "PIE sync; see `gw::play::apply_play_bootstrap_to_registry`).\n";
    }
}

namespace gw::editor {

bool editor_bld_pie_enter(void* ctx);
void editor_bld_pie_stop(void* ctx);

namespace {

constexpr uint32_t kFramesInFlight = 2;

[[noreturn]] void gw_fatal(const char* msg) {
    std::fprintf(stderr, "[editor] FATAL: %s\n", msg);
    std::exit(EXIT_FAILURE);
}

#define GW_VKCHECK(expr)                                                       \
    do {                                                                       \
        VkResult _r = (expr);                                                  \
        if (_r != VK_SUCCESS) {                                                \
            std::fprintf(stderr, "[editor] %s failed: VkResult=%d\n",          \
                         #expr, static_cast<int>(_r));                         \
            std::exit(EXIT_FAILURE);                                           \
        }                                                                      \
    } while (0)

constexpr uint32_t kEditorHistSampleDim = 128;

struct QueueFamilies {
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;
    [[nodiscard]] bool complete() const noexcept {
        return graphics.has_value() && present.has_value();
    }
};

QueueFamilies find_queue_families(VkPhysicalDevice gpu, VkSurfaceKHR surface) {
    QueueFamilies q{};
    uint32_t n = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &n, nullptr);
    std::vector<VkQueueFamilyProperties> props(n);
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &n, props.data());
    for (uint32_t i = 0; i < n; ++i) {
        if (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            q.graphics = i;
        VkBool32 present_supported = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, surface, &present_supported);
        if (present_supported)
            q.present = i;
        if (q.complete()) break;
    }
    return q;
}

VkPhysicalDevice pick_physical_device(VkInstance instance, VkSurfaceKHR surface) {
    uint32_t n = 0;
    vkEnumeratePhysicalDevices(instance, &n, nullptr);
    if (n == 0) gw_fatal("no Vulkan-capable physical devices");
    std::vector<VkPhysicalDevice> gpus(n);
    vkEnumeratePhysicalDevices(instance, &n, gpus.data());

    for (VkPhysicalDevice gpu : gpus) {
        VkPhysicalDeviceProperties p{};
        vkGetPhysicalDeviceProperties(gpu, &p);
        if (p.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
            find_queue_families(gpu, surface).complete()) {
            std::fprintf(stdout, "[editor] selected GPU: %s (Vulkan %u.%u)\n",
                         p.deviceName,
                         VK_API_VERSION_MAJOR(p.apiVersion),
                         VK_API_VERSION_MINOR(p.apiVersion));
            return gpu;
        }
    }
    for (VkPhysicalDevice gpu : gpus) {
        if (find_queue_families(gpu, surface).complete())
            return gpu;
    }
    gw_fatal("no Vulkan device exposes both graphics + present queue families");
}

[[maybe_unused]] VKAPI_ATTR VkBool32 VKAPI_CALL debug_messenger_cb(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT /*type*/,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* /*user*/) {
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::fprintf(stderr, "[vk] %s\n", data->pMessage);
    }
    return VK_FALSE;
}

}  // namespace

// ---------------------------------------------------------------------------
// EditorVulkanBackend — owns every VK handle the editor uses.
// ---------------------------------------------------------------------------
struct EditorVulkanBackend {
    VkInstance               instance          = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT messenger         = VK_NULL_HANDLE;
    VkSurfaceKHR             surface           = VK_NULL_HANDLE;
    VkPhysicalDevice         physical_device   = VK_NULL_HANDLE;
    VkDevice                 device            = VK_NULL_HANDLE;

    uint32_t                 graphics_family   = UINT32_MAX;
    uint32_t                 present_family    = UINT32_MAX;
    VkQueue                  graphics_queue    = VK_NULL_HANDLE;
    VkQueue                  present_queue     = VK_NULL_HANDLE;

    VkSwapchainKHR           swapchain         = VK_NULL_HANDLE;
    VkFormat                 swapchain_format  = VK_FORMAT_UNDEFINED;
    VkColorSpaceKHR          swapchain_cs      = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    VkExtent2D               swapchain_extent  = {};
    uint32_t                 swapchain_min_count = 0;
    std::vector<VkImage>     swapchain_images;
    std::vector<VkImageView> swapchain_views;

    VkCommandPool            cmd_pool          = VK_NULL_HANDLE;
    std::array<VkCommandBuffer, kFramesInFlight> cmd_buffers{};

    std::array<VkSemaphore, kFramesInFlight> image_available{};
    std::vector<VkSemaphore>                 render_finished;  // per swapchain image
    std::array<VkFence,     kFramesInFlight> in_flight{};

    VkDescriptorPool         imgui_descriptor_pool = VK_NULL_HANDLE;
    VkFormat                 imgui_color_format = VK_FORMAT_UNDEFINED;

    uint32_t                 frame_index       = 0;
    uint32_t                 acquired_image    = 0;
    bool                     imgui_initialised = false;

    // ----- VMA allocator (shared with engine_render's vmaCreateAllocator impl) -----
    VmaAllocator             allocator         = VK_NULL_HANDLE;

    /// Borrowed HAL device for `AssetDatabase` GPU mesh/texture loads (ADR vertical slice).
    std::unique_ptr<gw::render::hal::VulkanDevice> hal_for_assets{};

    // ----- Offscreen scene render target (Phase 7 §6.1) -----
    // Sized to the Viewport panel's content area; resized on demand from run().
    // RGBA8 colour + TRANSFER_SRC for G-buffer strip mirrors + luminance readback.
    VkImage                  scene_image       = VK_NULL_HANDLE;
    VmaAllocation            scene_alloc       = VK_NULL_HANDLE;
    VkImageView              scene_view        = VK_NULL_HANDLE;
    VkSampler                scene_sampler     = VK_NULL_HANDLE;
    VkDescriptorSet          scene_imgui_ds    = VK_NULL_HANDLE;
    VkExtent2D               scene_extent      = {};
    VkFormat                 scene_format      = VK_FORMAT_R8G8B8A8_UNORM;

    // Scene depth (D32 or packed D24S8 — cleared each frame; mesh pass will write).
    VkImage                  scene_depth_image  = VK_NULL_HANDLE;
    VmaAllocation            scene_depth_alloc  = VK_NULL_HANDLE;
    VkImageView              scene_depth_view   = VK_NULL_HANDLE;
    VkFormat                 scene_depth_format = VK_FORMAT_UNDEFINED;
    bool                     scene_depth_had_frame  = false;
    /// Per swapchain of `scene_image`: after the first end-to-end use, the image is
    // SHADER_READ_ONLY; subsequent frames must transition from that, not from UNDEFINED.
    bool                     scene_image_had_frame = false;

    // Cockpit G-buffer thumbnails (same extent as scene; filled via vkCmdCopyImage).
    static constexpr int     kGbufferThumbCount = 5;
    struct GbufferThumb {
        VkImage          image     = VK_NULL_HANDLE;
        VmaAllocation    alloc     = VK_NULL_HANDLE;
        VkImageView      view      = VK_NULL_HANDLE;
        VkDescriptorSet  imgui_ds  = VK_NULL_HANDLE;
    };
    std::array<GbufferThumb, kGbufferThumbCount> gbuffer_thumbs{};

    // Per-frame staging for GPU → CPU luminance tile (kEditorHistSampleDim² RGBA8).
    std::array<VkBuffer,     kFramesInFlight> hist_staging_buf{};
    std::array<VmaAllocation, kFramesInFlight> hist_staging_alloc{};
    std::array<uint32_t, kFramesInFlight> hist_sample_w{};
    std::array<uint32_t, kFramesInFlight> hist_sample_h{};
    std::array<bool, kFramesInFlight>      hist_staging_used{};
    std::array<bool, kGbufferThumbCount> gbuffer_thumb_seen{};

    VkQueryPool              timestamp_query_pool = VK_NULL_HANDLE;
    float                    gpu_timestamp_period_ns = 1.f;
    float                    last_gpu_time_ms        = 0.f;
    bool                     gpu_timestamps_enabled = false;
    int                      gpu_timestamp_warmup_frames = 3;
};

void clear_editor_luminance_readout(gw::editor::render::RenderSettings& rs) noexcept {
    rs.histogram.buckets.fill(0.f);
    rs.exposure.average_luminance     = 0.f;
    rs.exposure.luminance_sample_valid = false;
}

void build_luminance_histogram_from_rgba8(const std::uint8_t* px, std::uint32_t cw,
                                          std::uint32_t ch, std::uint32_t row_stride,
                                          gw::editor::render::LuminanceHistogram& out,
                                          float& average_luminance) noexcept {
    float           sum_l = 0.f;
    std::uint32_t   count = 0;
    for (std::uint32_t y = 0; y < ch; ++y) {
        const std::uint8_t* row = px + static_cast<std::size_t>(y) * row_stride;
        for (std::uint32_t x = 0; x < cw; ++x) {
            const std::uint8_t* p = row + x * 4u;
            const float         rf = static_cast<float>(p[0]) * (1.f / 255.f);
            const float         gf = static_cast<float>(p[1]) * (1.f / 255.f);
            const float         bf = static_cast<float>(p[2]) * (1.f / 255.f);
            const float         l  = std::max(0.0001f, 0.2126f * rf + 0.7152f * gf + 0.0722f * bf);
            sum_l += l;
            ++count;
            const int bin = static_cast<int>(l * 63.f + 0.5f);
            out.buckets[static_cast<std::size_t>(std::clamp(bin, 0, 63))] += 1.f;
        }
    }
    if (count > 0) {
        average_luminance = sum_l / static_cast<float>(count);
        float total = 0.f;
        for (float& b : out.buckets) total += b;
        if (total > 0.f) {
            for (float& b : out.buckets) b /= total;
        }
    } else {
        average_luminance = 0.f;
        out.buckets.fill(0.f);
    }
}

void refresh_editor_histogram_from_staging(EditorVulkanBackend* vk, uint32_t fi,
                                           gw::editor::render::RenderSettings& rs) {
    if (!vk || !vk->hist_staging_buf[fi] || !vk->hist_staging_alloc[fi] || !vk->allocator) {
        clear_editor_luminance_readout(rs);
        return;
    }
    if (vk->hist_sample_w[fi] == 0 || vk->hist_sample_h[fi] == 0) {
        clear_editor_luminance_readout(rs);
        return;
    }

    VmaAllocationInfo ainfo{};
    vmaGetAllocationInfo(vk->allocator, vk->hist_staging_alloc[fi], &ainfo);

    void* mapped = nullptr;
    if (vmaMapMemory(vk->allocator, vk->hist_staging_alloc[fi], &mapped) != VK_SUCCESS ||
        !mapped) {
        clear_editor_luminance_readout(rs);
        return;
    }

    VkMappedMemoryRange inv{};
    inv.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    inv.memory = ainfo.deviceMemory;
    inv.offset = ainfo.offset;
    inv.size   = VK_WHOLE_SIZE;
    vkInvalidateMappedMemoryRanges(vk->device, 1, &inv);

    rs.histogram.buckets.fill(0.f);
    const auto* bytes = static_cast<const std::uint8_t*>(mapped);
    build_luminance_histogram_from_rgba8(bytes, vk->hist_sample_w[fi], vk->hist_sample_h[fi],
                                         vk->hist_sample_w[fi] * 4u, rs.histogram,
                                         rs.exposure.average_luminance);
    rs.exposure.luminance_sample_valid = true;
    vmaUnmapMemory(vk->allocator, vk->hist_staging_alloc[fi]);
}

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------
EditorApplication::EditorApplication()
    : vk_(std::make_unique<EditorVulkanBackend>()) {
    {
        const float   a  = 16.f / 9.f;
        scene_raster_view_ = glm::lookAt(
            glm::vec3(6.f, 4.f, 6.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
        scene_raster_proj_ = glm::perspective(glm::radians(60.f), a, 0.1f, 2000.f);
    }
    // pre-ed-module-manifest: register twelve built-in editor modules at
    // startup. Panel/tool factories are still empty stubs (Phase 21+) but the
    // registry is now live so later waves can attach panels declaratively.
    gw::editor::modules::register_builtin_modules();

    // pre-ed-theme-menu: load persisted theme (default: Corrupted Relic).
    gw::editor::theme::ThemeRegistry::instance().set_active(
        gw::editor::config::load_saved_theme(
            gw::editor::theme::ThemeId::CorruptedRelic));

    // Load before `init_imgui` so the first atlas build can honour mono sizing.
    a11y_config_ = gw::editor::a11y::load_from_config();

    init_window();
    init_vulkan();
    init_imgui();

    // pre-ed-a11y-init: theme overrides + ImGui keyboard nav (requires context).
    gw::editor::a11y::apply(a11y_config_);
    apply_theme();

    // Panels — no raw new; unique_ptr owned by the registry. (Non-negotiable #5)
    auto vp = std::make_unique<ViewportPanel>();
    vp->set_window(window_);
    panels_.add(std::move(vp));
    panels_.add(std::make_unique<OutlinerPanel>());
    panels_.add(std::make_unique<InspectorPanel>());
    panels_.add(std::make_unique<AssetBrowserPanel>());
    panels_.add(std::make_unique<ConsolePanel>());
    panels_.add(std::make_unique<StatsPanel>(&render_settings_));
    panels_.add(std::make_unique<RenderSettingsPanel>(&render_settings_));
    panels_.add(std::make_unique<LightingPanel>(&render_settings_));
    panels_.add(std::make_unique<RenderTargetsPanel>());
    // Phase 8 week 047 — Visual Scripting node-graph panel (ImNodes over IR).
    panels_.add(std::make_unique<VScriptPanel>());
    // Phase 9 week 048 — Brewed Logic Directive chat panel.
    panels_.add(std::make_unique<gw::editor::agent::AgentPanel>());
    // Phase 18-B — Sequencer timeline + camera preview.
    panels_.add(std::make_unique<gw::editor::seq::SeqPanel>());

    // Phase 21 wire-up (pre-ed-sacrilege-panels): ten Sacrilege authoring
    // surfaces. Panels construct default; runtime state lands in Phase 21+.
    using namespace gw::editor::panels::sacrilege;
    panels_.add(std::make_unique<ActStatePanel>());
    panels_.add(std::make_unique<AiDirectorSandboxPanel>());
    panels_.add(std::make_unique<CircleEditorPanel>());
    panels_.add(std::make_unique<DialogueGraphPanel>());
    panels_.add(std::make_unique<EditorCopilotPanel>());
    panels_.add(std::make_unique<EncounterEditorPanel>());
    panels_.add(std::make_unique<MaterialForgePanel>());
    panels_.add(std::make_unique<PcgNodeGraphPanel>());
    panels_.add(std::make_unique<ShaderForgePanel>());
    panels_.add(std::make_unique<SinSignaturePanel>());
    panels_.add(std::make_unique<SacrilegeLibraryPanel>());
    panels_.add(std::make_unique<SacrilegeReadinessPanel>(&panels_));

    // Part C §23 wire-up (pre-ed-audit-panels): seven tooling surfaces.
    using namespace gw::editor::panels::audit;
    panels_.add(std::make_unique<AssetDepsPanel>());
    panels_.add(std::make_unique<BakePanel>());
    panels_.add(std::make_unique<HeatmapPanel>());
    panels_.add(std::make_unique<LocalizationPanel>());
    panels_.add(std::make_unique<ProfilerPanel>());
    panels_.add(std::make_unique<ShaderHotReloadPanel>());
    panels_.add(std::make_unique<ShaderPermutationsPanel>());

    for (std::string_view sv : gw::editor::panels::sacrilege::kRequiredSacrilegePanelNames) {
        if (!panels_.find(std::string{sv}.c_str())) {
            std::string msg = "Sacrilege panel not registered: ";
            msg += sv;
            gw_fatal(msg.c_str());
        }
    }

    // Wire BLD API globals.
    gw::editor::bld_api::g_globals.selection = &selection_;
    gw::editor::bld_api::g_globals.cmd_stack = &cmd_stack_;
    gw::editor::bld_api::g_globals.world     = &scene_world_;
    gw::editor::bld_api::g_globals.sequencer_world   = &sequencer_world_;
    gw::editor::bld_api::g_globals.cinematic_system  = &cinematic_camera_;
    gw::editor::bld_api::install_bld_host_callbacks();
    gw::editor::bld_api::g_globals.pie_enter_play   = &editor_bld_pie_enter;
    gw::editor::bld_api::g_globals.pie_stop_play    = &editor_bld_pie_stop;
    gw::editor::bld_api::g_globals.pie_host_context = this;

    // Seed the scene with three demo entities so the Outliner and Inspector
    // have something to display on first launch. A root with two children
    // exercises the hierarchy + ECS for_each path end-to-end.
    using gw::editor::scene::NameComponent;
    using gw::editor::scene::TransformComponent;
    using gw::editor::scene::VisibilityComponent;
    using gw::ecs::HierarchyComponent;

    const auto root   = scene_world_.create_entity();
    const auto childA = scene_world_.create_entity();
    const auto childB = scene_world_.create_entity();

    scene_world_.add_component(root,   NameComponent{"Scene Root"});
    scene_world_.add_component(root,   TransformComponent{});
    scene_world_.add_component(root,   VisibilityComponent{});

    scene_world_.add_component(childA, NameComponent{"Cube"});
    scene_world_.add_component(childA, TransformComponent{
        glm::dvec3{-1.5, 0.0, 0.0}, glm::quat{1.f, 0.f, 0.f, 0.f},
        glm::vec3{1.f, 1.f, 1.f}});
    scene_world_.add_component(childA, VisibilityComponent{});

    scene_world_.add_component(childB, NameComponent{"Sphere"});
    scene_world_.add_component(childB, TransformComponent{
        glm::dvec3{ 1.5, 0.0, 0.0}, glm::quat{1.f, 0.f, 0.f, 0.f},
        glm::vec3{1.f, 1.f, 1.f}});
    scene_world_.add_component(childB, VisibilityComponent{});

    (void)scene_world_.reparent(childA, root);
    (void)scene_world_.reparent(childB, root);

    seq_player_entity_ = scene_world_.create_entity();
    scene_world_.add_component(seq_player_entity_, gw::seq::SeqPlayerComponent{});
    gw::editor::bld_api::g_globals.seq_player_entity_bits = seq_player_entity_.raw_bits();

    recent_projects_ = gw::editor::shell::load_recent_projects();
    if (gw::editor::shell::env_skip_private_gate() ||
        !gw::editor::shell::private_gate_file_exists()) {
        shell_phase_ = EditorShellPhase::ChooseProject;
    } else {
        shell_phase_ = EditorShellPhase::Authenticate;
    }

    if (shell_phase_ == EditorShellPhase::ChooseProject &&
        !gw::editor::shell::env_no_auto_project()) {
        if (auto repo = gw::editor::shell::find_greywater_repo_root()) {
            open_project(*repo);
        }
    }
}

EditorApplication::~EditorApplication() {
    gw::editor::bld_api::g_globals.pie_enter_play   = nullptr;
    gw::editor::bld_api::g_globals.pie_stop_play    = nullptr;
    gw::editor::bld_api::g_globals.pie_host_context = nullptr;
    shutdown_imgui();
    shutdown_vulkan();
    shutdown_window();
}

bool EditorApplication::bld_pie_enter() {
    if (shell_phase_ != EditorShellPhase::MainEditor) return false;
    pie_ctx_.world = &scene_world_;
    pie_ctx_.time  = &pie_time_;
    pie_time_      = {};
    sync_pie_play_bootstrap_with_scene();
    return pie_.enter_play(pie_ctx_);
}

void EditorApplication::bld_pie_stop() {
    if (shell_phase_ != EditorShellPhase::MainEditor) return;
    pie_.stop(pie_ctx_);
    pie_time_ = {};
    clear_pie_play_bootstrap_paths();
}

// ---------------------------------------------------------------------------
// Main loop
// ---------------------------------------------------------------------------
void EditorApplication::run() {
    gw::core::log_init_from_environment();

    double last_time = glfwGetTime();

    while (!glfwWindowShouldClose(window_) && running_) {
        glfwPollEvents();

        const double now = glfwGetTime();
        const float  dt  = static_cast<float>(now - last_time);
        last_time        = now;
        last_dt_sec_     = (dt > 1e-6f) ? dt : last_dt_sec_;

        if (pending_folder_drop_) {
            namespace fs = std::filesystem;
            if (shell_phase_ == EditorShellPhase::ChooseProject &&
                gw::editor::shell::is_likely_project_root(*pending_folder_drop_)) {
                open_project(*pending_folder_drop_);
            }
            pending_folder_drop_.reset();
        }

        if (swapchain_resize_pending_) {
            recreate_swapchain();
            swapchain_resize_pending_ = false;
        }

        if (auto* vpp = dynamic_cast<ViewportPanel*>(panels_.find("Viewport"))) {
            if (vpp->resize_pending_) {
                vkDeviceWaitIdle(vk_->device);
                const uint32_t w = vpp->pending_w_;
                const uint32_t h = vpp->pending_h_;
                destroy_scene_rt();
                create_scene_rt(w, h);
                vpp->set_scene_texture(vk_->scene_imgui_ds, w, h);
                vpp->apply_resize(w, h);
                sync_render_targets_thumbnails();
            }
        }

        (void)gw::editor::scene::update_transforms(scene_world_);

        if (!begin_frame()) {
            continue;  // swapchain out-of-date; retry next iteration
        }

        // Keep the GameplayContext pointing at the live authoring world and
        // shared TimeState. These pointers are re-bound every frame so the
        // gameplay module (and enter_play snapshot path) always sees the
        // current engine-owned state.
        pie_ctx_.world = &scene_world_;
        pie_ctx_.time  = &pie_time_;

        if (pie_.in_play()) {
            const float scaled_dt =
                (pie_.state() == GameplayHost::PIEState::Paused)
                    ? 0.f
                    : dt * pie_time_.time_scale;
            pie_time_.dt_s    = scaled_dt;
            pie_time_.total_s += scaled_dt;
            ++pie_time_.frame;
            pie_.tick(pie_ctx_, scaled_dt);
        } else {
            pie_time_.dt_s = 0.f;
        }

        if (editor_asset_db_) {
            editor_asset_db_->tick();
        }

        gw::editor::theme::tick_pulse(pulse_, dt);

        gw::seq::SequencerSystem::tick(sequencer_world_, scene_world_, nullptr, dt);
        double play_head = 0.0;
        scene_world_.for_each<gw::seq::SeqPlayerComponent>(
            [&](const gw::ecs::Entity, const gw::seq::SeqPlayerComponent& pl) { play_head = pl.play_head_frame; });
        gw::seq::CutSystem::tick(scene_world_, play_head);
        cinematic_camera_.tick(scene_world_);

        render_settings_.stats.gpu_ms = vk_->last_gpu_time_ms;

        EditorContext ctx{
            .selection    = selection_,
            .cmd_stack    = cmd_stack_,
            .world        = &scene_world_,
            .asset_db     = (shell_phase_ == EditorShellPhase::MainEditor && editor_asset_db_)
                ? editor_asset_db_.get()
                : nullptr,
            .delta_time_s = dt,
            .framegraph_gpu_ms = vk_->last_gpu_time_ms,
            .in_pie       = pie_.in_play(),
            .pie_paused   = pie_.in_play() &&
                (pie_.state() == GameplayHost::PIEState::Paused),
            .pie_pause_toggle     = &EditorApplication::pie_pause_toggle_static,
            .pie_pause_user_data  = this,
            .sequencer    = &sequencer_world_,
            .cinematic    = &cinematic_camera_,
            .scene_color_descriptor = reinterpret_cast<void*>(vk_->scene_imgui_ds),
            .project_root = (shell_phase_ == EditorShellPhase::MainEditor)
                ? &project_root_
                : nullptr,
            .imgui_textures     = imgui_tex_cache_.get(),
            .scene_raster_view  = &scene_raster_view_,
            .scene_raster_proj  = &scene_raster_proj_,
        };

        build_ui();
        if (shell_phase_ == EditorShellPhase::MainEditor) {
            panels_.render_all(ctx);
            poll_bld_copilot_mcp();
        }
        gw::editor::theme::draw_pulse_overlay(pulse_);

        end_frame();

        pie_.reload_if_changed();
    }

    if (vk_->device)
        vkDeviceWaitIdle(vk_->device);
}

// ---------------------------------------------------------------------------
// Init: Window
// ---------------------------------------------------------------------------
void EditorApplication::init_window() {
    if (!glfwInit()) gw_fatal("glfwInit failed");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window_ = glfwCreateWindow(win_w_, win_h_, "Greywater Editor", nullptr, nullptr);
    if (!window_) gw_fatal("glfwCreateWindow failed");

    glfwSetWindowUserPointer(window_, this);
    glfwSetKeyCallback(window_, on_key_callback);
    glfwSetScrollCallback(window_, on_scroll_callback);
    glfwSetFramebufferSizeCallback(window_, on_framebuffer_resize);
    glfwSetDropCallback(window_, on_drop_paths);
}

// ---------------------------------------------------------------------------
// Init: Vulkan
// ---------------------------------------------------------------------------
void EditorApplication::init_vulkan() {
    GW_VKCHECK(volkInitialize());

    // --- Instance ----------------------------------------------------------
    VkApplicationInfo app{};
    app.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pApplicationName   = "Greywater Editor";
    app.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    app.pEngineName        = "Greywater_Engine";
    app.engineVersion      = VK_MAKE_VERSION(0, 0, 1);
    app.apiVersion         = VK_API_VERSION_1_3;  // RX 580 exposes 1.3

    uint32_t glfw_ext_count = 0;
    const char** glfw_exts = glfwGetRequiredInstanceExtensions(&glfw_ext_count);
    std::vector<const char*> instance_extensions(glfw_exts, glfw_exts + glfw_ext_count);

    std::vector<const char*> instance_layers;
#if GW_VK_VALIDATION
    instance_layers.push_back("VK_LAYER_KHRONOS_validation");
    instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    VkInstanceCreateInfo ici{};
    ici.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ici.pApplicationInfo        = &app;
    ici.enabledLayerCount       = static_cast<uint32_t>(instance_layers.size());
    ici.ppEnabledLayerNames     = instance_layers.data();
    ici.enabledExtensionCount   = static_cast<uint32_t>(instance_extensions.size());
    ici.ppEnabledExtensionNames = instance_extensions.data();
    GW_VKCHECK(vkCreateInstance(&ici, nullptr, &vk_->instance));
    volkLoadInstance(vk_->instance);

#if GW_VK_VALIDATION
    VkDebugUtilsMessengerCreateInfoEXT dmi{};
    dmi.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    dmi.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    dmi.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                          VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                          VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    dmi.pfnUserCallback = debug_messenger_cb;
    GW_VKCHECK(vkCreateDebugUtilsMessengerEXT(vk_->instance, &dmi, nullptr, &vk_->messenger));
#endif

    // --- Surface -----------------------------------------------------------
    GW_VKCHECK(glfwCreateWindowSurface(vk_->instance, window_, nullptr, &vk_->surface));

    // --- Physical device + queue families ---------------------------------
    vk_->physical_device = pick_physical_device(vk_->instance, vk_->surface);
    QueueFamilies q = find_queue_families(vk_->physical_device, vk_->surface);
    vk_->graphics_family = *q.graphics;
    vk_->present_family  = *q.present;

    // --- Logical device ----------------------------------------------------
    const float qp = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> qcis;
    std::vector<uint32_t> unique_families{vk_->graphics_family};
    if (vk_->present_family != vk_->graphics_family)
        unique_families.push_back(vk_->present_family);
    for (uint32_t fam : unique_families) {
        VkDeviceQueueCreateInfo qci{};
        qci.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qci.queueFamilyIndex = fam;
        qci.queueCount       = 1;
        qci.pQueuePriorities = &qp;
        qcis.push_back(qci);
    }

    const char* device_exts[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
    };

    VkPhysicalDeviceSynchronization2Features sync2{};
    sync2.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    sync2.synchronization2 = VK_TRUE;

    VkPhysicalDeviceDynamicRenderingFeatures dyn_rendering{};
    dyn_rendering.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dyn_rendering.pNext            = &sync2;
    dyn_rendering.dynamicRendering = VK_TRUE;

    VkDeviceCreateInfo dci{};
    dci.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dci.pNext                   = &dyn_rendering;
    dci.queueCreateInfoCount    = static_cast<uint32_t>(qcis.size());
    dci.pQueueCreateInfos       = qcis.data();
    dci.enabledExtensionCount   = static_cast<uint32_t>(std::size(device_exts));
    dci.ppEnabledExtensionNames = device_exts;
    GW_VKCHECK(vkCreateDevice(vk_->physical_device, &dci, nullptr, &vk_->device));
    volkLoadDevice(vk_->device);

    vkGetDeviceQueue(vk_->device, vk_->graphics_family, 0, &vk_->graphics_queue);
    vkGetDeviceQueue(vk_->device, vk_->present_family,  0, &vk_->present_queue);

    VkPhysicalDeviceProperties phys_props{};
    vkGetPhysicalDeviceProperties(vk_->physical_device, &phys_props);
    vk_->gpu_timestamp_period_ns = phys_props.limits.timestampPeriod;
    vk_->gpu_timestamps_enabled =
        phys_props.limits.timestampComputeAndGraphics == VK_TRUE &&
        phys_props.limits.timestampPeriod > 0.f;

    // --- Swapchain ---------------------------------------------------------
    recreate_swapchain();

    // --- Command pool + buffers -------------------------------------------
    VkCommandPoolCreateInfo cpi{};
    cpi.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cpi.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cpi.queueFamilyIndex = vk_->graphics_family;
    GW_VKCHECK(vkCreateCommandPool(vk_->device, &cpi, nullptr, &vk_->cmd_pool));

    VkCommandBufferAllocateInfo cbai{};
    cbai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.commandPool        = vk_->cmd_pool;
    cbai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = kFramesInFlight;
    GW_VKCHECK(vkAllocateCommandBuffers(vk_->device, &cbai, vk_->cmd_buffers.data()));

    // --- Sync primitives ---------------------------------------------------
    VkSemaphoreCreateInfo sci{};
    sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fci{};
    fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < kFramesInFlight; ++i) {
        GW_VKCHECK(vkCreateSemaphore(vk_->device, &sci, nullptr, &vk_->image_available[i]));
        GW_VKCHECK(vkCreateFence(vk_->device, &fci, nullptr, &vk_->in_flight[i]));
    }
    vk_->render_finished.resize(vk_->swapchain_images.size());
    for (size_t i = 0; i < vk_->render_finished.size(); ++i) {
        GW_VKCHECK(vkCreateSemaphore(vk_->device, &sci, nullptr, &vk_->render_finished[i]));
    }

    // --- ImGui descriptor pool --------------------------------------------
    // ImGui uses one combined-image-sampler per texture; 1000 should comfortably
    // cover editor icons + panel images for the v0.1 deliverable.
    const VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER,                1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         1000 },
    };
    VkDescriptorPoolCreateInfo dpi{};
    dpi.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dpi.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    dpi.maxSets       = 1000 * static_cast<uint32_t>(std::size(pool_sizes));
    dpi.poolSizeCount = static_cast<uint32_t>(std::size(pool_sizes));
    dpi.pPoolSizes    = pool_sizes;
    GW_VKCHECK(vkCreateDescriptorPool(vk_->device, &dpi, nullptr, &vk_->imgui_descriptor_pool));

    // --- VMA allocator (shared with engine_render's VMA_IMPLEMENTATION) ---
    VmaVulkanFunctions vma_fns{};
    vma_fns.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vma_fns.vkGetDeviceProcAddr   = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo ai{};
    ai.vulkanApiVersion = VK_API_VERSION_1_3;
    ai.physicalDevice   = vk_->physical_device;
    ai.device           = vk_->device;
    ai.instance         = vk_->instance;
    ai.pVulkanFunctions = &vma_fns;
    GW_VKCHECK(vmaCreateAllocator(&ai, &vk_->allocator));

    try {
        vk_->hal_for_assets = std::make_unique<gw::render::hal::VulkanDevice>(
            gw::render::hal::VulkanDevice::borrow_existing(
                vk_->instance, vk_->physical_device, vk_->device, vk_->allocator,
                vk_->graphics_queue, vk_->graphics_family));
    } catch (const std::exception& ex) {
        std::fprintf(stderr,
                     "[editor] VulkanDevice::borrow_existing failed (GPU AssetDatabase disabled): "
                     "%s\n",
                     ex.what());
        vk_->hal_for_assets.reset();
    }

    // Histogram readback staging (one buffer per frame-in-flight).
    {
        const VkDeviceSize sz =
            static_cast<VkDeviceSize>(kEditorHistSampleDim) * kEditorHistSampleDim * 4u;
        for (uint32_t i = 0; i < kFramesInFlight; ++i) {
            VkBufferCreateInfo bci{};
            bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bci.size  = sz;
            bci.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            VmaAllocationCreateInfo aci{};
            aci.usage = VMA_MEMORY_USAGE_AUTO;
            aci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
            GW_VKCHECK(vmaCreateBuffer(vk_->allocator, &bci, &aci, &vk_->hist_staging_buf[i],
                                       &vk_->hist_staging_alloc[i], nullptr));
        }
    }

    if (vk_->gpu_timestamps_enabled) {
        VkQueryPoolCreateInfo qpci{};
        qpci.sType      = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        qpci.queryType  = VK_QUERY_TYPE_TIMESTAMP;
        qpci.queryCount = kFramesInFlight * 2;
        const VkResult qp =
            vkCreateQueryPool(vk_->device, &qpci, nullptr, &vk_->timestamp_query_pool);
        if (qp != VK_SUCCESS) {
            vk_->gpu_timestamps_enabled = false;
            vk_->timestamp_query_pool   = VK_NULL_HANDLE;
        }
    }
}

// ---------------------------------------------------------------------------
// Scene render target (Phase 7 §6.1)
// ---------------------------------------------------------------------------
void EditorApplication::create_scene_rt(uint32_t w, uint32_t h) {
    if (w == 0 || h == 0) return;
    if (!vk_ || !vk_->device || !vk_->allocator) return;

    // Image: RGBA8, COLOR_ATTACHMENT + SAMPLED so ImGui can read it.
    VkImageCreateInfo ici{};
    ici.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ici.imageType     = VK_IMAGE_TYPE_2D;
    ici.format        = vk_->scene_format;
    ici.extent        = {w, h, 1};
    ici.mipLevels     = 1;
    ici.arrayLayers   = 1;
    ici.samples       = VK_SAMPLE_COUNT_1_BIT;
    ici.tiling        = VK_IMAGE_TILING_OPTIMAL;
    ici.usage         = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                        VK_IMAGE_USAGE_SAMPLED_BIT |
                        VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    ici.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo aci{};
    aci.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    GW_VKCHECK(vmaCreateImage(vk_->allocator, &ici, &aci,
                               &vk_->scene_image, &vk_->scene_alloc, nullptr));

    // View.
    VkImageViewCreateInfo ivi{};
    ivi.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ivi.image                           = vk_->scene_image;
    ivi.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    ivi.format                          = vk_->scene_format;
    ivi.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    ivi.subresourceRange.levelCount     = 1;
    ivi.subresourceRange.layerCount     = 1;
    GW_VKCHECK(vkCreateImageView(vk_->device, &ivi, nullptr, &vk_->scene_view));

    // Sampler (linear, clamp-to-edge — safe defaults for editor readback).
    VkSamplerCreateInfo sci{};
    sci.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sci.magFilter    = VK_FILTER_LINEAR;
    sci.minFilter    = VK_FILTER_LINEAR;
    sci.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sci.minLod       = 0.f;
    sci.maxLod       = 1.f;
    sci.borderColor  = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    GW_VKCHECK(vkCreateSampler(vk_->device, &sci, nullptr, &vk_->scene_sampler));

    // ImGui descriptor set — persistent until RemoveTexture.
    vk_->scene_imgui_ds = ImGui_ImplVulkan_AddTexture(
        vk_->scene_sampler, vk_->scene_view,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vk_->scene_extent = {w, h};

    vk_->scene_depth_image       = VK_NULL_HANDLE;
    vk_->scene_depth_alloc       = VK_NULL_HANDLE;
    vk_->scene_depth_view        = VK_NULL_HANDLE;
    vk_->scene_depth_format      = VK_FORMAT_UNDEFINED;
    vk_->scene_depth_had_frame   = false;
    vk_->scene_image_had_frame   = false;

    static constexpr VkFormat kDepthCandidates[] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
    };
    for (VkFormat dfmt : kDepthCandidates) {
        VkFormatProperties fp{};
        vkGetPhysicalDeviceFormatProperties(vk_->physical_device, dfmt, &fp);
        if (fp.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            VkImageCreateInfo dci{};
            dci.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            dci.imageType     = VK_IMAGE_TYPE_2D;
            dci.format        = dfmt;
            dci.extent        = {w, h, 1};
            dci.mipLevels     = 1;
            dci.arrayLayers   = 1;
            dci.samples       = VK_SAMPLE_COUNT_1_BIT;
            dci.tiling        = VK_IMAGE_TILING_OPTIMAL;
            dci.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                                VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            dci.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
            dci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            if (vmaCreateImage(vk_->allocator, &dci, &aci,
                               &vk_->scene_depth_image, &vk_->scene_depth_alloc, nullptr) != VK_SUCCESS) {
                vk_->scene_depth_image = VK_NULL_HANDLE;
                vk_->scene_depth_alloc = VK_NULL_HANDLE;
                continue;
            }

            // Depth-only subresource view is sufficient for the depth attachment.
            const VkImageAspectFlags aspect = VK_IMAGE_ASPECT_DEPTH_BIT;

            VkImageViewCreateInfo dvi{};
            dvi.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            dvi.image                           = vk_->scene_depth_image;
            dvi.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
            dvi.format                          = dfmt;
            dvi.subresourceRange.aspectMask     = aspect;
            dvi.subresourceRange.levelCount     = 1;
            dvi.subresourceRange.layerCount     = 1;
            if (vkCreateImageView(vk_->device, &dvi, nullptr, &vk_->scene_depth_view) != VK_SUCCESS) {
                vmaDestroyImage(vk_->allocator, vk_->scene_depth_image, vk_->scene_depth_alloc);
                vk_->scene_depth_image = VK_NULL_HANDLE;
                vk_->scene_depth_alloc = VK_NULL_HANDLE;
                vk_->scene_depth_view  = VK_NULL_HANDLE;
                continue;
            }
            vk_->scene_depth_format = dfmt;
            break;
        }
    }

    // G-buffer strip thumbnails (same format/size; mirrored from scene via copy).
    for (std::size_t ti = 0; ti < vk_->gbuffer_thumbs.size(); ++ti) {
        auto& t = vk_->gbuffer_thumbs[ti];
        VkImageCreateInfo tci{};
        tci.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        tci.imageType     = VK_IMAGE_TYPE_2D;
        tci.format        = vk_->scene_format;
        tci.extent        = {w, h, 1};
        tci.mipLevels     = 1;
        tci.arrayLayers   = 1;
        tci.samples       = VK_SAMPLE_COUNT_1_BIT;
        tci.tiling        = VK_IMAGE_TILING_OPTIMAL;
        tci.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        tci.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
        tci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        GW_VKCHECK(vmaCreateImage(vk_->allocator, &tci, &aci, &t.image, &t.alloc, nullptr));

        VkImageViewCreateInfo tvi{};
        tvi.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        tvi.image                           = t.image;
        tvi.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        tvi.format                          = vk_->scene_format;
        tvi.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        tvi.subresourceRange.levelCount     = 1;
        tvi.subresourceRange.layerCount     = 1;
        GW_VKCHECK(vkCreateImageView(vk_->device, &tvi, nullptr, &t.view));

        t.imgui_ds = ImGui_ImplVulkan_AddTexture(
            vk_->scene_sampler, t.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    if (!editor_scene_pass_) {
        editor_scene_pass_ = std::make_unique<render::EditorScenePass>();
    }
    (void)editor_scene_pass_->init_or_recreate(
        vk_->device, vk_->allocator, vk_->scene_format,
        (vk_->scene_depth_view != VK_NULL_HANDLE) ? vk_->scene_depth_format : VK_FORMAT_UNDEFINED);

    sync_render_targets_thumbnails();
}

void EditorApplication::sync_render_targets_thumbnails() {
    if (!vk_ || !vk_->scene_imgui_ds) {
        return;
    }
    auto* rtp = dynamic_cast<RenderTargetsPanel*>(panels_.find("Render Targets"));
    if (!rtp) {
        return;
    }
    rtp->set_slot(0, reinterpret_cast<ImTextureID>(vk_->scene_imgui_ds), true);
    for (std::size_t ti = 0; ti < vk_->gbuffer_thumbs.size(); ++ti) {
        if (vk_->gbuffer_thumbs[ti].imgui_ds) {
            rtp->set_slot(static_cast<int>(ti + 1),
                reinterpret_cast<ImTextureID>(vk_->gbuffer_thumbs[ti].imgui_ds), true);
        }
    }
}

void EditorApplication::destroy_scene_rt() {
    if (!vk_ || !vk_->device) return;

    editor_scene_pass_.reset();

    clear_editor_luminance_readout(render_settings_);

    vk_->hist_sample_w.fill(0);
    vk_->hist_sample_h.fill(0);
    vk_->hist_staging_used.fill(false);
    vk_->gbuffer_thumb_seen.fill(false);

    for (std::size_t ti = 0; ti < vk_->gbuffer_thumbs.size(); ++ti) {
        auto& t = vk_->gbuffer_thumbs[ti];
        if (t.imgui_ds) {
            ImGui_ImplVulkan_RemoveTexture(t.imgui_ds);
            t.imgui_ds = VK_NULL_HANDLE;
        }
        if (t.view) {
            vkDestroyImageView(vk_->device, t.view, nullptr);
            t.view = VK_NULL_HANDLE;
        }
        if (t.image && t.alloc) {
            vmaDestroyImage(vk_->allocator, t.image, t.alloc);
            t.image = VK_NULL_HANDLE;
            t.alloc = VK_NULL_HANDLE;
        }
    }

    if (vk_->scene_imgui_ds) {
        ImGui_ImplVulkan_RemoveTexture(vk_->scene_imgui_ds);
        vk_->scene_imgui_ds = VK_NULL_HANDLE;
    }
    if (vk_->scene_sampler) {
        vkDestroySampler(vk_->device, vk_->scene_sampler, nullptr);
        vk_->scene_sampler = VK_NULL_HANDLE;
    }
    if (vk_->scene_view) {
        vkDestroyImageView(vk_->device, vk_->scene_view, nullptr);
        vk_->scene_view = VK_NULL_HANDLE;
    }
    if (vk_->scene_image && vk_->scene_alloc) {
        vmaDestroyImage(vk_->allocator, vk_->scene_image, vk_->scene_alloc);
        vk_->scene_image = VK_NULL_HANDLE;
        vk_->scene_alloc = VK_NULL_HANDLE;
    }

    if (vk_->scene_depth_view) {
        vkDestroyImageView(vk_->device, vk_->scene_depth_view, nullptr);
        vk_->scene_depth_view = VK_NULL_HANDLE;
    }
    if (vk_->scene_depth_image && vk_->scene_depth_alloc) {
        vmaDestroyImage(vk_->allocator, vk_->scene_depth_image, vk_->scene_depth_alloc);
        vk_->scene_depth_image = VK_NULL_HANDLE;
        vk_->scene_depth_alloc = VK_NULL_HANDLE;
    }
    vk_->scene_depth_format    = VK_FORMAT_UNDEFINED;
    vk_->scene_depth_had_frame = false;
    vk_->scene_image_had_frame = false;

    vk_->scene_extent = {};
}

void EditorApplication::recreate_swapchain() {
    // Wait until window is non-zero (minimised).
    int w = 0, h = 0;
    glfwGetFramebufferSize(window_, &w, &h);
    while (w == 0 || h == 0) {
        glfwWaitEvents();
        glfwGetFramebufferSize(window_, &w, &h);
    }

    if (vk_->device)
        vkDeviceWaitIdle(vk_->device);

    // Destroy old views first, keep old swapchain to pass as oldSwapchain.
    for (VkImageView v : vk_->swapchain_views) {
        vkDestroyImageView(vk_->device, v, nullptr);
    }
    vk_->swapchain_views.clear();
    VkSwapchainKHR old_swapchain = vk_->swapchain;

    VkSurfaceCapabilitiesKHR caps{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_->physical_device, vk_->surface, &caps);

    uint32_t fmt_n = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(vk_->physical_device, vk_->surface, &fmt_n, nullptr);
    std::vector<VkSurfaceFormatKHR> fmts(fmt_n);
    vkGetPhysicalDeviceSurfaceFormatsKHR(vk_->physical_device, vk_->surface, &fmt_n, fmts.data());
    VkSurfaceFormatKHR chosen = fmts[0];
    for (const auto& f : fmts) {
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM &&
            f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            chosen = f;
            break;
        }
    }

    VkExtent2D extent = caps.currentExtent;
    if (extent.width == UINT32_MAX) {
        extent.width  = static_cast<uint32_t>(w);
        extent.height = static_cast<uint32_t>(h);
    }
    extent.width  = std::clamp(extent.width,  caps.minImageExtent.width,  caps.maxImageExtent.width);
    extent.height = std::clamp(extent.height, caps.minImageExtent.height, caps.maxImageExtent.height);

    uint32_t image_count = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && image_count > caps.maxImageCount)
        image_count = caps.maxImageCount;

    VkSwapchainCreateInfoKHR sci{};
    sci.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sci.surface          = vk_->surface;
    sci.minImageCount    = image_count;
    sci.imageFormat      = chosen.format;
    sci.imageColorSpace  = chosen.colorSpace;
    sci.imageExtent      = extent;
    sci.imageArrayLayers = 1;
    sci.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                           VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    uint32_t fams[2] = {vk_->graphics_family, vk_->present_family};
    if (vk_->graphics_family != vk_->present_family) {
        sci.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        sci.queueFamilyIndexCount = 2;
        sci.pQueueFamilyIndices   = fams;
    } else {
        sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    sci.preTransform   = caps.currentTransform;
    sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sci.presentMode    = VK_PRESENT_MODE_FIFO_KHR;
    sci.clipped        = VK_TRUE;
    sci.oldSwapchain   = old_swapchain;

    GW_VKCHECK(vkCreateSwapchainKHR(vk_->device, &sci, nullptr, &vk_->swapchain));

    if (old_swapchain != VK_NULL_HANDLE)
        vkDestroySwapchainKHR(vk_->device, old_swapchain, nullptr);

    vk_->swapchain_format    = chosen.format;
    vk_->swapchain_cs        = chosen.colorSpace;
    vk_->swapchain_extent    = extent;
    // imgui_impl_vulkan asserts MinImageCount >= 2. caps.minImageCount can be 1
    // on some drivers (or on e.g. MAILBOX), so clamp to at least 2. We also
    // guarantee the actual swapchain has >= 2 images via image_count above.
    vk_->swapchain_min_count = std::max(caps.minImageCount, 2u);

    // Keep ImGui's stored format pointer in sync — ImGui_ImplVulkan_Init
    // captures &vk_->imgui_color_format, so overwriting the value here is
    // enough for pipeline recreation to see the new format.
    vk_->imgui_color_format  = chosen.format;

    uint32_t img_n = 0;
    vkGetSwapchainImagesKHR(vk_->device, vk_->swapchain, &img_n, nullptr);
    vk_->swapchain_images.resize(img_n);
    vkGetSwapchainImagesKHR(vk_->device, vk_->swapchain, &img_n, vk_->swapchain_images.data());

    vk_->swapchain_views.resize(img_n);
    for (uint32_t i = 0; i < img_n; ++i) {
        VkImageViewCreateInfo ivi{};
        ivi.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ivi.image                           = vk_->swapchain_images[i];
        ivi.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        ivi.format                          = vk_->swapchain_format;
        ivi.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        ivi.subresourceRange.levelCount     = 1;
        ivi.subresourceRange.layerCount     = 1;
        GW_VKCHECK(vkCreateImageView(vk_->device, &ivi, nullptr, &vk_->swapchain_views[i]));
    }

    // Re-create render-finished semaphores if the image count changed.
    for (VkSemaphore s : vk_->render_finished)
        vkDestroySemaphore(vk_->device, s, nullptr);
    vk_->render_finished.clear();
    vk_->render_finished.resize(img_n);
    VkSemaphoreCreateInfo sem_ci{};
    sem_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for (uint32_t i = 0; i < img_n; ++i) {
        GW_VKCHECK(vkCreateSemaphore(vk_->device, &sem_ci, nullptr, &vk_->render_finished[i]));
    }

    if (vk_->imgui_initialised)
        ImGui_ImplVulkan_SetMinImageCount(vk_->swapchain_min_count);
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

    gw::editor::theme::load_editor_fonts(
        io, gw::editor::theme::ThemeRegistry::instance().active().typography,
        a11y_config_.force_mono_font);

    // Disable auto-save — we manage ini explicitly.
    io.IniFilename = nullptr;

    // Load saved layout when schema matches and required docking/windows exist.
    namespace fs = std::filesystem;
    const fs::path ini_path   = gw::editor::shell::layout_ini_path();
    const int      disk_schema = gw::editor::shell::read_layout_schema_version();
    if (disk_schema != gw::editor::shell::kLayoutSchemaVersion) {
        std::error_code ec;
        fs::remove(ini_path, ec);
    }
    if (disk_schema == gw::editor::shell::kLayoutSchemaVersion && fs::exists(ini_path)) {
        std::ifstream f{ini_path, std::ios::binary | std::ios::ate};
        if (f.good()) {
            const auto sz = f.tellg();
            f.seekg(0);
            std::string buf(static_cast<size_t>(sz), '\0');
            f.read(buf.data(), sz);
            if (gw::editor::shell::layout_ini_has_critical_windows(buf)) {
                ImGui::LoadIniSettingsFromMemory(buf.c_str(), buf.size());
                layout_built_ = true;
            }
        }
    }

    // --- Platform backend --------------------------------------------------
    ImGui_ImplGlfw_InitForVulkan(window_, true);

    // --- Vulkan renderer backend ------------------------------------------
    // imgui_impl_vulkan (v1.92+) uses the new RendererHasTextures API. It owns
    // the font atlas texture and uploads it lazily in ImGui_ImplVulkan_NewFrame;
    // no manual CreateFontsTexture call is needed.
    //
    // As of 2025/09/26 the InitInfo struct routes MSAA + dynamic-rendering
    // format description through PipelineInfoMain (and mirrors for secondary
    // viewports). The color-format storage must outlive Init() because the
    // backend dereferences pColorAttachmentFormats during pipeline creation —
    // we keep it as a member on vk_.
    vk_->imgui_color_format = vk_->swapchain_format;

    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.ApiVersion            = VK_API_VERSION_1_3;
    init_info.Instance              = vk_->instance;
    init_info.PhysicalDevice        = vk_->physical_device;
    init_info.Device                = vk_->device;
    init_info.QueueFamily           = vk_->graphics_family;
    init_info.Queue                 = vk_->graphics_queue;
    init_info.DescriptorPool        = vk_->imgui_descriptor_pool;
    init_info.MinImageCount         = vk_->swapchain_min_count;
    init_info.ImageCount            = static_cast<uint32_t>(vk_->swapchain_images.size());
    init_info.UseDynamicRendering   = true;

    auto fill_pipeline_info = [&](ImGui_ImplVulkan_PipelineInfo& p) {
        p.RenderPass                                    = VK_NULL_HANDLE;
        p.Subpass                                       = 0;
        p.MSAASamples                                   = VK_SAMPLE_COUNT_1_BIT;
        p.PipelineRenderingCreateInfo                   = VkPipelineRenderingCreateInfoKHR{};
        p.PipelineRenderingCreateInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
        p.PipelineRenderingCreateInfo.colorAttachmentCount    = 1;
        p.PipelineRenderingCreateInfo.pColorAttachmentFormats = &vk_->imgui_color_format;
    };
    fill_pipeline_info(init_info.PipelineInfoMain);
    fill_pipeline_info(init_info.PipelineInfoForViewports);
    init_info.PipelineInfoForViewports.SwapChainImageUsage =
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (!ImGui_ImplVulkan_Init(&init_info))
        gw_fatal("ImGui_ImplVulkan_Init failed");

    vk_->imgui_initialised = true;

    imgui_tex_cache_ = std::make_unique<gw::editor::render::ImGuiTextureCache>(
        vk_->device, vk_->physical_device, vk_->graphics_queue, vk_->graphics_family,
        vk_->cmd_pool, vk_->allocator);
}

// ---------------------------------------------------------------------------
// Shutdown
// ---------------------------------------------------------------------------
void EditorApplication::shutdown_imgui() {
    if (!vk_ || !vk_->device) return;

    vkDeviceWaitIdle(vk_->device);

    imgui_tex_cache_.reset();

    // Save layout before tearing down.
    namespace fs = std::filesystem;
    const fs::path ini_path = gw::editor::shell::layout_ini_path();
    std::error_code ec;
    fs::create_directories(ini_path.parent_path(), ec);
    std::ofstream f{ini_path, std::ios::binary};
    if (f.good()) {
        size_t sz = 0;
        const char* data = ImGui::SaveIniSettingsToMemory(&sz);
        f.write(data, static_cast<std::streamsize>(sz));
    }
    gw::editor::shell::write_layout_schema_version(gw::editor::shell::kLayoutSchemaVersion);

    // Scene RT bindings reference the ImGui Vulkan backend; drop them first.
    destroy_scene_rt();

    if (vk_->imgui_initialised) {
        ImGui_ImplVulkan_Shutdown();
        vk_->imgui_initialised = false;
    }
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void EditorApplication::shutdown_vulkan() {
    if (!vk_) return;

    if (vk_->device) {
        vkDeviceWaitIdle(vk_->device);

        editor_asset_db_.reset();
        if (vk_->hal_for_assets) {
            vk_->hal_for_assets.reset();
        }

        if (vk_->imgui_descriptor_pool) {
            vkDestroyDescriptorPool(vk_->device, vk_->imgui_descriptor_pool, nullptr);
            vk_->imgui_descriptor_pool = VK_NULL_HANDLE;
        }

        for (VkSemaphore s : vk_->render_finished)
            if (s) vkDestroySemaphore(vk_->device, s, nullptr);
        vk_->render_finished.clear();

        for (uint32_t i = 0; i < kFramesInFlight; ++i) {
            if (vk_->image_available[i]) vkDestroySemaphore(vk_->device, vk_->image_available[i], nullptr);
            if (vk_->in_flight[i])       vkDestroyFence    (vk_->device, vk_->in_flight[i],       nullptr);
        }

        if (vk_->cmd_pool) {
            vkDestroyCommandPool(vk_->device, vk_->cmd_pool, nullptr);
            vk_->cmd_pool = VK_NULL_HANDLE;
        }

        if (vk_->timestamp_query_pool) {
            vkDestroyQueryPool(vk_->device, vk_->timestamp_query_pool, nullptr);
            vk_->timestamp_query_pool = VK_NULL_HANDLE;
        }

        for (VkImageView v : vk_->swapchain_views)
            if (v) vkDestroyImageView(vk_->device, v, nullptr);
        vk_->swapchain_views.clear();

        if (vk_->swapchain) {
            vkDestroySwapchainKHR(vk_->device, vk_->swapchain, nullptr);
            vk_->swapchain = VK_NULL_HANDLE;
        }

        if (vk_->allocator) {
            for (uint32_t i = 0; i < kFramesInFlight; ++i) {
                if (vk_->hist_staging_buf[i]) {
                    vmaDestroyBuffer(vk_->allocator, vk_->hist_staging_buf[i],
                                     vk_->hist_staging_alloc[i]);
                    vk_->hist_staging_buf[i]   = VK_NULL_HANDLE;
                    vk_->hist_staging_alloc[i] = VK_NULL_HANDLE;
                }
            }
            vmaDestroyAllocator(vk_->allocator);
            vk_->allocator = VK_NULL_HANDLE;
        }

        vkDestroyDevice(vk_->device, nullptr);
        vk_->device = VK_NULL_HANDLE;
    }

    if (vk_->instance) {
        if (vk_->surface) {
            vkDestroySurfaceKHR(vk_->instance, vk_->surface, nullptr);
            vk_->surface = VK_NULL_HANDLE;
        }
#if GW_VK_VALIDATION
        if (vk_->messenger) {
            vkDestroyDebugUtilsMessengerEXT(vk_->instance, vk_->messenger, nullptr);
            vk_->messenger = VK_NULL_HANDLE;
        }
#endif
        vkDestroyInstance(vk_->instance, nullptr);
        vk_->instance = VK_NULL_HANDLE;
    }
}

void EditorApplication::shutdown_window() {
    if (window_) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
    glfwTerminate();
}

// ---------------------------------------------------------------------------
// Per-frame
// ---------------------------------------------------------------------------
bool EditorApplication::begin_frame() {
    // Wait for this frame-in-flight's fence.
    vkWaitForFences(vk_->device, 1, &vk_->in_flight[vk_->frame_index], VK_TRUE, UINT64_MAX);
    refresh_editor_histogram_from_staging(vk_.get(), vk_->frame_index, render_settings_);

    const uint32_t fi = vk_->frame_index;
    if (vk_->gpu_timestamps_enabled && vk_->timestamp_query_pool &&
        vk_->gpu_timestamp_warmup_frames <= 0) {
        std::uint64_t ts[2]{};
        const VkResult qr = vkGetQueryPoolResults(
            vk_->device, vk_->timestamp_query_pool, fi * 2, 2, sizeof(ts), ts,
            sizeof(std::uint64_t),
            VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
        if (qr == VK_SUCCESS && ts[1] > ts[0]) {
            const double delta_ns = static_cast<double>(ts[1] - ts[0]) *
                static_cast<double>(vk_->gpu_timestamp_period_ns);
            vk_->last_gpu_time_ms = static_cast<float>(delta_ns * 1e-6);
        }
    } else if (vk_->gpu_timestamp_warmup_frames > 0) {
        --vk_->gpu_timestamp_warmup_frames;
    }

    // Acquire an image.
    uint32_t image_index = 0;
    VkResult acq = vkAcquireNextImageKHR(
        vk_->device, vk_->swapchain, UINT64_MAX,
        vk_->image_available[vk_->frame_index], VK_NULL_HANDLE, &image_index);
    if (acq == VK_ERROR_OUT_OF_DATE_KHR) {
        recreate_swapchain();
        return false;
    }
    if (acq != VK_SUCCESS && acq != VK_SUBOPTIMAL_KHR) {
        gw_fatal("vkAcquireNextImageKHR failed");
    }

    vkResetFences(vk_->device, 1, &vk_->in_flight[vk_->frame_index]);
    vk_->acquired_image = image_index;

    {
        ImGuiIO& io = ImGui::GetIO();
        if (viewports_enabled_)
            io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        else
            io.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;
    }

    // ImGui new frame. The Vulkan renderer backend's NewFrame uploads the
    // font atlas on first call (ImGui 1.92 RendererHasTextures API).
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();

    // Open the primary command buffer. Rendering is recorded in end_frame
    // once ImGui::Render() has produced its draw data.
    VkCommandBuffer cmd = vk_->cmd_buffers[vk_->frame_index];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo bi{};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    GW_VKCHECK(vkBeginCommandBuffer(cmd, &bi));

    if (vk_->gpu_timestamps_enabled && vk_->timestamp_query_pool) {
        vkCmdResetQueryPool(cmd, vk_->timestamp_query_pool, fi * 2, 2);
    }

    // --- Offscreen scene render (Phase 7 §6.1) + cockpit G-buffer mirrors ----
    // Colour clear → optional mesh pass (future) → transfer to thumbnail RTs
    // and a small RGBA tile for the luminance histogram (GPU readback).
    if (vk_->scene_image) {
        std::array<VkImageMemoryBarrier2, 2> pre_scene{};
        std::uint32_t                      n_pre_scene = 0;

        VkImageMemoryBarrier2& to_color = pre_scene[n_pre_scene++];
        to_color.sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        to_color.dstStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        to_color.dstAccessMask  = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        to_color.newLayout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        to_color.image         = vk_->scene_image;
        to_color.subresourceRange.aspectMask   = VK_IMAGE_ASPECT_COLOR_BIT;
        to_color.subresourceRange.levelCount  = 1;
        to_color.subresourceRange.layerCount  = 1;
        if (vk_->scene_image_had_frame) {
            to_color.srcStageMask  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
            to_color.srcAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
            to_color.oldLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        } else {
            to_color.srcStageMask  = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            to_color.srcAccessMask = 0;
            to_color.oldLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
        }

        VkRenderingAttachmentInfo depth_att{};
        if (vk_->scene_depth_view) {
            VkImageMemoryBarrier2& to_depth = pre_scene[n_pre_scene++];
            to_depth.sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            to_depth.dstStageMask  = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
            to_depth.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                     VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            to_depth.newLayout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            to_depth.image         = vk_->scene_depth_image;
            to_depth.subresourceRange.aspectMask   = VK_IMAGE_ASPECT_DEPTH_BIT;
            to_depth.subresourceRange.levelCount   = 1;
            to_depth.subresourceRange.layerCount   = 1;
            if (vk_->scene_depth_had_frame) {
                to_depth.srcStageMask  = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
                to_depth.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                to_depth.oldLayout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            } else {
                to_depth.srcStageMask  = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
                to_depth.srcAccessMask = 0;
                to_depth.oldLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
            }

            depth_att.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            depth_att.imageView   = vk_->scene_depth_view;
            depth_att.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depth_att.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depth_att.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
            depth_att.clearValue.depthStencil     = {1.f, 0u};
        }

        VkDependencyInfo dep{};
        dep.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dep.imageMemoryBarrierCount = n_pre_scene;
        dep.pImageMemoryBarriers    = pre_scene.data();
        vkCmdPipelineBarrier2(cmd, &dep);

        VkRenderingAttachmentInfo scene_att{};
        scene_att.sType            = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        scene_att.imageView        = vk_->scene_view;
        scene_att.imageLayout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        scene_att.loadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR;
        scene_att.storeOp          = VK_ATTACHMENT_STORE_OP_STORE;
        scene_att.clearValue.color = {
            {scene_clear_[0], scene_clear_[1], scene_clear_[2], scene_clear_[3]}};

        VkRenderingInfo rinfo{};
        rinfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
        rinfo.renderArea.extent    = vk_->scene_extent;
        rinfo.layerCount           = 1;
        rinfo.colorAttachmentCount = 1;
        rinfo.pColorAttachments    = &scene_att;
        rinfo.pDepthAttachment       = vk_->scene_depth_view ? &depth_att : nullptr;
        vkCmdBeginRendering(cmd, &rinfo);
        if (editor_scene_pass_ && vk_->scene_depth_view) {
            gw::assets::AssetDatabase* const adb =
                (shell_phase_ == EditorShellPhase::MainEditor) ? editor_asset_db_.get() : nullptr;
            editor_scene_pass_->record(render::EditorScenePass::RecordFrame{
                .cmd                   = cmd,
                .device                = vk_->device,
                .allocator             = vk_->allocator,
                .extent_w              = vk_->scene_extent.width,
                .extent_h              = vk_->scene_extent.height,
                .view                  = scene_raster_view_,
                .proj                  = scene_raster_proj_,
                .world                 = &scene_world_,
                .frame_index           = fi,
                .max_frames_in_flight  = kFramesInFlight,
                .asset_db               = adb,
            });
        }
        vkCmdEndRendering(cmd);

        if (vk_->scene_depth_view) {
            vk_->scene_depth_had_frame = true;
        }

        const uint32_t sw = std::max(1u, vk_->scene_extent.width);
        const uint32_t sh = std::max(1u, vk_->scene_extent.height);
        const uint32_t hist_w = std::min(kEditorHistSampleDim, sw);
        const uint32_t hist_h = std::min(kEditorHistSampleDim, sh);

        std::array<VkImageMemoryBarrier2, 1 + 5> pre_copy{};
        std::size_t n_pre = 0;

        VkImageMemoryBarrier2& scene_to_xfer = pre_copy[n_pre++];
        scene_to_xfer.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        scene_to_xfer.srcStageMask                  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        scene_to_xfer.srcAccessMask                 = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        scene_to_xfer.dstStageMask                  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        scene_to_xfer.dstAccessMask                 = VK_ACCESS_2_TRANSFER_READ_BIT;
        scene_to_xfer.oldLayout                     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        scene_to_xfer.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        scene_to_xfer.image                         = vk_->scene_image;
        scene_to_xfer.subresourceRange.aspectMask   = VK_IMAGE_ASPECT_COLOR_BIT;
        scene_to_xfer.subresourceRange.levelCount   = 1;
        scene_to_xfer.subresourceRange.layerCount   = 1;

        for (std::size_t ti = 0; ti < vk_->gbuffer_thumbs.size(); ++ti) {
            VkImageMemoryBarrier2& tb = pre_copy[n_pre++];
            tb.sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            tb.dstStageMask  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            tb.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            if (vk_->gbuffer_thumb_seen[ti]) {
                tb.srcStageMask  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
                tb.srcAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
                tb.oldLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            } else {
                tb.srcStageMask  = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
                tb.srcAccessMask = 0;
                tb.oldLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
            }
            tb.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            tb.image                           = vk_->gbuffer_thumbs[ti].image;
            tb.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            tb.subresourceRange.levelCount     = 1;
            tb.subresourceRange.layerCount     = 1;
        }

        VkBufferMemoryBarrier2 buf_pre{};
        buf_pre.sType         = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        buf_pre.dstStageMask  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        buf_pre.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        buf_pre.buffer        = vk_->hist_staging_buf[fi];
        buf_pre.offset        = 0;
        buf_pre.size          = VK_WHOLE_SIZE;
        if (vk_->hist_staging_used[fi]) {
            buf_pre.srcStageMask  = VK_PIPELINE_STAGE_2_HOST_BIT;
            buf_pre.srcAccessMask = VK_ACCESS_2_HOST_READ_BIT;
        } else {
            buf_pre.srcStageMask  = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            buf_pre.srcAccessMask = 0;
        }

        VkDependencyInfo dep_pre{};
        dep_pre.sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dep_pre.imageMemoryBarrierCount  = static_cast<uint32_t>(n_pre);
        dep_pre.pImageMemoryBarriers     = pre_copy.data();
        dep_pre.bufferMemoryBarrierCount = 1;
        dep_pre.pBufferMemoryBarriers    = &buf_pre;
        vkCmdPipelineBarrier2(cmd, &dep_pre);

        VkImageCopy full_copy{};
        full_copy.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        full_copy.srcSubresource.mipLevel       = 0;
        full_copy.srcSubresource.baseArrayLayer = 0;
        full_copy.srcSubresource.layerCount     = 1;
        full_copy.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        full_copy.dstSubresource.mipLevel       = 0;
        full_copy.dstSubresource.baseArrayLayer = 0;
        full_copy.dstSubresource.layerCount     = 1;
        full_copy.extent                        = {sw, sh, 1};

        for (std::size_t ti = 0; ti < vk_->gbuffer_thumbs.size(); ++ti) {
            vkCmdCopyImage(cmd, vk_->scene_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           vk_->gbuffer_thumbs[ti].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1, &full_copy);
            vk_->gbuffer_thumb_seen[ti] = true;
        }

        VkBufferImageCopy bic{};
        bic.bufferOffset                    = 0;
        bic.bufferRowLength                 = 0;
        bic.bufferImageHeight               = 0;
        bic.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        bic.imageSubresource.mipLevel       = 0;
        bic.imageSubresource.baseArrayLayer = 0;
        bic.imageSubresource.layerCount     = 1;
        bic.imageOffset                     = {0, 0, 0};
        bic.imageExtent                     = {hist_w, hist_h, 1};
        vkCmdCopyImageToBuffer(cmd, vk_->scene_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               vk_->hist_staging_buf[fi], 1, &bic);
        vk_->hist_staging_used[fi] = true;

        std::array<VkImageMemoryBarrier2, 1 + 5> post_copy{};
        std::size_t n_post = 0;

        VkImageMemoryBarrier2& scene_to_sample = post_copy[n_post++];
        scene_to_sample.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        scene_to_sample.srcStageMask                  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        scene_to_sample.srcAccessMask                 = VK_ACCESS_2_TRANSFER_READ_BIT;
        scene_to_sample.dstStageMask                  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        scene_to_sample.dstAccessMask                 = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
        scene_to_sample.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        scene_to_sample.newLayout                     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        scene_to_sample.image                         = vk_->scene_image;
        scene_to_sample.subresourceRange.aspectMask   = VK_IMAGE_ASPECT_COLOR_BIT;
        scene_to_sample.subresourceRange.levelCount   = 1;
        scene_to_sample.subresourceRange.layerCount   = 1;

        for (std::size_t ti = 0; ti < vk_->gbuffer_thumbs.size(); ++ti) {
            VkImageMemoryBarrier2& tb = post_copy[n_post++];
            tb.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            tb.srcStageMask                    = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            tb.srcAccessMask                   = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            tb.dstStageMask                    = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
            tb.dstAccessMask                   = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
            tb.oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            tb.newLayout                       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            tb.image                           = vk_->gbuffer_thumbs[ti].image;
            tb.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            tb.subresourceRange.levelCount     = 1;
            tb.subresourceRange.layerCount     = 1;
        }

        VkBufferMemoryBarrier2 buf_post{};
        buf_post.sType                 = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        buf_post.srcStageMask          = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        buf_post.srcAccessMask         = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        buf_post.dstStageMask          = VK_PIPELINE_STAGE_2_HOST_BIT;
        buf_post.dstAccessMask         = VK_ACCESS_2_HOST_READ_BIT;
        buf_post.buffer                = vk_->hist_staging_buf[fi];
        buf_post.offset                = 0;
        buf_post.size                  = VK_WHOLE_SIZE;

        VkDependencyInfo dep_post{};
        dep_post.sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dep_post.imageMemoryBarrierCount  = static_cast<uint32_t>(n_post);
        dep_post.pImageMemoryBarriers     = post_copy.data();
        dep_post.bufferMemoryBarrierCount = 1;
        dep_post.pBufferMemoryBarriers    = &buf_post;
        vkCmdPipelineBarrier2(cmd, &dep_post);

        vk_->hist_sample_w[fi] = hist_w;
        vk_->hist_sample_h[fi] = hist_h;
        vk_->scene_image_had_frame = true;

        // Offscreen work is render + transfer + layout barriers — timestamp after
        // ALL_COMMANDS so the query sees the full cockpit RT + histogram path.
        if (vk_->gpu_timestamps_enabled && vk_->timestamp_query_pool) {
            vkCmdWriteTimestamp2(cmd, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                 vk_->timestamp_query_pool, fi * 2);
        }
    } else if (vk_->gpu_timestamps_enabled && vk_->timestamp_query_pool) {
        vkCmdWriteTimestamp2(cmd, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
                             vk_->timestamp_query_pool, fi * 2);
    }

    return true;
}

void EditorApplication::build_launcher_ui() {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::SetNextWindowViewport(vp->ID);

    constexpr ImGuiWindowFlags kLaunchFlags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus;

    ImGui::Begin("##GreywaterLauncher", nullptr, kLaunchFlags);

    if (shell_phase_ == EditorShellPhase::Authenticate) {
        ImGui::TextUnformatted("Greywater Editor");
        ImGui::TextDisabled("Private gate — local credentials only.");
        ImGui::Spacing();
        ImGui::InputText("User", shell_user_,
                         sizeof(shell_user_), ImGuiInputTextFlags_None);
        ImGui::InputText("Passphrase", shell_pass_, sizeof(shell_pass_),
                         ImGuiInputTextFlags_Password);
        if (ImGui::Button("Unlock", ImVec2{160.f, 0.f})) {
            shell_gate_err_[0] = '\0';
            auto bounded_len = [](const char* s, std::size_t cap) -> std::size_t {
                for (std::size_t i = 0; i < cap; ++i)
                    if (s[i] == '\0') return i;
                return cap;
            };
            const std::string_view u{shell_user_,
                                     bounded_len(shell_user_, sizeof(shell_user_))};
            const std::string_view pw{shell_pass_,
                                      bounded_len(shell_pass_, sizeof(shell_pass_))};
            if (gw::editor::shell::try_private_gate(u, pw, shell_gate_err_,
                                                     sizeof(shell_gate_err_))) {
                shell_pass_[0] = '\0';
                shell_phase_   = EditorShellPhase::ChooseProject;
            }
        }
        if (shell_gate_err_[0] != '\0')
            ImGui::TextColored(gw::editor::theme::active_accent_imgui(), "%s",
                               shell_gate_err_);
    } else if (shell_phase_ == EditorShellPhase::ChooseProject) {
        ImGui::TextUnformatted("Project workspace");
        ImGui::TextDisabled(
            "Pick a workspace, browse, drop a folder here, or open a recent project.");
        ImGui::TextDisabled(
            "Opening a project loads the full docking editor: Viewport, Outliner, Inspector, "
            "and authoring panels.");
        ImGui::TextDisabled(
            "Encounter / behaviour trees: Window → Authoring → Encounter Editor.");
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_ChildBg,
                              gw::editor::theme::imgui_vec4(
                                  gw::editor::theme::ThemeRegistry::instance().active()
                                      .palette.surface_2,
                                  0.92f));
        ImGui::BeginChild("##drop_zone", {0.f, 72.f}, true, ImGuiWindowFlags_NoScrollbar);
        ImGui::TextDisabled("Drop a project folder");
        ImGui::TextUnformatted("Release over this panel while Choose Project is open.");
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* pl = ImGui::AcceptDragDropPayload(nullptr)) {
                if (pl->Data && pl->DataSize > 0) {
                    const char* data = static_cast<const char*>(pl->Data);
                    const int len    = pl->DataSize - 1;
                    if (len > 0 && data[len] == '\0') {
                        namespace fs = std::filesystem;
                        std::error_code ec;
                        fs::path p{std::string_view{data, static_cast<std::size_t>(len)}};
                        p = fs::weakly_canonical(p, ec);
                        if (gw::editor::shell::is_likely_project_root(p)) open_project(p);
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::Spacing();

        const auto repo = gw::editor::shell::find_greywater_repo_root();
        if (repo) {
            const auto franchise_games =
                gw::editor::shell::list_sacrilege_franchise_games(*repo);
            if (franchise_games.empty()) {
                ImGui::TextColored(gw::editor::theme::active_warning_imgui(),
                    "No workspaces listed — add entries to franchises/sacrilege/games.manifest.");
            } else {
                ImGui::TextDisabled("Configured workspaces");
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {10.f, 10.f});
                const float card_w =
                    std::clamp(ImGui::GetContentRegionAvail().x * 0.92f, 240.f, 560.f);
                for (std::size_t i = 0; i < franchise_games.size(); ++i) {
                    const auto& g = franchise_games[i];
                    ImGui::PushID(static_cast<int>(i));
                    ImGui::BeginChild("##fcard", {card_w, 86.f}, true,
                                       ImGuiWindowFlags_NoScrollbar);
                    ImGui::TextUnformatted(g.display_name.c_str());
                    ImGui::TextDisabled("%s", g.slug.c_str());
                    {
                        namespace fs = std::filesystem;
                        std::error_code ec;
                        if (!fs::is_regular_file(g.root / "CMakeLists.txt", ec))
                            ImGui::TextDisabled(
                                "No CMakeLists.txt at title root (OK for content-only).");
                    }
                    if (ImGui::Button("Open", {120.f, 0.f})) open_project(g.root);
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Copy path"))
                        ImGui::SetClipboardText(g.root.string().c_str());
                    ImGui::EndChild();
                    ImGui::PopID();
                }
                ImGui::PopStyleVar();
            }
        } else {
            ImGui::TextColored(gw::editor::theme::active_warning_imgui(),
                "Greywater repo root not found from cwd or executable path.");
            ImGui::TextDisabled(
                "Run from the engine tree or use Browse / typed path below.");
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextDisabled("Other");
        if (ImGui::Button("Browse folder…", ImVec2{200.f, 0.f})) {
            const std::filesystem::path* start = nullptr;
            std::filesystem::path        start_storage;
            if (!recent_projects_.empty()) {
                gw::editor::shell::sort_recents_for_display(recent_projects_);
                start_storage = recent_projects_.front().root.parent_path();
                if (!start_storage.empty()) start = &start_storage;
            }
            if (auto picked = gw::editor::shell::pick_folder_from(start))
                open_project(*picked);
        }
        ImGui::Spacing();
        ImGui::TextDisabled("Recent projects");
        if (recent_projects_.empty()) {
            ImGui::TextDisabled("Nothing here yet — open a folder once and it will be remembered.");
        } else {
            gw::editor::shell::sort_recents_for_display(recent_projects_);
            const float rc_w =
                std::clamp(ImGui::GetContentRegionAvail().x * 0.48f, 260.f, 520.f);
            for (std::size_t i = 0; i < recent_projects_.size(); ++i) {
                const auto& r = recent_projects_[i];
                ImGui::PushID(static_cast<int>(i + 9000));
                const bool valid = gw::editor::shell::is_likely_project_root(r.root);
                ImGui::BeginChild("##rcard", {rc_w, 92.f}, true,
                                   ImGuiWindowFlags_NoScrollbar);
                if (r.pinned)
                    ImGui::TextColored(gw::editor::theme::active_accent_secondary_imgui(),
                                       "Pinned");
                else
                    ImGui::TextDisabled("Recent");
                ImGui::TextUnformatted(r.display_name.c_str());
                if (!valid)
                    ImGui::TextColored(gw::editor::theme::active_warning_imgui(),
                                       "Path may be invalid (missing content/assets).");
                ImGui::TextDisabled("%s", r.root.string().c_str());
                if (ImGui::Button("Open", {88.f, 0.f}) && valid) open_project(r.root);
                ImGui::SameLine();
                const char* pin_lbl = r.pinned ? "Unpin" : "Pin";
                if (ImGui::SmallButton(pin_lbl))
                    gw::editor::shell::set_recent_pinned(recent_projects_, r.root, !r.pinned);
                ImGui::SameLine();
                if (ImGui::SmallButton("Remove"))
                    gw::editor::shell::remove_recent_project(recent_projects_, r.root);
                ImGui::SameLine();
                if (ImGui::SmallButton("Copy path"))
                    ImGui::SetClipboardText(r.root.string().c_str());
                ImGui::EndChild();
                ImGui::PopID();
            }
        }
        ImGui::Spacing();
        ImGui::InputText("Path", shell_path_manual_, sizeof(shell_path_manual_));
        if (ImGui::Button("Open typed path", ImVec2{200.f, 0.f})) {
            namespace fs = std::filesystem;
            const fs::path typed{shell_path_manual_};
            std::error_code ec;
            if (fs::exists(typed, ec)) open_project(fs::absolute(typed, ec));
        }
    }

    ImGui::End();
}

void EditorApplication::open_project(const std::filesystem::path& root) {
    namespace fs = std::filesystem;
    project_root_ = root;
    bld_mcp_.reset();
    mcp_next_jsonrpc_id_ = 2;
    std::error_code ec;
    fs::current_path(project_root_, ec);
    gw::editor::shell::touch_recent_project(recent_projects_, project_root_);
    shell_phase_ = EditorShellPhase::MainEditor;

    editor_asset_db_.reset();
    editor_asset_vfs_ = std::make_unique<gw::assets::vfs::VirtualFilesystem>();
    editor_asset_vfs_->mount("assets/", (project_root_ / "assets").string(), 0);
    editor_asset_vfs_->mount("content/", (project_root_ / "content").string(), 0);
    if (vk_ && vk_->hal_for_assets) {
        editor_asset_db_ = std::make_unique<gw::assets::AssetDatabase>(
            *vk_->hal_for_assets, *editor_asset_vfs_);
    } else {
        std::fprintf(stderr,
                     "[editor] AssetDatabase: GPU HAL unavailable — using ModHarness (see "
                     "docs/ENGINE_EDITOR_RUNTIME_WIRING.md). Mesh/texture loads are not ship-quality.\n");
        editor_asset_db_ = std::make_unique<gw::assets::AssetDatabase>(
            gw::assets::AssetDatabaseModHarnessTag{}, *editor_asset_vfs_);
    }

    const fs::path def_scene =
        fs::path{"content"} / "scenes" / "sacrilege_editor_default.gwscene";
    const fs::path def_abs = project_root_ / def_scene;
    if (fs::is_regular_file(def_abs, ec)) {
        gw_editor_load_scene("content/scenes/sacrilege_editor_default.gwscene");
    } else {
        fs::create_directories(def_abs.parent_path(), ec);
        (void)gw::scene::save_scene(def_abs, scene_world_);
        gw::editor::bld_api::g_globals.active_scene_path_utf8 =
            def_scene.lexically_normal().generic_string();
    }
}

void EditorApplication::launch_detached_headless_runtime() {
    namespace fs = std::filesystem;
    std::error_code ec;
    std::string       rel = gw::editor::bld_api::g_globals.active_scene_path_utf8;
    if (rel.empty()) {
        rel = "content/scenes/_editor_play_export.gwscene";
    }
    const fs::path out = project_root_ / fs::path(rel);
    fs::create_directories(out.parent_path(), ec);
    const auto saved = gw::scene::save_scene(out, scene_world_);
    if (!saved) {
        std::fprintf(stderr, "[editor] export scene failed: %.*s\n",
                     static_cast<int>(gw::scene::to_string(saved.error()).size()),
                     gw::scene::to_string(saved.error()).data());
        return;
    }
    gw::editor::bld_api::g_globals.active_scene_path_utf8 = rel;

    const std::string cvars_rel = gw::play::play_cvars_rel_for_scene_path(rel);
    const fs::path    cvars_abs = project_root_ / fs::path(cvars_rel);
    touch_play_cvars_stub_file(cvars_abs);

    const std::string editor_exe = gw::platform::current_executable_path();
    if (editor_exe.empty()) {
        std::fprintf(stderr, "[editor] could not resolve editor executable path\n");
        return;
    }
    const fs::path bin_dir = fs::path(editor_exe).parent_path();
#if defined(_WIN32)
    const fs::path play = bin_dir / "sandbox_playable.exe";
#else
    const fs::path play = bin_dir / "sandbox_playable";
#endif
    if (!fs::is_regular_file(play, ec)) {
        std::fprintf(stderr, "[editor] sandbox_playable not found at %s\n",
                     play.string().c_str());
        return;
    }
    std::vector<std::string> argv;
    argv.emplace_back("--frames=120");
    argv.emplace_back(std::string("--scene=") + rel);
    argv.emplace_back(std::string("--cvars-toml=") + cvars_rel);
    argv.emplace_back(std::string("--seed=") +
                      std::to_string(gw::play::kDefaultPlayUniverseSeed));
    if (!gw::platform::spawn_detached(play.string(), argv)) {
        std::fprintf(stderr, "[editor] spawn_detached failed for %s\n",
                     play.string().c_str());
        return;
    }
    std::fprintf(stdout, "[editor] launched %s (scene %s)\n", play.string().c_str(),
                 rel.c_str());
}

void EditorApplication::poll_bld_copilot_mcp() {
    if (shell_phase_ != EditorShellPhase::MainEditor || project_root_.empty()) {
        return;
    }
    auto* raw = panels_.find("BLD Copilot");
    if (raw == nullptr) {
        return;
    }
    auto* panel = dynamic_cast<gw::editor::agent::AgentPanel*>(raw);
    if (panel == nullptr || !panel->has_pending_mcp_turn()) {
        return;
    }

    std::string user;
    if (!panel->take_pending_user_input(user)) {
        return;
    }

    const char* exe_env = std::getenv("GW_BLD_SERVER_EXE");
    if (exe_env == nullptr || exe_env[0] == '\0') {
        panel->push_assistant_delta(
            "Set environment variable GW_BLD_SERVER_EXE to the path of gw_bld_server.exe "
            "(build: cargo build -p bld-agent --features server --bin gw_bld_server), then send "
            "again.\n");
        panel->set_status(gw::editor::agent::SessionStatus::Disconnected);
        return;
    }

    panel->set_status(gw::editor::agent::SessionStatus::Thinking);
    std::string err;
    if (!bld_mcp_) {
        bld_mcp_ = std::make_unique<gw::editor::bld::McpStdioSession>();
    }
    if (!bld_mcp_->active()) {
        const std::string root = project_root_.string();
        if (!bld_mcp_->start(exe_env, root, err)) {
            panel->push_assistant_delta(std::string("MCP session failed to start: ") + err + "\n");
            bld_mcp_.reset();
            panel->set_status(gw::editor::agent::SessionStatus::Idle);
            return;
        }
        mcp_next_jsonrpc_id_ = 2;
    }

    const unsigned id = mcp_next_jsonrpc_id_++;
    std::ostringstream body;
    body << "{\"jsonrpc\":\"2.0\",\"id\":" << id
         << ",\"method\":\"tools/call\",\"params\":{"
            "\"name\":\"docs.search\","
            "\"arguments\":{"
            "\"query\":\""
         << gw::editor::bld::json_escape_string(user) << "\",\"k\":8}}}";

    std::string resp;
    if (!bld_mcp_->request(body.str(), resp, err)) {
        panel->push_assistant_delta(std::string("MCP request failed: ") + err + "\n");
        bld_mcp_.reset();
        panel->set_status(gw::editor::agent::SessionStatus::Idle);
        return;
    }
    panel->push_assistant_delta(resp);
    panel->push_assistant_delta("\n");
    panel->set_status(gw::editor::agent::SessionStatus::Idle);
}

void EditorApplication::build_ui() {
    if (shell_phase_ != EditorShellPhase::MainEditor) {
        build_launcher_ui();
        return;
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    const ImGuiWindowFlags host_flags =
        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_MenuBar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.f, 0.f});
    ImGui::Begin("##DockHost", nullptr, host_flags);
    ImGui::PopStyleVar(3);

    if (!photosensitivity_warn_ack_ &&
        !ImGui::IsPopupOpen("Photosensitivity##gw_editor")) {
        ImGui::OpenPopup("Photosensitivity##gw_editor");
    }
    if (ImGui::BeginPopupModal("Photosensitivity##gw_editor", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted(
            "This editor and in-engine content may include intense visuals and flashing effects.");
        ImGui::TextUnformatted(
            "If you are photosensitive, use accessibility settings and reduce motion before proceeding.");
        if (ImGui::Button("Acknowledge and continue", ImVec2{240.f, 0.f})) {
            photosensitivity_warn_ack_ = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

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
        // pre-ed-theme-menu: View → Theme submenu swaps the registry live and
        // re-runs apply_theme() so ImGui's style reflects the new palette on
        // the next frame.
        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Multi-viewports (floating windows)", nullptr,
                                 viewports_enabled_)) {
                viewports_enabled_ = !viewports_enabled_;
                apply_theme();
            }
            if (ImGui::MenuItem("Consolidate to main window", nullptr, false,
                                 viewports_enabled_)) {
                viewports_enabled_ = false;
                apply_theme();
            }
            ImGui::Separator();
            if (ImGui::BeginMenu("Theme")) {
                using gw::editor::theme::ThemeId;
                auto& registry = gw::editor::theme::ThemeRegistry::instance();
                const ThemeId active = registry.active_id();
                if (ImGui::MenuItem("Brewed Slate",    nullptr, active == ThemeId::BrewedSlate)) {
                    registry.set_active(ThemeId::BrewedSlate);
                    apply_theme();
                    gw::editor::config::save_theme(ThemeId::BrewedSlate);
                }
                if (ImGui::MenuItem("Corrupted Relic", nullptr, active == ThemeId::CorruptedRelic)) {
                    registry.set_active(ThemeId::CorruptedRelic);
                    apply_theme();
                    gw::editor::config::save_theme(ThemeId::CorruptedRelic);
                }
                if (ImGui::MenuItem("Field Test HC",   nullptr, active == ThemeId::FieldTestHC)) {
                    registry.set_active(ThemeId::FieldTestHC);
                    apply_theme();
                    gw::editor::config::save_theme(ThemeId::FieldTestHC);
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Accessibility...")) {
                a11y_modal_open_ = true;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Play")) {
            const bool in_play    = pie_.in_play();
            const bool is_paused  = pie_.state() == GameplayHost::PIEState::Paused;
            if (ImGui::MenuItem("Play", "F5", false, !in_play)) {
                pie_ctx_.world = &scene_world_;
                pie_ctx_.time  = &pie_time_;
                pie_time_      = {};
                sync_pie_play_bootstrap_with_scene();
                (void)pie_.enter_play(pie_ctx_);
            }
            if (ImGui::MenuItem("Pause", "F6", is_paused, in_play)) {
                if (is_paused) pie_.resume(); else pie_.pause();
            }
            if (ImGui::MenuItem("Stop", "Shift+F5", false, in_play)) {
                pie_.stop(pie_ctx_);
                pie_time_ = {};
                clear_pie_play_bootstrap_paths();
            }
            if (ImGui::MenuItem("Run headless runtime (export scene)…", nullptr, false,
                                 shell_phase_ == EditorShellPhase::MainEditor)) {
                launch_detached_headless_runtime();
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip(
                    "Writes the active scene to disk and spawns `sandbox_playable` from the "
                    "same build directory (CI-style headless tick). Full scene load inside "
                    "the detached runtime launcher is enabled; this closes the export + launch seam.");
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Window")) {
            auto toggle_one = [this](const char* n) {
                if (IPanel* p = panels_.find(n)) {
                    bool v = p->visible();
                    if (ImGui::MenuItem(n, nullptr, v)) p->set_visible(!v);
                }
            };
            auto toggle_group = [&toggle_one](const char* title,
                                              std::initializer_list<const char*> names) {
                if (ImGui::BeginMenu(title)) {
                    for (const char* n : names) toggle_one(n);
                    ImGui::EndMenu();
                }
            };
            toggle_group("Core",
                         {"Viewport", "Outliner", "Inspector", "Lighting", "Render Settings",
                          "Render Targets", "Stats", "Console"});
            if (ImGui::BeginMenu("Authoring")) {
                for (std::string_view sv :
                     gw::editor::panels::sacrilege::kRequiredSacrilegePanelNames) {
                    const std::string n{sv};
                    toggle_one(n.c_str());
                }
                ImGui::EndMenu();
            }
            toggle_group("Scripting & agents", {"VScript", "BLD Copilot", "Sequencer"});
            toggle_group("Audit",
                         {"Asset Dependencies", "Bake", "Heatmap", "Localization", "Profiler",
                          "Shader Hot-Reload", "Shader Permutations"});
            toggle_group("Tools", {"Asset Browser", "Material Forge", "Sacrilege Library",
                                   "Sacrilege Readiness"});
            ImGui::Separator();
            if (ImGui::MenuItem("Bake blockout to .gwmesh…", nullptr, false, false)) {}
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip(
                    "Phase 8 mesh export — merges selected blockout primitives to one .gwmesh "
                    "for cook or handoff.");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Reset docking layout")) {
                layout_pending_reset_ = true;
                gw::editor::shell::write_layout_schema_version(
                    gw::editor::shell::kLayoutSchemaVersion);
                namespace fs = std::filesystem;
                std::error_code ec;
                fs::remove(gw::editor::shell::layout_ini_path(), ec);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            ImGui::MenuItem("About Greywater Editor");
            ImGui::EndMenu();
        }

        if (cmd_stack_.is_dirty()) {
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20.f);
            ImGui::TextColored(gw::editor::theme::active_warning_imgui(), "*");
        }

        ImGui::EndMenuBar();
    }

    constexpr float kStatusH = 24.f;
    const ImVec2  dock_region = ImGui::GetContentRegionAvail();
    const float   dock_h      = std::max(64.f, dock_region.y - kStatusH);
    ImGui::BeginChild("##dock_body", {0.f, dock_h}, false,
                      ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);

    const ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
    if (layout_pending_reset_) {
        ImGui::DockBuilderRemoveNode(dockspace_id);
        build_docking_layout();
        layout_pending_reset_ = false;
        layout_built_         = true;
    } else if (!layout_built_ && ImGui::DockBuilderGetNode(dockspace_id) == nullptr) {
        build_docking_layout();
        layout_built_ = true;
    }
    ImGui::DockSpace(dockspace_id, {0.f, 0.f}, ImGuiDockNodeFlags_None);
    ImGui::EndChild();

    ImGui::PushStyleColor(ImGuiCol_ChildBg,
                          ImGui::GetStyleColorVec4(ImGuiCol_MenuBarBg));
    ImGui::BeginChild("##status_bar", {0.f, kStatusH}, true,
                       ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    {
        const float fps = (last_dt_sec_ > 1e-6f) ? (1.f / last_dt_sec_) : 0.f;
        ImGui::TextUnformatted(project_root_.empty() ? "No project"
                                                     : project_root_.string().c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        ImGui::Text("fps %.0f", static_cast<double>(fps));
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        if (auto* lib = dynamic_cast<gw::editor::panels::sacrilege::SacrilegeLibraryPanel*>(
                panels_.find("Sacrilege Library"))) {
            ImGui::Text("lib %d", lib->catalog_entry_count());
        } else {
            ImGui::TextDisabled("lib —");
        }
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        if (const char* hs = std::getenv("GW_HELL_SEED"); hs && hs[0] != '\0')
            ImGui::TextDisabled("seed %s", hs);
        else
            ImGui::TextDisabled("seed —");
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        ImGui::TextDisabled("%s", cmd_stack_.is_dirty() ? "dirty" : "clean");
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        ImGui::TextDisabled("layout v%d", gw::editor::shell::kLayoutSchemaVersion);
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::End();

    // pre-ed-a11y-init: modal toggle panel for accessibility flags. Changes
    // are applied immediately via gw::editor::a11y::apply() so the effect is
    // visible before the user closes the modal.
    if (a11y_modal_open_) {
        ImGui::OpenPopup("Accessibility");
        a11y_modal_open_ = false;
    }
    if (ImGui::BeginPopupModal("Accessibility", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        bool dirty = false;
        dirty |= ImGui::Checkbox("Reduce corruption (disable glitch/jitter)",
                                 &a11y_config_.reduce_corruption);
        dirty |= ImGui::Checkbox("Disable vignette",
                                 &a11y_config_.disable_vignette);
        dirty |= ImGui::Checkbox("Force high-contrast theme",
                                 &a11y_config_.force_high_contrast);
        dirty |= ImGui::Checkbox("Force mono font",
                                 &a11y_config_.force_mono_font);
        dirty |= ImGui::Checkbox("Keyboard-only navigation",
                                 &a11y_config_.keyboard_only_nav);
        dirty |= ImGui::Checkbox("Colour-blind (Wong) palette overlay",
                                 &a11y_config_.colour_blind_wong);
        dirty |= ImGui::Checkbox("Reduce motion (vignette pulse, glitch, jitter)",
                                 &a11y_config_.reduce_motion);
        if (dirty) {
            gw::editor::a11y::apply(a11y_config_);
            apply_theme();
            reload_imgui_fonts_for_a11y();
            gw::editor::config::save_theme(
                gw::editor::theme::ThemeRegistry::instance().active_id());
        }
        ImGui::Separator();
        if (ImGui::Button("Save & Close")) {
            gw::editor::a11y::save_to_config(a11y_config_);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

void EditorApplication::build_docking_layout() {
    const ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->WorkSize);

    ImGuiID dock_main = dockspace_id;
    ImGuiID dock_bottom =
        ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Down, 0.28f, nullptr, &dock_main);
    ImGuiID dock_right =
        ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right, 0.24f, nullptr, &dock_main);

    ImGui::DockBuilderDockWindow("Viewport", dock_main);
    ImGui::DockBuilderDockWindow("Sacrilege Library", dock_bottom);
    ImGui::DockBuilderDockWindow("Asset Browser", dock_bottom);
    ImGui::DockBuilderDockWindow("Outliner", dock_right);
    ImGui::DockBuilderDockWindow("Inspector", dock_right);

    ImGui::DockBuilderFinish(dockspace_id);
}

void EditorApplication::end_frame() {
    ImGui::Render();

    VkCommandBuffer cmd   = vk_->cmd_buffers[vk_->frame_index];
    const uint32_t  fi    = vk_->frame_index;
    const uint32_t  image = vk_->acquired_image;
    VkImage         swap_image = vk_->swapchain_images[image];
    VkImageView     swap_view  = vk_->swapchain_views[image];

    // UNDEFINED -> COLOR_ATTACHMENT_OPTIMAL.
    {
        VkImageMemoryBarrier2 bar{};
        bar.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        bar.srcStageMask                    = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
        bar.srcAccessMask                   = 0;
        bar.dstStageMask                    = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        bar.dstAccessMask                   = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        bar.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
        bar.newLayout                       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        bar.image                           = swap_image;
        bar.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        bar.subresourceRange.levelCount     = 1;
        bar.subresourceRange.layerCount     = 1;

        VkDependencyInfo dep{};
        dep.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dep.imageMemoryBarrierCount = 1;
        dep.pImageMemoryBarriers    = &bar;
        vkCmdPipelineBarrier2(cmd, &dep);
    }

    // Render ImGui into the swapchain image via dynamic rendering.
    VkRenderingAttachmentInfo color_att{};
    color_att.sType            = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    color_att.imageView        = swap_view;
    color_att.imageLayout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_att.loadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_att.storeOp          = VK_ATTACHMENT_STORE_OP_STORE;
    color_att.clearValue.color = {{swapchain_clear_[0], swapchain_clear_[1],
                                   swapchain_clear_[2], swapchain_clear_[3]}};

    VkRenderingInfo rinfo{};
    rinfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
    rinfo.renderArea.extent    = vk_->swapchain_extent;
    rinfo.layerCount           = 1;
    rinfo.colorAttachmentCount = 1;
    rinfo.pColorAttachments    = &color_att;

    vkCmdBeginRendering(cmd, &rinfo);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
    vkCmdEndRendering(cmd);

    if (vk_->gpu_timestamps_enabled && vk_->timestamp_query_pool) {
        vkCmdWriteTimestamp2(cmd, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                             vk_->timestamp_query_pool, fi * 2 + 1);
    }

    // COLOR_ATTACHMENT_OPTIMAL -> PRESENT_SRC_KHR.
    {
        VkImageMemoryBarrier2 bar{};
        bar.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        bar.srcStageMask                    = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        bar.srcAccessMask                   = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        bar.dstStageMask                    = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
        bar.dstAccessMask                   = 0;
        bar.oldLayout                       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        bar.newLayout                       = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        bar.image                           = swap_image;
        bar.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        bar.subresourceRange.levelCount     = 1;
        bar.subresourceRange.layerCount     = 1;

        VkDependencyInfo dep{};
        dep.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dep.imageMemoryBarrierCount = 1;
        dep.pImageMemoryBarriers    = &bar;
        vkCmdPipelineBarrier2(cmd, &dep);
    }

    GW_VKCHECK(vkEndCommandBuffer(cmd));

    // Submit.
    VkSemaphoreSubmitInfo wait{};
    wait.sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    wait.semaphore = vk_->image_available[vk_->frame_index];
    wait.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSemaphoreSubmitInfo signal{};
    signal.sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signal.semaphore = vk_->render_finished[image];
    signal.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;

    VkCommandBufferSubmitInfo cbsi{};
    cbsi.sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    cbsi.commandBuffer = cmd;

    VkSubmitInfo2 submit{};
    submit.sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submit.waitSemaphoreInfoCount   = 1;
    submit.pWaitSemaphoreInfos      = &wait;
    submit.commandBufferInfoCount   = 1;
    submit.pCommandBufferInfos      = &cbsi;
    submit.signalSemaphoreInfoCount = 1;
    submit.pSignalSemaphoreInfos    = &signal;
    GW_VKCHECK(vkQueueSubmit2(vk_->graphics_queue, 1, &submit, vk_->in_flight[vk_->frame_index]));

    // Present.
    VkPresentInfoKHR pi{};
    pi.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores    = &vk_->render_finished[image];
    pi.swapchainCount     = 1;
    pi.pSwapchains        = &vk_->swapchain;
    pi.pImageIndices      = &image;
    VkResult present_result = vkQueuePresentKHR(vk_->present_queue, &pi);
    if (present_result == VK_ERROR_OUT_OF_DATE_KHR ||
        present_result == VK_SUBOPTIMAL_KHR) {
        swapchain_resize_pending_ = true;
    } else if (present_result != VK_SUCCESS) {
        gw_fatal("vkQueuePresentKHR failed");
    }

    // ImGui multi-viewport: render & present extra OS windows.
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

    vk_->frame_index = (vk_->frame_index + 1) % kFramesInFlight;
}

// ---------------------------------------------------------------------------
// Theme — full palette → ImGuiStyle (ADR-0104).
// ---------------------------------------------------------------------------
void EditorApplication::update_clear_colors_from_theme() {
    using gw::editor::theme::Color32;
    using gw::editor::theme::ThemeRegistry;
    const auto& p = ThemeRegistry::instance().active().palette;
    auto set = [](std::array<float, 4>& dst, const Color32& c) {
        dst = {c.r / 255.f, c.g / 255.f, c.b / 255.f, c.a / 255.f};
    };
    set(swapchain_clear_, p.background);
    swapchain_clear_[3] = 1.f;

    Color32 mid{};
    mid.r = static_cast<std::uint8_t>((int{p.background.r} + int{p.panel.r}) / 2);
    mid.g = static_cast<std::uint8_t>((int{p.background.g} + int{p.panel.g}) / 2);
    mid.b = static_cast<std::uint8_t>((int{p.background.b} + int{p.panel.b}) / 2);
    mid.a = 255;
    set(scene_clear_, mid);
    scene_clear_[3] = 1.f;
}

void EditorApplication::apply_theme() {
    ImGuiStyle& style = ImGui::GetStyle();
    const bool vp = viewports_enabled_;
    const auto& pal =
        gw::editor::theme::ThemeRegistry::instance().active().palette;
    gw::editor::theme::apply_palette_to_imgui_style(pal, style, vp);
    update_clear_colors_from_theme();
}

void EditorApplication::reload_imgui_fonts_for_a11y() noexcept {
    if (!ImGui::GetCurrentContext()) return;
    ImGuiIO& io = ImGui::GetIO();
    gw::editor::theme::load_editor_fonts(
        io, gw::editor::theme::ThemeRegistry::instance().active().typography,
        a11y_config_.force_mono_font);
}

// ---------------------------------------------------------------------------
// GLFW callbacks
// ---------------------------------------------------------------------------
void EditorApplication::on_key_callback(GLFWwindow* w, int key,
                                         int /*scancode*/, int action, int mods) {
    auto* app = static_cast<EditorApplication*>(glfwGetWindowUserPointer(w));
    if (!app) return;

    if (action == GLFW_PRESS && (mods & GLFW_MOD_CONTROL)) {
        if (key == GLFW_KEY_Z) {
            if (mods & GLFW_MOD_SHIFT) app->cmd_stack_.redo();  // Ctrl+Shift+Z
            else                        app->cmd_stack_.undo();
            return;
        }
        if (key == GLFW_KEY_Y) { app->cmd_stack_.redo(); return; }
        if (key == GLFW_KEY_S) {
            if (app->shell_phase_ == EditorShellPhase::MainEditor)
                gw_editor_save_scene("content/untitled.gwscene");
            return;
        }
    }

    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_F5) {
            if (mods & GLFW_MOD_SHIFT) {
                if (app->pie_.in_play()) {
                    app->pie_.stop(app->pie_ctx_);
                    app->pie_time_ = {};
                    app->clear_pie_play_bootstrap_paths();
                }
            } else if (!app->pie_.in_play()) {
                app->pie_ctx_.world = &app->scene_world_;
                app->pie_ctx_.time  = &app->pie_time_;
                app->pie_time_      = {};
                app->sync_pie_play_bootstrap_with_scene();
                (void)app->pie_.enter_play(app->pie_ctx_);
            }
            return;
        }
        if (key == GLFW_KEY_F6 && app->pie_.in_play()) {
            if (app->pie_.state() == GameplayHost::PIEState::Paused)
                app->pie_.resume();
            else
                app->pie_.pause();
            return;
        }
    }

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

void EditorApplication::pie_pause_toggle_static(void* user_data) {
    auto* app = static_cast<EditorApplication*>(user_data);
    if (!app || !app->pie_.in_play()) return;
    if (app->pie_.state() == GameplayHost::PIEState::Paused)
        app->pie_.resume();
    else
        app->pie_.pause();
}

void EditorApplication::on_drop_paths(GLFWwindow* w, int count, const char** paths) {
    auto* app = static_cast<EditorApplication*>(glfwGetWindowUserPointer(w));
    if (!app || count < 1 || !paths || !paths[0]) return;
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::path p = fs::path{paths[0]};
    p          = fs::weakly_canonical(p, ec);
    if (!fs::is_directory(p, ec)) return;
    app->pending_folder_drop_ = std::move(p);
}

void EditorApplication::sync_pie_play_bootstrap_with_scene() {
    if (shell_phase_ != EditorShellPhase::MainEditor) return;
    namespace fs = std::filesystem;
    std::string rel = gw::editor::bld_api::g_globals.active_scene_path_utf8;
    if (rel.empty()) rel = "content/scenes/_editor_play_export.gwscene";
    const std::string cvars_rel = gw::play::play_cvars_rel_for_scene_path(rel);
    const fs::path    cvars_abs = project_root_ / fs::path(cvars_rel);
    touch_play_cvars_stub_file(cvars_abs);
    pie_play_cvars_abs_storage_       = cvars_abs.generic_string();
    pie_ctx_.version                  = 2;
    pie_ctx_.play_universe_seed       = gw::play::kDefaultPlayUniverseSeed;
    pie_ctx_.play_cvars_toml_abs_utf8 = pie_play_cvars_abs_storage_.c_str();

    pie_bootstrap_cvars_.emplace();
    (void)gw::config::register_standard_cvars(*pie_bootstrap_cvars_);
    gw::play::apply_play_bootstrap_to_registry(
        *pie_bootstrap_cvars_,
        std::optional<std::int64_t>{pie_ctx_.play_universe_seed},
        std::string_view{pie_play_cvars_abs_storage_});
}

void EditorApplication::clear_pie_play_bootstrap_paths() {
    pie_play_cvars_abs_storage_.clear();
    pie_ctx_.play_cvars_toml_abs_utf8 = nullptr;
    pie_bootstrap_cvars_.reset();
}

bool editor_bld_pie_enter(void* ctx) {
    return static_cast<EditorApplication*>(ctx)->bld_pie_enter();
}

void editor_bld_pie_stop(void* ctx) {
    static_cast<EditorApplication*>(ctx)->bld_pie_stop();
}

}  // namespace gw::editor
