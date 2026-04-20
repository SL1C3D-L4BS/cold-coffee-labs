#include "ibl.hpp"
#include "engine/render/hal/device.hpp"
#include "engine/render/hal/image.hpp"
#include "engine/render/hal/buffer.hpp"
#include "engine/core/log.hpp"
#include <algorithm>
#include <cmath>
#include <random>

namespace cold_coffee::engine::render::ibl {

// EnvironmentMap implementation
frame_graph::ResourceHandle EnvironmentMap::create_resource(frame_graph::FrameGraph& frame_graph) const {
    frame_graph::ResourceDesc desc = {};
    desc.type = frame_graph::ResourceType::ImageCube;
    desc.format = format;
    desc.width = size;
    desc.height = size;
    desc.depth = 1;
    desc.mip_levels = mip_levels;
    desc.array_layers = 6;
    desc.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    desc.lifetime = frame_graph::ResourceLifetime::Persistent;
    desc.name = "environment_map";
    
    return frame_graph.create_resource(desc).unwrap();
}

// IrradianceMap implementation
frame_graph::ResourceHandle IrradianceMap::create_resource(frame_graph::FrameGraph& frame_graph) const {
    frame_graph::ResourceDesc desc = {};
    desc.type = frame_graph::ResourceType::ImageCube;
    desc.format = format;
    desc.width = size;
    desc.height = size;
    desc.depth = 1;
    desc.mip_levels = 1;
    desc.array_layers = 6;
    desc.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    desc.lifetime = frame_graph::ResourceLifetime::Persistent;
    desc.name = "irradiance_map";
    
    return frame_graph.create_resource(desc).unwrap();
}

// PrefilteredMap implementation
frame_graph::ResourceHandle PrefilteredMap::create_resource(frame_graph::FrameGraph& frame_graph) const {
    frame_graph::ResourceDesc desc = {};
    desc.type = frame_graph::ResourceType::ImageCube;
    desc.format = format;
    desc.width = size;
    desc.height = size;
    desc.depth = 1;
    desc.mip_levels = mip_levels;
    desc.array_layers = 6;
    desc.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    desc.lifetime = frame_graph::ResourceLifetime::Persistent;
    desc.name = "prefiltered_map";
    
    return frame_graph.create_resource(desc).unwrap();
}

// BRDFLUT implementation
frame_graph::ResourceHandle BRDFLUT::create_resource(frame_graph::FrameGraph& frame_graph) const {
    frame_graph::ResourceDesc desc = {};
    desc.type = frame_graph::ResourceType::Image2D;
    desc.format = format;
    desc.width = size;
    desc.height = size;
    desc.depth = 1;
    desc.mip_levels = 1;
    desc.array_layers = 1;
    desc.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    desc.lifetime = frame_graph::ResourceLifetime::Persistent;
    desc.name = "brdf_lut";
    
    return frame_graph.create_resource(desc).unwrap();
}

// IBLSystem implementation
IBLSystem::IBLSystem(const hal::Device& device)
    : device_(device) {
}

IBLSystem::~IBLSystem() {
    cleanup_environment_map();
    cleanup_irradiance_map();
    cleanup_prefiltered_map();
    cleanup_brdf_lut();
}

Result<void> IBLSystem::initialize(const IBLConfig& config) {
    config_ = config;
    
    // Create IBL resources
    auto result = create_environment_map();
    if (result.is_err()) {
        return result;
    }
    
    result = create_irradiance_map();
    if (result.is_err()) {
        return result;
    }
    
    result = create_prefiltered_map();
    if (result.is_err()) {
        return result;
    }
    
    result = create_brdf_lut();
    if (result.is_err()) {
        return result;
    }
    
    return Ok();
}

Result<void> IBLSystem::load_environment_map(const std::string& filepath) {
    return load_equirectangular_map(filepath);
}

Result<void> IBLSystem::generate_ibl_resources(frame_graph::FrameGraph& frame_graph) {
    // Generate irradiance map
    auto irradiance_result = create_irradiance_pass(frame_graph, environment_map_, irradiance_map_);
    if (irradiance_result.is_err()) {
        return Err(irradiance_result.unwrap_err());
    }
    
    // Generate prefiltered map
    auto prefilter_result = create_prefilter_pass(frame_graph, environment_map_, prefiltered_map_);
    if (prefilter_result.is_err()) {
        return Err(prefilter_result.unwrap_err());
    }
    
    // Generate BRDF LUT
    auto brdf_result = create_brdf_lut_pass(frame_graph, brdf_lut_);
    if (brdf_result.is_err()) {
        return Err(brdf_result.unwrap_err());
    }
    
    return Ok();
}

Result<frame_graph::PassHandle> IBLSystem::create_irradiance_pass(
    frame_graph::FrameGraph& frame_graph,
    const EnvironmentMap& env_map,
    const IrradianceMap& irradiance_map) {
    
    frame_graph::PassDesc pass("IrradianceMapGeneration");
    pass.queue = frame_graph::QueueType::Graphics;
    
    // Read environment map
    auto env_handle = env_map.create_resource(frame_graph);
    pass.reads.push_back(env_handle);
    
    // Write irradiance map
    auto irradiance_handle = irradiance_map.create_resource(frame_graph);
    pass.writes.push_back(irradiance_handle);
    
    pass.execute = [this, env_handle, irradiance_handle](frame_graph::CommandBuffer& cmd) {
        // Render to each face of the irradiance cubemap
        for (uint32_t face = 0; face < 6; ++face) {
            // Set viewport for irradiance map size
            VkViewport viewport = {};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(config_.irradiance_map_size);
            viewport.height = static_cast<float>(config_.irradiance_map_size);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            
            cmd.set_viewport(0, 1, &viewport);
            
            // Set scissor
            VkRect2D scissor = {};
            scissor.offset = {0, 0};
            scissor.extent = {config_.irradiance_map_size, config_.irradiance_map_size};
            
            cmd.set_scissor(0, 1, &scissor);
            
            // Bind irradiance shader pipeline
            // cmd.bind_pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, irradiance_pipeline_);
            
            // Set face-specific view matrix
            // cmd.push_constants(...);
            
            // Draw fullscreen quad
            // cmd.draw(3, 1, 0, 0);
        }
    };
    
    return frame_graph.add_pass(std::move(pass));
}

Result<frame_graph::PassHandle> IBLSystem::create_prefilter_pass(
    frame_graph::FrameGraph& frame_graph,
    const EnvironmentMap& env_map,
    const PrefilteredMap& prefiltered_map) {
    
    frame_graph::PassDesc pass("PrefilteredMapGeneration");
    pass.queue = frame_graph::QueueType::Graphics;
    
    // Read environment map
    auto env_handle = env_map.create_resource(frame_graph);
    pass.reads.push_back(env_handle);
    
    // Write prefiltered map
    auto prefilter_handle = prefiltered_map.create_resource(frame_graph);
    pass.writes.push_back(prefilter_handle);
    
    pass.execute = [this, env_handle, prefilter_handle](frame_graph::CommandBuffer& cmd) {
        // Generate each mip level
        for (uint32_t mip = 0; mip < config_.prefilter_mip_levels; ++mip) {
            uint32_t mip_size = config_.prefiltered_map_size >> mip;
            
            // Render to each face of this mip level
            for (uint32_t face = 0; face < 6; ++face) {
                // Set viewport for this mip level
                VkViewport viewport = {};
                viewport.x = 0.0f;
                viewport.y = 0.0f;
                viewport.width = static_cast<float>(mip_size);
                viewport.height = static_cast<float>(mip_size);
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;
                
                cmd.set_viewport(0, 1, &viewport);
                
                // Set scissor
                VkRect2D scissor = {};
                scissor.offset = {0, 0};
                scissor.extent = {mip_size, mip_size};
                
                cmd.set_scissor(0, 1, &scissor);
                
                // Bind prefilter shader pipeline
                // cmd.bind_pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, prefilter_pipeline_);
                
                // Set face and mip-specific parameters
                float roughness = static_cast<float>(mip) / static_cast<float>(config_.prefilter_mip_levels - 1);
                // cmd.push_constants(...);
                
                // Draw fullscreen quad
                // cmd.draw(3, 1, 0, 0);
            }
        }
    };
    
    return frame_graph.add_pass(std::move(pass));
}

Result<frame_graph::PassHandle> IBLSystem::create_brdf_lut_pass(
    frame_graph::FrameGraph& frame_graph,
    const BRDFLUT& brdf_lut) {
    
    frame_graph::PassDesc pass("BRDFLUTGeneration");
    pass.queue = frame_graph::QueueType::Graphics;
    
    // Write BRDF LUT
    auto brdf_handle = brdf_lut.create_resource(frame_graph);
    pass.writes.push_back(brdf_handle);
    
    pass.execute = [this, brdf_handle](frame_graph::CommandBuffer& cmd) {
        // Set viewport for BRDF LUT size
        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(config_.brdf_lut_size);
        viewport.height = static_cast<float>(config_.brdf_lut_size);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        
        cmd.set_viewport(0, 1, &viewport);
        
        // Set scissor
        VkRect2D scissor = {};
        scissor.offset = {0, 0};
        scissor.extent = {config_.brdf_lut_size, config_.brdf_lut_size};
        
        cmd.set_scissor(0, 1, &scissor);
        
        // Bind BRDF LUT shader pipeline
        // cmd.bind_pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, brdf_lut_pipeline_);
        
        // Draw fullscreen quad
        // cmd.draw(3, 1, 0, 0);
    };
    
    return frame_graph.add_pass(std::move(pass));
}

Result<frame_graph::PassHandle> IBLSystem::create_tonemap_pass(
    frame_graph::FrameGraph& frame_graph,
    const TonemapConfig& tonemap_config) {
    
    frame_graph::PassDesc pass("Tonemapping");
    pass.queue = frame_graph::QueueType::Graphics;
    
    // Read HDR color buffer
    // auto hdr_handle = get_hdr_color_buffer();
    // pass.reads.push_back(hdr_handle);
    
    // Write LDR color buffer
    // auto ldr_handle = get_ldr_color_buffer();
    // pass.writes.push_back(ldr_handle);
    
    pass.execute = [this, tonemap_config](frame_graph::CommandBuffer& cmd) {
        // Set viewport for screen size
        // VkViewport viewport = {};
        // viewport.x = 0.0f;
        // viewport.y = 0.0f;
        // viewport.width = static_cast<float>(screen_width);
        // viewport.height = static_cast<float>(screen_height);
        // viewport.minDepth = 0.0f;
        // viewport.maxDepth = 1.0f;
        
        // cmd.set_viewport(0, 1, &viewport);
        
        // Bind tonemap shader pipeline
        // cmd.bind_pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, tonemap_pipeline_);
        
        // Set tonemap parameters
        IBLUniforms uniforms = get_uniforms(tonemap_config);
        // cmd.push_constants(...);
        
        // Draw fullscreen quad
        // cmd.draw(3, 1, 0, 0);
    };
    
    return frame_graph.add_pass(std::move(pass));
}

IBLSystem::IBLUniforms IBLSystem::get_uniforms(const TonemapConfig& tonemap_config) const {
    IBLUniforms uniforms = {};
    
    // These would be set from current camera state
    uniforms.view_matrix = glm::mat4(1.0f);
    uniforms.projection_matrix = glm::mat4(1.0f);
    
    uniforms.environment_params = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f); // intensity, rotation, mip_level, 0
    uniforms.tonemap_params = glm::vec4(
        tonemap_config.exposure,
        tonemap_config.gamma,
        tonemap_config.vignette_strength,
        tonemap_config.vignette_power
    );
    
