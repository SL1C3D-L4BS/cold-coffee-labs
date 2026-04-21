// sandbox/main.cpp — Greywater *First Light* Milestone 1A.
//
// Hello-triangle on Vulkan 1.2 baseline + dynamic rendering (1.3 opportunistic,
// gated by driver support). Uses GLFW for the window surface and volk for the
// Vulkan function loader. No HAL yet — Phase 1 predates engine/render/hal/,
// which lands in Phase 4. This binary proves the build pipeline end-to-end:
// Clang + Ninja + CPM deps + Corrosion (via greywater_core's linkage graph)
// + glslangValidator + runtime Vulkan.
//
// docs/05 week 004 · docs/10 §3 Week 02 · docs/00 §2.1–2.2.
// References:
//   Vulkan 1.2 spec — https://registry.khronos.org/vulkan/specs/1.2-extensions/html/
//   GLFW 3.4       — https://www.glfw.org/docs/latest/
//   volk           — https://github.com/zeux/volk
//   VMA 3.1        — https://gpuopen.com/vulkan-memory-allocator/

#include "engine/core/version.hpp"

#include <volk.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace {

// -----------------------------------------------------------------------------
// Policy & constants
// -----------------------------------------------------------------------------

constexpr uint32_t kWindowWidth  = 1280;
constexpr uint32_t kWindowHeight = 720;
constexpr uint32_t kFramesInFlight = 2;
constexpr const char* kWindowTitle = "Greywater Sandbox — First Light";

// -----------------------------------------------------------------------------
// Error handling: Phase 1 uses assert-on-error at the entry point. Engine
// subsystems (Phase 2+) move to gw::core::Result<T, E>. See docs/12 §A2.
// -----------------------------------------------------------------------------

[[noreturn]] void gw_fatal(const char* what) {
    std::fprintf(stderr, "[sandbox] FATAL: %s\n", what);
    std::exit(EXIT_FAILURE);
}

#define GW_CHECK_VK(expr) \
    do { \
        VkResult _r = (expr); \
        if (_r != VK_SUCCESS) { \
            std::fprintf(stderr, "[sandbox] Vulkan call failed (VkResult=%d): %s\n", \
                         static_cast<int>(_r), #expr); \
            std::exit(EXIT_FAILURE); \
        } \
    } while (0)

// -----------------------------------------------------------------------------
// Shader loader — reads a .spv produced at build time by glslangValidator.
// -----------------------------------------------------------------------------

std::vector<char> read_binary_file(const std::string& path) {
    std::ifstream f(path, std::ios::ate | std::ios::binary);
    if (!f) {
        std::fprintf(stderr, "[sandbox] cannot open shader: %s\n", path.c_str());
        std::exit(EXIT_FAILURE);
    }
    const std::streamsize size = f.tellg();
    std::vector<char> buf(static_cast<size_t>(size));
    f.seekg(0);
    f.read(buf.data(), size);
    return buf;
}

VkShaderModule make_shader_module(VkDevice device, const std::vector<char>& spv) {
    VkShaderModuleCreateInfo ci{};
    ci.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ci.codeSize = spv.size();
    ci.pCode    = reinterpret_cast<const uint32_t*>(spv.data());
    VkShaderModule mod = VK_NULL_HANDLE;
    GW_CHECK_VK(vkCreateShaderModule(device, &ci, nullptr, &mod));
    return mod;
}

// -----------------------------------------------------------------------------
// Validation-layer debug callback (docs/10 §1 Risk #3 mitigation).
// -----------------------------------------------------------------------------

VKAPI_ATTR VkBool32 VKAPI_CALL debug_messenger_cb(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT /*type*/,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* /*user*/) {
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::fprintf(stderr, "[vk] %s\n", data->pMessage);
    }
    return VK_FALSE;
}

// -----------------------------------------------------------------------------
// Device / queue selection
// -----------------------------------------------------------------------------

struct QueueFamilyIndices {
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;
    bool complete() const { return graphics && present; }
};

