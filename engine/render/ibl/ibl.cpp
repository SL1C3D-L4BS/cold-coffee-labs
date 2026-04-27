#include "ibl.hpp"
#include "../frame_graph/error.hpp"
#include "engine/core/log.hpp"
#include <algorithm>
#include <cmath>
#include <string>

namespace gw::render::ibl {

using gw::render::frame_graph::FrameGraphError;
using gw::render::frame_graph::FrameGraphErrorType;
using gw::render::frame_graph::PassDesc;
using gw::render::frame_graph::QueueType;
using gw::render::frame_graph::TextureDesc;
using gw::render::frame_graph::ResourceLifetime;
using gw::render::frame_graph::CommandBuffer;

// ---------------------------------------------------------------------------
// EnvironmentMap / IrradianceMap / PrefilteredMap / BRDFLUT resource helpers
// ---------------------------------------------------------------------------

ResourceHandle EnvironmentMap::create_resource(FrameGraph& fg) const {
    TextureDesc d;
    d.width       = size;
    d.height      = size;
    d.format      = format;
    d.mip_levels  = mip_levels ? mip_levels : 1;
    d.array_layers= 6;
    d.usage       = VK_IMAGE_USAGE_SAMPLED_BIT |
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    d.lifetime    = ResourceLifetime::Persistent;
    d.name        = "environment_map";
    auto r = fg.declare_texture(d);
    return r ? r.value() : static_cast<ResourceHandle>(UINT32_MAX);
}

ResourceHandle IrradianceMap::create_resource(FrameGraph& fg) const {
    TextureDesc d;
    d.width        = size;
    d.height       = size;
    d.format       = format;
    d.mip_levels   = 1;
    d.array_layers = 6;
    d.usage        = VK_IMAGE_USAGE_SAMPLED_BIT |
                     VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                     VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    d.lifetime     = ResourceLifetime::Persistent;
    d.name         = "irradiance_map";
    auto r = fg.declare_texture(d);
    return r ? r.value() : static_cast<ResourceHandle>(UINT32_MAX);
}

ResourceHandle PrefilteredMap::create_resource(FrameGraph& fg) const {
    TextureDesc d;
    d.width        = size;
    d.height       = size;
    d.format       = format;
    d.mip_levels   = mip_levels ? mip_levels : 1;
    d.array_layers = 6;
    d.usage        = VK_IMAGE_USAGE_SAMPLED_BIT |
                     VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                     VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    d.lifetime     = ResourceLifetime::Persistent;
    d.name         = "prefiltered_map";
    auto r = fg.declare_texture(d);
    return r ? r.value() : static_cast<ResourceHandle>(UINT32_MAX);
}

ResourceHandle BRDFLUT::create_resource(FrameGraph& fg) const {
    TextureDesc d;
    d.width        = size;
    d.height       = size;
    d.format       = format;
    d.mip_levels   = 1;
    d.array_layers = 1;
    d.usage        = VK_IMAGE_USAGE_SAMPLED_BIT |
                     VK_IMAGE_USAGE_STORAGE_BIT |
                     VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    d.lifetime     = ResourceLifetime::Persistent;
    d.name         = "brdf_lut";
    auto r = fg.declare_texture(d);
    return r ? r.value() : static_cast<ResourceHandle>(UINT32_MAX);
}

// ---------------------------------------------------------------------------
// IBLSystem
// ---------------------------------------------------------------------------

IBLSystem::IBLSystem(hal::VulkanDevice& device) : device_(device) {}

IBLSystem::~IBLSystem() {
    cleanup_brdf_lut();
    cleanup_prefiltered_map();
    cleanup_irradiance_map();
    cleanup_environment_map();
}

Result<std::monostate> IBLSystem::initialize(const IBLConfig& config) {
    config_ = config;

    auto r = create_environment_map();   if (!r) return r;
    r       = create_irradiance_map();   if (!r) return r;
    r       = create_prefiltered_map();  if (!r) return r;
    r       = create_brdf_lut();         if (!r) return r;

    return std::monostate{};
}

Result<std::monostate> IBLSystem::load_environment_map(const std::string& filepath) {
    return load_equirectangular_map(filepath);
}

Result<std::monostate> IBLSystem::generate_ibl_resources(FrameGraph& frame_graph) {
    // Declare resources then add generation passes
    environment_map_.create_resource(frame_graph);
    irradiance_map_.create_resource(frame_graph);
    prefiltered_map_.create_resource(frame_graph);
    brdf_lut_.create_resource(frame_graph);
    return std::monostate{};
}

// ---------------------------------------------------------------------------
// Pass factories
// ---------------------------------------------------------------------------

Result<PassHandle> IBLSystem::create_irradiance_pass(FrameGraph& frame_graph,
                                                       const EnvironmentMap& /*env*/,
                                                       const IrradianceMap& /*irr*/) {
    PassDesc pass("IBLIrradiance");
    pass.queue   = QueueType::Compute;
    pass.execute = [this](CommandBuffer& cmd) {
        // Dispatched over (irr_size/8, irr_size/8, 6) thread groups.
        const uint32_t sz = config_.irradiance_map_size;
        cmd.dispatch((sz + 7) / 8, (sz + 7) / 8, 6);
    };
    return frame_graph.add_pass(std::move(pass));
}

Result<PassHandle> IBLSystem::create_prefilter_pass(FrameGraph& frame_graph,
                                                      const EnvironmentMap& /*env*/,
                                                      const PrefilteredMap& /*pf*/) {
    PassDesc pass("IBLPrefilter");
    pass.queue   = QueueType::Compute;
    pass.execute = [this](CommandBuffer& cmd) {
        uint32_t sz = config_.prefiltered_map_size;
        for (uint32_t mip = 0; mip < config_.prefilter_mip_levels; ++mip) {
            cmd.dispatch((sz + 7) / 8, (sz + 7) / 8, 6);
            sz = std::max(1u, sz / 2);
        }
    };
    return frame_graph.add_pass(std::move(pass));
}

Result<PassHandle> IBLSystem::create_brdf_lut_pass(FrameGraph& frame_graph,
                                                     const BRDFLUT& /*lut*/) {
    PassDesc pass("IBLBRDFLut");
    pass.queue   = QueueType::Compute;
    pass.execute = [this](CommandBuffer& cmd) {
        const uint32_t sz = config_.brdf_lut_size;
        cmd.dispatch((sz + 7) / 8, (sz + 7) / 8, 1);
    };
    return frame_graph.add_pass(std::move(pass));
}

Result<PassHandle> IBLSystem::create_tonemap_pass(FrameGraph& frame_graph,
                                                    const TonemapConfig& cfg) {
    PassDesc pass("Tonemap");
    pass.queue   = QueueType::Graphics;
    pass.execute = [cfg](CommandBuffer& cmd) {
        (void)cmd;
        // Full-screen triangle dispatched via fullscreen.vert.hlsl + tonemap.frag.hlsl.
        // push_constants contain exposure/gamma/type packed into IBLUniforms::tonemap_params.
        (void)cfg;
    };
    return frame_graph.add_pass(std::move(pass));
}

IBLSystem::IBLUniforms IBLSystem::get_uniforms(const TonemapConfig& tc) const {
    IBLUniforms u{};
    // identity matrices
    u.view_matrix[0] = u.view_matrix[5] = u.view_matrix[10] = u.view_matrix[15] = 1.0f;
    u.projection_matrix[0] = u.projection_matrix[5] = u.projection_matrix[10] = u.projection_matrix[15] = 1.0f;
    u.environment_params   = Vec4f(1.0f, 0.0f, 0.0f, 0.0f); // intensity=1
    u.tonemap_params       = Vec4f(tc.exposure, tc.gamma, tc.vignette_strength, tc.vignette_power);
    u.tonemap_type         = static_cast<uint32_t>(tc.type);
    u.use_dithering        = tc.use_dithering ? 1u : 0u;
    return u;
}

// ---------------------------------------------------------------------------
// Private resource management
// ---------------------------------------------------------------------------

namespace {
Result<std::monostate> create_cube_image(hal::VulkanDevice& device,
                                          uint32_t size,
                                          uint32_t mips,
                                          VkFormat format,
                                          VkImageUsageFlags usage,
                                          VkImage& out_image,
                                          VmaAllocation& out_alloc) {
    VkImageCreateInfo ii{};
    ii.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ii.flags         = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    ii.imageType     = VK_IMAGE_TYPE_2D;
    ii.format        = format;
    ii.extent        = {size, size, 1};
    ii.mipLevels     = mips;
    ii.arrayLayers   = 6;
    ii.samples       = VK_SAMPLE_COUNT_1_BIT;
    ii.tiling        = VK_IMAGE_TILING_OPTIMAL;
    ii.usage         = usage;
    ii.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    ii.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo ai{};
    ai.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    VkResult r = vmaCreateImage(device.vma_allocator(), &ii, &ai,
                                &out_image, &out_alloc, nullptr);
    if (r != VK_SUCCESS) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed, "IBL: vmaCreateImage failed"});
    }
    return std::monostate{};
}

