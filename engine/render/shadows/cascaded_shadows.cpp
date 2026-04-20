#include "cascaded_shadows.hpp"
#include "engine/render/hal/device.hpp"
#include "engine/render/hal/buffer.hpp"
#include "engine/core/log.hpp"
#include <algorithm>
#include <cmath>
#include <optional>

namespace cold_coffee::engine::render::shadows {

// CascadeConfig implementation
void CascadeConfig::calculate_splits() {
    // Practical split scheme: mix of logarithmic and uniform distribution
    float lambda = cascade_split_lambda;
    float near_range = near_plane;
    float far_range = far_plane;
    
    for (uint32_t i = 0; i < cascade_count; ++i) {
        float uniform_split = near_range + (far_range - near_range) * 
                             (static_cast<float>(i + 1) / static_cast<float>(cascade_count));
        float log_split = near_range * std::pow(far_range / near_range, 
                                              static_cast<float>(i + 1) / static_cast<float>(cascade_count));
        
        cascade_splits[i] = lambda * log_split + (1.0f - lambda) * uniform_split;
    }
}

// ShadowMap implementation
ShadowMap::ShadowMap(const hal::Device& device, uint32_t size)
    : device_(device), size_(size) {
    
    auto result = create_image();
    if (result.is_err()) {
        LOG_ERROR("Failed to create shadow map image: {}", result.unwrap_err().message);
        return;
    }
    
    result = create_image_view();
    if (result.is_err()) {
        LOG_ERROR("Failed to create shadow map image view: {}", result.unwrap_err().message);
        return;
    }
    
    result = create_sampler();
    if (result.is_err()) {
        LOG_ERROR("Failed to create shadow map sampler: {}", result.unwrap_err().message);
        return;
    }
}

ShadowMap::~ShadowMap() {
    cleanup();
}

Result<void> ShadowMap::create_image() {
    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = size_;
    image_info.extent.height = size_;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = VK_FORMAT_D32_SFLOAT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | 
                      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    
    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    alloc_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    
    VkResult result = vmaCreateImage(device_.allocator(), &image_info, &alloc_info,
                                     &image_, &allocation_);
    if (result != VK_SUCCESS) {
        return Err(frame_graph::FrameGraphError{
            frame_graph::FrameGraphErrorType::ResourceCreationFailed,
            "Failed to create shadow map image"
        });
    }
    
    return Ok();
}

Result<void> ShadowMap::create_image_view() {
    VkImageViewCreateInfo view_info = {};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = image_;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = VK_FORMAT_D32_SFLOAT;
    view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;
    
    VkResult result = vkCreateImageView(device_.device(), &view_info, nullptr, &image_view_);
    if (result != VK_SUCCESS) {
        return Err(frame_graph::FrameGraphError{
            frame_graph::FrameGraphErrorType::ResourceCreationFailed,
            "Failed to create shadow map image view"
        });
    }
    
    return Ok();
}

Result<void> ShadowMap::create_sampler() {
    VkSamplerCreateInfo sampler_info = {};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler_info.anisotropyEnable = VK_FALSE;
    sampler_info.maxAnisotropy = 1.0f;
    sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_TRUE;
    sampler_info.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = VK_LOD_CLAMP_NONE;
    
    VkResult result = vkCreateSampler(device_.device(), &sampler_info, nullptr, &sampler_);
    if (result != VK_SUCCESS) {
        return Err(frame_graph::FrameGraphError{
            frame_graph::FrameGraphErrorType::ResourceCreationFailed,
            "Failed to create shadow map sampler"
        });
    }
    
    return Ok();
}

void ShadowMap::cleanup() {
    if (sampler_ != VK_NULL_HANDLE) {
        vkDestroySampler(device_.device(), sampler_, nullptr);
        sampler_ = VK_NULL_HANDLE;
    }
    
    if (image_view_ != VK_NULL_HANDLE) {
        vkDestroyImageView(device_.device(), image_view_, nullptr);
        image_view_ = VK_NULL_HANDLE;
    }
    
    if (image_ != VK_NULL_HANDLE) {
        vmaDestroyImage(device_.allocator(), image_, allocation_);
        image_ = VK_NULL_HANDLE;
        allocation_ = VK_NULL_HANDLE;
    }
}

frame_graph::ResourceHandle ShadowMap::create_resource(frame_graph::FrameGraph& frame_graph) const {
    frame_graph::ResourceDesc desc = {};
    desc.type = frame_graph::ResourceType::Image2D;
    desc.format = VK_FORMAT_D32_SFLOAT;
    desc.width = size_;
    desc.height = size_;
    desc.depth = 1;
    desc.mip_levels = 1;
    desc.array_layers = 1;
    desc.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    desc.lifetime = frame_graph::ResourceLifetime::Persistent;
    desc.name = "shadow_map";
    
    return frame_graph.create_resource(desc).unwrap();
}

// ShadowAtlas implementation
ShadowAtlas::ShadowAtlas(const hal::Device& device, uint32_t size, uint32_t tile_size)
    : device_(device), size_(size), tile_size_(tile_size) {
    
    tiles_per_side_ = size_ / tile_size_;
    
    // Initialize free tiles
    for (uint32_t i = 0; i < tiles_per_side_ * tiles_per_side_; ++i) {
        free_tiles_.push_back(i);
    }
    
    auto result = create_image();
    if (result.is_err()) {
        LOG_ERROR("Failed to create shadow atlas image: {}", result.unwrap_err().message);
        return;
    }
    
    result = create_image_view();
    if (result.is_err()) {
        LOG_ERROR("Failed to create shadow atlas image view: {}", result.unwrap_err().message);
        return;
    }
    
    result = create_sampler();
    if (result.is_err()) {
        LOG_ERROR("Failed to create shadow atlas sampler: {}", result.unwrap_err().message);
        return;
    }
}

ShadowAtlas::~ShadowAtlas() {
    cleanup();
}

std::optional<ShadowAtlas::TileAllocation> ShadowAtlas::allocate_tile() {
    if (free_tiles_.empty()) {
        return std::nullopt;
    }
    
    uint32_t tile_index = free_tiles_.back();
    free_tiles_.pop_back();
    
    uint32_t x, y;
    tile_index_to_coords(tile_index, x, y);
    
    TileAllocation allocation = {};
    allocation.x = x;
    allocation.y = y;
    allocation.size = tile_size_;
    allocation.uv_transform = glm::vec4(
        static_cast<float>(x) / static_cast<float>(size_),
        static_cast<float>(y) / static_cast<float>(size_),
        static_cast<float>(tile_size_) / static_cast<float>(size_),
        static_cast<float>(tile_size_) / static_cast<float>(size_)
    );
    
    allocated_tiles_.push_back(allocation);
    return allocation;
}

void ShadowAtlas::free_tile(const TileAllocation& allocation) {
    uint32_t tile_index = allocation.y * tiles_per_side_ + allocation.x;
    free_tiles_.push_back(tile_index);
    
    auto it = std::find(allocated_tiles_.begin(), allocated_tiles_.end(), allocation);
    if (it != allocated_tiles_.end()) {
        allocated_tiles_.erase(it);
    }
}

Result<void> ShadowAtlas::create_image() {
    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = size_;
    image_info.extent.height = size_;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = VK_FORMAT_D32_SFLOAT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | 
                      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    
    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    
    VkResult result = vmaCreateImage(device_.allocator(), &image_info, &alloc_info,
                                     &image_, &allocation_);
    if (result != VK_SUCCESS) {
        return Err(frame_graph::FrameGraphError{
            frame_graph::FrameGraphErrorType::ResourceCreationFailed,
            "Failed to create shadow atlas image"
        });
    }
    
    return Ok();
}

Result<void> ShadowAtlas::create_image_view() {
    VkImageViewCreateInfo view_info = {};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = image_;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = VK_FORMAT_D32_SFLOAT;
    view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;
    
    VkResult result = vkCreateImageView(device_.device(), &view_info, nullptr, &image_view_);
    if (result != VK_SUCCESS) {
        return Err(frame_graph::FrameGraphError{
            frame_graph::FrameGraphErrorType::ResourceCreationFailed,
            "Failed to create shadow atlas image view"
        });
    }
    
    return Ok();
}

Result<void> ShadowAtlas::create_sampler() {
    VkSamplerCreateInfo sampler_info = {};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler_info.anisotropyEnable = VK_FALSE;
    sampler_info.maxAnisotropy = 1.0f;
    sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_TRUE;
    sampler_info.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = VK_LOD_CLAMP_NONE;
    
    VkResult result = vkCreateSampler(device_.device(), &sampler_info, nullptr, &sampler_);
    if (result != VK_SUCCESS) {
        return Err(frame_graph::FrameGraphError{
            frame_graph::FrameGraphErrorType::ResourceCreationFailed,
            "Failed to create shadow atlas sampler"
        });
    }
    
    return Ok();
}

void ShadowAtlas::cleanup() {
    if (sampler_ != VK_NULL_HANDLE) {
        vkDestroySampler(device_.device(), sampler_, nullptr);
        sampler_ = VK_NULL_HANDLE;
    }
    
    if (image_view_ != VK_NULL_HANDLE) {
        vkDestroyImageView(device_.device(), image_view_, nullptr);
        image_view_ = VK_NULL_HANDLE;
    }
    
    if (image_ != VK_NULL_HANDLE) {
        vmaDestroyImage(device_.allocator(), image_, allocation_);
        image_ = VK_NULL_HANDLE;
        allocation_ = VK_NULL_HANDLE;
    }
}

uint32_t ShadowAtlas::tile_index_to_coords(uint32_t index, uint32_t& x, uint32_t& y) const {
    x = index % tiles_per_side_;
    y = index / tiles_per_side_;
    return index;
}

frame_graph::ResourceHandle ShadowAtlas::create_resource(frame_graph::FrameGraph& frame_graph) const {
    frame_graph::ResourceDesc desc = {};
    desc.type = frame_graph::ResourceType::Image2D;
    desc.format = VK_FORMAT_D32_SFLOAT;
    desc.width = size_;
    desc.height = size_;
    desc.depth = 1;
    desc.mip_levels = 1;
    desc.array_layers = 1;
    desc.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    desc.lifetime = frame_graph::ResourceLifetime::Persistent;
    desc.name = "shadow_atlas";
    
    return frame_graph.create_resource(desc).unwrap();
}

// CascadedShadowMapper implementation
CascadedShadowMapper::CascadedShadowMapper(const hal::Device& device)
    : device_(device), resources_(std::make_unique<ShadowMapResources>()) {
}

CascadedShadowMapper::~CascadedShadowMapper() = default;

Result<void> CascadedShadowMapper::initialize(const CascadeConfig& config) {
    config_ = config;
    cascades_.resize(config_.cascade_count);
    
    // Create shadow maps for each cascade
    for (uint32_t i = 0; i < config_.cascade_count; ++i) {
        auto shadow_map = std::make_unique<ShadowMap>(device_, config_.shadow_map_size);
        resources_->cascades.push_back(std::move(shadow_map));
    }
    
    // Create atlases for spot and point lights
    resources_->spot_light_atlas = std::make_unique<ShadowAtlas>(device_, 4096, 512);
    resources_->point_light_atlas = std::make_unique<ShadowAtlas>(device_, 4096, 256);
    
    return Ok();
}

void CascadedShadowMapper::update_cascades(const glm::mat4& view_matrix, 
                                           const glm::mat4& projection_matrix,
                                           const glm::vec3& light_direction) {
    calculate_cascade_matrices(view_matrix, projection_matrix, light_direction);
}

Result<frame_graph::PassHandle> CascadedShadowMapper::create_shadow_pass(
    frame_graph::FrameGraph& frame_graph,
    const glm::mat4& view_matrix,
    const glm::mat4& projection_matrix,
    const glm::vec3& light_direction) {
    
    // Update cascade matrices
    update_cascades(view_matrix, projection_matrix, light_direction);
    
    // Create shadow rendering pass
    frame_graph::PassDesc pass("CascadedShadowMapping");
    pass.queue = frame_graph::QueueType::Graphics;
    
    // For each cascade, we would create a subpass or separate pass
    // This is simplified - in practice you'd batch them efficiently
    
    pass.execute = [this](frame_graph::CommandBuffer& cmd) {
        // Render scene geometry from light perspective for each cascade
        // This would involve binding cascade-specific view-projection matrices
        // and rendering the scene
        
        for (uint32_t cascade = 0; cascade < config_.cascade_count; ++cascade) {
            // Set viewport for this cascade
            VkViewport viewport = {};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(config_.shadow_map_size);
            viewport.height = static_cast<float>(config_.shadow_map_size);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            
            cmd.set_viewport(0, 1, &viewport);
            
            // Set scissor
            VkRect2D scissor = {};
            scissor.offset = {0, 0};
            scissor.extent = {config_.shadow_map_size, config_.shadow_map_size};
            
            cmd.set_scissor(0, 1, &scissor);
            
            // Bind cascade-specific view-projection matrix
            // This would be done via push constants or uniform buffer
            // cmd.push_constants(...);
            
            // Render scene geometry
            // cmd.draw(...);
        }
    };
    
    return frame_graph.add_pass(std::move(pass));
}

CascadedShadowMapper::ShadowUniforms CascadedShadowMapper::get_uniforms() const {
    ShadowUniforms uniforms = {};
    
    for (uint32_t i = 0; i < config_.cascade_count; ++i) {
        uniforms.cascade_view_projections[i] = cascades_[i].view_projection_matrix;
    }
    
    uniforms.cascade_splits = glm::vec4(
        cascades_[0].split_distance,
        cascades_[1].split_distance,
        cascades_[2].split_distance,
        cascades_[3].split_distance
    );
    
    uniforms.shadow_params = glm::vec4(
        config_.bias_multiplier,
        1.0f / static_cast<float>(config_.shadow_map_size),
        0.0f,
        0.0f
    );
    
    if (!cascades_.empty()) {
        uniforms.light_direction = glm::vec4(cascades_[0].light_direction, 0.0f);
    }
    
    return uniforms;
}

void CascadedShadowMapper::calculate_cascade_matrices(const glm::mat4& view_matrix, 
                                                      const glm::mat4& projection_matrix,
                                                      const glm::vec3& light_direction) {
    for (uint32_t i = 0; i < config_.cascade_count; ++i) {
        float near_split = (i == 0) ? config_.near_plane : config_.cascade_splits[i - 1];
        float far_split = config_.cascade_splits[i];
        
        // Get frustum corners for this cascade
        auto corners = get_frustum_corners(view_matrix, projection_matrix, near_split, far_split);
        
        // Calculate bounding box in light space
        glm::mat4 light_view = calculate_cascade_view(light_direction, {}, 0.0f);
        auto light_space_bounds = calculate_light_space_bounding_box(corners, light_view);
        
        // Calculate cascade projection matrix
        glm::mat4 light_projection = glm::ortho(
            light_space_bounds.min_bound.x,
            light_space_bounds.max_bound.x,
            light_space_bounds.min_bound.y,
            light_space_bounds.max_bound.y,
            light_space_bounds.min_bound.z,
            light_space_bounds.max_bound.z
        );
        
        // Stabilize cascades to reduce shimmering
        stabilize_cascades(light_view, light_projection, i);
        
        cascades_[i].view_matrix = light_view;
        cascades_[i].projection_matrix = light_projection;
        cascades_[i].view_projection_matrix = light_projection * light_view;
        cascades_[i].light_direction = light_direction;
        cascades_[i].split_distance = far_split;
        cascades_[i].index = i;
        cascades_[i].texel_size = 1.0f / static_cast<float>(config_.shadow_map_size);
    }
}

glm::mat4 CascadedShadowMapper::calculate_cascade_projection(uint32_t cascade_index, 
                                                           const glm::mat4& view_matrix,
                                                           const glm::mat4& projection_matrix) {
    // This would calculate the orthographic projection for a specific cascade
    // Implementation depends on the specific cascade calculation approach
    return glm::mat4(1.0f);
}

glm::mat4 CascadedShadowMapper::calculate_cascade_view(const glm::vec3& light_direction,
                                                       const glm::vec3& cascade_center,
                                                       float cascade_radius) {
    glm::vec3 light_pos = cascade_center - light_direction * cascade_radius;
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    
    // Handle case where light direction is parallel to up vector
    if (std::abs(glm::dot(light_direction, up)) > 0.999f) {
        up = glm::vec3(1.0f, 0.0f, 0.0f);
    }
    
    return glm::lookAt(light_pos, cascade_center, up);
}

void CascadedShadowMapper::stabilize_cascades(glm::mat4& view_matrix, glm::mat4& projection_matrix,
                                             uint32_t cascade_index) {
    // Stabilize cascades by snapping to texel grid
    // This reduces shimmering when camera moves
    
    glm::mat4 view_projection = projection_matrix * view_matrix;
    glm::mat4 inverse_view_projection = glm::inverse(view_projection);
    
    // Get cascade corners in world space
    glm::vec4 corners[8] = {
        glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f),
        glm::vec4(1.0f, -1.0f, 0.0f, 1.0f),
        glm::vec4(1.0f, 1.0f, 0.0f, 1.0f),
        glm::vec4(-1.0f, 1.0f, 0.0f, 1.0f),
        glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f),
        glm::vec4(1.0f, -1.0f, 1.0f, 1.0f),
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        glm::vec4(-1.0f, 1.0f, 1.0f, 1.0f)
    };
    
    // Transform to world space
    for (int i = 0; i < 8; ++i) {
        corners[i] = inverse_view_projection * corners[i];
        corners[i] /= corners[i].w;
    }
    
    // Calculate bounds in light space and snap to texel grid
    // This is a simplified version - full implementation would be more complex
}

