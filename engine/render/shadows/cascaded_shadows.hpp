#pragma once

#include "../frame_graph/frame_graph.hpp"
#include "../frame_graph/error.hpp"
#include "../hal/vulkan_device.hpp"
#include "engine/math/vec.hpp"
#include "engine/math/mat.hpp"
#include <optional>
#include <vector>
#include <memory>
#include <vk_mem_alloc.h>
#include <volk.h>

namespace gw::render::shadows {

using gw::render::frame_graph::FrameGraph;
using gw::render::frame_graph::PassHandle;
using gw::render::frame_graph::ResourceHandle;
using gw::render::frame_graph::Result;
using gw::math::Mat4f;
using gw::math::Vec2f;
using gw::math::Vec3f;
using gw::math::Vec4f;

// Shadow map cascade configuration
struct CascadeConfig {
    uint32_t cascade_count = 4;
    uint32_t shadow_map_size = 2048;
    float near_plane = 0.1f;
    float far_plane = 1000.0f;
    float cascade_split_lambda = 0.95f;
    float bias_multiplier = 1.0f;
    float max_bias = 0.005f;

    std::vector<float> cascade_splits;

    CascadeConfig() {
        cascade_splits.resize(cascade_count);
        calculate_splits();
    }

    void calculate_splits();
};

// Shadow cascade data
struct ShadowCascade {
    Mat4f view_matrix;
    Mat4f projection_matrix;
    Mat4f view_projection_matrix;
    Vec3f light_direction;
    float split_distance = 0.0f;
    float texel_size = 0.0f;
    uint32_t index = 0;
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
    ShadowMap(hal::VulkanDevice& device, uint32_t size);
    ~ShadowMap();

    ShadowMap(const ShadowMap&) = delete;
    ShadowMap& operator=(const ShadowMap&) = delete;
    ShadowMap(ShadowMap&&) = default;
    ShadowMap& operator=(ShadowMap&&) = default;

    [[nodiscard]] VkImage      image()      const noexcept { return image_; }
    [[nodiscard]] VkImageView  image_view() const noexcept { return image_view_; }
    [[nodiscard]] VkSampler    sampler()    const noexcept { return sampler_; }
    [[nodiscard]] uint32_t     size()       const noexcept { return size_; }

    ResourceHandle create_resource(FrameGraph& frame_graph) const;

private:
    hal::VulkanDevice& device_;
    uint32_t size_;
    VkImage      image_      = VK_NULL_HANDLE;
    VkImageView  image_view_ = VK_NULL_HANDLE;
    VkSampler    sampler_    = VK_NULL_HANDLE;
    VmaAllocation allocation_ = VK_NULL_HANDLE;

    Result<std::monostate> create_image();
    Result<std::monostate> create_image_view();
    Result<std::monostate> create_sampler();
    void cleanup();
};

// Shadow atlas for spot/point lights
class ShadowAtlas {
public:
    ShadowAtlas(hal::VulkanDevice& device, uint32_t size, uint32_t tile_size);
    ~ShadowAtlas();

    ShadowAtlas(const ShadowAtlas&) = delete;
    ShadowAtlas& operator=(const ShadowAtlas&) = delete;
    ShadowAtlas(ShadowAtlas&&) = default;
    ShadowAtlas& operator=(ShadowAtlas&&) = default;

    struct TileAllocation {
        uint32_t x = 0;
        uint32_t y = 0;
        uint32_t size = 0;
        Vec4f uv_transform;  // (u_offset, v_offset, u_scale, v_scale)
    };

    std::optional<TileAllocation> allocate_tile();
    void free_tile(const TileAllocation& allocation);

    [[nodiscard]] VkImage     image()      const noexcept { return image_; }
    [[nodiscard]] VkImageView image_view() const noexcept { return image_view_; }
    [[nodiscard]] VkSampler   sampler()    const noexcept { return sampler_; }
    [[nodiscard]] uint32_t    size()       const noexcept { return size_; }
    [[nodiscard]] uint32_t    tile_size()  const noexcept { return tile_size_; }