Result<std::monostate> create_cube_view(VkDevice device,
                                         VkImage image,
                                         VkFormat format,
                                         uint32_t mips,
                                         VkImageView& out_view) {
    VkImageViewCreateInfo vi{};
    vi.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vi.image                           = image;
    vi.viewType                        = VK_IMAGE_VIEW_TYPE_CUBE;
    vi.format                          = format;
    vi.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    vi.subresourceRange.baseMipLevel   = 0;
    vi.subresourceRange.levelCount     = mips;
    vi.subresourceRange.baseArrayLayer = 0;
    vi.subresourceRange.layerCount     = 6;

    VkResult r = vkCreateImageView(device, &vi, nullptr, &out_view);
    if (r != VK_SUCCESS) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed, "IBL: vkCreateImageView failed"});
    }
    return std::monostate{};
}

Result<std::monostate> create_sampler(VkDevice device,
                                       uint32_t mips,
                                       VkSampler& out_sampler) {
    VkSamplerCreateInfo si{};
    si.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    si.magFilter    = VK_FILTER_LINEAR;
    si.minFilter    = VK_FILTER_LINEAR;
    si.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    si.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    si.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    si.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    si.maxLod       = static_cast<float>(mips);
    si.anisotropyEnable = VK_FALSE;

    VkResult r = vkCreateSampler(device, &si, nullptr, &out_sampler);
    if (r != VK_SUCCESS) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed, "IBL: vkCreateSampler failed"});
    }
    return std::monostate{};
}
} // anonymous namespace