    uniforms.tonemap_type = static_cast<uint32_t>(tonemap_config.type);
    uniforms.use_dithering = tonemap_config.use_dithering ? 1 : 0;
    
    return uniforms;
}

Result<void> IBLSystem::create_environment_map() {
    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = config_.environment_map_size;
    image_info.extent.height = config_.environment_map_size;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1; // Will be calculated
    image_info.arrayLayers = 6;
    image_info.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    
    // Calculate mip levels
    image_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(config_.environment_map_size, 1u)))) + 1;
    
    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    
    VkResult result = vmaCreateImage(device_.allocator(), &image_info, &alloc_info,
                                     &environment_map_.cubemap, &environment_map_.allocation);
    if (result != VK_SUCCESS) {
        return Err(frame_graph::FrameGraphError{
            frame_graph::FrameGraphErrorType::ResourceCreationFailed,
            "Failed to create environment map image"
        });
    }
    
    environment_map_.size = config_.environment_map_size;
    environment_map_.mip_levels = image_info.mipLevels;
    environment_map_.format = image_info.format;
    
    // Create image view
    VkImageViewCreateInfo view_info = {};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = environment_map_.cubemap;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    view_info.format = image_info.format;
    view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = image_info.mipLevels;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 6;
    
    result = vkCreateImageView(device_.device(), &view_info, nullptr, &environment_map_.cubemap_view);
    if (result != VK_SUCCESS) {
        return Err(frame_graph::FrameGraphError{
            frame_graph::FrameGraphErrorType::ResourceCreationFailed,
            "Failed to create environment map image view"
        });
    }
    
    // Create sampler
    VkSamplerCreateInfo sampler_info = {};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.anisotropyEnable = VK_TRUE;
    sampler_info.maxAnisotropy = 16.0f;
    sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = static_cast<float>(image_info.mipLevels);
    
    result = vkCreateSampler(device_.device(), &sampler_info, nullptr, &environment_map_.sampler);
    if (result != VK_SUCCESS) {
        return Err(frame_graph::FrameGraphError{
            frame_graph::FrameGraphErrorType::ResourceCreationFailed,
            "Failed to create environment map sampler"
        });
    }
    
    return Ok();
}

