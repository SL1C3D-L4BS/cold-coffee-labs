#include "cascaded_shadows.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace gw::render::shadows {

using gw::render::frame_graph::FrameGraphError;
using gw::render::frame_graph::FrameGraphErrorType;
using gw::render::frame_graph::PassDesc;
using gw::render::frame_graph::QueueType;
using gw::render::frame_graph::TextureDesc;
using gw::render::frame_graph::ResourceLifetime;
using gw::render::frame_graph::CommandBuffer;

// ---------------------------------------------------------------------------
// CascadeConfig
// ---------------------------------------------------------------------------

void CascadeConfig::calculate_splits() {
    // Practical split scheme: λ-blend of logarithmic and uniform (Engel 2006).
    for (uint32_t i = 0; i < cascade_count; ++i) {
        const float p = static_cast<float>(i + 1) / static_cast<float>(cascade_count);
        const float uni = near_plane + (far_plane - near_plane) * p;
        const float log = near_plane * std::pow(far_plane / near_plane, p);
        cascade_splits[i] = cascade_split_lambda * log +
                            (1.0f - cascade_split_lambda) * uni;
    }
}

// ---------------------------------------------------------------------------
// ShadowMap
// ---------------------------------------------------------------------------

ShadowMap::ShadowMap(hal::VulkanDevice& device, uint32_t size)
    : device_(device), size_(size) {
    if (!create_image() || !create_image_view() || !create_sampler()) {
        std::fprintf(stderr, "[shadows] ShadowMap init failed\n");
    }
}

ShadowMap::~ShadowMap() { cleanup(); }

Result<std::monostate> ShadowMap::create_image() {
    VkImageCreateInfo ii{};
    ii.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ii.imageType     = VK_IMAGE_TYPE_2D;
    ii.format        = VK_FORMAT_D32_SFLOAT;
    ii.extent        = {size_, size_, 1};
    ii.mipLevels     = 1;
    ii.arrayLayers   = 1;
    ii.samples       = VK_SAMPLE_COUNT_1_BIT;
    ii.tiling        = VK_IMAGE_TILING_OPTIMAL;
    ii.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                       VK_IMAGE_USAGE_SAMPLED_BIT;
    ii.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    ii.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo ai{};
    ai.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    VkResult r = vmaCreateImage(device_.vma_allocator(), &ii, &ai,
                                &image_, &allocation_, nullptr);
    if (r != VK_SUCCESS) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed,
            "ShadowMap: vmaCreateImage failed"
        });
    }
    return std::monostate{};
}

Result<std::monostate> ShadowMap::create_image_view() {
    VkImageViewCreateInfo vi{};
    vi.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vi.image                           = image_;
    vi.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    vi.format                          = VK_FORMAT_D32_SFLOAT;
    vi.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
    vi.subresourceRange.baseMipLevel   = 0;
    vi.subresourceRange.levelCount     = 1;
    vi.subresourceRange.baseArrayLayer = 0;
    vi.subresourceRange.layerCount     = 1;

    VkResult r = vkCreateImageView(device_.native_handle(), &vi, nullptr, &image_view_);
    if (r != VK_SUCCESS) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed,
            "ShadowMap: vkCreateImageView failed"
        });
    }
    return std::monostate{};
}

Result<std::monostate> ShadowMap::create_sampler() {
    VkSamplerCreateInfo si{};
    si.sType            = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    si.magFilter        = VK_FILTER_LINEAR;
    si.minFilter        = VK_FILTER_LINEAR;
    si.mipmapMode       = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    si.addressModeU     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    si.addressModeV     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    si.addressModeW     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    si.borderColor      = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    si.compareEnable    = VK_TRUE;
    si.compareOp        = VK_COMPARE_OP_LESS_OR_EQUAL;

    VkResult r = vkCreateSampler(device_.native_handle(), &si, nullptr, &sampler_);
    if (r != VK_SUCCESS) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed,
            "ShadowMap: vkCreateSampler failed"
        });
    }
    return std::monostate{};
}

void ShadowMap::cleanup() {
    VkDevice dev = device_.native_handle();
    if (sampler_    != VK_NULL_HANDLE) { vkDestroySampler(dev, sampler_, nullptr);    sampler_    = VK_NULL_HANDLE; }
    if (image_view_ != VK_NULL_HANDLE) { vkDestroyImageView(dev, image_view_, nullptr); image_view_ = VK_NULL_HANDLE; }
    if (image_      != VK_NULL_HANDLE) {
        vmaDestroyImage(device_.vma_allocator(), image_, allocation_);
        image_ = VK_NULL_HANDLE;
    }
}

