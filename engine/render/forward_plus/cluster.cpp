#include "cluster.hpp"
#include "../frame_graph/error.hpp"
#include "../frame_graph/frame_graph.hpp"
#include <algorithm>
#include <cmath>
#include <expected>

namespace gw::render::forward_plus {

using gw::render::frame_graph::BufferDesc;
using gw::render::frame_graph::CommandBuffer;
using gw::render::frame_graph::PassDesc;
using gw::render::frame_graph::QueueType;
using gw::render::frame_graph::FrameGraphError;
using gw::render::frame_graph::FrameGraphErrorType;
using gw::render::frame_graph::ResourceLifetime;

// Math utility functions
float dot(const float3& a, const float3& b) {
    return a.x() * b.x() + a.y() * b.y() + a.z() * b.z();
}

float length(const float3& v) {
    return std::sqrt(v.x() * v.x() + v.y() * v.y() + v.z() * v.z());
}

float3 normalize(const float3& v) {
    float len = length(v);
    if (len > 0.0f) {
        return float3(v.x() / len, v.y() / len, v.z() / len);
    }
    return float3(0.0f, 0.0f, 0.0f);
}

float4 mul(const float4x4& m, const float4& v) {
    return m * v;
}

float3 mul(const float4x4& m, const float3& v) {
    float4 result = m * float4(v, 1.0f);
    return float3(result.x() / result.w(), result.y() / result.w(), result.z() / result.w());
}

ClusteredLighting::ClusteredLighting(VkDevice device, VmaAllocator allocator)
    : device_(device)
    , allocator_(allocator) {
}

ClusteredLighting::~ClusteredLighting() {
    destroy_compute_resources();
    destroy_graphics_resources();
    destroy_buffers();
}

Result<std::monostate> ClusteredLighting::initialize(const ClusterConfig& config) {
    config_ = config;
    
    // Initialize light grid
    light_grid_.resize(config_.total_cluster_count());
    light_grid_.clear();
    
    // Create resources
    auto buffer_result = create_buffers();
    if (!buffer_result) {
        return std::unexpected(buffer_result.error());
    }
    
    auto compute_result = create_compute_resources();
    if (!compute_result) {
        return std::unexpected(compute_result.error());
    }
    
    auto graphics_result = create_graphics_resources();
    if (!graphics_result) {
        return std::unexpected(graphics_result.error());
    }
    
    return std::monostate{};
}

void ClusteredLighting::set_lights(const std::vector<Light>& lights) {
    lights_ = lights;
}

Result<ResourceHandle> ClusteredLighting::create_light_grid_buffer() {
    BufferDesc desc{};
    desc.size = config_.total_cluster_count() * sizeof(uint32_t) * 2;  // offset + count
    desc.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    desc.lifetime = ResourceLifetime::Transient;
    desc.name = "light_grid_buffer";
    
    // This would be called from FrameGraph context
    return ResourceHandle{0};  // Placeholder
}

Result<ResourceHandle> ClusteredLighting::create_light_list_buffer() {
    BufferDesc desc{};
    desc.size = lights_.size() * sizeof(uint32_t);  // Max light indices
    desc.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    desc.lifetime = ResourceLifetime::Transient;
    desc.name = "light_list_buffer";
    
    return ResourceHandle{1};  // Placeholder
}

Result<ResourceHandle> ClusteredLighting::create_cluster_aabb_buffer() {
    BufferDesc desc{};
    desc.size = config_.total_cluster_count() * sizeof(ClusterAABB);
    desc.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    desc.lifetime = ResourceLifetime::Transient;
    desc.name = "cluster_aabb_buffer";
    
    return ResourceHandle{2};  // Placeholder
}

Result<PassHandle> ClusteredLighting::create_light_culling_pass(FrameGraph& frame_graph) {
    // Create compute pass for light culling
    PassDesc pass("LightCulling");
    pass.queue = QueueType::Compute;
    
    // Pass reads: light data, cluster AABBs, camera matrices
    // Pass writes: light grid, light index list
    
    pass.execute = [this](CommandBuffer& cmd) {
        // Dispatch one thread per cluster
        uint32_t group_x = (config_.cluster_count_x + 7) / 8;
        uint32_t group_y = (config_.cluster_count_y + 7) / 8;
        uint32_t group_z = (config_.cluster_count_z + 7) / 8;
        
        cmd.dispatch(group_x, group_y, group_z);
    };
    
    return frame_graph.add_pass(std::move(pass));
}

Result<PassHandle> ClusteredLighting::create_forward_pass(FrameGraph& frame_graph) {
    // Create graphics pass for forward+ rendering
    PassDesc pass("ForwardPlus");
    pass.queue = QueueType::Graphics;
    
    // Pass reads: light grid, light index list, material data
    // Pass writes: color target, depth target
    
    pass.execute = [](CommandBuffer& cmd) {
        // Standard forward rendering with clustered light lookup
        // cmd.draw(...);
    };
    
    return frame_graph.add_pass(std::move(pass));
}

void ClusteredLighting::dump_cluster_info() const {
    (void)config_;
    (void)lights_;
    (void)light_grid_;
    // Hook for GW_LOG / render-debug overlay (Phase 5 instrumentation).
}

Result<std::monostate> ClusteredLighting::create_compute_resources() {
    // Create descriptor set layout for compute shader
    std::vector<VkDescriptorSetLayoutBinding> bindings = {
        // Binding 0: Light data
        {
            0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
            VK_SHADER_STAGE_COMPUTE_BIT, nullptr
        },
        // Binding 1: Cluster AABBs
        {
            1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
            VK_SHADER_STAGE_COMPUTE_BIT, nullptr
        },
        // Binding 2: Light grid (offsets + counts)
        {
            2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
            VK_SHADER_STAGE_COMPUTE_BIT, nullptr
        },
        // Binding 3: Light index list
        {
            3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
            VK_SHADER_STAGE_COMPUTE_BIT, nullptr
        }
    };
    
    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
    layout_info.pBindings = bindings.data();
    
    VkResult result = vkCreateDescriptorSetLayout(device_, &layout_info, nullptr, 
                                                &compute_descriptor_set_layout_);
    if (result != VK_SUCCESS) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed,
            "Failed to create compute descriptor set layout"
        });
    }
    
    // Create pipeline layout
    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &compute_descriptor_set_layout_;
    
    result = vkCreatePipelineLayout(device_, &pipeline_layout_info, nullptr, 
                                 &compute_pipeline_layout_);
    if (result != VK_SUCCESS) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed,
            "Failed to create compute pipeline layout"
        });
    }
    
    // TODO: Create compute pipeline with shader manager
    // This would use the light_culling.comp.hlsl shader
    
    return std::monostate{};
}