Result<void> IBLSystem::create_irradiance_map() {
    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = config_.irradiance_map_size;
    image_info.extent.height = config_.irradiance_map_size;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 6;
    image_info.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    
    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    
    VkResult result = vmaCreateImage(device_.allocator(), &image_info, &alloc_info,
                                     &irradiance_map_.cubemap, &irradiance_map_.allocation);
    if (result != VK_SUCCESS) {
        return Err(frame_graph::FrameGraphError{
            frame_graph::FrameGraphErrorType::ResourceCreationFailed,
            "Failed to create irradiance map image"
        });
    }
    
    irradiance_map_.size = config_.irradiance_map_size;
    irradiance_map_.format = image_info.format;
    
    // Create image view
    VkImageViewCreateInfo view_info = {};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = irradiance_map_.cubemap;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    view_info.format = image_info.format;
    view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 6;
    
    result = vkCreateImageView(device_.device(), &view_info, nullptr, &irradiance_map_.cubemap_view);
    if (result != VK_SUCCESS) {
        return Err(frame_graph::FrameGraphError{
            frame_graph::FrameGraphErrorType::ResourceCreationFailed,
            "Failed to create irradiance map image view"
        });
    }
    
    // Create sampler
    VkSamplerCreateInfo sampler_info = {};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.anisotropyEnable = VK_FALSE;
    sampler_info.maxAnisotropy = 1.0f;
    sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = VK_LOD_CLAMP_NONE;
    
    result = vkCreateSampler(device_.device(), &sampler_info, nullptr, &irradiance_map_.sampler);
    if (result != VK_SUCCESS) {
        return Err(frame_graph::FrameGraphError{
            frame_graph::FrameGraphErrorType::ResourceCreationFailed,
            "Failed to create irradiance map sampler"
        });
    }
    
    return Ok();
}

