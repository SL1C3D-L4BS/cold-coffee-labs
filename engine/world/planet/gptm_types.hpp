#pragma once
// engine/world/planet/gptm_types.hpp — Phase 20-A GPTM core types (Greywater Planetary Topology Model).

#include "engine/core/hash_map.hpp"
#include "engine/math/vec.hpp"
#include "engine/world/streaming/chunk_coord.hpp"
#include "engine/world/streaming/chunk_data.hpp"

#include <array>
#include <cmath>
#include <cstdint>

namespace gw::world::planet {

using gw::math::Vec3f;
using gw::world::streaming::BiomeId;
using gw::world::streaming::ChunkCoord;
using gw::world::streaming::ChunkCoordHash;

/// Level-of-detail band for GPTM terrain tiles (arena-scale Nine Circles geometry).
enum class GptmLod : std::uint8_t {
    Lod0 = 0, ///< Full resolution — camera within ~32 m of chunk influence.
    Lod1 = 1,
    Lod2 = 2,
    Lod3 = 3,
    Lod4 = 4, ///< Lowest — camera beyond ~1024 m unless screen error forces finer.
};

/// GPU lifetime handles for a single Vulkan buffer + VMA allocation (opaque to keep headers lean).
struct BufferHandle {
    std::uintptr_t buffer{};      ///< `VkBuffer` bitcast.
    std::uintptr_t allocation{}; ///< `VmaAllocation` bitcast.
};

/// Axis-aligned bounds in absolute world metres (f64 simulation space).
struct Aabb3_f64 {
    gw::world::streaming::Vec3_f64 min{};
    gw::world::streaming::Vec3_f64 max{};
};

/// One terrain draw submission for a chunk at a specific LOD.
struct GptmTile {
    ChunkCoord    coord{};
    GptmLod       lod{GptmLod::Lod0};
    BufferHandle  vertex_buffer{};
    BufferHandle  index_buffer{};
    std::uint32_t index_count{0};
    Aabb3_f64     world_bounds{};
    /// Representative biome for instanced shading (first corner of the build mesh).
    std::uint8_t dominant_biome{0};
    std::uint8_t _reserved0{0};
    std::uint16_t _reserved1{0};
};

/// Vertex layout for terrain — 32 bytes, two cache lines touch at most one vertex.
/// Normals are stored as 16-bit octahedral encodings (`n_oct_x`, `n_oct_y`); decode with `gptm_decode_oct_normal`.
/// Positions use raw `float[3]` (not `Vec3f`) so every toolchain agrees on a 12-byte block; `_stride_tail` bytes pad
/// the struct to exactly 32 bytes for GPU vertex strides (scalar bytes avoid MSVC/clangd tail-alignment glitches).
/// Layout is enforced in `gptm_layout_check.cpp` (compile-time) so IDE/clangd parsers that mis-size MSVC-ish
/// layouts do not spam diagnostics on every include of this header.
struct GptmVertex {
    float         local_pos[3]{};
    std::uint16_t n_oct_x{0};
    std::uint16_t n_oct_y{0};
    float         uv[2]{};
    std::uint8_t biome_id{0};
    std::uint8_t _pad[3]{};
    /// Reserved; keep zero. Same 4-byte span as HLSL `VsIn._struct_tail_pad` (offset 28, stride 32).
    std::uint8_t _stride_tail[4]{};
};

[[nodiscard]] inline Vec3f gptm_decode_oct_normal(std::uint16_t ox, std::uint16_t oy) noexcept {
    const auto f = [](std::uint16_t v) noexcept -> float {
        return (static_cast<float>(v) / 65535.f) * 2.f - 1.f;
    };
    const float x = f(ox);
    const float y = f(oy);
    const float z = 1.f - std::fabs(x) - std::fabs(y);
    float         px = x;
    float         py = y;
    if (z < 0.f) {
        px = (x >= 0.f ? 1.f : -1.f) - x;
        py = (y >= 0.f ? 1.f : -1.f) - y;
    }
    const Vec3f n(px, py, z);
    const float l = std::sqrt(n.dot(n));
    if (l <= 1e-20f) {
        return Vec3f(0.f, 1.f, 0.f);
    }
    return Vec3f(px / l, py / l, z / l);
}

/// Up to five resident LOD meshes per chunk (Lod0 … Lod4).
using GptmLodChain = std::array<GptmTile, 5>;

/// LRU / streaming side owns this map — coordinates to the full LOD chain for GPTM draws.
using GptmMeshCache = gw::core::HashMap<ChunkCoord, GptmLodChain, ChunkCoordHash>;

/// Per-draw instance data for the terrain shader (f64 chunk origin split for GPU reconstruction).
/// Stored as scalar floats so HLSL `StructuredBuffer` scalar layout matches C++ byte-for-byte.
struct GptmTileInstance {
    float chunk_origin_x_lo{};
    float chunk_origin_y_lo{};
    float chunk_origin_z_lo{};
    float chunk_origin_x_hi{};
    float chunk_origin_y_hi{};
    float chunk_origin_z_hi{};
    std::uint32_t biome{};
    std::uint32_t padding{};
};

} // namespace gw::world::planet
