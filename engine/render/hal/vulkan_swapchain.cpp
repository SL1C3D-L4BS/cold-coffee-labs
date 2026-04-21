#include "vulkan_swapchain.hpp"
#include <volk.h>
#include <stdexcept>
#include <algorithm>

namespace gw {
namespace render {
namespace hal {

VulkanSwapchain::VulkanSwapchain(
        VkDevice device,
        VkPhysicalDevice physical_device,
        VkSurfaceKHR surface,
        uint32_t width,
        uint32_t height)
    : device_(device)
    , physical_device_(physical_device) {
    
    // Get surface capabilities
    VkSurfaceCapabilitiesKHR capabilities;
    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        physical_device_, surface, &capabilities);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to get surface capabilities");
    }
    
    // Choose format (prefer SRGB if available)
    std::vector<VkSurfaceFormatKHR> formats;
    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface, &format_count, nullptr);
    formats.resize(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface, &format_count, formats.data());
    
    // Select format
    format_ = formats[0];  // Default to first format
    for (const auto& fmt : formats) {
        if (fmt.format == VK_FORMAT_B8G8R8_SRGB && 
            fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            format_ = fmt;
            break;
        }
    }
    
    // Choose present mode
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;  // Default to FIFO for RX 580
    std::vector<VkPresentModeKHR> modes;
    uint32_t mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device_, surface, &mode_count, nullptr);
    modes.resize(mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device_, surface, &mode_count, modes.data());
    
    for (const auto& mode : modes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            present_mode = mode;
            break;
        }
    }
    
    // Determine extent
    extent_ = capabilities.currentExtent;
    if (extent_.width == UINT32_MAX) {
        extent_.width = width;
        extent_.height = height;
    }
    
    // Clamp to min/max
    extent_.width = std::clamp(extent_.width, 
                              capabilities.minImageExtent.width,
                              capabilities.maxImageExtent.width);
    extent_.height = std::clamp(extent_.height,
                               capabilities.minImageExtent.height,
                               capabilities.maxImageExtent.height);
    
    // Determine image count (triple buffering for RX 580).
    // Spec: VkSurfaceCapabilitiesKHR::maxImageCount == 0 means "no maximum
    // except memory" — NOT "zero maximum". Clamp only when a real max exists.
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfaceCapabilitiesKHR.html
    const uint32_t desired = std::max(3u, capabilities.minImageCount + 1u);
    uint32_t image_count = (capabilities.maxImageCount == 0u)
                               ? desired
                               : std::min(desired, capabilities.maxImageCount);
    
    // Create swapchain
    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.pNext = nullptr;
    create_info.surface = surface;
    create_info.minImageCount = image_count;
    create_info.imageFormat = format_.format;
    create_info.imageColorSpace = format_.colorSpace;
    create_info.imageExtent = extent_;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.preTransform = capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_FALSE;
    create_info.oldSwapchain = swapchain_;
    
    VkResult swapchain_result = vkCreateSwapchainKHR(device_, &create_info, nullptr, &swapchain_);
    if (swapchain_result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swapchain");
    }
    
    if (create_info.oldSwapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device_, create_info.oldSwapchain, nullptr);
    }
    
    // Get swapchain images
    uint32_t actual_image_count = 0;
    vkGetSwapchainImagesKHR(device_, swapchain_, &actual_image_count, nullptr);
    images_.resize(actual_image_count);
    vkGetSwapchainImagesKHR(device_, swapchain_, &actual_image_count, images_.data());
    
    // Create image views
    image_views_.resize(actual_image_count);
    for (uint32_t i = 0; i < actual_image_count; ++i) {
        VkImageViewCreateInfo view_info{};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = images_[i];
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = format_.format;
        view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;
        
        VkImageView view;
        VkResult view_result = vkCreateImageView(device_, &view_info, nullptr, &view);
        if (view_result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image view");
        }
        
        image_views_[i] = view;
    }
}

VulkanSwapchain::~VulkanSwapchain() {
    release();
}

VulkanSwapchain::VulkanSwapchain(VulkanSwapchain&& other) noexcept
    : swapchain_(other.swapchain_)
    , device_(other.device_)
    , physical_device_(other.physical_device_)
    , format_(other.format_)
    , extent_(other.extent_)
    , images_(std::move(other.images_))
    , image_views_(std::move(other.image_views_)) {
    other.swapchain_ = VK_NULL_HANDLE;
    other.device_ = VK_NULL_HANDLE;
    other.physical_device_ = VK_NULL_HANDLE;
}

VulkanSwapchain& VulkanSwapchain::operator=(VulkanSwapchain&& other) noexcept {
    if (this != &other) {
        release();
        
        swapchain_ = other.swapchain_;
        device_ = other.device_;
        physical_device_ = other.physical_device_;
        format_ = other.format_;
        extent_ = other.extent_;
        images_ = std::move(other.images_);
        image_views_ = std::move(other.image_views_);
        
        other.swapchain_ = VK_NULL_HANDLE;
        other.device_ = VK_NULL_HANDLE;
        other.physical_device_ = VK_NULL_HANDLE;
    }
    return *this;
}

void VulkanSwapchain::release() noexcept {
    if (swapchain_) {
        // Destroy image views first
        for (auto view : image_views_) {
            vkDestroyImageView(device_, view, nullptr);
        }
        image_views_.clear();
        
        // Swapchain images are owned by swapchain, no need to destroy separately
        
        vkDestroySwapchainKHR(device_, swapchain_, nullptr);
        swapchain_ = VK_NULL_HANDLE;
    }
}

}  // namespace hal
}  // namespace render
}  // namespace gw