Result<void> IBLSystem::create_prefiltered_map() {
    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = config_.prefiltered_map_size;
    image_info.extent.height = config_.prefiltered_map_size;
    image_info.extent.depth = 1;
    image_info.mipLevels = config_.prefilter_mip_levels;
    image_info.arrayLayers = 6;
    image_info.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    
    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    
    VkResult result = vmaCreateImage(device_.allocator(), &image_info, &alloc_info,
                                     &prefiltered_map_.cubemap, &prefiltered_map_.allocation);
    if (result != VK_SUCCESS) {
        return Err(frame_graph::FrameGraphError{
            frame_graph::FrameGraphErrorType::ResourceCreationFailed,
            "Failed to create prefiltered map image"
        });
    }
    
    prefiltered_map_.size = config_.prefiltered_map_size;
    prefiltered_map_.mip_levels = config_.prefilter_mip_levels;
    prefiltered_map_.format = image_info.format;
    
    // Create image view
    VkImageViewCreateInfo view_info = {};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = prefiltered_map_.cubemap;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    view_info.format = image_info.format;
    view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = config_.prefilter_mip_levels;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 6;
    
    result = vkCreateImageView(device_.device(), &view_info, nullptr, &prefiltered_map_.cubemap_view);
    if (result != VK_SUCCESS) {
        return Err(frame_graph::FrameGraphError{
            frame_graph::FrameGraphErrorType::ResourceCreationFailed,
            "Failed to create prefiltered map image view"
        });
    }
    
    // Create sampler
    VkSamplerCreateInfo sampler_info = {};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.anisotropyEnable = VK_FALSE;
    sampler_info.maxAnisotropy = 1.0f;
    sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = static_cast<float>(config_.prefilter_mip_levels);
    
    result = vkCreateSampler(device_.device(), &sampler_info, nullptr, &prefiltered_map_.sampler);
    if (result != VK_SUCCESS) {
        return Err(frame_graph::FrameGraphError{
            frame_graph::FrameGraphErrorType::ResourceCreationFailed,
            "Failed to create prefiltered map sampler"
        });
    }
    
    return Ok();
}

