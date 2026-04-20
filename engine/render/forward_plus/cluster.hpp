#pragma once

#include "../frame_graph/types.hpp"
#include "../frame_graph/error.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "engine/math/vec.hpp"
#include "engine/math/mat.hpp"
#include <vk_mem_alloc.h>
#include <volk.h>
#include <vector>
#include <unordered_map>

// Type aliases for compatibility
using float2 = gw::math::Vec2<float>;
using float3 = gw::math::Vec3<float>;
using float4 = gw::math::Vec4<float>;
using float4x4 = gw::math::Mat4<float>;

namespace gw::render::forward_plus {

using gw::render::frame_graph::FrameGraph;
using gw::render::frame_graph::PassHandle;
using gw::render::frame_graph::ResourceHandle;
using gw::render::frame_graph::Result;

// Cluster configuration (Week 029)
struct ClusterConfig {
    uint32_t cluster_count_x = 16;
    uint32_t cluster_count_y = 8;
    uint32_t cluster_count_z = 24;
    float z_near = 0.1f;
    float z_far = 1000.0f;
    float fov_y = 60.0f;  // Vertical FOV in degrees
    
    uint32_t total_cluster_count() const {
        return cluster_count_x * cluster_count_y * cluster_count_z;
    }
};

// Light data structures
struct Light {
    float3 position;
    float3 color;
    float intensity;
    float radius;
    uint32_t type;  // 0 = point, 1 = spot, 2 = directional
    float3 direction;  // For spot/directional lights
    float inner_cone;  // For spot lights
    float outer_cone;  // For spot lights
};

// Cluster AABB for light culling
struct ClusterAABB {
    float3 min_bounds;
    float3 max_bounds;
};

// Light grid data
struct LightGrid {
    // Per-cluster light index lists
    std::vector<uint32_t> light_index_list;
    std::vector<uint32_t> light_offset_list;  // Offset into light_index_list per cluster
    std::vector<uint32_t> light_count_list;  // Light count per cluster
    
    // Cluster AABBs (for debugging/visualization)
    std::vector<ClusterAABB> cluster_aabbs;
    
    void clear() {
        light_index_list.clear();
        light_offset_list.clear();
        light_count_list.clear();
        cluster_aabbs.clear();
    }
    
    void resize(uint32_t cluster_count) {
        light_offset_list.resize(cluster_count, 0);
        light_count_list.resize(cluster_count, 0);
        cluster_aabbs.resize(cluster_count);
    }
};

// Clustered forward lighting system (Week 029)
class ClusteredLighting {
public:
    explicit ClusteredLighting(VkDevice device, VmaAllocator allocator);
    ~ClusteredLighting();
    
    // Non-copyable, movable
    ClusteredLighting(const ClusteredLighting&) = delete;
    ClusteredLighting& operator=(const ClusteredLighting&) = delete;
    ClusteredLighting(ClusteredLighting&&) = default;
    ClusteredLighting& operator=(ClusteredLighting&&) = default;
    
    // Configuration
    Result<std::monostate> initialize(const ClusterConfig& config);
    void set_lights(const std::vector<Light>& lights);
    
    // Frame graph integration
    Result<ResourceHandle> create_light_grid_buffer();
    Result<ResourceHandle> create_light_list_buffer();
    Result<ResourceHandle> create_cluster_aabb_buffer();
    
    // Pass creation
    Result<PassHandle> create_light_culling_pass(FrameGraph& frame_graph);
    Result<PassHandle> create_forward_pass(FrameGraph& frame_graph);
    
    // Getters
    const ClusterConfig& config() const { return config_; }
    const LightGrid& light_grid() const { return light_grid_; }
    
    // Debug helpers
    void dump_cluster_info() const;
    
private:
    VkDevice device_;
    VmaAllocator allocator_;
    ClusterConfig config_;
    
    // Light data
    std::vector<Light> lights_;
    LightGrid light_grid_;
    
    // Vulkan resources
    VkBuffer light_grid_buffer_ = VK_NULL_HANDLE;
    VkBuffer light_list_buffer_ = VK_NULL_HANDLE;
    VkBuffer cluster_aabb_buffer_ = VK_NULL_HANDLE;
    VmaAllocation light_grid_allocation_ = VK_NULL_HANDLE;
    VmaAllocation light_list_allocation_ = VK_NULL_HANDLE;
    VmaAllocation cluster_aabb_allocation_ = VK_NULL_HANDLE;
    
    // Compute resources
    VkDescriptorSetLayout compute_descriptor_set_layout_ = VK_NULL_HANDLE;
    VkPipelineLayout compute_pipeline_layout_ = VK_NULL_HANDLE;
    VkPipeline compute_pipeline_ = VK_NULL_HANDLE;
    VkDescriptorSet compute_descriptor_set_ = VK_NULL_HANDLE;
    
    // Graphics resources
    VkDescriptorSetLayout graphics_descriptor_set_layout_ = VK_NULL_HANDLE;
    VkPipelineLayout graphics_pipeline_layout_ = VK_NULL_HANDLE;
    
    // Helper functions
    Result<std::monostate> create_compute_resources();
    Result<std::monostate> create_graphics_resources();
    Result<std::monostate> create_buffers();
    
    void destroy_compute_resources();
    void destroy_graphics_resources();
    void destroy_buffers();
    
    // Cluster computation
    std::vector<ClusterAABB> compute_cluster_aabbs(
        const float4x4& view_matrix,
        const float4x4& projection_matrix);
    
    ClusterAABB compute_cluster_aabb(
        uint32_t x, uint32_t y, uint32_t z,
        const float4x4& view_matrix,
        const float4x4& projection_matrix);
    
    // Light culling
    void perform_light_culling(
        const std::vector<ClusterAABB>& clusters,
        const std::vector<Light>& lights);
    
    bool sphere_intersects_aabb(const Light& light, const ClusterAABB& aabb);
    bool spot_intersects_aabb(const Light& light, const ClusterAABB& aabb);
};

// Utility functions
float3 screen_to_view_space(float2 screen_pos, float depth, const float4x4& inv_projection);
float3 view_to_world_space(float3 view_pos, const float4x4& inv_view);
uint32_t get_cluster_index(float3 view_pos, const ClusterConfig& config);
float linear_depth(float depth, float near_plane, float far_plane);

} // namespace gw::render::forward_plus