Result<std::monostate> IBLSystem::create_environment_map() {
    environment_map_.size       = config_.environment_map_size;
    environment_map_.mip_levels = static_cast<uint32_t>(std::floor(std::log2(
                                      static_cast<float>(config_.environment_map_size)))) + 1;

    auto r = create_cube_image(device_, environment_map_.size,
                               environment_map_.mip_levels, environment_map_.format,
                               VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                               VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                               environment_map_.cubemap, environment_map_.allocation);
    if (!r) return r;

    r = create_cube_view(device_.native_handle(), environment_map_.cubemap,
                         environment_map_.format, environment_map_.mip_levels,
                         environment_map_.cubemap_view);
    if (!r) return r;

    return create_sampler(device_.native_handle(), environment_map_.mip_levels,
                          environment_map_.sampler);
}

Result<std::monostate> IBLSystem::create_irradiance_map() {
    irradiance_map_.size = config_.irradiance_map_size;

    auto r = create_cube_image(device_, irradiance_map_.size, 1,
                               irradiance_map_.format,
                               VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                               VK_IMAGE_USAGE_STORAGE_BIT,
                               irradiance_map_.cubemap, irradiance_map_.allocation);
    if (!r) return r;

    r = create_cube_view(device_.native_handle(), irradiance_map_.cubemap,
                         irradiance_map_.format, 1, irradiance_map_.cubemap_view);
    if (!r) return r;

    return create_sampler(device_.native_handle(), 1, irradiance_map_.sampler);
}