Result<void> IBLSystem::create_brdf_lut() {
    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = config_.brdf_lut_size;
    image_info.extent.height = config_.brdf_lut_size;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = VK_FORMAT_R16G16_SFLOAT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    
    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    
    VkResult result = vmaCreateImage(device_.allocator(), &image_info, &alloc_info,
                                     &brdf_lut_.texture, &brdf_lut_.allocation);
    if (result != VK_SUCCESS) {
        return Err(frame_graph::FrameGraphError{
            frame_graph::FrameGraphErrorType::ResourceCreationFailed,
            "Failed to create BRDF LUT image"
        });
    }
    
    brdf_lut_.size = config_.brdf_lut_size;
    brdf_lut_.format = image_info.format;
    
    // Create image view
    VkImageViewCreateInfo view_info = {};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = brdf_lut_.texture;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = image_info.format;
    view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;
    
    result = vkCreateImageView(device_.device(), &view_info, nullptr, &brdf_lut_.texture_view);
    if (result != VK_SUCCESS) {
        return Err(frame_graph::FrameGraphError{
            frame_graph::FrameGraphErrorType::ResourceCreationFailed,
            "Failed to create BRDF LUT image view"
        });
    }
    
    // Create sampler
    VkSamplerCreateInfo sampler_info = {};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.anisotropyEnable = VK_FALSE;
    sampler_info.maxAnisotropy = 1.0f;
    sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = VK_LOD_CLAMP_NONE;
    
    result = vkCreateSampler(device_.device(), &sampler_info, nullptr, &brdf_lut_.sampler);
    if (result != VK_SUCCESS) {
        return Err(frame_graph::FrameGraphError{
            frame_graph::FrameGraphErrorType::ResourceCreationFailed,
            "Failed to create BRDF LUT sampler"
        });
    }
    
    return Ok();
}

void IBLSystem::cleanup_environment_map() {
    if (environment_map_.sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device_.device(), environment_map_.sampler, nullptr);
        environment_map_.sampler = VK_NULL_HANDLE;
    }
    
    if (environment_map_.cubemap_view != VK_NULL_HANDLE) {
        vkDestroyImageView(device_.device(), environment_map_.cubemap_view, nullptr);
        environment_map_.cubemap_view = VK_NULL_HANDLE;
    }
    
    if (environment_map_.cubemap != VK_NULL_HANDLE) {
        vmaDestroyImage(device_.allocator(), environment_map_.cubemap, environment_map_.allocation);
        environment_map_.cubemap = VK_NULL_HANDLE;
        environment_map_.allocation = VK_NULL_HANDLE;
    }
}

void IBLSystem::cleanup_irradiance_map() {
    if (irradiance_map_.sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device_.device(), irradiance_map_.sampler, nullptr);
        irradiance_map_.sampler = VK_NULL_HANDLE;
    }
    
    if (irradiance_map_.cubemap_view != VK_NULL_HANDLE) {
        vkDestroyImageView(device_.device(), irradiance_map_.cubemap_view, nullptr);
        irradiance_map_.cubemap_view = VK_NULL_HANDLE;
    }
    
    if (irradiance_map_.cubemap != VK_NULL_HANDLE) {
        vmaDestroyImage(device_.allocator(), irradiance_map_.cubemap, irradiance_map_.allocation);
        irradiance_map_.cubemap = VK_NULL_HANDLE;
        irradiance_map_.allocation = VK_NULL_HANDLE;
    }
}

void IBLSystem::cleanup_prefiltered_map() {
    if (prefiltered_map_.sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device_.device(), prefiltered_map_.sampler, nullptr);
        prefiltered_map_.sampler = VK_NULL_HANDLE;
    }
    
    if (prefiltered_map_.cubemap_view != VK_NULL_HANDLE) {
        vkDestroyImageView(device_.device(), prefiltered_map_.cubemap_view, nullptr);
        prefiltered_map_.cubemap_view = VK_NULL_HANDLE;
    }
    
    if (prefiltered_map_.cubemap != VK_NULL_HANDLE) {
        vmaDestroyImage(device_.allocator(), prefiltered_map_.cubemap, prefiltered_map_.allocation);
        prefiltered_map_.cubemap = VK_NULL_HANDLE;
        prefiltered_map_.allocation = VK_NULL_HANDLE;
    }
}

