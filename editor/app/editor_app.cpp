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
#include "editor_app.hpp"
#include "editor/bld_api/editor_bld_api.hpp"

#include "editor/panels/outliner_panel.hpp"
#include "editor/panels/inspector_panel.hpp"
#include "editor/panels/viewport_panel.hpp"
#include "editor/panels/asset_browser_panel.hpp"
#include "editor/panels/console_panel.hpp"
#include "editor/scene/components.hpp"
#include "engine/ecs/hierarchy.hpp"

// volk must precede every Vulkan include in this TU so that imgui_impl_vulkan
// picks up volk's dispatch tables (IMGUI_IMPL_VULKAN_USE_VOLK is set by the
// editor CMakeLists).
#include <volk.h>
#include <vk_mem_alloc.h>

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <ImGuizmo.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "engine/core/log.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

namespace gw::editor {

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

    // ----- Offscreen scene render target (Phase 7 §6.1) -----
    // Sized to the Viewport panel's content area; resized on demand from run().
    // Color-only RGBA8 for now; depth + gbuffers will accrete in Phase 8.
    VkImage                  scene_image       = VK_NULL_HANDLE;
    VmaAllocation            scene_alloc       = VK_NULL_HANDLE;
    VkImageView              scene_view        = VK_NULL_HANDLE;
    VkSampler                scene_sampler     = VK_NULL_HANDLE;
    VkDescriptorSet          scene_imgui_ds    = VK_NULL_HANDLE;
    VkExtent2D               scene_extent      = {};
    VkFormat                 scene_format      = VK_FORMAT_R8G8B8A8_UNORM;
};

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------
EditorApplication::EditorApplication()
    : vk_(std::make_unique<EditorVulkanBackend>()) {
    init_window();
    init_vulkan();
    init_imgui();
    apply_theme();

    // Panels — no raw new; unique_ptr owned by the registry. (Non-negotiable #5)
    auto vp = std::make_unique<ViewportPanel>();
    vp->set_window(window_);
    panels_.add(std::move(vp));
    panels_.add(std::make_unique<OutlinerPanel>());
    panels_.add(std::make_unique<InspectorPanel>());
    panels_.add(std::make_unique<AssetBrowserPanel>());
    panels_.add(std::make_unique<ConsolePanel>());

    // Wire BLD API globals.
    gw::editor::bld_api::g_globals.selection = &selection_;
    gw::editor::bld_api::g_globals.cmd_stack = &cmd_stack_;
    gw::editor::bld_api::g_globals.world     = &scene_world_;

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
        glm::vec3{-1.5f, 0.f, 0.f}, glm::quat{1.f, 0.f, 0.f, 0.f},
        glm::vec3{1.f, 1.f, 1.f}});
    scene_world_.add_component(childA, VisibilityComponent{});

    scene_world_.add_component(childB, NameComponent{"Sphere"});
    scene_world_.add_component(childB, TransformComponent{
        glm::vec3{ 1.5f, 0.f, 0.f}, glm::quat{1.f, 0.f, 0.f, 0.f},
        glm::vec3{1.f, 1.f, 1.f}});
    scene_world_.add_component(childB, VisibilityComponent{});

    (void)scene_world_.reparent(childA, root);
    (void)scene_world_.reparent(childB, root);
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

        const double now = glfwGetTime();
        const float  dt  = static_cast<float>(now - last_time);
        last_time = now;

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
            }
        }

        if (!begin_frame()) {
            continue;  // swapchain out-of-date; retry next iteration
        }

        EditorContext ctx{
            .selection    = selection_,
            .cmd_stack    = cmd_stack_,
            .world        = &scene_world_,
            .asset_db     = nullptr,   // Phase 8
            .delta_time_s = dt,
            .in_pie       = pie_.in_play()
        };

        build_ui();
        panels_.render_all(ctx);

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

    window_ = glfwCreateWindow(win_w_, win_h_,
        "Greywater Editor  |  Cold Coffee Labs", nullptr, nullptr);
    if (!window_) gw_fatal("glfwCreateWindow failed");

    glfwSetWindowUserPointer(window_, this);
    glfwSetKeyCallback(window_, on_key_callback);
    glfwSetScrollCallback(window_, on_scroll_callback);
    glfwSetFramebufferSizeCallback(window_, on_framebuffer_resize);
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
                        VK_IMAGE_USAGE_SAMPLED_BIT;
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
}

