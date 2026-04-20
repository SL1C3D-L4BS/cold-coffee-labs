#pragma once

#include "types.hpp"
#include "../hal/vulkan_command_buffer.hpp"
#include <volk.h>

namespace gw::render::frame_graph {

// Abstract command buffer interface for frame graph passes
class CommandBuffer {
public:
    explicit CommandBuffer(::gw::render::hal::VulkanCommandBuffer& vk_cmd_buffer);
    ~CommandBuffer() = default;
    
    // Non-copyable, movable
    CommandBuffer(const CommandBuffer&) = delete;
    CommandBuffer& operator=(const CommandBuffer&) = delete;
    CommandBuffer(CommandBuffer&&) noexcept = default;
    CommandBuffer& operator=(CommandBuffer&&) = delete;
    
    // Pipeline barrier with synchronization2 (Week 026)
    void pipeline_barrier2(
        VkDependencyFlags dependency_flags,
        const std::vector<VkImageMemoryBarrier2>& image_barriers = {},
        const std::vector<VkBufferMemoryBarrier2>& buffer_barriers = {},
        const std::vector<VkMemoryBarrier2>& memory_barriers = {});
    
    // Basic operations
    void copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
    void begin_render_pass(VkRenderPass render_pass, VkFramebuffer framebuffer, 
                          VkExtent2D extent, const std::vector<VkClearValue>& clear_values = {});
    void end_render_pass();
    void bind_pipeline(VkPipeline pipeline);
    void bind_descriptor_sets(VkPipelineLayout layout, const std::vector<VkDescriptorSet>& sets,
                             uint32_t first_set = 0);
    void set_viewport(uint32_t first_viewport, const std::vector<VkViewport>& viewports);
    void set_scissor(uint32_t first_scissor, const std::vector<VkRect2D>& scissors);
    void draw(uint32_t vertex_count, uint32_t instance_count = 1, 
              uint32_t first_vertex = 0, uint32_t first_instance = 0);
    void draw_indexed(uint32_t index_count, uint32_t instance_count = 1,
                      uint32_t first_index = 0, int32_t vertex_offset = 0, uint32_t first_instance = 0);
    void dispatch(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z);
    
    // Resource binding helpers (Week 026+)
    void bind_vertex_buffer(VkBuffer buffer, VkDeviceSize offset = 0);
    void bind_index_buffer(VkBuffer buffer, VkDeviceSize offset = 0, VkIndexType index_type = VK_INDEX_TYPE_UINT32);
    
private:
    ::gw::render::hal::VulkanCommandBuffer& vk_cmd_buffer_;
};

} // namespace gw::render::frame_graph
