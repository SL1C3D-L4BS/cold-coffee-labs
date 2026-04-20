#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <volk.h>
#include <vk_mem_alloc.h>

namespace gw {
namespace render {
namespace hal {

class VulkanMemoryManager final {
public:
    explicit VulkanMemoryManager(VkDevice device);
    ~VulkanMemoryManager();

    VulkanMemoryManager(const VulkanMemoryManager&) = delete;
    VulkanMemoryManager& operator=(const VulkanMemoryManager&) = delete;

    VulkanMemoryManager(VulkanMemoryManager&& other) noexcept;
    VulkanMemoryManager& operator=(VulkanMemoryManager&& other) noexcept;

    [[nodiscard]] VmaAllocator native_allocator() const noexcept { return allocator_; }
    [[nodiscard]] VkDevice device() const noexcept { return device_; }

    // Buffer allocation
    [[nodiscard]] VmaAllocation allocate_buffer(
        const VkBufferCreateInfo& create_info,
        VmaMemoryUsage usage,
        VkBuffer* out_buffer);
    
    // Image allocation
    [[nodiscard]] VmaAllocation allocate_image(
        const VkImageCreateInfo& create_info,
        VmaMemoryUsage usage,
        VkImage* out_image);

    // Deallocation helpers
    void deallocate_buffer(VkBuffer buffer, VmaAllocation allocation);
    void deallocate_image(VkImage image, VmaAllocation allocation);

private:
    void release() noexcept;

    VmaAllocator allocator_{VK_NULL_HANDLE};
    VkDevice device_{VK_NULL_HANDLE};
};

}  // namespace hal
}  // namespace render
}  // namespace gw