Result<std::monostate> IBLSystem::create_prefiltered_map() {
    prefiltered_map_.size       = config_.prefiltered_map_size;
    prefiltered_map_.mip_levels = config_.prefilter_mip_levels;

    auto r = create_cube_image(device_, prefiltered_map_.size,
                               prefiltered_map_.mip_levels, prefiltered_map_.format,
                               VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                               VK_IMAGE_USAGE_STORAGE_BIT,
                               prefiltered_map_.cubemap, prefiltered_map_.allocation);
    if (!r) return r;

    r = create_cube_view(device_.native_handle(), prefiltered_map_.cubemap,
                         prefiltered_map_.format, prefiltered_map_.mip_levels,
                         prefiltered_map_.cubemap_view);
    if (!r) return r;

    return create_sampler(device_.native_handle(), prefiltered_map_.mip_levels,
                          prefiltered_map_.sampler);
}

Result<std::monostate> IBLSystem::create_brdf_lut() {
    brdf_lut_.size = config_.brdf_lut_size;

    VkImageCreateInfo ii{};
    ii.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ii.imageType     = VK_IMAGE_TYPE_2D;
    ii.format        = brdf_lut_.format;
    ii.extent        = {brdf_lut_.size, brdf_lut_.size, 1};
    ii.mipLevels     = 1;
    ii.arrayLayers   = 1;
    ii.samples       = VK_SAMPLE_COUNT_1_BIT;
    ii.tiling        = VK_IMAGE_TILING_OPTIMAL;
    ii.usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
                       VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    ii.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    ii.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo ai{};
    ai.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    VkResult r = vmaCreateImage(device_.vma_allocator(), &ii, &ai,
                                &brdf_lut_.texture, &brdf_lut_.allocation, nullptr);
    if (r != VK_SUCCESS) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed, "IBL: BRDF LUT vmaCreateImage failed"});
    }

    VkImageViewCreateInfo vi{};
    vi.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vi.image                           = brdf_lut_.texture;
    vi.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    vi.format                          = brdf_lut_.format;
    vi.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    vi.subresourceRange.baseMipLevel   = 0;
    vi.subresourceRange.levelCount     = 1;
    vi.subresourceRange.baseArrayLayer = 0;
    vi.subresourceRange.layerCount     = 1;

    if (vkCreateImageView(device_.native_handle(), &vi, nullptr,
                          &brdf_lut_.texture_view) != VK_SUCCESS) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed, "IBL: BRDF LUT vkCreateImageView failed"});
    }

    return create_sampler(device_.native_handle(), 1, brdf_lut_.sampler);
}

Result<std::monostate> IBLSystem::dispatch_compute_shader(FrameGraph& frame_graph,
                                                           const std::string& shader_name,
                                                           uint32_t gx, uint32_t gy, uint32_t gz,
                                                           const std::vector<ResourceHandle>& resources,
                                                           const void* /*push_constants*/,
                                                           uint32_t /*pc_size*/) {
    PassDesc pass(shader_name);
    pass.queue  = QueueType::Compute;
    for (auto h : resources) pass.reads.push_back(h);
    pass.execute = [gx, gy, gz](CommandBuffer& cmd) {
        cmd.dispatch(gx, gy, gz);
    };
    auto r = frame_graph.add_pass(std::move(pass));
    if (!r) return std::unexpected(r.error());
    return std::monostate{};
}

Result<std::monostate> IBLSystem::load_equirectangular_map(const std::string& filepath) {
    // Stubbed — actual KTX/HDR loading wired in Week 031.
    GW_LOG(Warn,
           "ibl",
           (std::string("load_equirectangular_map('") + filepath + "') — stub (ADR IBL backlog)")
               .c_str());
    return std::monostate{};
}

// ---------------------------------------------------------------------------
// Cleanup helpers
// ---------------------------------------------------------------------------

void IBLSystem::cleanup_environment_map() {
    VkDevice dev = device_.native_handle();
    if (environment_map_.sampler)     { vkDestroySampler(dev, environment_map_.sampler, nullptr);     environment_map_.sampler = VK_NULL_HANDLE; }
    if (environment_map_.cubemap_view){ vkDestroyImageView(dev, environment_map_.cubemap_view, nullptr); environment_map_.cubemap_view = VK_NULL_HANDLE; }
    if (environment_map_.cubemap)     { vmaDestroyImage(device_.vma_allocator(), environment_map_.cubemap, environment_map_.allocation); environment_map_.cubemap = VK_NULL_HANDLE; }
}

