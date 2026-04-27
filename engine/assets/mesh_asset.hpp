#pragma once
// engine/assets/mesh_asset.hpp
// Runtime mesh asset — GPU-resident, loaded from a .gwmesh binary.
// Phase 6 spec §6 (Week 034).

#include "asset_handle.hpp"
#include "asset_error.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

// Forward-declare VMA / Vulkan buffer handle (avoid incomplete `enum VkIndexType`
// forward-declare: MSVC/clang-cl -Wmicrosoft-enum-forward-reference).
struct VmaAllocation_T;
using VmaAllocation = VmaAllocation_T*;
struct VkBuffer_T;
using VkBuffer = VkBuffer_T*;

// Forward-declare VulkanDevice so this header stays standalone.
namespace gw::render::hal { class VulkanDevice; }

namespace gw::assets {

// Matches VkIndexType for UINT16 / UINT32 (Vulkan 1.0). Cast at Vulkan boundaries.
enum class MeshIndexElementType : std::uint32_t {
    Uint16 = 0,
    Uint32 = 1,
};

// ---------------------------------------------------------------------------
// .gwmesh binary header (little-endian).
// v1 on-disk: first 92 bytes (version field == 1). stream2/joint_count absent.
// v2 on-disk: 108 bytes; stream2 holds inverse bind matrices + packed joint
// weights when kMeshFlagSkinned is set (see mesh_cooker / runtime upload).
// ---------------------------------------------------------------------------
inline constexpr std::size_t kGwMeshHeaderV1Bytes = 92;
inline constexpr std::size_t kGwMeshHeaderV2Bytes = 108;

#pragma pack(push, 1)
struct GwMeshHeader {
    uint32_t magic;           // 0x4853574D ('MWSH')
    uint16_t version;         // 1 = legacy rigid; 2 = stream2 tail valid
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
    // v2 extension (must be zero when version < 2 or when not skinned)
    uint32_t stream2_offset;
    uint32_t stream2_size;
    uint32_t joint_count;
    uint32_t reserved0;
};
static_assert(sizeof(GwMeshHeader) == kGwMeshHeaderV2Bytes, "GwMeshHeader size mismatch");
#pragma pack(pop)

// Flags
static constexpr uint16_t kMeshFlagHasUV1   = 1u << 0;
static constexpr uint16_t kMeshFlagHasColor = 1u << 1;
static constexpr uint16_t kMeshFlagSkinned  = 1u << 2;

/// Column-major 4x4 float inverse bind block in stream2 / glTF (64 bytes per joint).
inline constexpr std::size_t kInverseBindMat4Bytes = 64u;

// stream2 (skinned, version 2): `joint_count` inverse binds
// (column-major float 4x4, 64 bytes each), 16-byte padded; then per vertex
// { uint16_t joint[4]; uint8_t weight[4] } with weights summing to 255.

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
    [[nodiscard]] VkBuffer    stream2()     const noexcept { return stream2_buf_; }
    [[nodiscard]] VkBuffer    index()       const noexcept { return index_buf_;   }
    [[nodiscard]] MeshIndexElementType index_type() const noexcept { return index_type_; }

    [[nodiscard]] std::span<const Submesh>  submeshes()  const noexcept { return submeshes_; }
    [[nodiscard]] std::span<const LODRange> lod_ranges() const noexcept { return lod_ranges_; }
    [[nodiscard]] const AABB&               bounds()     const noexcept { return aabb_; }
    [[nodiscard]] const GwMeshHeader&       header()     const noexcept { return header_; }

    [[nodiscard]] bool ready() const noexcept { return stream0_buf_ != nullptr; }

    [[nodiscard]] bool skinned() const noexcept {
        return (header_.flags & kMeshFlagSkinned) != 0 && header_.joint_count > 0u;
    }
    [[nodiscard]] std::uint32_t joint_count() const noexcept { return header_.joint_count; }

    /// Byte offset from the start of the stream2 VkBuffer to packed per-vertex
    /// `{u16 j[4], u8 w[4]}` skin data (after inverse binds + 16-byte pad). Zero
    /// when not skinned.
    [[nodiscard]] std::uint32_t stream2_skin_packed_vertex_byte_offset() const noexcept;

    /// Inverse bind matrices as a tight blob: `joint_count * kInverseBindMat4Bytes`,
    /// column-major float32 per joint (matches stream2 IBM block). Empty when not
    /// skinned or before successful upload.
    [[nodiscard]] std::span<const std::byte> inverse_bind_matrices_bytes() const noexcept {
        return {inverse_bind_bytes_.data(), inverse_bind_bytes_.size()};
    }

private:
    void destroy_gpu() noexcept;

    GwMeshHeader           header_{};
    AABB                   aabb_{};
    std::vector<Submesh>   submeshes_;
    std::vector<LODRange>  lod_ranges_;

    // GPU objects (VMA-owned)
    VkBuffer       stream0_buf_  = nullptr;
    VkBuffer       stream1_buf_  = nullptr;
    VkBuffer       stream2_buf_  = nullptr;
    VkBuffer       index_buf_    = nullptr;
    VmaAllocation  stream0_alloc_= nullptr;
    VmaAllocation  stream1_alloc_= nullptr;
    VmaAllocation  stream2_alloc_= nullptr;
    VmaAllocation  index_alloc_  = nullptr;
    MeshIndexElementType index_type_{};

    // Optional CPU-side copy retained for hot-reload.
    std::vector<uint8_t> raw_bytes_;

    // Parsed from stream2 at upload (retained for skinning after CPU copy release).
    std::vector<std::byte> inverse_bind_bytes_{};

    // Back-pointer for GPU destroy (set during upload).
    render::hal::VulkanDevice* device_ = nullptr;
};

using MeshHandle = TypedHandle<MeshAsset>;

} // namespace gw::assets
