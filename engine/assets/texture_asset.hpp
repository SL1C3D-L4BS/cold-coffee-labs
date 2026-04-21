#pragma once
// engine/assets/texture_asset.hpp
// Runtime texture asset — GPU-resident, loaded from a .gwtex binary.
// Phase 6 spec §7 (Week 035).

#include "asset_handle.hpp"
#include "asset_error.hpp"
#include <cstdint>
#include <span>
#include <vector>

struct VmaAllocation_T;
using VmaAllocation = VmaAllocation_T*;
struct VkImage_T;
using VkImage = VkImage_T*;
struct VkImageView_T;
using VkImageView = VkImageView_T*;
struct VkSampler_T;
using VkSampler = VkSampler_T*;

namespace gw::render::hal { class VulkanDevice; }

namespace gw::assets {

// ---------------------------------------------------------------------------
// .gwtex binary header (48 bytes, little-endian).
// ---------------------------------------------------------------------------
#pragma pack(push, 1)
struct GwTexHeader {
    uint32_t magic;         // 0x58455447 ('GTEX')
    uint16_t version;       // = 1
    uint8_t  tex_type;      // 0=2D, 1=2DArray, 2=Cube, 3=3D
    uint8_t  channels;      // 1-4
    uint16_t width;
    uint16_t height;
    uint16_t depth_layers;  // depth for 3D; layer count for Array; 6 for Cube
    uint8_t  mip_count;
    uint8_t  pad0;
    uint32_t vk_format;     // VkFormat of the target platform GPU format
    uint8_t  color_space;   // 0=linear, 1=sRGB
    uint8_t  has_alpha;
    uint16_t pad1;
    uint32_t ktx2_size;     // byte count of KTX2 payload
    uint32_t ktx2_offset;   // byte offset of KTX2 payload (= 48 right after header)
};
static_assert(sizeof(GwTexHeader) == 32, "GwTexHeader size mismatch");
#pragma pack(pop)

// Texture type enum matching the header tex_type byte.
enum class TexType : uint8_t { Tex2D = 0, Tex2DArray = 1, TexCube = 2, Tex3D = 3 };

// ---------------------------------------------------------------------------
// TextureAsset
// ---------------------------------------------------------------------------
class TextureAsset {
public:
    static constexpr uint16_t kAssetTypeTag =
        static_cast<uint16_t>(AssetType::Texture);

    TextureAsset()  = default;
    ~TextureAsset() { destroy_gpu(); }

    TextureAsset(const TextureAsset&)            = delete;
    TextureAsset& operator=(const TextureAsset&) = delete;
    TextureAsset(TextureAsset&&)                 = default;
    TextureAsset& operator=(TextureAsset&&)      = default;

    // Decode .gwtex bytes and upload to GPU via staging buffer.
    [[nodiscard]] AssetOk
    upload(render::hal::VulkanDevice& device, std::span<const uint8_t> raw);

    void release_cpu_copy() noexcept { raw_bytes_.clear(); }

    // --- Render-thread accessors -------------------------------------------
    [[nodiscard]] VkImage     image()           const noexcept { return image_;    }
    [[nodiscard]] VkImageView image_view()      const noexcept { return view_;     }
    [[nodiscard]] VkSampler   default_sampler() const noexcept { return sampler_;  }
    [[nodiscard]] const GwTexHeader& header()   const noexcept { return header_;   }
    [[nodiscard]] bool ready()                  const noexcept { return image_ != nullptr; }

private:
    void destroy_gpu() noexcept;

    GwTexHeader             header_{};
    VkImage                 image_   = nullptr;
    VkImageView             view_    = nullptr;
    VkSampler               sampler_ = nullptr;
    VmaAllocation           alloc_   = nullptr;
    std::vector<uint8_t>    raw_bytes_;
    render::hal::VulkanDevice* device_ = nullptr;
};

using TextureHandle = TypedHandle<TextureAsset>;

} // namespace gw::assets
