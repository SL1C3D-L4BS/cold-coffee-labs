#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <volk.h>

namespace gw {
namespace render {
namespace hal {

class VulkanCommandBuffer final {
public:
    explicit VulkanCommandBuffer(VkDevice device, VkCommandPool pool);
    ~VulkanCommandBuffer();

    VulkanCommandBuffer(const VulkanCommandBuffer&) = delete;
    VulkanCommandBuffer& operator=(const VulkanCommandBuffer&) = delete;

    VulkanCommandBuffer(VulkanCommandBuffer&& other) noexcept;
    VulkanCommandBuffer& operator=(VulkanCommandBuffer&& other) noexcept;

    [[nodiscard]] VkCommandBuffer native_handle() const noexcept { return buffer_; }
    [[nodiscard]] bool is_recording() const noexcept { return state_ == State::Recording; }

    // Recording operations
    void begin(VkCommandBufferUsageFlags flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    void end();
    void reset();

    // synchronization2 barrier (primary API for Phase 5+)
    void pipeline_barrier2(
        VkDependencyFlags dependency_flags,
        const std::vector<VkImageMemoryBarrier2>&  image_barriers   = {},
        const std::vector<VkBufferMemoryBarrier2>& buffer_barriers  = {},
        const std::vector<VkMemoryBarrier2>&       memory_barriers  = {});

    // Legacy barrier (kept for compatibility with Phase 1 sandbox path)
    void pipeline_barrier(
        VkPipelineStageFlags src_stage,
        VkPipelineStageFlags dst_stage,
        VkDependencyFlags dependency_flags = VK_DEPENDENCY_BY_REGION_BIT);

    void copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);

private:
    enum class State {
        Initial,
        Recording,
        Executable,
    };

    void release() noexcept;

    VkCommandBuffer buffer_{VK_NULL_HANDLE};
    VkCommandPool pool_{VK_NULL_HANDLE};
    VkDevice device_{VK_NULL_HANDLE};
    State state_{State::Initial};
};

}  // namespace hal
}  // namespace render
}  // namespace gw