void EditorApplication::destroy_scene_rt() {
    if (!vk_ || !vk_->device) return;

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

    // Disable auto-save — we manage ini explicitly.
    io.IniFilename = nullptr;

    // Load saved layout.
    namespace fs = std::filesystem;
    fs::path ini_path;
#ifdef _WIN32
    if (const char* appdata = std::getenv("APPDATA"); appdata)
        ini_path = fs::path{appdata} / "GreywaterEditor" / "layout.ini";
    else
        ini_path = "layout.ini";
#else
    if (const char* home = std::getenv("HOME"); home)
        ini_path = fs::path{home} / ".config" / "greywater_editor" / "layout.ini";
    else
        ini_path = "layout.ini";
#endif

    if (fs::exists(ini_path)) {
        std::ifstream f{ini_path, std::ios::binary | std::ios::ate};
        if (f.good()) {
            const auto sz = f.tellg();
            f.seekg(0);
            std::string buf(static_cast<size_t>(sz), '\0');
            f.read(buf.data(), sz);
            ImGui::LoadIniSettingsFromMemory(buf.c_str(), buf.size());
            layout_built_ = true;
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
}

// ---------------------------------------------------------------------------
// Shutdown
// ---------------------------------------------------------------------------
void EditorApplication::shutdown_imgui() {
    if (!vk_ || !vk_->device) return;

    vkDeviceWaitIdle(vk_->device);

    // Save layout before tearing down.
    namespace fs = std::filesystem;
    fs::path ini_dir;
#ifdef _WIN32
    if (const char* appdata = std::getenv("APPDATA"); appdata)
        ini_dir = fs::path{appdata} / "GreywaterEditor";
    else
        ini_dir = ".";
#else
    if (const char* home = std::getenv("HOME"); home)
        ini_dir = fs::path{home} / ".config" / "greywater_editor";
    else
        ini_dir = ".";
#endif

    std::error_code ec;
    fs::create_directories(ini_dir, ec);
    std::ofstream f{ini_dir / "layout.ini", std::ios::binary};
    if (f.good()) {
        size_t sz = 0;
        const char* data = ImGui::SaveIniSettingsToMemory(&sz);
        f.write(data, static_cast<std::streamsize>(sz));
    }

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

        for (VkImageView v : vk_->swapchain_views)
            if (v) vkDestroyImageView(vk_->device, v, nullptr);
        vk_->swapchain_views.clear();

        if (vk_->swapchain) {
            vkDestroySwapchainKHR(vk_->device, vk_->swapchain, nullptr);
            vk_->swapchain = VK_NULL_HANDLE;
        }

        if (vk_->allocator) {
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

    // --- Offscreen scene render (Phase 7 §6.1) ---------------------------
    // Transition UNDEFINED → COLOR_ATTACHMENT_OPTIMAL, clear to a distinctive
    // editor blue, then transition → SHADER_READ_ONLY_OPTIMAL so the Viewport
    // panel's ImGui::Image samples a ready texture in end_frame. Real scene
    // rendering (meshes, debug-draw) lands in later gates; until then a clear
    // proves the VMA RT + ImGui binding end-to-end.
    if (vk_->scene_image) {
        // -> COLOR_ATTACHMENT_OPTIMAL
        VkImageMemoryBarrier2 to_color{};
        to_color.sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        to_color.srcStageMask                = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        to_color.srcAccessMask               = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
        to_color.dstStageMask                = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        to_color.dstAccessMask               = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        to_color.oldLayout                   = VK_IMAGE_LAYOUT_UNDEFINED;
        to_color.newLayout                   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        to_color.image                       = vk_->scene_image;
        to_color.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        to_color.subresourceRange.levelCount = 1;
        to_color.subresourceRange.layerCount = 1;

        VkDependencyInfo dep{};
        dep.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dep.imageMemoryBarrierCount = 1;
        dep.pImageMemoryBarriers    = &to_color;
        vkCmdPipelineBarrier2(cmd, &dep);

        VkRenderingAttachmentInfo scene_att{};
        scene_att.sType            = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        scene_att.imageView        = vk_->scene_view;
        scene_att.imageLayout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        scene_att.loadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR;
        scene_att.storeOp          = VK_ATTACHMENT_STORE_OP_STORE;
        scene_att.clearValue.color = {{0.10f, 0.13f, 0.18f, 1.0f}};

        VkRenderingInfo rinfo{};
        rinfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
        rinfo.renderArea.extent    = vk_->scene_extent;
        rinfo.layerCount           = 1;
        rinfo.colorAttachmentCount = 1;
        rinfo.pColorAttachments    = &scene_att;
        vkCmdBeginRendering(cmd, &rinfo);
        // (Phase 8 will record mesh draws here.)
        vkCmdEndRendering(cmd);

        // -> SHADER_READ_ONLY_OPTIMAL for ImGui sampling.
        VkImageMemoryBarrier2 to_sampled{};
        to_sampled.sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        to_sampled.srcStageMask                = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        to_sampled.srcAccessMask               = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        to_sampled.dstStageMask                = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        to_sampled.dstAccessMask               = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
        to_sampled.oldLayout                   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        to_sampled.newLayout                   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        to_sampled.image                       = vk_->scene_image;
        to_sampled.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        to_sampled.subresourceRange.levelCount = 1;
        to_sampled.subresourceRange.layerCount = 1;

        dep.imageMemoryBarrierCount = 1;
        dep.pImageMemoryBarriers    = &to_sampled;
        vkCmdPipelineBarrier2(cmd, &dep);
    }

    return true;
}

void EditorApplication::build_ui() {
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

        if (cmd_stack_.is_dirty()) {
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20.f);
            ImGui::TextColored({0.9f, 0.6f, 0.2f, 1.f}, "*");
        }

        ImGui::EndMenuBar();
    }

    const ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspace_id, {0.f, 0.f}, ImGuiDockNodeFlags_None);

    if (!layout_built_ && ImGui::DockBuilderGetNode(dockspace_id) == nullptr) {
        build_docking_layout();
        layout_built_ = true;
    }

    ImGui::End();
}

