#include "vulkan_command_buffer.hpp"
#include <volk.h>
#include <stdexcept>

namespace gw {
namespace render {
namespace hal {

VulkanCommandBuffer::VulkanCommandBuffer(VkDevice device, VkCommandPool pool)
    : pool_(pool)
    , device_(device) {
    
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;
    
    VkResult result = vkAllocateCommandBuffers(device, &alloc_info, &buffer_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffer");
    }
    
    state_ = State::Initial;
}

VulkanCommandBuffer::~VulkanCommandBuffer() {
    release();
}

VulkanCommandBuffer::VulkanCommandBuffer(VulkanCommandBuffer&& other) noexcept
    : buffer_(other.buffer_)
    , pool_(other.pool_)
    , device_(other.device_)
    , state_(other.state_) {
    other.buffer_ = VK_NULL_HANDLE;
    other.pool_ = VK_NULL_HANDLE;
    other.device_ = VK_NULL_HANDLE;
}

VulkanCommandBuffer& VulkanCommandBuffer::operator=(VulkanCommandBuffer&& other) noexcept {
    if (this != &other) {
        release();
        
        buffer_ = other.buffer_;
        pool_ = other.pool_;
        device_ = other.device_;
        state_ = other.state_;
        
        other.buffer_ = VK_NULL_HANDLE;
        other.pool_ = VK_NULL_HANDLE;
        other.device_ = VK_NULL_HANDLE;
    }
    return *this;
}

void VulkanCommandBuffer::release() noexcept {
    if (buffer_) {
        if (state_ == State::Recording) {
            end();
        }
        vkFreeCommandBuffers(device_, pool_, 1, &buffer_);
        buffer_ = VK_NULL_HANDLE;
    }
}

void VulkanCommandBuffer::begin(VkCommandBufferUsageFlags flags) {
    if (state_ != State::Initial) {
        throw std::runtime_error("Command buffer must be in initial state");
    }
    
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = flags;
    
    VkResult result = vkBeginCommandBuffer(buffer_, &begin_info);
    if (result == VK_SUCCESS) {
        state_ = State::Recording;
    }
}

void VulkanCommandBuffer::end() {
    if (state_ != State::Recording) {
        throw std::runtime_error("Command buffer is not recording");
    }
    
    VkResult result = vkEndCommandBuffer(buffer_);
    if (result == VK_SUCCESS) {
        state_ = State::Executable;
    }
}

void VulkanCommandBuffer::reset() {
    if (state_ != State::Initial && state_ != State::Executable) {
        throw std::runtime_error("Command buffer must be executable to reset");
    }
    
    vkResetCommandBuffer(buffer_, 0);
    state_ = State::Initial;
}

void VulkanCommandBuffer::pipeline_barrier(
        VkPipelineStageFlags src_stage,
        VkPipelineStageFlags dst_stage,
        VkDependencyFlags dependency_flags) {
    
    VkMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
    barrier.srcStageMask = src_stage;
    barrier.dstStageMask = dst_stage;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    
    VkDependencyInfo dependency_info{};
    dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependency_info.memoryBarrierCount = 1;
    dependency_info.pMemoryBarriers = &barrier;
    
    vkCmdPipelineBarrier2(buffer_, &dependency_info);
}

void VulkanCommandBuffer::copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
    VkBufferCopy copy_region{};
    copy_region.srcOffset = 0;
    copy_region.dstOffset = 0;
    copy_region.size = size;
    
    vkCmdCopyBuffer(buffer_, src, dst, 1, &copy_region);
}

}  // namespace hal
}  // namespace render
}  // namespace gw