Result<std::monostate> ClusteredLighting::create_graphics_resources() {
    // Create descriptor set layout for forward+ shader
    std::vector<VkDescriptorSetLayoutBinding> bindings = {
        // Binding 0: Camera matrices
        {
            0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr
        },
        // Binding 1: Light grid
        {
            1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
            VK_SHADER_STAGE_FRAGMENT_BIT, nullptr
        },
        // Binding 2: Light index list
        {
            2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
            VK_SHADER_STAGE_FRAGMENT_BIT, nullptr
        },
        // Binding 3: Material textures
        {
            3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4,
            VK_SHADER_STAGE_FRAGMENT_BIT, nullptr
        }
    };
    
    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
    layout_info.pBindings = bindings.data();
    
    VkResult result = vkCreateDescriptorSetLayout(device_, &layout_info, nullptr, 
                                                &graphics_descriptor_set_layout_);
    if (result != VK_SUCCESS) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed,
            "Failed to create graphics descriptor set layout"
        });
    }
    
    // Create pipeline layout
    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &graphics_descriptor_set_layout_;
    
    result = vkCreatePipelineLayout(device_, &pipeline_layout_info, nullptr, 
                                 &graphics_pipeline_layout_);
    if (result != VK_SUCCESS) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed,
            "Failed to create graphics pipeline layout"
        });
    }
    
    return std::monostate{};
}

Result<std::monostate> ClusteredLighting::create_buffers() {
    // Create light grid buffer
    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = config_.total_cluster_count() * sizeof(uint32_t) * 2;
    buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | 
                       VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    
    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    
    VkResult result = vmaCreateBuffer(allocator_, &buffer_info, &alloc_info,
                                     &light_grid_buffer_, &light_grid_allocation_, nullptr);
    if (result != VK_SUCCESS) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed,
            "Failed to create light grid buffer"
        });
    }
    
    // Create light list buffer
    buffer_info.size = lights_.size() * sizeof(uint32_t) * 8;  // Max 8 lights per cluster estimate
    result = vmaCreateBuffer(allocator_, &buffer_info, &alloc_info,
                            &light_list_buffer_, &light_list_allocation_, nullptr);
    if (result != VK_SUCCESS) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed,
            "Failed to create light list buffer"
        });
    }
    
    // Create cluster AABB buffer
    buffer_info.size = config_.total_cluster_count() * sizeof(ClusterAABB);
    result = vmaCreateBuffer(allocator_, &buffer_info, &alloc_info,
                            &cluster_aabb_buffer_, &cluster_aabb_allocation_, nullptr);
    if (result != VK_SUCCESS) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed,
            "Failed to create cluster AABB buffer"
        });
    }
    
    return std::monostate{};
}