void EditorApplication::build_docking_layout() {
    const ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->WorkSize);

    ImGuiID left  = 0, centre = dockspace_id, right = 0, bottom = 0;
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

    VkCommandBuffer cmd   = vk_->cmd_buffers[vk_->frame_index];
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
    color_att.clearValue.color = {{0.051f, 0.059f, 0.078f, 1.0f}};  // editor bg

    VkRenderingInfo rinfo{};
    rinfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
    rinfo.renderArea.extent    = vk_->swapchain_extent;
    rinfo.layerCount           = 1;
    rinfo.colorAttachmentCount = 1;
    rinfo.pColorAttachments    = &color_att;

    vkCmdBeginRendering(cmd, &rinfo);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
    vkCmdEndRendering(cmd);

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
// Theme — dark sci-fi / Cold Coffee Labs aesthetic.
// ---------------------------------------------------------------------------
void EditorApplication::apply_theme() {
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding    = 4.f;
    style.ChildRounding     = 3.f;
    style.FrameRounding     = 3.f;
    style.GrabRounding      = 3.f;
    style.PopupRounding     = 4.f;
    style.ScrollbarRounding = 3.f;
    style.TabRounding       = 4.f;

    style.WindowPadding     = {8.f,  8.f};
    style.FramePadding      = {5.f,  3.f};
    style.ItemSpacing       = {6.f,  4.f};
    style.ScrollbarSize     = 12.f;
    style.GrabMinSize       = 8.f;
    style.WindowBorderSize  = 1.f;
    style.FrameBorderSize   = 0.f;
    style.TabBarBorderSize  = 1.f;

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
    col[ImGuiCol_ScrollbarGrabActive]   = C( 76, 201, 165);
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
    col[ImGuiCol_PlotLinesHovered]      = C(245, 166,  35);
    col[ImGuiCol_PlotHistogram]         = C( 76, 201, 165);
    col[ImGuiCol_PlotHistogramHovered]  = C(245, 166,  35);
    col[ImGuiCol_TableHeaderBg]         = C( 20,  26,  38);
    col[ImGuiCol_TableBorderStrong]     = C( 40,  46,  62);
    col[ImGuiCol_TableBorderLight]      = C( 28,  34,  48);
    col[ImGuiCol_TableRowBg]            = C(  0,   0,   0,   0);
    col[ImGuiCol_TableRowBgAlt]         = C(255, 255, 255,   8);
    col[ImGuiCol_TextSelectedBg]        = C( 76, 201, 165,  60);
    col[ImGuiCol_DragDropTarget]        = C(245, 166,  35, 200);
    col[ImGuiCol_NavHighlight]          = C( 76, 201, 165);
    col[ImGuiCol_NavWindowingHighlight] = C(245, 166,  35, 200);
    col[ImGuiCol_NavWindowingDimBg]     = C(  0,   0,   0, 100);
    col[ImGuiCol_ModalWindowDimBg]      = C(  0,   0,   0, 140);

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.f;
        col[ImGuiCol_WindowBg].w = 1.f;
    }
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
        if (key == GLFW_KEY_S) { gw_editor_save_scene("content/untitled.gwscene"); return; }
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

}  // namespace gw::editor