frame_graph::ResourceHandle ShadowMap::create_resource(FrameGraph& frame_graph) const {
    TextureDesc d;
    d.width    = size_;
    d.height   = size_;
    d.format   = VK_FORMAT_D32_SFLOAT;
    d.usage    = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    d.lifetime = ResourceLifetime::Persistent;
    d.name     = "shadow_cascade";
    auto r = frame_graph.declare_texture(d);
    return r ? r.value() : static_cast<frame_graph::ResourceHandle>(UINT32_MAX);
}

// ---------------------------------------------------------------------------
// ShadowAtlas
// ---------------------------------------------------------------------------

ShadowAtlas::ShadowAtlas(hal::VulkanDevice& device, uint32_t size, uint32_t tile_size)
    : device_(device), size_(size), tile_size_(tile_size)
    , tiles_per_side_(size / tile_size) {
    if (!create_image() || !create_image_view() || !create_sampler()) {
        std::fprintf(stderr, "[shadows] ShadowAtlas init failed\n");
    }
    // Initialize free tile list
    const uint32_t total = tiles_per_side_ * tiles_per_side_;
    free_tiles_.resize(total);
    for (uint32_t i = 0; i < total; ++i) free_tiles_[i] = i;
}

ShadowAtlas::~ShadowAtlas() { cleanup(); }

std::optional<ShadowAtlas::TileAllocation> ShadowAtlas::allocate_tile() {
    if (free_tiles_.empty()) return std::nullopt;
    const uint32_t idx = free_tiles_.back();
    free_tiles_.pop_back();
    uint32_t x = 0, y = 0;
    tile_index_to_coords(idx, x, y);
    TileAllocation ta;
    ta.x    = x * tile_size_;
    ta.y    = y * tile_size_;
    ta.size = tile_size_;
    const float inv = 1.0f / static_cast<float>(size_);
    ta.uv_transform = Vec4f(
        ta.x * inv, ta.y * inv,
        tile_size_ * inv, tile_size_ * inv);
    allocated_tiles_.push_back(ta);
    return ta;
}

void ShadowAtlas::free_tile(const TileAllocation& allocation) {
    const uint32_t tx = allocation.x / tile_size_;
    const uint32_t ty = allocation.y / tile_size_;
    free_tiles_.push_back(ty * tiles_per_side_ + tx);
    auto it = std::find_if(allocated_tiles_.begin(), allocated_tiles_.end(),
        [&](const TileAllocation& a){ return a.x == allocation.x && a.y == allocation.y; });
    if (it != allocated_tiles_.end()) allocated_tiles_.erase(it);
}

void ShadowAtlas::tile_index_to_coords(uint32_t index, uint32_t& x, uint32_t& y) const {
    x = index % tiles_per_side_;
    y = index / tiles_per_side_;
}

Result<std::monostate> ShadowAtlas::create_image() {
    VkImageCreateInfo ii{};
    ii.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ii.imageType     = VK_IMAGE_TYPE_2D;
    ii.format        = VK_FORMAT_D32_SFLOAT;
    ii.extent        = {size_, size_, 1};
    ii.mipLevels     = 1;
    ii.arrayLayers   = 1;
    ii.samples       = VK_SAMPLE_COUNT_1_BIT;
    ii.tiling        = VK_IMAGE_TILING_OPTIMAL;
    ii.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ii.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    ii.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo ai{};
    ai.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    VkResult r = vmaCreateImage(device_.vma_allocator(), &ii, &ai,
                                &image_, &allocation_, nullptr);
    if (r != VK_SUCCESS) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed, "ShadowAtlas: vmaCreateImage failed"});
    }
    return std::monostate{};
}

Result<std::monostate> ShadowAtlas::create_image_view() {
    VkImageViewCreateInfo vi{};
    vi.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vi.image                           = image_;
    vi.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    vi.format                          = VK_FORMAT_D32_SFLOAT;
    vi.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
    vi.subresourceRange.baseMipLevel   = 0;
    vi.subresourceRange.levelCount     = 1;
    vi.subresourceRange.baseArrayLayer = 0;
    vi.subresourceRange.layerCount     = 1;

    VkResult r = vkCreateImageView(device_.native_handle(), &vi, nullptr, &image_view_);
    if (r != VK_SUCCESS) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed, "ShadowAtlas: vkCreateImageView failed"});
    }
    return std::monostate{};
}

