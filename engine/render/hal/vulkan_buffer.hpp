#pragma once

#include <cstdint>
#include <memory>
#include <volk.h>
#include <vk_mem_alloc.h>

namespace gw {
namespace render {
namespace hal {

class VulkanBuffer final {
public:
    VulkanBuffer(VmaAllocator allocator, VkBuffer buffer, VmaAllocation allocation);
    ~VulkanBuffer();

    VulkanBuffer(const VulkanBuffer&) = delete;
    VulkanBuffer& operator=(const VulkanBuffer&) = delete;

    VulkanBuffer(VulkanBuffer&& other) noexcept = default;
    VulkanBuffer& operator=(VulkanBuffer&& other) noexcept = default;

    [[nodiscard]] VkBuffer native_handle() const noexcept { return buffer_; }
    [[nodiscard]] VkBuffer buffer() const { return buffer_; }
    [[nodiscard]] VmaAllocation allocation() const { return allocation_; }
    [[nodiscard]] void* mapped_data() const { return mapped_data_; }
    [[nodiscard]] std::size_t size() const noexcept { return size_; }

    // Map/unmap for CPU access
    void* map();
    void unmap();

private:
    VkBuffer buffer_{VK_NULL_HANDLE};
    VmaAllocation allocation_{VK_NULL_HANDLE};
    VmaAllocator allocator_{VK_NULL_HANDLE};
    std::size_t size_{0};
    void* mapped_data_{nullptr};
};

}  // namespace hal
}  // namespace render
}  // namespace gw
