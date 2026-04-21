#include "barrier_compiler.hpp"
#include <expected>
#include <stdexcept>

namespace gw::render::frame_graph {

Result<std::vector<PassBarriers>> BarrierCompiler::compile(
    const std::vector<PassData>& passes,
    const std::vector<ResourceData>& resources,
    const std::vector<PassHandle>& execution_order) {
    
    // Initialize resource states
    auto init_result = initialize_resource_states(resources);
    if (!init_result) {
        return std::unexpected(init_result.error());
    }
    
    std::vector<PassBarriers> all_barriers;
    all_barriers.reserve(passes.size());
    
    // Compile barriers for each pass in execution order
    for (PassHandle pass_handle : execution_order) {
        const auto& pass = passes[pass_handle];
        
        auto barriers_result = compile_pass_barriers(pass, pass_handle, resources);
        if (!barriers_result) {
            return std::unexpected(barriers_result.error());
        }
        
        all_barriers.push_back(std::move(barriers_result.value()));
        
        // Update resource states after this pass
        auto update_result = update_resource_states(pass, resources);
        if (!update_result) {
            return std::unexpected(update_result.error());
        }
    }
    
    // Store final states
    final_states_ = current_states_;
    
    return all_barriers;
}

Result<std::monostate> BarrierCompiler::initialize_resource_states(
    const std::vector<ResourceData>& resources) {
    
    current_states_.clear();
    
    for (size_t i = 0; i < resources.size(); ++i) {
        ResourceHandle handle = static_cast<ResourceHandle>(i);
        const auto& res_data = resources[i];
        
        ResourceState state{};
        
        if (std::holds_alternative<TextureDesc>(res_data.desc)) {
            const auto& tex_desc = std::get<TextureDesc>(res_data.desc);
            state.is_image = true;
            state.aspect_mask = tex_desc.is_depth() ? 
                VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
            state.base_mip_level = 0;
            state.level_count = tex_desc.mip_levels;
            state.base_array_layer = 0;
            state.layer_count = tex_desc.array_layers;
            
            // Initial layout depends on usage
            if (tex_desc.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                state.image_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            } else if (tex_desc.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
                state.image_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            } else {
                state.image_layout = VK_IMAGE_LAYOUT_UNDEFINED;
            }
        } else {
            const auto& buf_desc = std::get<BufferDesc>(res_data.desc);
            state.is_image = false;
            state.offset = 0;
            state.size = buf_desc.size;
        }
        
        current_states_[handle] = state;
    }
    
    return std::monostate{};
}

Result<PassBarriers> BarrierCompiler::compile_pass_barriers(
    const PassData& pass,
    PassHandle pass_handle,
    const std::vector<ResourceData>& resources) {
    
    PassBarriers barriers;
    
    // Process read resources - need to ensure they're in readable state
    for (ResourceHandle read_res : pass.desc.reads) {
        if (!current_states_.count(read_res)) {
            auto err = make_invalid_handle_error("read resource", read_res);
            return std::unexpected(err.error());
        }

        const auto& res_data = resources[read_res];
        const auto& current_state = current_states_[read_res];
        auto required_state = get_required_read_state(res_data, pass.desc.queue);
        
        // If current state doesn't match required, create barrier
        if (current_state.image_layout != required_state.image_layout ||
            current_state.access_flags != required_state.access_flags ||
            current_state.pipeline_stage != required_state.pipeline_stage) {
            
            if (required_state.is_image) {
                auto barrier = create_image_barrier(read_res, current_state, required_state, res_data);
                barriers.image_barriers.push_back(barrier);
            } else {
                auto barrier = create_buffer_barrier(read_res, current_state, required_state, res_data);
                barriers.buffer_barriers.push_back(barrier);
            }
        }
    }
    
    // Process write resources - need to ensure they're in writable state
    for (ResourceHandle write_res : pass.desc.writes) {
        if (!current_states_.count(write_res)) {
            auto err = make_invalid_handle_error("write resource", write_res);
            return std::unexpected(err.error());
        }
        
        const auto& res_data = resources[write_res];
        const auto& current_state = current_states_[write_res];
        auto required_state = get_required_write_state(res_data);
        
        // If current state doesn't match required, create barrier
        if (current_state.image_layout != required_state.image_layout ||
            current_state.access_flags != required_state.access_flags ||
            current_state.pipeline_stage != required_state.pipeline_stage) {
            
            if (required_state.is_image) {
                auto barrier = create_image_barrier(write_res, current_state, required_state, res_data);
                barriers.image_barriers.push_back(barrier);
            } else {
                auto barrier = create_buffer_barrier(write_res, current_state, required_state, res_data);
                barriers.buffer_barriers.push_back(barrier);
            }
        }
    }
    
    return barriers;
}

Result<std::monostate> BarrierCompiler::update_resource_states(
    const PassData& pass,
    const std::vector<ResourceData>& resources) {
    
    // Update states for written resources
    for (ResourceHandle write_res : pass.desc.writes) {
        if (!current_states_.count(write_res)) {
            return make_invalid_handle_error("write resource", write_res);
        }
        
        const auto& res_data = resources[write_res];
        current_states_[write_res] = get_required_write_state(res_data);
    }
    
    return std::monostate{};
}

VkImageMemoryBarrier2 BarrierCompiler::create_image_barrier(
    ResourceHandle res_handle,
    const ResourceState& old_state,
    const ResourceState& new_state,
    const ResourceData& res_data) {
    
    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.pNext = nullptr;
    barrier.srcStageMask = old_state.pipeline_stage;
    barrier.srcAccessMask = old_state.access_flags;
    barrier.dstStageMask = new_state.pipeline_stage;
    barrier.dstAccessMask = new_state.access_flags;
    barrier.oldLayout = old_state.image_layout;
    barrier.newLayout = new_state.image_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;  // Week 027 will handle this
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = VK_NULL_HANDLE;  // Will be filled by actual resource binding
    barrier.subresourceRange.aspectMask = new_state.aspect_mask;
    barrier.subresourceRange.baseMipLevel = new_state.base_mip_level;
    barrier.subresourceRange.levelCount = new_state.level_count;
    barrier.subresourceRange.baseArrayLayer = new_state.base_array_layer;
    barrier.subresourceRange.layerCount = new_state.layer_count;
    
    return barrier;
}

VkBufferMemoryBarrier2 BarrierCompiler::create_buffer_barrier(
    ResourceHandle res_handle,
    const ResourceState& old_state,
    const ResourceState& new_state,
    const ResourceData& res_data) {
    
    VkBufferMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
    barrier.pNext = nullptr;
    barrier.srcStageMask = old_state.pipeline_stage;
    barrier.srcAccessMask = old_state.access_flags;
    barrier.dstStageMask = new_state.pipeline_stage;
    barrier.dstAccessMask = new_state.access_flags;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;  // Week 027 will handle this
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.buffer = VK_NULL_HANDLE;  // Will be filled by actual resource binding
    barrier.offset = new_state.offset;
    barrier.size = new_state.size;
    
    return barrier;
}

ResourceState BarrierCompiler::get_required_read_state(const ResourceData& res_data,
                                                        QueueType queue_type) const {
    ResourceState state{};

    const bool is_compute = (queue_type == QueueType::Compute);

    if (std::holds_alternative<TextureDesc>(res_data.desc)) {
        const auto& tex_desc = std::get<TextureDesc>(res_data.desc);
        state.is_image = true;
        state.aspect_mask = tex_desc.is_depth() ?
            VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        state.base_mip_level = 0;
        state.level_count = tex_desc.mip_levels;
        state.base_array_layer = 0;
        state.layer_count = tex_desc.array_layers;

        if (tex_desc.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            state.image_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            state.access_flags = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            state.pipeline_stage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
        } else if (tex_desc.usage & VK_IMAGE_USAGE_SAMPLED_BIT) {
            state.image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            state.access_flags = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
            state.pipeline_stage = is_compute
                ? VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT
                : VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        } else {
            // Storage image or other: use GENERAL layout
            state.image_layout = VK_IMAGE_LAYOUT_GENERAL;
            state.access_flags = VK_ACCESS_2_SHADER_READ_BIT;
            state.pipeline_stage = is_compute
                ? VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT
                : VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        }
    } else {
        const auto& buf_desc = std::get<BufferDesc>(res_data.desc);
        state.is_image = false;
        state.offset = 0;
        state.size = buf_desc.size;

        if (buf_desc.usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) {
            state.access_flags = VK_ACCESS_2_UNIFORM_READ_BIT;
            state.pipeline_stage = is_compute
                ? VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT
                : (VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
                   VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        } else if (buf_desc.usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) {
            state.access_flags = VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
            state.pipeline_stage = is_compute
                ? VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT
                : VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        } else {
            state.access_flags = VK_ACCESS_2_TRANSFER_READ_BIT;
            state.pipeline_stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        }
    }

    return state;
}

ResourceState BarrierCompiler::get_required_write_state(const ResourceData& res_data) const {
    ResourceState state{};
    
    if (std::holds_alternative<TextureDesc>(res_data.desc)) {
        const auto& tex_desc = std::get<TextureDesc>(res_data.desc);
        state.is_image = true;
        state.aspect_mask = tex_desc.is_depth() ? 
            VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        state.base_mip_level = 0;
        state.level_count = tex_desc.mip_levels;
        state.base_array_layer = 0;
        state.layer_count = tex_desc.array_layers;
        
        // Determine layout and access based on usage
        if (tex_desc.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            state.image_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            state.access_flags = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            state.pipeline_stage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                                 VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
        } else if (tex_desc.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
            state.image_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            state.access_flags = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
            state.pipeline_stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        } else if (tex_desc.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
            state.image_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            state.access_flags = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            state.pipeline_stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        } else {
            state.image_layout = VK_IMAGE_LAYOUT_GENERAL;
            state.access_flags = VK_ACCESS_2_SHADER_WRITE_BIT;
            state.pipeline_stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
        }
    } else {
        const auto& buf_desc = std::get<BufferDesc>(res_data.desc);
        state.is_image = false;
        state.offset = 0;
        state.size = buf_desc.size;
        
        // Determine access based on usage
        if (buf_desc.usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) {
            state.access_flags = VK_ACCESS_2_SHADER_WRITE_BIT;
            state.pipeline_stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
        } else if (buf_desc.usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT) {
            state.access_flags = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            state.pipeline_stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        } else {
            state.access_flags = VK_ACCESS_2_SHADER_WRITE_BIT;
            state.pipeline_stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
        }
    }
    
    return state;
}

} // namespace gw::render::frame_graph