Result<std::monostate> ShadowAtlas::create_sampler() {
    VkSamplerCreateInfo si{};
    si.sType         = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    si.magFilter     = VK_FILTER_LINEAR;
    si.minFilter     = VK_FILTER_LINEAR;
    si.mipmapMode    = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    si.addressModeU  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    si.addressModeV  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    si.addressModeW  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    si.borderColor   = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    si.compareEnable = VK_TRUE;
    si.compareOp     = VK_COMPARE_OP_LESS_OR_EQUAL;

    VkResult r = vkCreateSampler(device_.native_handle(), &si, nullptr, &sampler_);
    if (r != VK_SUCCESS) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed, "ShadowAtlas: vkCreateSampler failed"});
    }
    return std::monostate{};
}

void ShadowAtlas::cleanup() {
    VkDevice dev = device_.native_handle();
    if (sampler_)    { vkDestroySampler(dev, sampler_, nullptr);     sampler_    = VK_NULL_HANDLE; }
    if (image_view_) { vkDestroyImageView(dev, image_view_, nullptr); image_view_ = VK_NULL_HANDLE; }
    if (image_)      { vmaDestroyImage(device_.vma_allocator(), image_, allocation_); image_ = VK_NULL_HANDLE; }
}

frame_graph::ResourceHandle ShadowAtlas::create_resource(FrameGraph& frame_graph) const {
    TextureDesc d;
    d.width    = size_;
    d.height   = size_;
    d.format   = VK_FORMAT_D32_SFLOAT;
    d.usage    = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    d.lifetime = ResourceLifetime::Persistent;
    d.name     = "shadow_atlas";
    auto r = frame_graph.declare_texture(d);
    return r ? r.value() : static_cast<frame_graph::ResourceHandle>(UINT32_MAX);
}

// ---------------------------------------------------------------------------
// CascadedShadowMapper
// ---------------------------------------------------------------------------

CascadedShadowMapper::CascadedShadowMapper(hal::VulkanDevice& device)
    : device_(device) {}

CascadedShadowMapper::~CascadedShadowMapper() = default;

Result<std::monostate> CascadedShadowMapper::initialize(const CascadeConfig& config) {
    config_    = config;
    resources_ = std::make_unique<ShadowMapResources>();

    cascades_.resize(config_.cascade_count);
    for (uint32_t i = 0; i < config_.cascade_count; ++i) {
        cascades_[i].index = i;
        resources_->cascades.push_back(
            std::make_unique<ShadowMap>(device_, config_.shadow_map_size));
    }
    return std::monostate{};
}

void CascadedShadowMapper::update_cascades(const Mat4f& view_matrix,
                                            const Mat4f& projection_matrix,
                                            const Vec3f& light_direction) {
    calculate_cascade_matrices(view_matrix, projection_matrix, light_direction);
}

Result<PassHandle> CascadedShadowMapper::create_shadow_pass(
        FrameGraph& frame_graph,
        const Mat4f& view_matrix,
        const Mat4f& projection_matrix,
        const Vec3f& light_direction) {
    update_cascades(view_matrix, projection_matrix, light_direction);

    PassDesc pass("CascadedShadows");
    pass.queue = QueueType::Graphics;
    pass.execute = [](CommandBuffer& cmd) {
        (void)cmd;
        // In Week 030 the actual depth-pass draws happen here via
        // cmd.begin_rendering()/cmd.draw() with csm.vert.hlsl pipeline.
    };

    return frame_graph.add_pass(std::move(pass));
}