QueueFamilyIndices find_queue_families(VkPhysicalDevice gpu, VkSurfaceKHR surface) {
    QueueFamilyIndices idx{};
    uint32_t n = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &n, nullptr);
    std::vector<VkQueueFamilyProperties> props(n);
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &n, props.data());

    for (uint32_t i = 0; i < n; ++i) {
        if (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            idx.graphics = i;
        }
        VkBool32 present_supported = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, surface, &present_supported);
        if (present_supported) {
            idx.present = i;
        }
        if (idx.complete()) {
            break;
        }
    }
    return idx;
}

VkPhysicalDevice pick_physical_device(VkInstance instance, VkSurfaceKHR surface) {
    uint32_t n = 0;
    vkEnumeratePhysicalDevices(instance, &n, nullptr);
    if (n == 0) {
        gw_fatal("no Vulkan-capable physical devices");
    }
    std::vector<VkPhysicalDevice> gpus(n);
    vkEnumeratePhysicalDevices(instance, &n, gpus.data());

    // Prefer a discrete GPU with a complete queue family set.
    VkPhysicalDevice chosen = VK_NULL_HANDLE;
    for (VkPhysicalDevice gpu : gpus) {
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(gpu, &props);
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
            && find_queue_families(gpu, surface).complete()) {
            chosen = gpu;
            std::fprintf(stdout, "[sandbox] selected GPU: %s (Vulkan %u.%u)\n",
                         props.deviceName,
                         VK_API_VERSION_MAJOR(props.apiVersion),
                         VK_API_VERSION_MINOR(props.apiVersion));
            return chosen;
        }
    }
    // Fallback: first device with the required queue families.
    for (VkPhysicalDevice gpu : gpus) {
        if (find_queue_families(gpu, surface).complete()) {
            return gpu;
        }
    }
    gw_fatal("no Vulkan device exposes a graphics + present queue family");
}

// -----------------------------------------------------------------------------
// Swapchain
// -----------------------------------------------------------------------------

struct Swapchain {
    VkSwapchainKHR       handle       = VK_NULL_HANDLE;
    VkFormat             format       = VK_FORMAT_UNDEFINED;
    VkExtent2D           extent       = {};
    std::vector<VkImage>     images;
    std::vector<VkImageView> views;
};

Swapchain create_swapchain(VkPhysicalDevice gpu, VkDevice device, VkSurfaceKHR surface,
                           uint32_t graphics_q, uint32_t present_q, GLFWwindow* window) {
    VkSurfaceCapabilitiesKHR caps{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &caps);

    uint32_t fmt_n = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &fmt_n, nullptr);
    std::vector<VkSurfaceFormatKHR> fmts(fmt_n);
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &fmt_n, fmts.data());
    VkSurfaceFormatKHR chosen_fmt = fmts[0];
    for (const auto& f : fmts) {
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB
            && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            chosen_fmt = f;
            break;
        }
    }

    VkExtent2D extent = caps.currentExtent;
    if (extent.width == UINT32_MAX) {
        int w = 0, h = 0;
        glfwGetFramebufferSize(window, &w, &h);
        extent.width  = static_cast<uint32_t>(w);
        extent.height = static_cast<uint32_t>(h);
    }

    uint32_t image_count = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && image_count > caps.maxImageCount) {
        image_count = caps.maxImageCount;
    }

    VkSwapchainCreateInfoKHR ci{};
    ci.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    ci.surface          = surface;
    ci.minImageCount    = image_count;
    ci.imageFormat      = chosen_fmt.format;
    ci.imageColorSpace  = chosen_fmt.colorSpace;
    ci.imageExtent      = extent;
    ci.imageArrayLayers = 1;
    ci.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    uint32_t qfs[2] = {graphics_q, present_q};
    if (graphics_q != present_q) {
        ci.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        ci.queueFamilyIndexCount = 2;
        ci.pQueueFamilyIndices   = qfs;
    } else {
        ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    ci.preTransform   = caps.currentTransform;
    ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    ci.presentMode    = VK_PRESENT_MODE_FIFO_KHR;  // v-sync, always supported
    ci.clipped        = VK_TRUE;

    Swapchain sc{};
    sc.format = chosen_fmt.format;
    sc.extent = extent;
    GW_CHECK_VK(vkCreateSwapchainKHR(device, &ci, nullptr, &sc.handle));

    uint32_t img_n = 0;
    vkGetSwapchainImagesKHR(device, sc.handle, &img_n, nullptr);
    sc.images.resize(img_n);
    vkGetSwapchainImagesKHR(device, sc.handle, &img_n, sc.images.data());

    sc.views.resize(img_n);
    for (uint32_t i = 0; i < img_n; ++i) {
        VkImageViewCreateInfo vi{};
        vi.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vi.image                           = sc.images[i];
        vi.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        vi.format                          = sc.format;
        vi.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        vi.subresourceRange.baseMipLevel   = 0;
        vi.subresourceRange.levelCount     = 1;
        vi.subresourceRange.baseArrayLayer = 0;
        vi.subresourceRange.layerCount     = 1;
        GW_CHECK_VK(vkCreateImageView(device, &vi, nullptr, &sc.views[i]));
    }
    return sc;
}

