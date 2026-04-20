#include "command_buffer.hpp"
#include "../hal/vulkan_command_buffer.hpp"
#include <stdexcept>

namespace gw::render::frame_graph {

CommandBuffer::CommandBuffer(::gw::render::hal::VulkanCommandBuffer& vk_cmd_buffer)
    : vk_cmd_buffer_(vk_cmd_buffer) {
}

void CommandBuffer::pipeline_barrier2(
    VkDependencyFlags dependency_flags,
    const std::vector<VkImageMemoryBarrier2>& image_barriers,
    const std::vector<VkBufferMemoryBarrier2>& buffer_barriers,
    const std::vector<VkMemoryBarrier2>& memory_barriers) {
    
    // Create dependency info for synchronization2
    VkDependencyInfo dependency_info{};
    dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependency_info.dependencyFlags = dependency_flags;
    
    dependency_info.memoryBarrierCount = static_cast<uint32_t>(memory_barriers.size());
    dependency_info.pMemoryBarriers = memory_barriers.empty() ? nullptr : memory_barriers.data();
    
    dependency_info.bufferMemoryBarrierCount = static_cast<uint32_t>(buffer_barriers.size());
    dependency_info.pBufferMemoryBarriers = buffer_barriers.empty() ? nullptr : buffer_barriers.data();
    
    dependency_info.imageMemoryBarrierCount = static_cast<uint32_t>(image_barriers.size());
    dependency_info.pImageMemoryBarriers = image_barriers.empty() ? nullptr : image_barriers.data();
    
    // Cast to VkCommandBuffer for the Vulkan call
    VkCommandBuffer cmd_buffer = static_cast<VkCommandBuffer>(vk_cmd_buffer_.native_handle());
    
    // Use vkCmdPipelineBarrier2 if available (synchronization2 extension)
    // For now, fall back to regular pipeline barrier
    // TODO: Check for VK_KHR_synchronization2 support in Week 026
    vkCmdPipelineBarrier2KHR(cmd_buffer, &dependency_info);
}

void CommandBuffer::copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
    VkBufferCopy copy_region{};
    copy_region.srcOffset = 0;
    copy_region.dstOffset = 0;
    copy_region.size = size;
    
    VkCommandBuffer cmd_buffer = static_cast<VkCommandBuffer>(vk_cmd_buffer_.native_handle());
    vkCmdCopyBuffer(cmd_buffer, src, dst, 1, &copy_region);
}

void CommandBuffer::begin_render_pass(VkRenderPass render_pass, VkFramebuffer framebuffer, 
                                     VkExtent2D extent, const std::vector<VkClearValue>& clear_values) {
    VkRenderPassBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    begin_info.renderPass = render_pass;
    begin_info.framebuffer = framebuffer;
    begin_info.renderArea.offset = {0, 0};
    begin_info.renderArea.extent = extent;
    begin_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
    begin_info.pClearValues = clear_values.empty() ? nullptr : clear_values.data();
    
    VkCommandBuffer cmd_buffer = static_cast<VkCommandBuffer>(vk_cmd_buffer_.native_handle());
    vkCmdBeginRenderPass(cmd_buffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

void CommandBuffer::end_render_pass() {
    VkCommandBuffer cmd_buffer = static_cast<VkCommandBuffer>(vk_cmd_buffer_.native_handle());
    vkCmdEndRenderPass(cmd_buffer);
}

void CommandBuffer::bind_pipeline(VkPipeline pipeline) {
    VkCommandBuffer cmd_buffer = static_cast<VkCommandBuffer>(vk_cmd_buffer_.native_handle());
    vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

void CommandBuffer::bind_descriptor_sets(VkPipelineLayout layout, const std::vector<VkDescriptorSet>& sets,
                                        uint32_t first_set) {
    VkCommandBuffer cmd_buffer = static_cast<VkCommandBuffer>(vk_cmd_buffer_.native_handle());
    vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, first_set,
                           static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
}

void CommandBuffer::set_viewport(uint32_t first_viewport, const std::vector<VkViewport>& viewports) {
    VkCommandBuffer cmd_buffer = static_cast<VkCommandBuffer>(vk_cmd_buffer_.native_handle());
    vkCmdSetViewport(cmd_buffer, first_viewport, static_cast<uint32_t>(viewports.size()), viewports.data());
}

void CommandBuffer::set_scissor(uint32_t first_scissor, const std::vector<VkRect2D>& scissors) {
    VkCommandBuffer cmd_buffer = static_cast<VkCommandBuffer>(vk_cmd_buffer_.native_handle());
    vkCmdSetScissor(cmd_buffer, first_scissor, static_cast<uint32_t>(scissors.size()), scissors.data());
}

void CommandBuffer::draw(uint32_t vertex_count, uint32_t instance_count, 
                         uint32_t first_vertex, uint32_t first_instance) {
    VkCommandBuffer cmd_buffer = static_cast<VkCommandBuffer>(vk_cmd_buffer_.native_handle());
    vkCmdDraw(cmd_buffer, vertex_count, instance_count, first_vertex, first_instance);
}

void CommandBuffer::draw_indexed(uint32_t index_count, uint32_t instance_count,
                                uint32_t first_index, int32_t vertex_offset, uint32_t first_instance) {
    VkCommandBuffer cmd_buffer = static_cast<VkCommandBuffer>(vk_cmd_buffer_.native_handle());
    vkCmdDrawIndexed(cmd_buffer, index_count, instance_count, first_index, vertex_offset, first_instance);
}

void CommandBuffer::dispatch(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z) {
    VkCommandBuffer cmd_buffer = static_cast<VkCommandBuffer>(vk_cmd_buffer_.native_handle());
    vkCmdDispatch(cmd_buffer, group_count_x, group_count_y, group_count_z);
}

void CommandBuffer::bind_vertex_buffer(VkBuffer buffer, VkDeviceSize offset) {
    VkCommandBuffer cmd_buffer = static_cast<VkCommandBuffer>(vk_cmd_buffer_.native_handle());
    VkDeviceSize offsets[] = {offset};
    vkCmdBindVertexBuffers(cmd_buffer, 0, 1, &buffer, offsets);
}

void CommandBuffer::bind_index_buffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType index_type) {
    VkCommandBuffer cmd_buffer = static_cast<VkCommandBuffer>(vk_cmd_buffer_.native_handle());
    vkCmdBindIndexBuffer(cmd_buffer, buffer, offset, index_type);
}

} // namespace gw::render::frame_graph