void IBLSystem::cleanup_brdf_lut() {
    if (brdf_lut_.sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device_.device(), brdf_lut_.sampler, nullptr);
        brdf_lut_.sampler = VK_NULL_HANDLE;
    }
    
    if (brdf_lut_.texture_view != VK_NULL_HANDLE) {
        vkDestroyImageView(device_.device(), brdf_lut_.texture_view, nullptr);
        brdf_lut_.texture_view = VK_NULL_HANDLE;
    }
    
    if (brdf_lut_.texture != VK_NULL_HANDLE) {
        vmaDestroyImage(device_.allocator(), brdf_lut_.texture, brdf_lut_.allocation);
        brdf_lut_.texture = VK_NULL_HANDLE;
        brdf_lut_.allocation = VK_NULL_HANDLE;
    }
}

Result<void> IBLSystem::load_equirectangular_map(const std::string& filepath) {
    // This would load an HDR image (e.g., .hdr, .exr) and convert to cubemap
    // For now, placeholder implementation
    
    // TODO: Implement HDR image loading (stb_image_hdr or similar)
    // TODO: Convert equirectangular to cubemap using compute shader
    
    return Ok();
}

Result<void> IBLSystem::equirectangular_to_cubemap(const hal::Image& equirectangular) {
    // Convert equirectangular map to cubemap using compute shader
    // This would dispatch a compute shader that samples the equirectangular map
    // and writes to each face of the cubemap
    
    return Ok();
}

// Tonemapping namespace implementation
namespace tonemap {

glm::vec3 aces_tonemap(const glm::vec3& color) {
    // ACES filmic tonemapper approximation
    glm::vec3 x = color * 0.59719f + glm::vec3(0.07600f, 0.09256f, 0.07465f);
    glm::vec3 y = color * 0.35458f + glm::vec3(0.04323f, 0.09571f, 0.04444f);
    glm::vec3 z = color * 0.04823f + glm::vec3(0.02299f, 0.01508f, 0.02066f);
    
    glm::vec3 a = x * y;
    glm::vec3 b = z * z;
    glm::vec3 c = glm::sqrt(a);
    glm::vec3 d = glm::sqrt(b);
    glm::vec3 e = glm::sqrt(c);
    
    glm::vec3 f = e - glm::vec3(0.00266f);
    glm::vec3 g = glm::max(glm::vec3(0.0f), f);
    
    return (g * 1.45142f) / (glm::vec3(0.05556f) + g * 1.45142f);
}

glm::vec3 reinhard_tonemap(const glm::vec3& color) {
    return color / (glm::vec3(1.0f) + color);
}

glm::vec3 filmic_tonemap(const glm::vec3& color) {
    // Filmic tonemapper (Unreal engine style)
    glm::vec3 x = glm::max(glm::vec3(0.0f), color - glm::vec3(0.004f));
    return (x * (6.2f * x + 0.5f)) / (x * (6.2f * x + 1.7f) + 0.06f);
}

glm::vec3 uncharted2_tonemap(const glm::vec3& color) {
    float A = 0.15f;
    float B = 0.50f;
    float C = 0.10f;
    float D = 0.20f;
    float E = 0.02f;
    float F = 0.30f;
    
    glm::vec3 x = ((color * (A * color + C * B) + D * E) / 
                   (color * (A * color + B) + D * F)) - E / F;
    
    return x;
}

glm::vec3 apply_dithering(const glm::vec3& color, uint32_t x, uint32_t y) {
    // Simple 8x8 Bayer matrix dithering
    static const uint8_t bayer8x8[64] = {
        0,  32, 8,  40, 2,  34, 10, 42,
        48, 16, 56, 24, 50, 18, 58, 26,
        12, 44, 4,  36, 14, 46, 6,  38,
        60, 28, 52, 20, 62, 30, 54, 22,
        3,  35, 11, 43, 1,  33, 9,  41,
        51, 19, 59, 27, 49, 17, 57, 25,
        15, 47, 7,  39, 13, 45, 5,  37,
        63, 31, 55, 23, 61, 29, 53, 21
    };
    
    uint32_t index = (y & 7) * 8 + (x & 7);
    float dither = static_cast<float>(bayer8x8[index]) / 64.0f;
    
    return color + glm::vec3(dither * 0.01f); // Small dither amount
}

glm::vec3 apply_vignette(const glm::vec3& color, float2 uv, 
                        float strength, float power) {
    if (strength <= 0.0f) {
        return color;
    }
    
    float2 center = float2(0.5f, 0.5f);
    float2 dist = uv - center;
    float vignette = 1.0f - glm::length(dist) * strength;
    vignette = glm::pow(glm::max(0.0f, vignette), power);
    
    return color * vignette;
}

} // namespace tonemap

