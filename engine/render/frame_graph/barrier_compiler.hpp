#pragma once

#include "types.hpp"
#include "error.hpp"
#include <volk.h>
#include <vector>
#include <unordered_map>

namespace gw::render::frame_graph {

// Resource state tracking for barrier insertion (Week 026)
struct ResourceState {
    VkImageLayout image_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkAccessFlags2 access_flags = VK_ACCESS_2_NONE;
    VkPipelineStageFlags2 pipeline_stage = VK_PIPELINE_STAGE_2_NONE;
    
    // For images
    VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
    uint32_t base_mip_level = 0;
    uint32_t level_count = 1;
    uint32_t base_array_layer = 0;
    uint32_t layer_count = 1;
    
    // For buffers
    VkDeviceSize offset = 0;
    VkDeviceSize size = VK_WHOLE_SIZE;
    
    bool is_image = false;
    bool initialized = false;
};

// Barrier batch for a single pass
struct PassBarriers {
    std::vector<VkImageMemoryBarrier2> image_barriers;
    std::vector<VkBufferMemoryBarrier2> buffer_barriers;
    std::vector<VkMemoryBarrier2> memory_barriers;
};

// Barrier compiler - generates synchronization2 barriers between passes
class BarrierCompiler {
public:
    explicit BarrierCompiler() = default;
    ~BarrierCompiler() = default;
    
    // Non-copyable, movable
    BarrierCompiler(const BarrierCompiler&) = delete;
    BarrierCompiler& operator=(const BarrierCompiler&) = delete;
    BarrierCompiler(BarrierCompiler&&) = default;
    BarrierCompiler& operator=(BarrierCompiler&&) = default;
    
    // Main compilation function
    Result<std::vector<PassBarriers>> compile(
        const std::vector<PassData>& passes,
        const std::vector<ResourceData>& resources,
        const std::vector<PassHandle>& execution_order);
    
    // Get final resource states after compilation
    const std::unordered_map<ResourceHandle, ResourceState>& get_final_states() const {
        return final_states_;
    }
    
private:
    // Resource state tracking
    std::unordered_map<ResourceHandle, ResourceState> current_states_;
    std::unordered_map<ResourceHandle, ResourceState> final_states_;
    
    // Helper functions
    Result<std::monostate> initialize_resource_states(
        const std::vector<ResourceData>& resources);
    
    Result<PassBarriers> compile_pass_barriers(
        const PassData& pass,
        PassHandle pass_handle,
        const std::vector<ResourceData>& resources);

    Result<std::monostate> update_resource_states(
        const PassData& pass,
        const std::vector<ResourceData>& resources);

    // State transition helpers
    VkImageMemoryBarrier2 create_image_barrier(
        ResourceHandle res_handle,
        const ResourceState& old_state,
        const ResourceState& new_state,
        const ResourceData& res_data);

    VkBufferMemoryBarrier2 create_buffer_barrier(
        ResourceHandle res_handle,
        const ResourceState& old_state,
        const ResourceState& new_state,
        const ResourceData& res_data);

    // Determine required states for read/write access patterns.
    // queue_type drives the pipeline stage for reads (compute vs fragment).
    ResourceState get_required_read_state(const ResourceData& res_data,
                                          QueueType queue_type) const;
    ResourceState get_required_write_state(const ResourceData& res_data) const;
};

} // namespace gw::render::frame_graph