void ClusteredLighting::destroy_compute_resources() {
    if (compute_descriptor_set_layout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device_, compute_descriptor_set_layout_, nullptr);
        compute_descriptor_set_layout_ = VK_NULL_HANDLE;
    }
    
    if (compute_pipeline_layout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device_, compute_pipeline_layout_, nullptr);
        compute_pipeline_layout_ = VK_NULL_HANDLE;
    }
    
    if (compute_pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device_, compute_pipeline_, nullptr);
        compute_pipeline_ = VK_NULL_HANDLE;
    }
}

void ClusteredLighting::destroy_graphics_resources() {
    if (graphics_descriptor_set_layout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device_, graphics_descriptor_set_layout_, nullptr);
        graphics_descriptor_set_layout_ = VK_NULL_HANDLE;
    }
    
    if (graphics_pipeline_layout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device_, graphics_pipeline_layout_, nullptr);
        graphics_pipeline_layout_ = VK_NULL_HANDLE;
    }
}

void ClusteredLighting::destroy_buffers() {
    if (light_grid_buffer_ != VK_NULL_HANDLE) {
        vmaDestroyBuffer(allocator_, light_grid_buffer_, light_grid_allocation_);
        light_grid_buffer_ = VK_NULL_HANDLE;
        light_grid_allocation_ = VK_NULL_HANDLE;
    }
    
    if (light_list_buffer_ != VK_NULL_HANDLE) {
        vmaDestroyBuffer(allocator_, light_list_buffer_, light_list_allocation_);
        light_list_buffer_ = VK_NULL_HANDLE;
        light_list_allocation_ = VK_NULL_HANDLE;
    }
    
    if (cluster_aabb_buffer_ != VK_NULL_HANDLE) {
        vmaDestroyBuffer(allocator_, cluster_aabb_buffer_, cluster_aabb_allocation_);
        cluster_aabb_buffer_ = VK_NULL_HANDLE;
        cluster_aabb_allocation_ = VK_NULL_HANDLE;
    }
}

std::vector<ClusterAABB> ClusteredLighting::compute_cluster_aabbs(
    const float4x4& view_matrix,
    const float4x4& projection_matrix) {
    
    std::vector<ClusterAABB> clusters;
    clusters.reserve(config_.total_cluster_count());
    
    for (uint32_t z = 0; z < config_.cluster_count_z; ++z) {
        for (uint32_t y = 0; y < config_.cluster_count_y; ++y) {
            for (uint32_t x = 0; x < config_.cluster_count_x; ++x) {
                ClusterAABB aabb = compute_cluster_aabb(x, y, z, view_matrix, projection_matrix);
                clusters.push_back(aabb);
            }
        }
    }
    
    return clusters;
}

ClusterAABB ClusteredLighting::compute_cluster_aabb(
    uint32_t x, uint32_t y, uint32_t z,
    const float4x4& view_matrix,
    const float4x4& projection_matrix) {
    
    // Calculate cluster boundaries in screen space
    float x_size = 1.0f / config_.cluster_count_x;
    float y_size = 1.0f / config_.cluster_count_y;
    float z_size = 1.0f / config_.cluster_count_z;
    
    // Convert to clip space and then to view space
    // This is simplified - actual implementation would be more complex
    ClusterAABB aabb{};
    aabb.min_bounds = float3(x * x_size, y * y_size, z * z_size);
    aabb.max_bounds = float3((x + 1) * x_size, (y + 1) * y_size, (z + 1) * z_size);
    
    return aabb;
}

void ClusteredLighting::perform_light_culling(
    const std::vector<ClusterAABB>& clusters,
    const std::vector<Light>& lights) {
    
    light_grid_.clear();
    light_grid_.resize(config_.total_cluster_count());
    
    // For each cluster, find intersecting lights
    for (uint32_t cluster_idx = 0; cluster_idx < clusters.size(); ++cluster_idx) {
        const auto& cluster = clusters[cluster_idx];
        
        uint32_t light_count = 0;
        uint32_t start_offset = static_cast<uint32_t>(light_grid_.light_index_list.size());
        
        // Test each light against cluster
        for (uint32_t light_idx = 0; light_idx < lights.size(); ++light_idx) {
            const auto& light = lights[light_idx];
            
            bool intersects = false;
            switch (light.type) {
                case 0: // Point light
                    intersects = sphere_intersects_aabb(light, cluster);
                    break;
                case 1: // Spot light
                    intersects = spot_intersects_aabb(light, cluster);
                    break;
                case 2: // Directional light
                    // Directional lights affect all clusters
                    intersects = true;
                    break;
            }
            
            if (intersects) {
                light_grid_.light_index_list.push_back(light_idx);
                light_count++;
            }
        }
        
        // Set cluster data
        light_grid_.light_offset_list[cluster_idx] = start_offset;
        light_grid_.light_count_list[cluster_idx] = light_count;
    }
}

