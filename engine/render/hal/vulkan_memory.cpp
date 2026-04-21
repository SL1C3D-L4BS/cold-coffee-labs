// VMA_IMPLEMENTATION is defined exactly once in vulkan_device.cpp.
// Do NOT redefine it here — that would trigger an ODR violation and
// multiply-defined-symbol linker errors when both TUs are in the same target.
#include "vulkan_memory.hpp"
#include <volk.h>
#include <stdexcept>

namespace gw {
namespace render {
namespace hal {

VulkanMemoryManager::VulkanMemoryManager(VkDevice device)
    : device_(device) {
    
    // Create VMA allocator
    VmaAllocatorCreateInfo allocator_info{};
    allocator_info.flags = 0;
    allocator_info.device = device;
    
    // RX 580 supports these features
    allocator_info.vulkanApiVersion = VK_API_VERSION_1_2;
    allocator_info.instance = nullptr;  // Not needed for device allocator
    
    VkResult result = vmaCreateAllocator(&allocator_info, &allocator_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create VMA allocator");
    }
}

VulkanMemoryManager::~VulkanMemoryManager() {
    release();
}

VulkanMemoryManager::VulkanMemoryManager(VulkanMemoryManager&& other) noexcept
    : allocator_(other.allocator_)
    , device_(other.device_) {
    other.allocator_ = VK_NULL_HANDLE;
    other.device_ = VK_NULL_HANDLE;
}

VulkanMemoryManager& VulkanMemoryManager::operator=(VulkanMemoryManager&& other) noexcept {
    if (this != &other) {
        release();
        
        allocator_ = other.allocator_;
        device_ = other.device_;
        
        other.allocator_ = VK_NULL_HANDLE;
        other.device_ = VK_NULL_HANDLE;
    }
    return *this;
}

void VulkanMemoryManager::release() noexcept {
    if (allocator_) {
        vmaDestroyAllocator(allocator_);
        allocator_ = VK_NULL_HANDLE;
    }
}

VmaAllocation VulkanMemoryManager::allocate_buffer(
    const VkBufferCreateInfo& create_info,
    VmaMemoryUsage usage,
    VkBuffer* out_buffer) {
    
    VmaAllocationCreateInfo alloc_info{};
    alloc_info.flags = 0;
    alloc_info.usage = usage;
    
    VmaAllocation allocation;
    VmaAllocationInfo alloc_info_result;
    VkResult result = vmaCreateBuffer(
        allocator_, &create_info, &alloc_info, out_buffer, &allocation, &alloc_info_result);
    
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate buffer");
    }
    
    return allocation;
}

VmaAllocation VulkanMemoryManager::allocate_image(
    const VkImageCreateInfo& create_info,
    VmaMemoryUsage usage,
    VkImage* out_image) {
    
    VmaAllocationCreateInfo alloc_info{};
    alloc_info.flags = 0;
    alloc_info.usage = usage;
    
    VmaAllocation allocation;
    VmaAllocationInfo alloc_info_result;
    VkResult result = vmaCreateImage(
        allocator_, &create_info, &alloc_info, out_image, &allocation, &alloc_info_result);
    
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate image");
    }
    
    return allocation;
}

void VulkanMemoryManager::deallocate_buffer(VkBuffer buffer, VmaAllocation allocation) {
    vmaDestroyBuffer(allocator_, buffer, allocation);
}

void VulkanMemoryManager::deallocate_image(VkImage image, VmaAllocation allocation) {
    vmaDestroyImage(allocator_, image, allocation);
}

}  // namespace hal
}  // namespace render
}  // namespace gw
