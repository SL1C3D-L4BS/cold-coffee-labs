#pragma once

#include <cstdint>
#include <vector>
#include <memory>
// volk.h must precede vk_mem_alloc.h: it defines VK_NO_PROTOTYPES, preventing
// vulkan_core.h from emitting inline prototypes that would clash with volk's
// extern PFN_* declarations.
#include <volk.h>
#include <vk_mem_alloc.h>

#include "capabilities.hpp"

namespace gw {
namespace render {
namespace hal {

class VulkanDevice final {
public:
    explicit VulkanDevice(VkPhysicalDevice physical_device);
    ~VulkanDevice();

    VulkanDevice(const VulkanDevice&) = delete;
    VulkanDevice& operator=(const VulkanDevice&) = delete;

    VulkanDevice(VulkanDevice&& other) noexcept;
    VulkanDevice& operator=(VulkanDevice&& other) noexcept;

    // Core handles
    [[nodiscard]] VkDevice         native_handle()    const noexcept { return device_; }
    [[nodiscard]] VkPhysicalDevice physical_device()  const noexcept { return physical_device_; }
    [[nodiscard]] VmaAllocator     vma_allocator()    const noexcept { return allocator_; }

    // Queue handles
    [[nodiscard]] VkQueue graphics_queue()  const noexcept { return graphics_queue_; }
    [[nodiscard]] VkQueue compute_queue()   const noexcept { return compute_queue_; }
    [[nodiscard]] VkQueue transfer_queue()  const noexcept { return transfer_queue_; }

    // Queue family indices
    [[nodiscard]] uint32_t graphics_queue_family() const noexcept { return graphics_queue_family_; }
    [[nodiscard]] uint32_t compute_queue_family()  const noexcept { return compute_queue_family_; }
    [[nodiscard]] uint32_t transfer_queue_family() const noexcept { return transfer_queue_family_; }

    // Feature queries
    [[nodiscard]] bool supports_anisotropy() const noexcept { return features_.samplerAnisotropy; }

    // RHI capability snapshot — the canonical "what does this GPU support"
    // feature-flag struct. Populated once at construction; read-only.
    // See docs/adr/0003-rhi-capabilities.md.
    [[nodiscard]] const RHICapabilities& capabilities() const noexcept { return caps_; }

    // Graphics command pool (one-per-frame-in-flight callers own the buffer lifetime)
    [[nodiscard]] VkCommandPool graphics_command_pool() const noexcept { return graphics_cmd_pool_; }

private:
    void release() noexcept;
    void init_queues() noexcept;
    void init_vma();
    void init_command_pools();

    VkDevice         device_{VK_NULL_HANDLE};
    VkPhysicalDevice physical_device_{VK_NULL_HANDLE};
    VmaAllocator     allocator_{VK_NULL_HANDLE};

    VkQueue  graphics_queue_{VK_NULL_HANDLE};
    VkQueue  compute_queue_{VK_NULL_HANDLE};
    VkQueue  transfer_queue_{VK_NULL_HANDLE};

    uint32_t graphics_queue_family_{UINT32_MAX};
    uint32_t compute_queue_family_{UINT32_MAX};
    uint32_t transfer_queue_family_{UINT32_MAX};

    VkCommandPool graphics_cmd_pool_{VK_NULL_HANDLE};

    VkPhysicalDeviceFeatures         features_{};
    VkPhysicalDeviceProperties       properties_{};
    VkPhysicalDeviceMemoryProperties memory_properties_{};

    RHICapabilities                  caps_{};
};

}  // namespace hal
}  // namespace render
}  // namespace gw
