#pragma once
// engine/assets/mesh_asset.hpp
// Runtime mesh asset — GPU-resident, loaded from a .gwmesh binary.
// Phase 6 spec §6 (Week 034).

#include "asset_handle.hpp"
#include "asset_error.hpp"
#include <array>
#include <cstdint>
#include <span>
#include <vector>

// Forward-declare VMA / Vulkan types to avoid pulling heavy headers here.
struct VmaAllocation_T;
using VmaAllocation = VmaAllocation_T*;
struct VkBuffer_T;
using VkBuffer = VkBuffer_T*;
enum VkIndexType;

// Forward-declare VulkanDevice so this header stays standalone.
namespace gw::render::hal { class VulkanDevice; }

namespace gw::assets {

// ---------------------------------------------------------------------------
// .gwmesh binary header (64 bytes, little-endian).
// ---------------------------------------------------------------------------
#pragma pack(push, 1)
struct GwMeshHeader {
    uint32_t magic;           // 0x4853574D ('MWSH')
    uint16_t version;         // = 1
    uint16_t flags;           // bit0=has_uv1, bit1=has_color0, bit2=skinned
    uint8_t  lod_count;
    uint8_t  submesh_count;
    uint16_t pad0;
    float    aabb_min[3];
    float    aabb_max[3];
    float    pos_scale[3];    // dequant: p = q/32767.0f * scale + bias
    float    pos_bias[3];
    uint32_t stream0_offset;  // byte offset into file
    uint32_t stream0_size;
    uint32_t stream1_offset;
    uint32_t stream1_size;
    uint32_t index_offset;
    uint32_t index_size;
    uint32_t lod_table_off;
    uint32_t submesh_table_off;
};
static_assert(sizeof(GwMeshHeader) == 92, "GwMeshHeader size mismatch");
#pragma pack(pop)

// Flags
static constexpr uint16_t kMeshFlagHasUV1   = 1u << 0;
static constexpr uint16_t kMeshFlagHasColor = 1u << 1;
static constexpr uint16_t kMeshFlagSkinned  = 1u << 2;

// ---------------------------------------------------------------------------
// Runtime per-LOD and per-submesh descriptors.
// ---------------------------------------------------------------------------
struct LODRange {
    uint32_t index_offset;  // byte offset into GPU index buffer
    uint32_t index_count;
};

struct AABB {
    float min[3]{};
    float max[3]{};
};

struct Submesh {
    uint32_t name_hash    = 0;
    uint16_t material_idx = 0;
    uint16_t lod_first    = 0;
    std::array<LODRange, 4> lods{};
    uint32_t vertex_start = 0;
    uint32_t vertex_count = 0;
};

// ---------------------------------------------------------------------------
// MeshAsset
// ---------------------------------------------------------------------------
class MeshAsset {
public:
    static constexpr uint16_t kAssetTypeTag =
        static_cast<uint16_t>(AssetType::Mesh);

    MeshAsset()  = default;
    ~MeshAsset() { destroy_gpu(); }

    MeshAsset(const MeshAsset&)            = delete;
    MeshAsset& operator=(const MeshAsset&) = delete;
    MeshAsset(MeshAsset&&)                 = default;
    MeshAsset& operator=(MeshAsset&&)      = default;

    // Decode the raw .gwmesh bytes and upload to GPU via the transfer queue.
    // Called from an async loader thread; safe to call concurrently for
    // different assets.
    [[nodiscard]] AssetOk
    upload(render::hal::VulkanDevice& device, std::span<const uint8_t> raw);

    // Release the CPU-side copy (raw bytes) after a successful upload if
    // hot-reload is disabled.
    void release_cpu_copy() noexcept { raw_bytes_.clear(); }

    // --- Render-thread accessors -------------------------------------------

    [[nodiscard]] VkBuffer    stream0()     const noexcept { return stream0_buf_; }
    [[nodiscard]] VkBuffer    stream1()     const noexcept { return stream1_buf_; }
    [[nodiscard]] VkBuffer    index()       const noexcept { return index_buf_;   }
    [[nodiscard]] VkIndexType index_type()  const noexcept { return index_type_;  }

    [[nodiscard]] std::span<const Submesh>  submeshes()  const noexcept { return submeshes_; }
    [[nodiscard]] std::span<const LODRange> lod_ranges() const noexcept { return lod_ranges_; }
    [[nodiscard]] const AABB&               bounds()     const noexcept { return aabb_; }
    [[nodiscard]] const GwMeshHeader&       header()     const noexcept { return header_; }

    [[nodiscard]] bool ready() const noexcept { return stream0_buf_ != nullptr; }

private:
    void destroy_gpu() noexcept;

    GwMeshHeader           header_{};
    AABB                   aabb_{};
    std::vector<Submesh>   submeshes_;
    std::vector<LODRange>  lod_ranges_;

    // GPU objects (VMA-owned)
    VkBuffer       stream0_buf_  = nullptr;
    VkBuffer       stream1_buf_  = nullptr;
    VkBuffer       index_buf_    = nullptr;
    VmaAllocation  stream0_alloc_= nullptr;
    VmaAllocation  stream1_alloc_= nullptr;
    VmaAllocation  index_alloc_  = nullptr;
    VkIndexType    index_type_{};

    // Optional CPU-side copy retained for hot-reload.
    std::vector<uint8_t> raw_bytes_;

    // Back-pointer for GPU destroy (set during upload).
    render::hal::VulkanDevice* device_ = nullptr;
};

using MeshHandle = TypedHandle<MeshAsset>;

} // namespace gw::assets
