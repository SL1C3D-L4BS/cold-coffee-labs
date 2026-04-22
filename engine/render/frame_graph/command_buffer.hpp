#pragma once

#include "types.hpp"
#include <volk.h>
#include <vector>

namespace gw::render::frame_graph {

// Thin wrapper around VkCommandBuffer for use inside frame graph pass callbacks.
// The command buffer must be in recording state when the callback is invoked.
class CommandBuffer {
public:
    // Construct from a live VkCommandBuffer that is already recording.
    // Passing VK_NULL_HANDLE is allowed during offline validation (no-ops all calls).
    explicit CommandBuffer(VkCommandBuffer cmd) noexcept : cmd_(cmd) {}
    ~CommandBuffer() = default;

    CommandBuffer(const CommandBuffer&) = delete;
    CommandBuffer& operator=(const CommandBuffer&) = delete;
    CommandBuffer(CommandBuffer&&) noexcept = default;
    CommandBuffer& operator=(CommandBuffer&&) = delete;

    [[nodiscard]] VkCommandBuffer native_handle() const noexcept { return cmd_; }
    [[nodiscard]] bool            valid()         const noexcept { return cmd_ != VK_NULL_HANDLE; }

    // synchronization2 barrier
    void pipeline_barrier2(
        VkDependencyFlags                          dependency_flags,
        const std::vector<VkImageMemoryBarrier2>&  image_barriers   = {},
        const std::vector<VkBufferMemoryBarrier2>& buffer_barriers  = {},
        const std::vector<VkMemoryBarrier2>&       memory_barriers  = {});

    // Dynamic rendering
    void begin_rendering(const VkRenderingInfo& rendering_info);
    void end_rendering();

    // Pipeline / descriptor state
    void bind_pipeline(VkPipelineBindPoint bind_point, VkPipeline pipeline);
    void bind_descriptor_sets(VkPipelineBindPoint bind_point,
                               VkPipelineLayout layout,
                               const std::vector<VkDescriptorSet>& sets,
                               uint32_t first_set = 0);
    void push_constants(VkPipelineLayout layout, VkShaderStageFlags stages,
                        uint32_t offset, uint32_t size, const void* data);

    // Viewport / scissor
    void set_viewport(const VkViewport& vp);
    void set_scissor(const VkRect2D& sc);

    // Draw / dispatch
    void draw(uint32_t vertex_count, uint32_t instance_count = 1,
              uint32_t first_vertex = 0, uint32_t first_instance = 0);
    void draw_indexed(uint32_t index_count, uint32_t instance_count = 1,
                      uint32_t first_index = 0, int32_t vertex_offset = 0,
                      uint32_t first_instance = 0);
    void draw_indexed_indirect(VkBuffer       buffer,
                               VkDeviceSize   offset,
                               uint32_t       draw_count,
                               uint32_t       stride);
    void dispatch(uint32_t group_x, uint32_t group_y, uint32_t group_z);

    // Buffer ops
    void copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
    void bind_vertex_buffer(VkBuffer buffer, VkDeviceSize offset = 0);
    void bind_index_buffer(VkBuffer buffer, VkDeviceSize offset = 0,
                           VkIndexType index_type = VK_INDEX_TYPE_UINT32);

    // Clear operations
    void clear_color_image(VkImage image, VkImageLayout layout,
                            const VkClearColorValue& color,
                            const VkImageSubresourceRange& range);

private:
    VkCommandBuffer cmd_;
};

} // namespace gw::render::frame_graph
