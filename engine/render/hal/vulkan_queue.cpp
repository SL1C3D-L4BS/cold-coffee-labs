#include "vulkan_queue.hpp"
#include <volk.h>
#include <stdexcept>

namespace gw {
namespace render {
namespace hal {

VulkanQueue::VulkanQueue(VkQueue queue, uint32_t family_index)
    : queue_(queue)
    , family_index_(family_index) {
}

void VulkanQueue::submit(uint32_t submit_count, const VkSubmitInfo* submit_infos, VkFence fence) {
    VkResult result = vkQueueSubmit(queue_, submit_count, submit_infos, fence);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit to queue");
    }
}

void VulkanQueue::wait_idle() const {
    VkResult result = vkQueueWaitIdle(queue_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to wait for queue idle");
    }
}

}  // namespace hal
}  // namespace render
}  // namespace gw
