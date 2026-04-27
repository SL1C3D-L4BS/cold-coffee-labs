#pragma once

#include "../frame_graph/frame_graph.hpp"
#include "../frame_graph/error.hpp"
#include "../hal/vulkan_device.hpp"
#include "engine/math/vec.hpp"
#include "engine/math/mat.hpp"
#include <string>
#include <vector>
#include <vk_mem_alloc.h>
#include <volk.h>

namespace gw::render::ibl {

using gw::render::frame_graph::FrameGraph;
using gw::render::frame_graph::PassHandle;
using gw::render::frame_graph::ResourceHandle;
using gw::render::frame_graph::Result;
using gw::math::Mat4f;
using gw::math::Vec2f;
using gw::math::Vec3f;
using gw::math::Vec4f;

// IBL environment map configuration
struct IBLConfig {
    uint32_t environment_map_size  = 1024;
    uint32_t irradiance_map_size   = 32;
    uint32_t prefiltered_map_size  = 256;
    uint32_t brdf_lut_size         = 512;
    uint32_t prefilter_mip_levels  = 5;
    float    max_prefilter_roughness = 1.0f;
    uint32_t sample_count          = 1024;
};

struct EnvironmentMap {
    VkImage       cubemap      = VK_NULL_HANDLE;
    VkImageView   cubemap_view = VK_NULL_HANDLE;
    VkSampler     sampler      = VK_NULL_HANDLE;
    VmaAllocation allocation   = VK_NULL_HANDLE;
    uint32_t      size         = 0;
    uint32_t      mip_levels   = 0;
    VkFormat      format       = VK_FORMAT_R16G16B16A16_SFLOAT;

    ResourceHandle create_resource(FrameGraph& frame_graph) const;
};

struct IrradianceMap {
    VkImage       cubemap      = VK_NULL_HANDLE;
    VkImageView   cubemap_view = VK_NULL_HANDLE;
    VkSampler     sampler      = VK_NULL_HANDLE;
    VmaAllocation allocation   = VK_NULL_HANDLE;
    uint32_t      size         = 0;
    VkFormat      format       = VK_FORMAT_R16G16B16A16_SFLOAT;

    ResourceHandle create_resource(FrameGraph& frame_graph) const;
};

struct PrefilteredMap {
    VkImage       cubemap      = VK_NULL_HANDLE;
    VkImageView   cubemap_view = VK_NULL_HANDLE;
    VkSampler     sampler      = VK_NULL_HANDLE;
    VmaAllocation allocation   = VK_NULL_HANDLE;
    uint32_t      size         = 0;
    uint32_t      mip_levels   = 0;
    VkFormat      format       = VK_FORMAT_R16G16B16A16_SFLOAT;

    ResourceHandle create_resource(FrameGraph& frame_graph) const;
};

struct BRDFLUT {
    VkImage       texture      = VK_NULL_HANDLE;
    VkImageView   texture_view = VK_NULL_HANDLE;
    VkSampler     sampler      = VK_NULL_HANDLE;
    VmaAllocation allocation   = VK_NULL_HANDLE;
    uint32_t      size         = 0;
    VkFormat      format       = VK_FORMAT_R16G16_SFLOAT;

    ResourceHandle create_resource(FrameGraph& frame_graph) const;
};

struct TonemapConfig {
    enum class Type { None, ACES, Reinhard, Filmic, Uncharted2 };
    Type  type              = Type::ACES;
    float exposure          = 1.0f;
    float gamma             = 2.2f;
    bool  use_dithering     = true;
    float vignette_strength = 0.0f;
    float vignette_power    = 1.0f;
};

class IBLSystem {
public:
    explicit IBLSystem(hal::VulkanDevice& device);
    ~IBLSystem();

    IBLSystem(const IBLSystem&) = delete;
    IBLSystem& operator=(const IBLSystem&) = delete;
    IBLSystem(IBLSystem&&) = default;
    IBLSystem& operator=(IBLSystem&&) = delete;

    Result<std::monostate> initialize(const IBLConfig& config);
    Result<std::monostate> load_environment_map(const std::string& filepath);
    Result<std::monostate> generate_ibl_resources(FrameGraph& frame_graph);

