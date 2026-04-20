#pragma once

#include "../frame_graph/frame_graph.hpp"
#include "engine/core/math.hpp"
#include "engine/core/result.hpp"
#include <vector>
#include <memory>
#include <optional>

namespace cold_coffee::engine::render::ibl {

// IBL environment map configuration
struct IBLConfig {
    uint32_t environment_map_size = 1024;
    uint32_t irradiance_map_size = 32;
    uint32_t prefiltered_map_size = 256;
    uint32_t brdf_lut_size = 512;
    uint32_t prefilter_mip_levels = 5;
    float max_prefilter_roughness = 1.0f;
    uint32_t sample_count = 1024;
};

// Environment map formats
struct EnvironmentMap {
    VkImage cubemap = VK_NULL_HANDLE;
    VkImageView cubemap_view = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    uint32_t size = 0;
    uint32_t mip_levels = 0;
    VkFormat format = VK_FORMAT_R16G16B16A16_SFLOAT;
    
    // Frame graph integration
    frame_graph::ResourceHandle create_resource(frame_graph::FrameGraph& frame_graph) const;
};

// Irradiance map (diffuse IBL)
struct IrradianceMap {
    VkImage cubemap = VK_NULL_HANDLE;
    VkImageView cubemap_view = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    uint32_t size = 0;
    VkFormat format = VK_FORMAT_R16G16B16A16_SFLOAT;
    
    // Frame graph integration
    frame_graph::ResourceHandle create_resource(frame_graph::FrameGraph& frame_graph) const;
};

// Prefiltered environment map (specular IBL)
struct PrefilteredMap {
    VkImage cubemap = VK_NULL_HANDLE;
    VkImageView cubemap_view = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    uint32_t size = 0;
    uint32_t mip_levels = 0;
    VkFormat format = VK_FORMAT_R16G16B16A16_SFLOAT;
    
    // Frame graph integration
    frame_graph::ResourceHandle create_resource(frame_graph::FrameGraph& frame_graph) const;
};

// BRDF Look-Up Table for split-sum approximation
struct BRDFLUT {
    VkImage texture = VK_NULL_HANDLE;
    VkImageView texture_view = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    uint32_t size = 0;
    VkFormat format = VK_FORMAT_R16G16_SFLOAT;
    
    // Frame graph integration
    frame_graph::ResourceHandle create_resource(frame_graph::FrameGraph& frame_graph) const;
};

// Tonemapping configuration
struct TonemapConfig {
    enum class Type {
        None,
        ACES,
        Reinhard,
        Filmic,
        Uncharted2
    };
    
    Type type = Type::ACES;
    float exposure = 1.0f;
    float gamma = 2.2f;
    bool use_dithering = true;
    float vignette_strength = 0.0f;
    float vignette_power = 1.0f;
};

// IBL system class
class IBLSystem {
public:
    IBLSystem(const class hal::Device& device);
    ~IBLSystem();
    
    // Non-copyable, movable
    IBLSystem(const IBLSystem&) = delete;
    IBLSystem& operator=(const IBLSystem&) = delete;
    IBLSystem(IBLSystem&&) = default;
    IBLSystem& operator=(IBLSystem&&) = default;
    
    // Initialize the IBL system
    Result<void> initialize(const IBLConfig& config);
    
    // Load HDR environment map
    Result<void> load_environment_map(const std::string& filepath);
    
    // Generate IBL resources
    Result<void> generate_ibl_resources(frame_graph::FrameGraph& frame_graph);
    
    // Create IBL rendering passes
    Result<frame_graph::PassHandle> create_irradiance_pass(
        frame_graph::FrameGraph& frame_graph,
        const EnvironmentMap& env_map,
        const IrradianceMap& irradiance_map);
    
    Result<frame_graph::PassHandle> create_prefilter_pass(
        frame_graph::FrameGraph& frame_graph,
        const EnvironmentMap& env_map,
        const PrefilteredMap& prefiltered_map);
    
    Result<frame_graph::PassHandle> create_brdf_lut_pass(
        frame_graph::FrameGraph& frame_graph,
        const BRDFLUT& brdf_lut);
    
    Result<frame_graph::PassHandle> create_tonemap_pass(
        frame_graph::FrameGraph& frame_graph,
        const TonemapConfig& tonemap_config);
    