std::vector<glm::vec3> CascadedShadowMapper::get_frustum_corners(const glm::mat4& view_matrix,
                                                                  const glm::mat4& projection_matrix,
                                                                  float near_split, float far_split) {
    std::vector<glm::vec3> corners;
    
    // Calculate inverse matrices
    glm::mat4 inverse_view = glm::inverse(view_matrix);
    glm::mat4 inverse_projection = glm::inverse(projection_matrix);
    
    // Calculate split projection matrices
    glm::mat4 split_projection = glm::perspective(
        glm::radians(45.0f), // This should come from actual camera FOV
        16.0f / 9.0f,       // This should come from actual aspect ratio
        near_split,
        far_split
    );
    
    glm::mat4 inverse_split_projection = glm::inverse(split_projection);
    
    // Get frustum corners in clip space
    glm::vec4 clip_corners[8] = {
        glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f),
        glm::vec4(1.0f, -1.0f, 0.0f, 1.0f),
        glm::vec4(1.0f, 1.0f, 0.0f, 1.0f),
        glm::vec4(-1.0f, 1.0f, 0.0f, 1.0f),
        glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f),
        glm::vec4(1.0f, -1.0f, 1.0f, 1.0f),
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        glm::vec4(-1.0f, 1.0f, 1.0f, 1.0f)
    };
    
    // Transform to world space
    for (int i = 0; i < 8; ++i) {
        glm::vec4 view_corner = inverse_split_projection * clip_corners[i];
        view_corner /= view_corner.w;
        glm::vec4 world_corner = inverse_view * view_corner;
        corners.push_back(glm::vec3(world_corner));
    }
    
    return corners;
}