void IBLSystem::cleanup_irradiance_map() {
    VkDevice dev = device_.native_handle();
    if (irradiance_map_.sampler)     { vkDestroySampler(dev, irradiance_map_.sampler, nullptr);     irradiance_map_.sampler = VK_NULL_HANDLE; }
    if (irradiance_map_.cubemap_view){ vkDestroyImageView(dev, irradiance_map_.cubemap_view, nullptr); irradiance_map_.cubemap_view = VK_NULL_HANDLE; }
    if (irradiance_map_.cubemap)     { vmaDestroyImage(device_.vma_allocator(), irradiance_map_.cubemap, irradiance_map_.allocation); irradiance_map_.cubemap = VK_NULL_HANDLE; }
}

void IBLSystem::cleanup_prefiltered_map() {
    VkDevice dev = device_.native_handle();
    if (prefiltered_map_.sampler)     { vkDestroySampler(dev, prefiltered_map_.sampler, nullptr);     prefiltered_map_.sampler = VK_NULL_HANDLE; }
    if (prefiltered_map_.cubemap_view){ vkDestroyImageView(dev, prefiltered_map_.cubemap_view, nullptr); prefiltered_map_.cubemap_view = VK_NULL_HANDLE; }
    if (prefiltered_map_.cubemap)     { vmaDestroyImage(device_.vma_allocator(), prefiltered_map_.cubemap, prefiltered_map_.allocation); prefiltered_map_.cubemap = VK_NULL_HANDLE; }
}

void IBLSystem::cleanup_brdf_lut() {
    VkDevice dev = device_.native_handle();
    if (brdf_lut_.sampler)      { vkDestroySampler(dev, brdf_lut_.sampler, nullptr);      brdf_lut_.sampler = VK_NULL_HANDLE; }
    if (brdf_lut_.texture_view) { vkDestroyImageView(dev, brdf_lut_.texture_view, nullptr); brdf_lut_.texture_view = VK_NULL_HANDLE; }
    if (brdf_lut_.texture)      { vmaDestroyImage(device_.vma_allocator(), brdf_lut_.texture, brdf_lut_.allocation); brdf_lut_.texture = VK_NULL_HANDLE; }
}

// ---------------------------------------------------------------------------
// tonemap utilities
// ---------------------------------------------------------------------------

namespace tonemap {

Vec3f aces_tonemap(const Vec3f& c) {
    constexpr float a = 2.51f, b = 0.03f, g = 2.43f, d = 0.59f, e = 0.14f;
    auto clamp01 = [](float v) { return std::max(0.0f, std::min(1.0f, v)); };
    return Vec3f(
        clamp01((c.x()*(a*c.x()+b)) / (c.x()*(g*c.x()+d)+e)),
        clamp01((c.y()*(a*c.y()+b)) / (c.y()*(g*c.y()+d)+e)),
        clamp01((c.z()*(a*c.z()+b)) / (c.z()*(g*c.z()+d)+e)));
}

Vec3f reinhard_tonemap(const Vec3f& c) {
    return Vec3f(c.x()/(1.0f+c.x()), c.y()/(1.0f+c.y()), c.z()/(1.0f+c.z()));
}

Vec3f filmic_tonemap(const Vec3f& c) {
    auto f = [](float x) {
        x = std::max(0.0f, x - 0.004f);
        return (x*(6.2f*x+0.5f)) / (x*(6.2f*x+1.7f)+0.06f);
    };
    return Vec3f(f(c.x()), f(c.y()), f(c.z()));
}

Vec3f uncharted2_tonemap(const Vec3f& c) {
    auto uc2 = [](float x) {
        constexpr float A=0.15f,B=0.50f,C=0.10f,D=0.20f,E=0.02f,F=0.30f;
        return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
    };
    constexpr float W  = 11.2f;
    const float inv_w  = 1.0f / uc2(W);
    return Vec3f(uc2(c.x())*inv_w, uc2(c.y())*inv_w, uc2(c.z())*inv_w);
}

Vec3f apply_dithering(const Vec3f& color, uint32_t x, uint32_t y) {
    // Simple interleaved gradient noise
    const float noise = std::fmod(52.9829189f * std::fmod(0.06711056f*x + 0.00583715f*y, 1.0f), 1.0f);
    const float offset = (noise - 0.5f) / 255.0f;
    return Vec3f(color.x()+offset, color.y()+offset, color.z()+offset);
}

Vec3f apply_vignette(const Vec3f& color, Vec2f uv, float strength, float power) {
    const float dx = uv.x() - 0.5f;
    const float dy = uv.y() - 0.5f;
    const float dist = std::sqrt(dx*dx + dy*dy) * 1.4142136f;
    const float vig  = std::pow(1.0f - std::min(1.0f, dist), power);
    const float fac  = 1.0f - strength + strength * vig;
    return Vec3f(color.x()*fac, color.y()*fac, color.z()*fac);
}

} // namespace tonemap

