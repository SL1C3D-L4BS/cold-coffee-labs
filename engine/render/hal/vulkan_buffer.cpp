#include "vulkan_buffer.hpp"
#include <volk.h>
#include <vk_mem_alloc.h>
#include <stdexcept>

namespace gw {
namespace render {
namespace hal {

VulkanBuffer::VulkanBuffer(VmaAllocator allocator, VkBuffer buffer, VmaAllocation allocation)
    : allocator_(allocator)
    , buffer_(buffer)
    , allocation_(allocation) {
    
    // Get allocation size
    VmaAllocationInfo alloc_info;
    vmaGetAllocationInfo(allocator_, allocation_, &alloc_info);
    size_ = alloc_info.size;
}

VulkanBuffer::~VulkanBuffer() {
    if (buffer_ != VK_NULL_HANDLE) {
        vmaDestroyBuffer(allocator_, buffer_, allocation_);
    }
}

void* VulkanBuffer::map() {
    if (!mapped_data_) {
        VkResult result = vmaMapMemory(allocator_, allocation_, &mapped_data_);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to map buffer memory");
        }
    }
    return mapped_data_;
}

void VulkanBuffer::unmap() {
    if (mapped_data_) {
        vmaUnmapMemory(allocator_, allocation_);
        mapped_data_ = nullptr;
    }
}

}  // namespace hal
}  // namespace render
}  // namespace gw
