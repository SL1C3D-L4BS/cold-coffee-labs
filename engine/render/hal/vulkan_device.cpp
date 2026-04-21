#include "vulkan_device.hpp"
#include <volk.h>
#include <stdexcept>
#include <algorithm>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace gw {
namespace render {
namespace hal {

VulkanDevice::VulkanDevice(VkPhysicalDevice physical_device)
    : physical_device_(physical_device) {

    // Get device properties and features
    vkGetPhysicalDeviceProperties(physical_device, &properties_);
    vkGetPhysicalDeviceFeatures(physical_device, &features_);
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties_);

    // Probe RHI capabilities BEFORE building the device-create lists. The
    // capability snapshot is the canonical gate for every optional extension
    // and feature struct below. See docs/adr/0003-rhi-capabilities.md.
    caps_ = probe_capabilities(physical_device);

    // Baseline (Tier-A) features — absent, the renderer cannot function.
    // Treat as a hard failure rather than a silent downgrade.
    //
    // ADR-0003 amendment (2026-04-20 late-night): synchronization_2 and
    // dynamic_rendering are promoted from Tier-B ("opportunistic") to Tier-A
    // ("baseline"). The frame-graph (engine/render/frame_graph/frame_graph.cpp)
    // and command-buffer wrapper (command_buffer.cpp) both emit sync2 /
    // dynamic-rendering calls unconditionally; there is no sync1 recording
    // path today. Failing device creation here is strictly better than the
    // previous behaviour of silently dropping barriers at execute_pass time.
    // A real sync1 + legacy render-pass backend stays as a Phase-8+ Tier-G
    // task, gated on the cross-hardware CI expansion (lavapipe / MoltenVK).
    if (!caps_.has_timeline_semaphores()) {
        throw std::runtime_error(
            "VulkanDevice: timeline semaphores missing (Vulkan 1.2 baseline); "
            "device is below Greywater's minimum requirement.");
    }
    if (!caps_.has_descriptor_indexing()) {
        throw std::runtime_error(
            "VulkanDevice: descriptor indexing missing; device is below "
            "Greywater's Vulkan 1.2 baseline.");
    }
    if (!caps_.has_sync2()) {
        throw std::runtime_error(
            "VulkanDevice: VK_KHR_synchronization2 missing; Greywater's Tier-A "
            "baseline requires sync2 because the frame graph unconditionally "
            "records vkCmdPipelineBarrier2. See docs/adr/0003-rhi-capabilities.md.");
    }
    if (!caps_.has_dynamic_rendering()) {
        throw std::runtime_error(
            "VulkanDevice: VK_KHR_dynamic_rendering missing; Greywater's Tier-A "
            "baseline requires it because the command-buffer wrapper "
            "unconditionally records vkCmdBeginRendering. See "
            "docs/adr/0003-rhi-capabilities.md.");
    }

    // Find queue families
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());
    
