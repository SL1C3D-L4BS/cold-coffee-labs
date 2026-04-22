#include "command_buffer.hpp"

namespace gw::render::frame_graph {

void CommandBuffer::pipeline_barrier2(
        VkDependencyFlags                          dependency_flags,
        const std::vector<VkImageMemoryBarrier2>&  image_barriers,
        const std::vector<VkBufferMemoryBarrier2>& buffer_barriers,
        const std::vector<VkMemoryBarrier2>&       memory_barriers) {
    if (!cmd_) return;

    VkDependencyInfo di{};
    di.sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    di.dependencyFlags          = dependency_flags;
    di.memoryBarrierCount       = static_cast<uint32_t>(memory_barriers.size());
    di.pMemoryBarriers          = memory_barriers.empty()  ? nullptr : memory_barriers.data();
    di.bufferMemoryBarrierCount = static_cast<uint32_t>(buffer_barriers.size());
    di.pBufferMemoryBarriers    = buffer_barriers.empty()  ? nullptr : buffer_barriers.data();
    di.imageMemoryBarrierCount  = static_cast<uint32_t>(image_barriers.size());
    di.pImageMemoryBarriers     = image_barriers.empty()   ? nullptr : image_barriers.data();

    vkCmdPipelineBarrier2(cmd_, &di);
}

void CommandBuffer::begin_rendering(const VkRenderingInfo& rendering_info) {
    if (cmd_) vkCmdBeginRendering(cmd_, &rendering_info);
}

void CommandBuffer::end_rendering() {
    if (cmd_) vkCmdEndRendering(cmd_);
}

void CommandBuffer::bind_pipeline(VkPipelineBindPoint bind_point, VkPipeline pipeline) {
    if (cmd_) vkCmdBindPipeline(cmd_, bind_point, pipeline);
}

void CommandBuffer::bind_descriptor_sets(VkPipelineBindPoint bind_point,
                                          VkPipelineLayout layout,
                                          const std::vector<VkDescriptorSet>& sets,
                                          uint32_t first_set) {
    if (cmd_) {
        vkCmdBindDescriptorSets(cmd_, bind_point, layout, first_set,
                                static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
    }
}

void CommandBuffer::push_constants(VkPipelineLayout layout, VkShaderStageFlags stages,
                                    uint32_t offset, uint32_t size, const void* data) {
    if (cmd_) vkCmdPushConstants(cmd_, layout, stages, offset, size, data);
}

void CommandBuffer::set_viewport(const VkViewport& vp) {
    if (cmd_) vkCmdSetViewport(cmd_, 0, 1, &vp);
}

void CommandBuffer::set_scissor(const VkRect2D& sc) {
    if (cmd_) vkCmdSetScissor(cmd_, 0, 1, &sc);
}

void CommandBuffer::draw(uint32_t vertex_count, uint32_t instance_count,
                          uint32_t first_vertex, uint32_t first_instance) {
    if (cmd_) vkCmdDraw(cmd_, vertex_count, instance_count, first_vertex, first_instance);
}

void CommandBuffer::draw_indexed(uint32_t index_count, uint32_t instance_count,
                                  uint32_t first_index, int32_t vertex_offset,
                                  uint32_t first_instance) {
    if (cmd_) {
        vkCmdDrawIndexed(cmd_, index_count, instance_count, first_index,
                         vertex_offset, first_instance);
    }
}

void CommandBuffer::draw_indexed_indirect(VkBuffer       buffer,
                                          VkDeviceSize   offset,
                                          uint32_t       draw_count,
                                          uint32_t       stride) {
    if (cmd_) {
        vkCmdDrawIndexedIndirect(cmd_, buffer, offset, draw_count, stride);
    }
}

void CommandBuffer::dispatch(uint32_t gx, uint32_t gy, uint32_t gz) {
    if (cmd_) vkCmdDispatch(cmd_, gx, gy, gz);
}

void CommandBuffer::copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
    if (!cmd_) return;
    VkBufferCopy region{0, 0, size};
    vkCmdCopyBuffer(cmd_, src, dst, 1, &region);
}

void CommandBuffer::bind_vertex_buffer(VkBuffer buffer, VkDeviceSize offset) {
    if (cmd_) vkCmdBindVertexBuffers(cmd_, 0, 1, &buffer, &offset);
}

void CommandBuffer::bind_index_buffer(VkBuffer buffer, VkDeviceSize offset,
                                       VkIndexType index_type) {
    if (cmd_) vkCmdBindIndexBuffer(cmd_, buffer, offset, index_type);
}

void CommandBuffer::clear_color_image(VkImage image, VkImageLayout layout,
                                       const VkClearColorValue& color,
                                       const VkImageSubresourceRange& range) {
    if (cmd_) vkCmdClearColorImage(cmd_, image, layout, &color, 1, &range);
}

} // namespace gw::render::frame_graph