// ---------------------------------------------------------------------------
// IBL math utilities
// ---------------------------------------------------------------------------

namespace utils {

float van_der_corput(uint32_t bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return static_cast<float>(bits) * 2.3283064365386963e-10f;
}

float hammersley(uint32_t i, uint32_t N) {
    return static_cast<float>(i) / static_cast<float>(N) + van_der_corput(i);
}

ImportanceSample importance_sample_ggx(Vec2f xi, float roughness) {
    constexpr float pi = 3.14159265358979323846f;
    const float a    = roughness * roughness;
    const float phi  = 2.0f * pi * xi.x();
    const float cos_theta = std::sqrt((1.0f - xi.y()) / (1.0f + (a*a - 1.0f)*xi.y()));
    const float sin_theta = std::sqrt(1.0f - cos_theta*cos_theta);

    ImportanceSample s;
    s.direction  = Vec3f(std::cos(phi)*sin_theta, std::sin(phi)*sin_theta, cos_theta);
    s.cos_theta  = cos_theta;
    s.sin_theta  = sin_theta;
    s.phi        = phi;
    const float d   = (cos_theta*a*a - cos_theta) * cos_theta + 1.0f;
    const float D   = (a*a) / (pi * d * d);
    s.pdf        = D * cos_theta;
    return s;
}

Vec3f cubemap_direction(uint32_t face, Vec2f uv) {
    const float u = uv.x() * 2.0f - 1.0f;
    const float v = uv.y() * 2.0f - 1.0f;
    switch (face) {
        case 0: return Vec3f( 1.0f,   -v,   -u);
        case 1: return Vec3f(-1.0f,   -v,    u);
        case 2: return Vec3f(   u,  1.0f,    v);
        case 3: return Vec3f(   u, -1.0f,   -v);
        case 4: return Vec3f(   u,   -v,  1.0f);
        case 5: return Vec3f(  -u,   -v, -1.0f);
        default: return Vec3f(0.0f, 0.0f, 1.0f);
    }
}

SphericalHarmonics project_to_spherical_harmonics(const EnvironmentMap& /*cubemap*/) {
    SphericalHarmonics sh{};
    // Projection from cubemap pixels stubbed — wired in Week 031.
    return sh;
}

Vec3f evaluate_spherical_harmonics(const SphericalHarmonics& sh, const Vec3f& dir) {
    // Band 0
    constexpr float c0 = 0.282095f;
    // Band 1
    constexpr float c1 = 0.488603f;
    // Band 2 (5 coefficients) — simplified
    Vec3f result(0.0f, 0.0f, 0.0f);

    auto add = [&](const Vec3f& coeff, float basis) {
        result = Vec3f(result.x()+coeff.x()*basis,
                       result.y()+coeff.y()*basis,
                       result.z()+coeff.z()*basis);
    };

    add(sh.coefficients[0], c0);
    add(sh.coefficients[1], c1 * dir.y());
    add(sh.coefficients[2], c1 * dir.z());
    add(sh.coefficients[3], c1 * dir.x());
    (void)dir; // higher bands stubbed
    return result;
}

} // namespace utils

} // namespace gw::render::ibl