    Result<PassHandle> create_irradiance_pass(FrameGraph& frame_graph,
                                               const EnvironmentMap& env_map,
                                               const IrradianceMap& irradiance_map);

    Result<PassHandle> create_prefilter_pass(FrameGraph& frame_graph,
                                              const EnvironmentMap& env_map,
                                              const PrefilteredMap& prefiltered_map);

    Result<PassHandle> create_brdf_lut_pass(FrameGraph& frame_graph,
                                             const BRDFLUT& brdf_lut);

    Result<PassHandle> create_tonemap_pass(FrameGraph& frame_graph,
                                            const TonemapConfig& tonemap_config);

    [[nodiscard]] const EnvironmentMap&  environment_map()  const { return environment_map_; }
    [[nodiscard]] const IrradianceMap&   irradiance_map()   const { return irradiance_map_; }
    [[nodiscard]] const PrefilteredMap&  prefiltered_map()  const { return prefiltered_map_; }
    [[nodiscard]] const BRDFLUT&         brdf_lut()         const { return brdf_lut_; }
    [[nodiscard]] const IBLConfig&       config()           const { return config_; }

    // Packed uniform data for shader consumption
    struct IBLUniforms {
        float    view_matrix[16];         // Mat4f as flat array
        float    projection_matrix[16];
        Vec4f    environment_params;      // (intensity, rotation, mip_level, 0)
        Vec4f    tonemap_params;          // (exposure, gamma, vignette_strength, vignette_power)
        uint32_t tonemap_type  = 0;
        uint32_t use_dithering = 0;
        uint32_t padding[2]    = {};
    };

    IBLUniforms get_uniforms(const TonemapConfig& tonemap_config) const;

private:
    hal::VulkanDevice& device_;
    IBLConfig config_;

    EnvironmentMap environment_map_;
    IrradianceMap  irradiance_map_;
    PrefilteredMap prefiltered_map_;
    BRDFLUT        brdf_lut_;

    Result<std::monostate> create_environment_map();
    Result<std::monostate> create_irradiance_map();
    Result<std::monostate> create_prefiltered_map();
    Result<std::monostate> create_brdf_lut();

    void cleanup_environment_map();
    void cleanup_irradiance_map();
    void cleanup_prefiltered_map();
    void cleanup_brdf_lut();

    Result<std::monostate> load_equirectangular_map(const std::string& filepath);

    Result<std::monostate> dispatch_compute_shader(
        FrameGraph& frame_graph,
        const std::string& shader_name,
        uint32_t group_x, uint32_t group_y, uint32_t group_z,
        const std::vector<ResourceHandle>& resources,
        const void* push_constants = nullptr,
        uint32_t push_constant_size = 0);
};

// CPU-side tonemap math utilities (used by validator + offline tools)
namespace tonemap {
    Vec3f aces_tonemap(const Vec3f& color);
    Vec3f reinhard_tonemap(const Vec3f& color);
    Vec3f filmic_tonemap(const Vec3f& color);
    Vec3f uncharted2_tonemap(const Vec3f& color);
    Vec3f apply_dithering(const Vec3f& color, uint32_t x, uint32_t y);
    Vec3f apply_vignette(const Vec3f& color, Vec2f uv, float strength, float power);
}

// CPU-side IBL math utilities
namespace utils {
    struct ImportanceSample {
        Vec3f direction;
        float pdf        = 0.0f;
        float cos_theta  = 0.0f;
        float sin_theta  = 0.0f;
        float phi        = 0.0f;
    };

    ImportanceSample importance_sample_ggx(Vec2f xi, float roughness);
    float hammersley(uint32_t i, uint32_t N);
    float van_der_corput(uint32_t bits);
    Vec3f cubemap_direction(uint32_t face, Vec2f uv);

    struct SphericalHarmonics {
        Vec3f coefficients[9];
    };

    SphericalHarmonics project_to_spherical_harmonics(const EnvironmentMap& cubemap);
    Vec3f evaluate_spherical_harmonics(const SphericalHarmonics& sh,
                                       const Vec3f& direction);
}

} // namespace gw::render::ibl