    ResourceHandle create_resource(FrameGraph& frame_graph) const;

private:
    hal::VulkanDevice& device_;
    uint32_t size_;
    uint32_t tile_size_;
    uint32_t tiles_per_side_;

    VkImage      image_      = VK_NULL_HANDLE;
    VkImageView  image_view_ = VK_NULL_HANDLE;
    VkSampler    sampler_    = VK_NULL_HANDLE;
    VmaAllocation allocation_ = VK_NULL_HANDLE;

    std::vector<uint32_t>      free_tiles_;
    std::vector<TileAllocation> allocated_tiles_;

    Result<std::monostate> create_image();
    Result<std::monostate> create_image_view();
    Result<std::monostate> create_sampler();
    void cleanup();
    void tile_index_to_coords(uint32_t index, uint32_t& x, uint32_t& y) const;
};

// Main cascaded shadow map system
class CascadedShadowMapper {
public:
    explicit CascadedShadowMapper(hal::VulkanDevice& device);
    ~CascadedShadowMapper();

    CascadedShadowMapper(const CascadedShadowMapper&) = delete;
    CascadedShadowMapper& operator=(const CascadedShadowMapper&) = delete;
    CascadedShadowMapper(CascadedShadowMapper&&) = default;
    CascadedShadowMapper& operator=(CascadedShadowMapper&&) = default;

    Result<std::monostate> initialize(const CascadeConfig& config);

    void update_cascades(const Mat4f& view_matrix,
                         const Mat4f& projection_matrix,
                         const Vec3f& light_direction);

    Result<PassHandle> create_shadow_pass(FrameGraph& frame_graph,
                                          const Mat4f& view_matrix,
                                          const Mat4f& projection_matrix,
                                          const Vec3f& light_direction);

    [[nodiscard]] const std::vector<ShadowCascade>& cascades()  const { return cascades_; }
    [[nodiscard]] const CascadeConfig&              config()    const { return config_; }
    [[nodiscard]] const ShadowMapResources&         resources() const { return *resources_; }

    // Packed uniform data for shader consumption
    struct ShadowUniforms {
        float cascade_view_projections[4][16];  // 4 × Mat4f as flat float arrays
        Vec4f cascade_splits;
        Vec4f shadow_params;    // (bias, texel_size, 0, 0)
        Vec4f light_direction;  // direction in .xyz, 0 in .w
    };

    ShadowUniforms get_uniforms() const;

private:
    hal::VulkanDevice& device_;
    CascadeConfig config_;
    std::vector<ShadowCascade> cascades_;
    std::unique_ptr<ShadowMapResources> resources_;

    void calculate_cascade_matrices(const Mat4f& view_matrix,
                                    const Mat4f& projection_matrix,
                                    const Vec3f& light_direction);

    Mat4f calculate_cascade_projection(uint32_t cascade_index,
                                       const Mat4f& view_matrix,
                                       const Mat4f& projection_matrix);

    Mat4f calculate_cascade_view(const Vec3f& light_direction,
                                 const Vec3f& cascade_center,
                                 float cascade_radius);

    void stabilize_cascades(Mat4f& view_matrix,
                            Mat4f& projection_matrix,
                            uint32_t cascade_index);

    std::vector<Vec3f> get_frustum_corners(const Mat4f& view_matrix,
                                           const Mat4f& projection_matrix,
                                           float near_split,
                                           float far_split);

    struct BoundingBox {
        Vec3f min_bound;
        Vec3f max_bound;
    };

    BoundingBox calculate_bounding_box(const std::vector<Vec3f>& points);
    BoundingBox calculate_light_space_bounding_box(const std::vector<Vec3f>& corners,
                                                   const Mat4f& light_view_matrix);
};

// PCF filtering for shadow sampling
class PCFFilter {
public:
    enum class FilterType { None, PCF_2x2, PCF_3x3, PCF_5x5, PCF_Random };

    static void apply_filter(FilterType type, float texel_size,
                             Vec2f& uv_offset, float& weight);

private:
    static constexpr uint32_t MAX_SAMPLES = 25;
    static Vec2f poisson_disk_[MAX_SAMPLES];
    static bool  poisson_initialized_;
    static void  initialize_poisson_disk();
};

} // namespace gw::render::shadows