    // Find graphics queue family (required for RX 580 support)
    for (uint32_t i = 0; i < queue_family_count; ++i) {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphics_queue_family_ = i;
            break;
        }
    }
    
    // Find compute queue family
    for (uint32_t i = 0; i < queue_family_count; ++i) {
        if (queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            compute_queue_family_ = i;
            break;
        }
    }
    
    // Find transfer queue family (prefer dedicated)
    for (uint32_t i = 0; i < queue_family_count; ++i) {
        if (queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
            // Check if this is a dedicated transfer queue
            if (!(queue_families[i].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))) {
                transfer_queue_family_ = i;
                break;
            }
        }
    }
    
    // Fallback to graphics queue for transfer if no dedicated queue
    if (transfer_queue_family_ == UINT32_MAX) {
        transfer_queue_family_ = graphics_queue_family_;
    }
    
    // Create logical device
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    std::vector<float> queue_priorities;
    
    // Graphics queue
    if (graphics_queue_family_ != UINT32_MAX) {
        VkDeviceQueueCreateInfo queue_info{};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = graphics_queue_family_;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &queue_priorities.emplace_back(1.0f);
        queue_create_infos.push_back(queue_info);
    }
    
    // Compute queue (if different from graphics)
    if (compute_queue_family_ != UINT32_MAX && compute_queue_family_ != graphics_queue_family_) {
        VkDeviceQueueCreateInfo queue_info{};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = compute_queue_family_;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &queue_priorities.emplace_back(0.5f);
        queue_create_infos.push_back(queue_info);
    }
    
    // Transfer queue (if different from graphics and compute)
    if (transfer_queue_family_ != UINT32_MAX && 
        transfer_queue_family_ != graphics_queue_family_ && 
        transfer_queue_family_ != compute_queue_family_) {
        VkDeviceQueueCreateInfo queue_info{};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = transfer_queue_family_;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &queue_priorities.emplace_back(0.5f);
        queue_create_infos.push_back(queue_info);
    }
    
    // Build the feature chain. Tier-A features (sync2, dynamic_rendering)
    // are unconditional per the ADR-0003 amendment above — the capability
    // check already failed fast if they're absent. Tier-B / Tier-G feature
    // structs (mesh shaders, ray tracing, work graphs, …) still follow the
    // "never request what the driver didn't say yes to" rule; they will be
    // added here once consumer code exists.
    VkPhysicalDeviceFeatures2 features2{};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features2.features.samplerAnisotropy = features_.samplerAnisotropy;

    VkPhysicalDeviceSynchronization2Features sync2_feat{};
    sync2_feat.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    sync2_feat.synchronization2 = VK_TRUE;

    VkPhysicalDeviceDynamicRenderingFeatures dyn_render_feat{};
    dyn_render_feat.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dyn_render_feat.dynamicRendering = VK_TRUE;

    sync2_feat.pNext      = nullptr;
    dyn_render_feat.pNext = &sync2_feat;
    features2.pNext       = &dyn_render_feat;

    // Required extensions (Tier-A — hard-fail if absent).
    std::vector<const char*> device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_MAINTENANCE1_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
    };

    // Sanity-verify the required extensions exist. Anything in the
    // `device_extensions` list at this point is either Tier-A baseline or
    // confirmed-present via `caps_`, so any failure is a driver / loader
    // misconfiguration (not an expected runtime branch).
    uint32_t ext_count = 0;
    vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &ext_count, nullptr);
    std::vector<VkExtensionProperties> available_ext(ext_count);
    vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &ext_count, available_ext.data());

    for (const auto& req_ext : device_extensions) {
        bool found = false;
        for (const auto& avail_ext : available_ext) {
            if (strcmp(req_ext, avail_ext.extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            throw std::runtime_error("Required device extension not supported");
        }
    }

    VkDeviceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pQueueCreateInfos = queue_create_infos.data();
    create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    // When pNext carries VkPhysicalDeviceFeatures2, pEnabledFeatures MUST be
    // NULL — the features live inside features2 instead.
    create_info.pEnabledFeatures = nullptr;
    create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
    create_info.ppEnabledExtensionNames = device_extensions.data();
    create_info.pNext = &features2;

    VkResult result = vkCreateDevice(physical_device, &create_info, nullptr, &device_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan device");
    }

    volkLoadDevice(device_);
    init_queues();
    init_vma();
    init_command_pools();
}

void VulkanDevice::init_queues() noexcept {
    if (graphics_queue_family_ != UINT32_MAX)
        vkGetDeviceQueue(device_, graphics_queue_family_, 0, &graphics_queue_);
    if (compute_queue_family_ != UINT32_MAX)
        vkGetDeviceQueue(device_, compute_queue_family_,  0, &compute_queue_);
    if (transfer_queue_family_ != UINT32_MAX)
        vkGetDeviceQueue(device_, transfer_queue_family_, 0, &transfer_queue_);
}

void VulkanDevice::init_vma() {
    VmaVulkanFunctions vk_fns{};
    vk_fns.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vk_fns.vkGetDeviceProcAddr   = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo ai{};
    ai.physicalDevice   = physical_device_;
    ai.device           = device_;
    ai.instance         = VK_NULL_HANDLE;  // optional; filled when instance is passed in
    ai.vulkanApiVersion = VK_API_VERSION_1_2;
    ai.pVulkanFunctions = &vk_fns;

    VkResult r = vmaCreateAllocator(&ai, &allocator_);
    if (r != VK_SUCCESS) {
        throw std::runtime_error("Failed to create VMA allocator");
    }
}

void VulkanDevice::init_command_pools() {
    if (graphics_queue_family_ == UINT32_MAX) return;

    VkCommandPoolCreateInfo ci{};
    ci.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    ci.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    ci.queueFamilyIndex = graphics_queue_family_;

    VkResult r = vkCreateCommandPool(device_, &ci, nullptr, &graphics_cmd_pool_);
    if (r != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics command pool");
    }
}

VulkanDevice::~VulkanDevice() {
    release();
}

VulkanDevice::VulkanDevice(VulkanDevice&& other) noexcept
    : device_(other.device_)
    , physical_device_(other.physical_device_)
    , allocator_(other.allocator_)
    , graphics_queue_(other.graphics_queue_)
    , compute_queue_(other.compute_queue_)
    , transfer_queue_(other.transfer_queue_)
    , graphics_queue_family_(other.graphics_queue_family_)
    , compute_queue_family_(other.compute_queue_family_)
    , transfer_queue_family_(other.transfer_queue_family_)
    , graphics_cmd_pool_(other.graphics_cmd_pool_)
    , features_(other.features_)
    , properties_(other.properties_)
    , memory_properties_(other.memory_properties_)
    , caps_(other.caps_) {
    other.device_            = VK_NULL_HANDLE;
    other.physical_device_   = VK_NULL_HANDLE;
    other.allocator_         = VK_NULL_HANDLE;
    other.graphics_cmd_pool_ = VK_NULL_HANDLE;
}

VulkanDevice& VulkanDevice::operator=(VulkanDevice&& other) noexcept {
    if (this != &other) {
        release();
        device_                = other.device_;
        physical_device_       = other.physical_device_;
        allocator_             = other.allocator_;
        graphics_queue_        = other.graphics_queue_;
        compute_queue_         = other.compute_queue_;
        transfer_queue_        = other.transfer_queue_;
        graphics_queue_family_ = other.graphics_queue_family_;
        compute_queue_family_  = other.compute_queue_family_;
        transfer_queue_family_ = other.transfer_queue_family_;
        graphics_cmd_pool_     = other.graphics_cmd_pool_;
        features_              = other.features_;
        properties_            = other.properties_;
        memory_properties_     = other.memory_properties_;
        caps_                  = other.caps_;
        other.device_            = VK_NULL_HANDLE;
        other.physical_device_   = VK_NULL_HANDLE;
        other.allocator_         = VK_NULL_HANDLE;
        other.graphics_cmd_pool_ = VK_NULL_HANDLE;
    }
    return *this;
}

void VulkanDevice::release() noexcept {
    if (device_) {
        vkDeviceWaitIdle(device_);
        if (graphics_cmd_pool_ != VK_NULL_HANDLE) {
            vkDestroyCommandPool(device_, graphics_cmd_pool_, nullptr);
            graphics_cmd_pool_ = VK_NULL_HANDLE;
        }
        if (allocator_ != VK_NULL_HANDLE) {
            vmaDestroyAllocator(allocator_);
            allocator_ = VK_NULL_HANDLE;
        }
        vkDestroyDevice(device_, nullptr);
        device_ = VK_NULL_HANDLE;
    }
}

}  // namespace hal
}  // namespace render
}  // namespace gw