CascadedShadowMapper::BoundingBox CascadedShadowMapper::calculate_bounding_box(const std::vector<glm::vec3>& points) {
    BoundingBox bounds = {};
    
    if (points.empty()) {
        bounds.min_bound = glm::vec3(0.0f);
        bounds.max_bound = glm::vec3(0.0f);
        return bounds;
    }
    
    bounds.min_bound = points[0];
    bounds.max_bound = points[0];
    
    for (const auto& point : points) {
        bounds.min_bound = glm::min(bounds.min_bound, point);
        bounds.max_bound = glm::max(bounds.max_bound, point);
    }
    
    return bounds;
}

CascadedShadowMapper::BoundingBox CascadedShadowMapper::calculate_light_space_bounding_box(
    const std::vector<glm::vec3>& corners,
    const glm::mat4& light_view_matrix) {
    
    std::vector<glm::vec3> light_space_corners;
    
    // Transform corners to light space
    for (const auto& corner : corners) {
        glm::vec4 light_corner = light_view_matrix * glm::vec4(corner, 1.0f);
        light_space_corners.push_back(glm::vec3(light_corner));
    }
    
    return calculate_bounding_box(light_space_corners);
}

// PCFFilter implementation
bool PCFFilter::poisson_initialized_ = false;
glm::vec2 PCFFilter::poisson_disk_[MAX_SAMPLES];

