#pragma once

#include "../frame_graph/frame_graph.hpp"
#include "engine/core/math.hpp"
#include "engine/core/result.hpp"
#include <vector>
#include <memory>

namespace cold_coffee::engine::render::shadows {

// Shadow map cascade configuration
struct CascadeConfig {
    uint32_t cascade_count = 4;
    uint32_t shadow_map_size = 2048;
    float near_plane = 0.1f;
    float far_plane = 1000.0f;
    float cascade_split_lambda = 0.95f; // For practical split scheme
    float bias_multiplier = 1.0f;
    float max_bias = 0.005f;
    
    // Cascade distance splits (calculated)
    std::vector<float> cascade_splits;
    
    CascadeConfig() {
        cascade_splits.resize(cascade_count);
        calculate_splits();
    }
    
    void calculate_splits();
};

// Shadow cascade data
struct ShadowCascade {
    glm::mat4 view_matrix;
    glm::mat4 projection_matrix;
    glm::mat4 view_projection_matrix;
    glm::vec3 light_direction;
    float split_distance;
    float texel_size;
    uint32_t index;
};

// Shadow map resources
struct ShadowMapResources {
    std::vector<std::unique_ptr<class ShadowMap>> cascades;
    std::unique_ptr<class ShadowAtlas> spot_light_atlas;
    std::unique_ptr<class ShadowAtlas> point_light_atlas;
};

// Shadow map class for individual cascades
class ShadowMap {
public:
    ShadowMap(const class hal::Device& device, uint32_t size);
    ~ShadowMap();
    
    // Non-copyable, movable
    ShadowMap(const ShadowMap&) = delete;
    ShadowMap& operator=(const ShadowMap&) = delete;
    ShadowMap(ShadowMap&&) = default;
    ShadowMap& operator=(ShadowMap&&) = default;
    
    // Getters
    VkImage image() const { return image_; }
    VkImageView image_view() const { return image_view_; }
    VkSampler sampler() const { return sampler_; }
    uint32_t size() const { return size_; }
    
    // Frame graph integration
    frame_graph::ResourceHandle create_resource(frame_graph::FrameGraph& frame_graph) const;
    
private:
    const class hal::Device& device_;
    uint32_t size_;
    VkImage image_ = VK_NULL_HANDLE;
    VkImageView image_view_ = VK_NULL_HANDLE;
    VkSampler sampler_ = VK_NULL_HANDLE;
    VmaAllocation allocation_ = VK_NULL_HANDLE;
    
    Result<void> create_image();
    Result<void> create_image_view();
    Result<void> create_sampler();
    void cleanup();
};

// Shadow atlas for spot/point lights
class ShadowAtlas {
public:
    ShadowAtlas(const class hal::Device& device, uint32_t size, uint32_t tile_size);
    ~ShadowAtlas();
    
    // Non-copyable, movable
    ShadowAtlas(const ShadowAtlas&) = delete;
    ShadowAtlas& operator=(const ShadowAtlas&) = delete;
    ShadowAtlas(ShadowAtlas&&) = default;
    ShadowAtlas& operator=(ShadowAtlas&&) = default;
    
    // Allocate a tile in the atlas
    struct TileAllocation {
        uint32_t x, y;
        uint32_t size;
        glm::vec4 uv_transform; // (u_offset, v_offset, u_scale, v_scale)
    };
    
    std::optional<TileAllocation> allocate_tile();
    void free_tile(const TileAllocation& allocation);
    
    // Getters
    VkImage image() const { return image_; }
    VkImageView image_view() const { return image_view_; }
    VkSampler sampler() const { return sampler_; }
    uint32_t size() const { return size_; }
    uint32_t tile_size() const { return tile_size_; }
    
    // Frame graph integration
    frame_graph::ResourceHandle create_resource(frame_graph::FrameGraph& frame_graph) const;
    
private:
    const class hal::Device& device_;
    uint32_t size_;
    uint32_t tile_size_;
    uint32_t tiles_per_side_;
    
    VkImage image_ = VK_NULL_HANDLE;
    VkImageView image_view_ = VK_NULL_HANDLE;
    VkSampler sampler_ = VK_NULL_HANDLE;
    VmaAllocation allocation_ = VK_NULL_HANDLE;
    