// Utils namespace implementation
namespace utils {

ImportanceSample importance_sample_ggx(float2 xi, float roughness) {
    ImportanceSample sample;
    
    float a = roughness * roughness;
    
    float phi = 2.0f * 3.14159265f * xi.x;
    float cos_theta = glm::sqrt((1.0f - xi.y) / (1.0f + (a * a - 1.0f) * xi.y));
    float sin_theta = glm::sqrt(1.0f - cos_theta * cos_theta);
    
    sample.sin_theta = sin_theta;
    sample.cos_theta = cos_theta;
    sample.phi = phi;
    
    // Convert spherical to Cartesian coordinates
    float3 h;
    h.x = sin_theta * glm::cos(phi);
    h.y = sin_theta * glm::sin(phi);
    h.z = cos_theta;
    
    sample.direction = h;
    
    // Calculate PDF
    float a2 = a * a;
    sample.pdf = (a2 * glm::cos(theta)) / (3.14159265f * glm::pow((a2 - 1.0f) * cos_theta * cos_theta + 1.0f, 2.0f));
    
    return sample;
}

float hammersley(uint32_t i, uint32_t N) {
    return van_der_corput(i) / static_cast<float>(N);
}

float van_der_corput(uint32_t bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return static_cast<float>(bits) * 2.3283064365386963e-10f; // / 0x100000000
}

glm::vec3 cubemap_direction(uint32_t face, float2 uv) {
    // Convert UV to [-1, 1] range
    float2 ndc = uv * 2.0f - 1.0f;
    
    switch (face) {
        case 0:  // +X
            return glm::vec3(1.0f, -ndc.y, -ndc.x);
        case 1:  // -X
            return glm::vec3(-1.0f, -ndc.y, ndc.x);
        case 2:  // +Y
            return glm::vec3(ndc.x, 1.0f, ndc.y);
        case 3:  // -Y
            return glm::vec3(ndc.x, -1.0f, -ndc.y);
        case 4:  // +Z
            return glm::vec3(ndc.x, -ndc.y, 1.0f);
        case 5:  // -Z
            return glm::vec3(-ndc.x, -ndc.y, -1.0f);
        default:
            return glm::vec3(0.0f);
    }
}

glm::vec3 filter_cubemap(const EnvironmentMap& cubemap, 
                        const glm::vec3& direction, 
                        float roughness,
                        uint32_t sample_count) {
    // Simplified cubemap filtering
    // In practice, this would use importance sampling and fetch from the cubemap
    
    glm::vec3 result = glm::vec3(0.0f);
    float total_weight = 0.0f;
    
    for (uint32_t i = 0; i < sample_count; ++i) {
        float2 xi = float2(hammersley(i, sample_count), van_der_corput(i));
        ImportanceSample sample = importance_sample_ggx(xi, roughness);
        
        // TODO: Sample cubemap at sample.direction
        // glm::vec3 sample_color = sample_cubemap(cubemap, sample.direction);
        glm::vec3 sample_color = glm::vec3(0.5f); // Placeholder
        
        result += sample_color * sample.cos_theta;
        total_weight += sample.cos_theta;
    }
    
    return result / total_weight;
}

SphericalHarmonics project_to_spherical_harmonics(const EnvironmentMap& cubemap) {
    SphericalHarmonics sh = {};
    
    // TODO: Project cubemap onto spherical harmonics
    // This would sample the cubemap and project onto SH basis functions
    
    return sh;
}

glm::vec3 evaluate_spherical_harmonics(const SphericalHarmonics& sh, 
                                      const glm::vec3& direction) {
    // Evaluate SH at given direction
    // Simplified implementation
    
    return sh.coefficients[0]; // Placeholder
}

} // namespace utils

} // namespace cold_coffee::engine::render::ibl
