#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <volk.h>

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

    [[nodiscard]] VkDevice native_handle() const noexcept { return device_; }
    [[nodiscard]] VkPhysicalDevice physical_device() const noexcept { return physical_device_; }
    
    // Queue family access
    [[nodiscard]] uint32_t graphics_queue_family() const noexcept { return graphics_queue_family_; }
    [[nodiscard]] uint32_t compute_queue_family() const noexcept { return compute_queue_family_; }
    [[nodiscard]] uint32_t transfer_queue_family() const noexcept { return transfer_queue_family_; }
    
    [[nodiscard]] bool supports_anisotropy() const noexcept { return features_.samplerAnisotropy; }
    [[nodiscard]] bool supports_sampler_mirror_clamp_to_edge() const noexcept { return false; } // RX 580 doesn't support this

private:
    void release() noexcept;

    VkDevice device_{VK_NULL_HANDLE};
    VkPhysicalDevice physical_device_{VK_NULL_HANDLE};
    uint32_t graphics_queue_family_{UINT32_MAX};
    uint32_t compute_queue_family_{UINT32_MAX};
    uint32_t transfer_queue_family_{UINT32_MAX};
    
    VkPhysicalDeviceFeatures features_{};
    VkPhysicalDeviceProperties properties_{};
    VkPhysicalDeviceMemoryProperties memory_properties_{};
};

}  // namespace hal
}  // namespace render
}  // namespace gw
