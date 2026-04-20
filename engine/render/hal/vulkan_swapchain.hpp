#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <volk.h>

namespace gw {
namespace render {
namespace hal {

class VulkanSwapchain final {
public:
    explicit VulkanSwapchain(
        VkDevice device,
        VkPhysicalDevice physical_device,
        VkSurfaceKHR surface,
        uint32_t width,
        uint32_t height);
    ~VulkanSwapchain();

    VulkanSwapchain(const VulkanSwapchain&) = delete;
    VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;

    VulkanSwapchain(VulkanSwapchain&& other) noexcept;
    VulkanSwapchain& operator=(VulkanSwapchain&& other) noexcept;

    [[nodiscard]] VkSwapchainKHR native_handle() const noexcept { return swapchain_; }
    [[nodiscard]] VkSurfaceFormatKHR format() const noexcept { return format_; }
    [[nodiscard]] VkExtent2D extent() const noexcept { return extent_; }
    [[nodiscard]] uint32_t image_count() const noexcept { return static_cast<uint32_t>(images_.size()); }
    
    [[nodiscard]] const VkImage& image(uint32_t index) const { return images_[index]; }

private:
    void release() noexcept;

    VkSwapchainKHR swapchain_{VK_NULL_HANDLE};
    VkDevice device_{VK_NULL_HANDLE};
    VkPhysicalDevice physical_device_{VK_NULL_HANDLE};
    VkSurfaceFormatKHR format_{};
    VkExtent2D extent_{};
    std::vector<VkImage> images_;
    std::vector<VkImageView> image_views_;
};

}  // namespace hal
}  // namespace render
}  // namespace gw