// -----------------------------------------------------------------------------
// Graphics pipeline — dynamic rendering, no render pass.
// -----------------------------------------------------------------------------

VkPipeline make_triangle_pipeline(VkDevice device, VkPipelineLayout layout,
                                  VkFormat color_format,
                                  VkShaderModule vs, VkShaderModule fs) {
    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vs;
    stages[0].pName  = "main";
    stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fs;
    stages[1].pName  = "main";

    VkPipelineVertexInputStateCreateInfo vi{};
    vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo ia{};
    ia.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo vp{};
    vp.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    vp.scissorCount  = 1;

    VkPipelineRasterizationStateCreateInfo rs{};
    rs.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode    = VK_CULL_MODE_NONE;
    rs.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth   = 1.0f;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState cba{};
    cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                       | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo cb{};
    cb.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb.attachmentCount = 1;
    cb.pAttachments    = &cba;

    const VkDynamicState dyn[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dyns{};
    dyns.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyns.dynamicStateCount = 2;
    dyns.pDynamicStates    = dyn;

    VkPipelineRenderingCreateInfo rendering{};
    rendering.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    rendering.colorAttachmentCount    = 1;
    rendering.pColorAttachmentFormats = &color_format;

    VkGraphicsPipelineCreateInfo pci{};
    pci.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pci.pNext               = &rendering;
    pci.stageCount          = 2;
    pci.pStages             = stages;
    pci.pVertexInputState   = &vi;
    pci.pInputAssemblyState = &ia;
    pci.pViewportState      = &vp;
    pci.pRasterizationState = &rs;
    pci.pMultisampleState   = &ms;
    pci.pColorBlendState    = &cb;
    pci.pDynamicState       = &dyns;
    pci.layout              = layout;

    VkPipeline pipeline = VK_NULL_HANDLE;
    GW_CHECK_VK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pci, nullptr, &pipeline));
    return pipeline;
}

// -----------------------------------------------------------------------------
// Image-layout barrier helper (sync2).
// -----------------------------------------------------------------------------

void transition_image(VkCommandBuffer cmd, VkImage image,
                      VkImageLayout old_layout, VkImageLayout new_layout,
                      VkPipelineStageFlags2 src_stage, VkAccessFlags2 src_access,
                      VkPipelineStageFlags2 dst_stage, VkAccessFlags2 dst_access) {
    VkImageMemoryBarrier2 bar{};
    bar.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    bar.srcStageMask                    = src_stage;
    bar.srcAccessMask                   = src_access;
    bar.dstStageMask                    = dst_stage;
    bar.dstAccessMask                   = dst_access;
    bar.oldLayout                       = old_layout;
    bar.newLayout                       = new_layout;
    bar.image                           = image;
    bar.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    bar.subresourceRange.baseMipLevel   = 0;
    bar.subresourceRange.levelCount     = 1;
    bar.subresourceRange.baseArrayLayer = 0;
    bar.subresourceRange.layerCount     = 1;

    VkDependencyInfo dep{};
    dep.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.imageMemoryBarrierCount = 1;
    dep.pImageMemoryBarriers    = &bar;
    vkCmdPipelineBarrier2(cmd, &dep);
}

}  // namespace