bool ClusteredLighting::sphere_intersects_aabb(const Light& light, const ClusterAABB& aabb) {
    // Simple sphere-AABB intersection test
    float3 closest_point = float3(
        std::max(aabb.min_bounds.x(), std::min(light.position.x(), aabb.max_bounds.x())),
        std::max(aabb.min_bounds.y(), std::min(light.position.y(), aabb.max_bounds.y())),
        std::max(aabb.min_bounds.z(), std::min(light.position.z(), aabb.max_bounds.z()))
    );
    
    float3 distance_vec = float3(
        light.position.x() - closest_point.x(),
        light.position.y() - closest_point.y(),
        light.position.z() - closest_point.z()
    );
    float distance_squared = dot(distance_vec, distance_vec);
    
    return distance_squared <= (light.radius * light.radius);
}

bool ClusteredLighting::spot_intersects_aabb(const Light& light, const ClusterAABB& aabb) {
    // For spot lights, first test sphere intersection, then cone test
    if (!sphere_intersects_aabb(light, aabb)) {
        return false;
    }
    
    // Simplified cone test - actual implementation would be more complex
    float3 cluster_center = float3(
        (aabb.min_bounds.x() + aabb.max_bounds.x()) * 0.5f,
        (aabb.min_bounds.y() + aabb.max_bounds.y()) * 0.5f,
        (aabb.min_bounds.z() + aabb.max_bounds.z()) * 0.5f
    );
    float3 light_to_cluster = float3(
        cluster_center.x() - light.position.x(),
        cluster_center.y() - light.position.y(),
        cluster_center.z() - light.position.z()
    );
    float distance = length(light_to_cluster);
    
    if (distance > light.radius) {
        return false;
    }
    
    float3 light_to_cluster_normalized = float3(
        light_to_cluster.x() / distance,
        light_to_cluster.y() / distance,
        light_to_cluster.z() / distance
    );
    float cos_angle = dot(light.direction, light_to_cluster_normalized);
    
    return cos_angle >= light.outer_cone;
}

// Utility functions
float3 screen_to_view_space(float2 screen_pos, float depth, const float4x4& inv_projection) {
    float4 clip_pos = float4(
        screen_pos.x() * 2.0f - 1.0f, 
        screen_pos.y() * 2.0f - 1.0f, 
        depth, 
        1.0f
    );
    float4 view_pos = mul(inv_projection, clip_pos);
    return float3(
        view_pos.x() / view_pos.w(), 
        view_pos.y() / view_pos.w(), 
        view_pos.z() / view_pos.w()
    );
}

float3 view_to_world_space(float3 view_pos, const float4x4& inv_view) {
    float4 world_pos = mul(inv_view, float4(view_pos, 1.0f));
    return float3(
        world_pos.x() / world_pos.w(), 
        world_pos.y() / world_pos.w(), 
        world_pos.z() / world_pos.w()
    );
}

uint32_t get_cluster_index(float3 view_pos, const ClusterConfig& config) {
    // Convert view position to cluster coordinates
    float z_slice = config.cluster_count_z * 
                   (1.0f - linear_depth(view_pos.z(), config.z_near, config.z_far));
    
    uint32_t z = static_cast<uint32_t>(std::floor(z_slice));
    z = std::min(z, config.cluster_count_z - 1);
    
    // X and Y calculation would depend on projection
    // Simplified here - actual implementation would need proper screen space conversion
    uint32_t x = static_cast<uint32_t>(std::floor(view_pos.x()));
    uint32_t y = static_cast<uint32_t>(std::floor(view_pos.y()));
    
    x = std::min(x, config.cluster_count_x - 1);
    y = std::min(y, config.cluster_count_y - 1);
    
    return z * config.cluster_count_x * config.cluster_count_y + 
           y * config.cluster_count_x + x;
}

float linear_depth(float depth, float near_plane, float far_plane) {
    return (near_plane * far_plane) / (far_plane - depth * (far_plane - near_plane));
}

} // namespace gw::render::forward_plus
