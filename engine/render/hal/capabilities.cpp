#include "capabilities.hpp"

#include <cstring>
#include <vector>

namespace gw {
namespace render {
namespace hal {

namespace {

[[nodiscard]] bool has_extension(const std::vector<VkExtensionProperties>& exts,
                                 const char*                                 name) noexcept {
    for (const auto& e : exts) {
        if (std::strcmp(e.extensionName, name) == 0) return true;
    }
    return false;
}

}  // namespace

RHICapabilities probe_capabilities(VkPhysicalDevice physical_device) noexcept {
    RHICapabilities caps{};
    if (physical_device == VK_NULL_HANDLE) return caps;

    // Basic properties (api_version, limits)
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(physical_device, &props);
    caps.api_version = props.apiVersion;

    // Enumerate device extensions.
    std::uint32_t ext_count = 0;
    vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &ext_count, nullptr);
    std::vector<VkExtensionProperties> exts(ext_count);
    if (ext_count > 0) {
        vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &ext_count, exts.data());
    }

    // Chain the feature queries we care about. `VkPhysicalDeviceFeatures2`
    // accepts a pNext chain of per-feature structs; we populate each and
    // then mine the booleans we need.
    VkPhysicalDeviceTimelineSemaphoreFeatures timeline_feat{};
    timeline_feat.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;

    VkPhysicalDeviceBufferDeviceAddressFeatures bda_feat{};
    bda_feat.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    bda_feat.pNext = &timeline_feat;

    VkPhysicalDeviceDescriptorIndexingFeatures di_feat{};
    di_feat.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    di_feat.pNext = &bda_feat;

    VkPhysicalDeviceDynamicRenderingFeatures dr_feat{};
    dr_feat.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dr_feat.pNext = &di_feat;

    VkPhysicalDeviceSynchronization2Features sync2_feat{};
    sync2_feat.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    sync2_feat.pNext = &dr_feat;

    VkPhysicalDeviceFeatures2 features2{};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features2.pNext = &sync2_feat;

    vkGetPhysicalDeviceFeatures2(physical_device, &features2);

    caps.timeline_semaphores   = timeline_feat.timelineSemaphore == VK_TRUE;
    caps.buffer_device_address = bda_feat.bufferDeviceAddress    == VK_TRUE;
    caps.descriptor_indexing   = di_feat.runtimeDescriptorArray  == VK_TRUE;
    caps.dynamic_rendering     = dr_feat.dynamicRendering        == VK_TRUE;
    caps.synchronization_2     = sync2_feat.synchronization2     == VK_TRUE;

    // Descriptor-indexing properties (limits) — used by bindless callers.
    VkPhysicalDeviceDescriptorIndexingProperties di_props{};
    di_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES;

    VkPhysicalDeviceProperties2 props2{};
    props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    props2.pNext = &di_props;
    vkGetPhysicalDeviceProperties2(physical_device, &props2);
    caps.max_descriptor_indexed_array_size =
        di_props.maxPerStageDescriptorUpdateAfterBindSampledImages;

    // Extension-gated Tier-G features: presence of the extension string
    // is the only signal needed here; feature-struct booleans follow on
    // device creation.
    caps.mesh_shaders         = has_extension(exts, "VK_EXT_mesh_shader");
    caps.ray_tracing_pipeline = has_extension(exts, "VK_KHR_ray_tracing_pipeline");
    caps.ray_query            = has_extension(exts, "VK_KHR_ray_query");
    caps.ext_memory_budget    = has_extension(exts, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);

    // Vulkan 1.4 feature — present on a small set of bleeding-edge drivers.
    // We detect by api_version rather than extension string because 1.4
    // rolled work-graphs into core; the extension string is preserved for
    // 1.3 targets but the feature is effectively unreachable there.
    caps.gpu_work_graphs = (caps.api_major() >= 1u && caps.api_minor() >= 4u);

    // Device-local heap size. Sum all DEVICE_LOCAL heaps — multi-heap GPUs
    // report per-tier memory (rare on discrete AMD/NVIDIA, routine on
    // integrated) and we want the aggregate for streaming budgeting.
    VkPhysicalDeviceMemoryProperties mem_props{};
    vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_props);
    caps.total_device_local_bytes = 0u;
    for (std::uint32_t i = 0; i < mem_props.memoryHeapCount; ++i) {
        if (mem_props.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
            caps.total_device_local_bytes += mem_props.memoryHeaps[i].size;
        }
    }

    return caps;
}

}  // namespace hal
}  // namespace render
}  // namespace gw
