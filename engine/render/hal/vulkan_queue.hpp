#pragma once

#include <cstdint>
#include <volk.h>

namespace gw {
namespace render {
namespace hal {

class VulkanQueue final {
public:
    explicit VulkanQueue(VkQueue queue, uint32_t family_index);
    ~VulkanQueue() = default;

    VulkanQueue(const VulkanQueue&) = delete;
    VulkanQueue& operator=(const VulkanQueue&) = delete;

    VulkanQueue(VulkanQueue&& other) noexcept = default;
    VulkanQueue& operator=(VulkanQueue&& other) noexcept = default;

    [[nodiscard]] VkQueue native_handle() const noexcept { return queue_; }
    [[nodiscard]] uint32_t family_index() const noexcept { return family_index_; }

    // Queue operations
    void submit(uint32_t submit_count, const VkSubmitInfo* submit_infos, VkFence fence);
    void wait_idle() const;

private:
    VkQueue queue_{VK_NULL_HANDLE};
    uint32_t family_index_{0};
};

}  // namespace hal
}  // namespace render
}  // namespace gw