    // Getters
    const EnvironmentMap& environment_map() const { return environment_map_; }
    const IrradianceMap& irradiance_map() const { return irradiance_map_; }
    const PrefilteredMap& prefiltered_map() const { return prefiltered_map_; }
    const BRDFLUT& brdf_lut() const { return brdf_lut_; }
    const IBLConfig& config() const { return config_; }
    
    // Shader uniform data
    struct IBLUniforms {
        glm::mat4 view_matrix;
        glm::mat4 projection_matrix;
        glm::vec4 environment_params; // (intensity, rotation, mip_level, 0)
        glm::vec4 tonemap_params;     // (exposure, gamma, vignette_strength, vignette_power)
        uint32_t tonemap_type;
        uint32_t use_dithering;
        uint2 padding;
    };
    
    IBLUniforms get_uniforms(const TonemapConfig& tonemap_config) const;
    
private:
    const class hal::Device& device_;
    IBLConfig config_;
    
    EnvironmentMap environment_map_;
    IrradianceMap irradiance_map_;
    PrefilteredMap prefiltered_map_;
    BRDFLUT brdf_lut_;
    
    // Resource creation helpers
    Result<void> create_environment_map();
    Result<void> create_irradiance_map();
    Result<void> create_prefiltered_map();
    Result<void> create_brdf_lut();
    
    // Cleanup helpers
    void cleanup_environment_map();
    void cleanup_irradiance_map();
    void cleanup_prefiltered_map();
    void cleanup_brdf_lut();
    
    // Cube map utilities
    Result<void> load_equirectangular_map(const std::string& filepath);
    Result<void> equirectangular_to_cubemap(const class hal::Image& equirectangular);
    
    // Compute shader utilities
    Result<void> dispatch_compute_shader(
        frame_graph::FrameGraph& frame_graph,
        const std::string& shader_name,
        uint32_t group_x, uint32_t group_y, uint32_t group_z,
        const std::vector<frame_graph::ResourceHandle>& resources,
        const void* push_constants = nullptr,
        uint32_t push_constant_size = 0);
};

// Tonemapping functions
namespace tonemap {
    // ACES filmic tonemapper
    glm::vec3 aces_tonemap(const glm::vec3& color);
    
    // Reinhard tonemapper
    glm::vec3 reinhard_tonemap(const glm::vec3& color);
    
    // Filmic tonemapper (Unreal engine style)
    glm::vec3 filmic_tonemap(const glm::vec3& color);
    
    // Uncharted 2 tonemapper
    glm::vec3 uncharted2_tonemap(const glm::vec3& color);
    
    // Dithering for banding reduction
    glm::vec3 apply_dithering(const glm::vec3& color, uint32_t x, uint32_t y);
    
    // Vignette effect
    glm::vec3 apply_vignette(const glm::vec3& color, float2 uv, 
                            float strength, float power);
}

// IBL utility functions
namespace utils {
    // Importance sampling for GGX distribution
    struct ImportanceSample {
        glm::vec3 direction;
        float pdf;
        float cos_theta;
        float sin_theta;
        float phi;
    };
    
    ImportanceSample importance_sample_ggx(float2 xi, float roughness);
    
    // Hammersley sequence for low-discrepancy sampling
    float hammersley(uint32_t i, uint32_t N);
    
    // Van der Corput sequence
    float van_der_corput(uint32_t bits);
    
    // Cube map face utilities
    glm::vec3 cubemap_direction(uint32_t face, float2 uv);
    
    // Cube map filtering utilities
    glm::vec3 filter_cubemap(const EnvironmentMap& cubemap, 
                            const glm::vec3& direction, 
                            float roughness,
                            uint32_t sample_count = 1024);
    
    // Spherical harmonics for irradiance
    struct SphericalHarmonics {
        glm::vec3 coefficients[9]; // 3rd order SH
    };
    
    SphericalHarmonics project_to_spherical_harmonics(const EnvironmentMap& cubemap);
    glm::vec3 evaluate_spherical_harmonics(const SphericalHarmonics& sh, 
                                          const glm::vec3& direction);
}

} // namespace cold_coffee::engine::render::ibl