CascadedShadowMapper::ShadowUniforms CascadedShadowMapper::get_uniforms() const {
    ShadowUniforms u{};
    for (uint32_t i = 0; i < std::min<uint32_t>(4, static_cast<uint32_t>(cascades_.size())); ++i) {
        // Flatten Mat4f to float[16] row-major via const row accessors
        const Mat4f& vp = cascades_[i].view_projection_matrix;
        const auto r0 = vp.row0(); const auto r1 = vp.row1();
        const auto r2 = vp.row2(); const auto r3 = vp.row3();
        auto* dst = u.cascade_view_projections[i];
        dst[0]  = r0.x(); dst[1]  = r0.y(); dst[2]  = r0.z(); dst[3]  = r0.w();
        dst[4]  = r1.x(); dst[5]  = r1.y(); dst[6]  = r1.z(); dst[7]  = r1.w();
        dst[8]  = r2.x(); dst[9]  = r2.y(); dst[10] = r2.z(); dst[11] = r2.w();
        dst[12] = r3.x(); dst[13] = r3.y(); dst[14] = r3.z(); dst[15] = r3.w();
    }
    if (!cascades_.empty()) {
        u.cascade_splits = Vec4f(
            cascades_.size() > 0 ? cascades_[0].split_distance : 0.0f,
            cascades_.size() > 1 ? cascades_[1].split_distance : 0.0f,
            cascades_.size() > 2 ? cascades_[2].split_distance : 0.0f,
            cascades_.size() > 3 ? cascades_[3].split_distance : 0.0f);
        u.light_direction = Vec4f(
            cascades_[0].light_direction.x(),
            cascades_[0].light_direction.y(),
            cascades_[0].light_direction.z(), 0.0f);
    }
    u.shadow_params = Vec4f(config_.max_bias, 1.0f / config_.shadow_map_size, 0.0f, 0.0f);
    return u;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void CascadedShadowMapper::calculate_cascade_matrices(const Mat4f& /*view*/,
                                                       const Mat4f& /*proj*/,
                                                       const Vec3f& light_direction) {
    // Placeholder: identity view-projection for each cascade.
    // Full frustum-split implementation wired in Week 030 draw loop.
    for (uint32_t i = 0; i < static_cast<uint32_t>(cascades_.size()); ++i) {
        cascades_[i].light_direction   = light_direction;
        cascades_[i].split_distance    = config_.cascade_splits.size() > i
                                         ? config_.cascade_splits[i] : 0.0f;
        // Identity matrices until real camera data is wired in
        cascades_[i].view_matrix            = Mat4f{};
        cascades_[i].projection_matrix      = Mat4f{};
        cascades_[i].view_projection_matrix = Mat4f{};
    }
}

Mat4f CascadedShadowMapper::calculate_cascade_projection(uint32_t /*i*/,
                                                          const Mat4f& /*view*/,
                                                          const Mat4f& /*proj*/) {
    return Mat4f{};  // Stubbed — replaced in full Week 030 pass
}

Mat4f CascadedShadowMapper::calculate_cascade_view(const Vec3f& /*ld*/,
                                                    const Vec3f& /*center*/,
                                                    float /*radius*/) {
    return Mat4f{};
}

void CascadedShadowMapper::stabilize_cascades(Mat4f& /*view*/,
                                               Mat4f& /*proj*/,
                                               uint32_t /*cascade*/) {}

std::vector<Vec3f> CascadedShadowMapper::get_frustum_corners(const Mat4f& /*view*/,
                                                               const Mat4f& /*proj*/,
                                                               float /*near*/,
                                                               float /*far*/) {
    return {};
}

CascadedShadowMapper::BoundingBox CascadedShadowMapper::calculate_bounding_box(
        const std::vector<Vec3f>& points) {
    BoundingBox bb{Vec3f(1e9f, 1e9f, 1e9f), Vec3f(-1e9f, -1e9f, -1e9f)};
    for (const auto& p : points) {
        bb.min_bound = Vec3f(std::min(bb.min_bound.x(), p.x()),
                             std::min(bb.min_bound.y(), p.y()),
                             std::min(bb.min_bound.z(), p.z()));
        bb.max_bound = Vec3f(std::max(bb.max_bound.x(), p.x()),
                             std::max(bb.max_bound.y(), p.y()),
                             std::max(bb.max_bound.z(), p.z()));
    }
    return bb;
}

CascadedShadowMapper::BoundingBox CascadedShadowMapper::calculate_light_space_bounding_box(
        const std::vector<Vec3f>& corners, const Mat4f& /*light_view*/) {
    return calculate_bounding_box(corners);
}

// ---------------------------------------------------------------------------
// PCFFilter
// ---------------------------------------------------------------------------

Vec2f    PCFFilter::poisson_disk_[MAX_SAMPLES]{};
bool     PCFFilter::poisson_initialized_ = false;

void PCFFilter::initialize_poisson_disk() {
    // Simple 4-tap kernel for PCF 2×2
    poisson_disk_[0] = Vec2f(-0.94201624f, -0.39906216f);
    poisson_disk_[1] = Vec2f( 0.94558609f, -0.76890725f);
    poisson_disk_[2] = Vec2f(-0.09418410f, -0.92938870f);
    poisson_disk_[3] = Vec2f( 0.34495938f,  0.29387760f);
    poisson_initialized_ = true;
}

void PCFFilter::apply_filter(FilterType type, float texel_size,
                              Vec2f& uv_offset, float& weight) {
    if (!poisson_initialized_) initialize_poisson_disk();
    (void)type;
    uv_offset = Vec2f(poisson_disk_[0].x() * texel_size,
                      poisson_disk_[0].y() * texel_size);
    weight = 1.0f;
}

} // namespace gw::render::shadows