void PCFFilter::apply_filter(FilterType type, float texel_size, 
                             glm::vec2& uv_offset, float& weight) {
    switch (type) {
        case FilterType::None:
            uv_offset = glm::vec2(0.0f);
            weight = 1.0f;
            break;
            
        case FilterType::PCF_2x2:
            {
                static const glm::vec2 offsets[4] = {
                    glm::vec2(-0.5f, -0.5f),
                    glm::vec2(0.5f, -0.5f),
                    glm::vec2(-0.5f, 0.5f),
                    glm::vec2(0.5f, 0.5f)
                };
                uint32_t index = static_cast<uint32_t>(weight * 4) % 4;
                uv_offset = offsets[index] * texel_size;
                weight = 0.25f;
            }
            break;
            
        case FilterType::PCF_3x3:
            {
                static const glm::vec2 offsets[9] = {
                    glm::vec2(-1.0f, -1.0f), glm::vec2(0.0f, -1.0f), glm::vec2(1.0f, -1.0f),
                    glm::vec2(-1.0f, 0.0f),  glm::vec2(0.0f, 0.0f),  glm::vec2(1.0f, 0.0f),
                    glm::vec2(-1.0f, 1.0f),  glm::vec2(0.0f, 1.0f),  glm::vec2(1.0f, 1.0f)
                };
                uint32_t index = static_cast<uint32_t>(weight * 9) % 9;
                uv_offset = offsets[index] * texel_size;
                weight = 1.0f / 9.0f;
            }
            break;
            
        case FilterType::PCF_5x5:
            {
                // Generate 5x5 grid offset
                int x = static_cast<int>(weight * 5) % 5 - 2;
                int y = static_cast<int>(weight * 25) % 5 - 2;
                uv_offset = glm::vec2(x, y) * texel_size;
                weight = 1.0f / 25.0f;
            }
            break;
            
        case FilterType::PCF_Random:
            {
                if (!poisson_initialized_) {
                    initialize_poisson_disk();
                }
                uint32_t index = static_cast<uint32_t>(weight * MAX_SAMPLES) % MAX_SAMPLES;
                uv_offset = poisson_disk_[index] * texel_size * 2.0f;
                weight = 1.0f / static_cast<float>(MAX_SAMPLES);
            }
            break;
    }
}

void PCFFilter::initialize_poisson_disk() {
    // Simple Poisson disk sampling for PCF
    for (uint32_t i = 0; i < MAX_SAMPLES; ++i) {
        float angle = 2.0f * 3.14159265f * static_cast<float>(i) / static_cast<float>(MAX_SAMPLES);
        float radius = std::sqrt(static_cast<float>(i) / static_cast<float>(MAX_SAMPLES));
        poisson_disk_[i] = glm::vec2(std::cos(angle), std::sin(angle)) * radius;
    }
    poisson_initialized_ = true;
}

} // namespace cold_coffee::engine::render::shadows