    // Simple free list for tile allocation
    std::vector<uint32_t> free_tiles_;
    std::vector<TileAllocation> allocated_tiles_;
    
    Result<void> create_image();
    Result<void> create_image_view();
    Result<void> create_sampler();
    void cleanup();
    uint32_t tile_index_to_coords(uint32_t index, uint32_t& x, uint32_t& y) const;
};

// Main cascaded shadow map system
class CascadedShadowMapper {
public:
    CascadedShadowMapper(const class hal::Device& device);
    ~CascadedShadowMapper();
    
    // Non-copyable, movable
    CascadedShadowMapper(const CascadedShadowMapper&) = delete;
    CascadedShadowMapper& operator=(const CascadedShadowMapper&) = delete;
    CascadedShadowMapper(CascadedShadowMapper&&) = default;
    CascadedShadowMapper& operator=(CascadedShadowMapper&&) = default;
    
    // Initialize the system
    Result<void> initialize(const CascadeConfig& config);
    
    // Update cascade matrices for current frame
    void update_cascades(const glm::mat4& view_matrix, const glm::mat4& projection_matrix,
                         const glm::vec3& light_direction);
    
    // Create shadow rendering passes
    Result<frame_graph::PassHandle> create_shadow_pass(
        frame_graph::FrameGraph& frame_graph,
        const glm::mat4& view_matrix,
        const glm::mat4& projection_matrix,
        const glm::vec3& light_direction);
    
    // Get cascade data for shader
    const std::vector<ShadowCascade>& cascades() const { return cascades_; }
    const CascadeConfig& config() const { return config_; }
    const ShadowMapResources& resources() const { return *resources_; }
    
    // Shader uniform data
    struct ShadowUniforms {
        glm::mat4 cascade_view_projections[4];
        glm::vec4 cascade_splits; // Only .xyz used
        glm::vec4 shadow_params;  // (bias, texel_size, 0, 0)
        glm::vec4 light_direction;
    };
    
    ShadowUniforms get_uniforms() const;
    
private:
    const class hal::Device& device_;
    CascadeConfig config_;
    std::vector<ShadowCascade> cascades_;
    std::unique_ptr<ShadowMapResources> resources_;
    
    // Cascade calculation helpers
    void calculate_cascade_matrices(const glm::mat4& view_matrix, 
                                   const glm::mat4& projection_matrix,
                                   const glm::vec3& light_direction);
    
    glm::mat4 calculate_cascade_projection(uint32_t cascade_index, 
                                          const glm::mat4& view_matrix,
                                          const glm::mat4& projection_matrix);
    
    glm::mat4 calculate_cascade_view(const glm::vec3& light_direction,
                                    const glm::vec3& cascade_center,
                                    float cascade_radius);
    
    void stabilize_cascades(glm::mat4& view_matrix, glm::mat4& projection_matrix,
                           uint32_t cascade_index);
    
    // Frustum corner calculation
    std::vector<glm::vec3> get_frustum_corners(const glm::mat4& view_matrix,
                                               const glm::mat4& projection_matrix,
                                               float near_split, float far_split);
    
    // Bounding box calculation
    struct BoundingBox {
        glm::vec3 min_bound;
        glm::vec3 max_bound;
    };
    
    BoundingBox calculate_bounding_box(const std::vector<glm::vec3>& points);
    BoundingBox calculate_light_space_bounding_box(const std::vector<glm::vec3>& corners,
                                                  const glm::mat4& light_view_matrix);
};

// PCF filtering for shadow sampling
class PCFFilter {
public:
    enum class FilterType {
        None,
        PCF_2x2,
        PCF_3x3,
        PCF_5x5,
        PCF_Random
    };
    
    static void apply_filter(FilterType type, float texel_size, 
                            glm::vec2& uv_offset, float& weight);
    
private:
    static constexpr uint32_t MAX_SAMPLES = 25;
    static glm::vec2 poisson_disk_[MAX_SAMPLES];
    static bool poisson_initialized_;
    static void initialize_poisson_disk();
};

} // namespace cold_coffee::engine::render::shadows