// -----------------------------------------------------------------------------
// Entry point
// -----------------------------------------------------------------------------

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    std::setvbuf(stderr, nullptr, _IONBF, 0);
    std::fprintf(stdout, "[sandbox] starting %s\n", gw::core::version_string());

    // --- GLFW ----------------------------------------------------------------
    if (!glfwInit()) {
        gw_fatal("glfwInit failed");
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window =
        glfwCreateWindow(static_cast<int>(kWindowWidth),
                         static_cast<int>(kWindowHeight),
                         kWindowTitle, nullptr, nullptr);
    if (!window) {
        gw_fatal("glfwCreateWindow failed");
    }

    // --- volk + instance -----------------------------------------------------
    GW_CHECK_VK(volkInitialize());

    VkApplicationInfo app{};
    app.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pApplicationName   = "Greywater Sandbox";
    app.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    app.pEngineName        = "Greywater_Engine";
    app.engineVersion      = VK_MAKE_VERSION(0, 0, 1);
    // RX 580 exposes Vulkan 1.3; request 1.3 so volk loads all promoted entry
    // points (vkCmdPipelineBarrier2, vkCmdBeginRendering, vkQueueSubmit2, …).
    // The engine baseline is 1.2 opportunistic-1.3, so 1.3 is valid for dev hardware.
    app.apiVersion         = VK_API_VERSION_1_3;

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

    VkInstance instance = VK_NULL_HANDLE;
    GW_CHECK_VK(vkCreateInstance(&ici, nullptr, &instance));
    volkLoadInstance(instance);

    // --- Debug messenger (Debug only) ----------------------------------------
    VkDebugUtilsMessengerEXT messenger = VK_NULL_HANDLE;
#if GW_VK_VALIDATION
    VkDebugUtilsMessengerCreateInfoEXT dmi{};
    dmi.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    dmi.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    dmi.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT     |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT  |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    dmi.pfnUserCallback = debug_messenger_cb;
    GW_CHECK_VK(vkCreateDebugUtilsMessengerEXT(instance, &dmi, nullptr, &messenger));
#endif

    // --- Surface -------------------------------------------------------------
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    GW_CHECK_VK(glfwCreateWindowSurface(instance, window, nullptr, &surface));

    // --- Physical device + queues -------------------------------------------
    VkPhysicalDevice gpu = pick_physical_device(instance, surface);
    QueueFamilyIndices qfi = find_queue_families(gpu, surface);

    const float qp = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> qcis;
    std::vector<uint32_t> qfs_unique{*qfi.graphics};
    if (*qfi.present != *qfi.graphics) {
        qfs_unique.push_back(*qfi.present);
    }
    for (uint32_t q : qfs_unique) {
        VkDeviceQueueCreateInfo qci{};
        qci.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qci.queueFamilyIndex = q;
        qci.queueCount       = 1;
        qci.pQueuePriorities = &qp;
        qcis.push_back(qci);
    }

    const char* device_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
    };

    VkPhysicalDeviceSynchronization2Features sync2{};
    sync2.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    sync2.synchronization2 = VK_TRUE;

    VkPhysicalDeviceDynamicRenderingFeatures dyn_render{};
    dyn_render.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dyn_render.pNext            = &sync2;
    dyn_render.dynamicRendering = VK_TRUE;

    VkDeviceCreateInfo dci{};
    dci.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dci.pNext                   = &dyn_render;
    dci.queueCreateInfoCount    = static_cast<uint32_t>(qcis.size());
    dci.pQueueCreateInfos       = qcis.data();
    dci.enabledExtensionCount   = static_cast<uint32_t>(std::size(device_extensions));
    dci.ppEnabledExtensionNames = device_extensions;

    VkDevice device = VK_NULL_HANDLE;
    GW_CHECK_VK(vkCreateDevice(gpu, &dci, nullptr, &device));
    volkLoadDevice(device);

    VkQueue graphics_queue = VK_NULL_HANDLE;
    VkQueue present_queue  = VK_NULL_HANDLE;
    vkGetDeviceQueue(device, *qfi.graphics, 0, &graphics_queue);
    vkGetDeviceQueue(device, *qfi.present, 0, &present_queue);

    // --- Swapchain -----------------------------------------------------------
    Swapchain sc = create_swapchain(gpu, device, surface, *qfi.graphics, *qfi.present, window);
    std::fprintf(stdout, "[sandbox] swapchain %ux%u (fmt=%d, %zu images)\n",
        sc.extent.width, sc.extent.height, sc.format, sc.images.size());

    // --- Pipeline ------------------------------------------------------------
    const std::string shader_dir = GW_SHADER_DIR;
    auto vs_code = read_binary_file(shader_dir + "/triangle.vert.spv");
    auto fs_code = read_binary_file(shader_dir + "/triangle.frag.spv");
    VkShaderModule vs = make_shader_module(device, vs_code);
    VkShaderModule fs = make_shader_module(device, fs_code);

    VkPipelineLayoutCreateInfo pli{};
    pli.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    GW_CHECK_VK(vkCreatePipelineLayout(device, &pli, nullptr, &pipeline_layout));

    VkPipeline pipeline = make_triangle_pipeline(device, pipeline_layout, sc.format, vs, fs);
    std::fprintf(stdout, "[sandbox] pipeline ready — entering render loop\n");

    // Shaders are retained by the pipeline; we can destroy the modules now.
    vkDestroyShaderModule(device, vs, nullptr);
    vkDestroyShaderModule(device, fs, nullptr);

    // --- Command pool + per-frame resources ----------------------------------
    VkCommandPoolCreateInfo cpi{};
    cpi.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cpi.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cpi.queueFamilyIndex = *qfi.graphics;
    VkCommandPool cmd_pool = VK_NULL_HANDLE;
    GW_CHECK_VK(vkCreateCommandPool(device, &cpi, nullptr, &cmd_pool));

    std::vector<VkCommandBuffer> cmd_buffers(kFramesInFlight);
    VkCommandBufferAllocateInfo cbai{};
    cbai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.commandPool        = cmd_pool;
    cbai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = kFramesInFlight;
    GW_CHECK_VK(vkAllocateCommandBuffers(device, &cbai, cmd_buffers.data()));

    std::vector<VkSemaphore> image_available(kFramesInFlight);
    std::vector<VkSemaphore> render_finished(sc.images.size());
    std::vector<VkFence>     in_flight(kFramesInFlight);

    VkSemaphoreCreateInfo sci{};
    sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fci{};
    fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for (uint32_t i = 0; i < kFramesInFlight; ++i) {
        GW_CHECK_VK(vkCreateSemaphore(device, &sci, nullptr, &image_available[i]));
        GW_CHECK_VK(vkCreateFence(device, &fci, nullptr, &in_flight[i]));
    }
    for (size_t i = 0; i < sc.images.size(); ++i) {
        GW_CHECK_VK(vkCreateSemaphore(device, &sci, nullptr, &render_finished[i]));
    }

    // --- Frame loop ----------------------------------------------------------
    uint32_t frame = 0;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        vkWaitForFences(device, 1, &in_flight[frame], VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &in_flight[frame]);

        uint32_t image_index = 0;
        VkResult acq = vkAcquireNextImageKHR(
            device, sc.handle, UINT64_MAX,
            image_available[frame], VK_NULL_HANDLE, &image_index);
        if (acq == VK_ERROR_OUT_OF_DATE_KHR) {
            continue;
        }
        if (acq != VK_SUCCESS && acq != VK_SUBOPTIMAL_KHR) {
            gw_fatal("vkAcquireNextImageKHR failed");
        }

        VkCommandBuffer cmd = cmd_buffers[frame];
        vkResetCommandBuffer(cmd, 0);
        VkCommandBufferBeginInfo bi{};
        bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        GW_CHECK_VK(vkBeginCommandBuffer(cmd, &bi));

        // UNDEFINED -> COLOR_ATTACHMENT_OPTIMAL.
        transition_image(
            cmd, sc.images[image_index],
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);

        VkRenderingAttachmentInfo color_att{};
        color_att.sType              = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        color_att.imageView          = sc.views[image_index];
        color_att.imageLayout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_att.loadOp             = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_att.storeOp            = VK_ATTACHMENT_STORE_OP_STORE;
        color_att.clearValue.color   = {{0.04f, 0.07f, 0.08f, 1.0f}};  // Obsidian

        VkRenderingInfo rinfo{};
        rinfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
        rinfo.renderArea.offset    = {0, 0};
        rinfo.renderArea.extent    = sc.extent;
        rinfo.layerCount           = 1;
        rinfo.colorAttachmentCount = 1;
        rinfo.pColorAttachments    = &color_att;

        vkCmdBeginRendering(cmd, &rinfo);

        VkViewport vp{};
        vp.x        = 0.0f;
        vp.y        = 0.0f;
        vp.width    = static_cast<float>(sc.extent.width);
        vp.height   = static_cast<float>(sc.extent.height);
        vp.minDepth = 0.0f;
        vp.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &vp);
        VkRect2D sr{};
        sr.offset = {0, 0};
        sr.extent = sc.extent;
        vkCmdSetScissor(cmd, 0, 1, &sr);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdDraw(cmd, 3, 1, 0, 0);

        vkCmdEndRendering(cmd);

        transition_image(
            cmd, sc.images[image_index],
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, 0);

        GW_CHECK_VK(vkEndCommandBuffer(cmd));

        VkCommandBufferSubmitInfo cbsi{};
        cbsi.sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        cbsi.commandBuffer = cmd;

        VkSemaphoreSubmitInfo wait_info{};
        wait_info.sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        wait_info.semaphore = image_available[frame];
        wait_info.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSemaphoreSubmitInfo signal_info{};
        signal_info.sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        signal_info.semaphore = render_finished[image_index];
        signal_info.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;

        VkSubmitInfo2 submit{};
        submit.sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submit.waitSemaphoreInfoCount   = 1;
        submit.pWaitSemaphoreInfos      = &wait_info;
        submit.commandBufferInfoCount   = 1;
        submit.pCommandBufferInfos      = &cbsi;
        submit.signalSemaphoreInfoCount = 1;
        submit.pSignalSemaphoreInfos    = &signal_info;
        GW_CHECK_VK(vkQueueSubmit2(graphics_queue, 1, &submit, in_flight[frame]));

        VkPresentInfoKHR pi{};
        pi.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        pi.waitSemaphoreCount = 1;
        pi.pWaitSemaphores    = &render_finished[image_index];
        pi.swapchainCount     = 1;
        pi.pSwapchains        = &sc.handle;
        pi.pImageIndices      = &image_index;
        vkQueuePresentKHR(present_queue, &pi);

        frame = (frame + 1) % kFramesInFlight;
    }

    // --- Teardown ------------------------------------------------------------
    vkDeviceWaitIdle(device);

    for (VkSemaphore s : render_finished)  vkDestroySemaphore(device, s, nullptr);
    for (VkSemaphore s : image_available)  vkDestroySemaphore(device, s, nullptr);
    for (VkFence f : in_flight)            vkDestroyFence(device, f, nullptr);

    vkDestroyCommandPool(device, cmd_pool, nullptr);
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
    for (VkImageView v : sc.views)         vkDestroyImageView(device, v, nullptr);
    vkDestroySwapchainKHR(device, sc.handle, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
#if GW_VK_VALIDATION
    vkDestroyDebugUtilsMessengerEXT(instance, messenger, nullptr);
#endif
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
