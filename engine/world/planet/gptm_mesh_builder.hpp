#pragma once
// engine/world/planet/gptm_mesh_builder.hpp — CPU heightfield → GPTM triangle meshes + optional GPU upload.

#include "engine/memory/arena_allocator.hpp"
#include "engine/world/planet/gptm_types.hpp"
#include "engine/world/streaming/chunk_data.hpp"

#include <cstddef>
#include <cstdint>

namespace gw::render::hal {
class VulkanDevice;
}

namespace gw::world::planet {

/// Arena-backed bump allocator surface used by `GptmMeshBuilder::build_lod`.
class IAllocator {
public:
    virtual ~IAllocator() = default;
    [[nodiscard]] virtual void* allocate(std::size_t bytes, std::size_t alignment) = 0;
};

/// Adapts `gw::memory::ArenaAllocator` to `IAllocator` without widening the world allocator API.
class ArenaAllocatorRef final : public IAllocator {
public:
    explicit ArenaAllocatorRef(gw::memory::ArenaAllocator& arena) noexcept : arena_(arena) {}

    [[nodiscard]] void* allocate(const std::size_t bytes, const std::size_t alignment) override {
        return arena_.allocate(bytes, alignment);
    }

private:
    gw::memory::ArenaAllocator& arena_;
};

/// CPU-side mesh payload (views into `IAllocator` storage until the arena resets).
struct GptmMeshCpuData {
    const GptmVertex* vertices{};
    const std::uint32_t* indices{};
    std::uint32_t       vertex_count{0};
    std::uint32_t       index_count{0};
    Aabb3_f64           world_bounds{};
};

class GptmMeshBuilder {
public:
    /// Regular grid quads at `cells × cells` with four independent vertices per quad plus
    /// degenerate skirt indices (no extra vertices — crack prevention along chunk borders).
    [[nodiscard]] static GptmMeshCpuData build_lod(const gw::world::streaming::HeightmapChunk& chunk,
                                                   GptmLod                                  lod,
                                                   IAllocator&                              arena) noexcept;

    /// Grid resolution (cells per axis) for a GPTM LOD level.
    [[nodiscard]] static std::uint32_t lod_cells(GptmLod lod) noexcept;

    /// Expected vertex / index counts for the mesh builder (includes skirt degenerates in the index buffer).
    [[nodiscard]] static std::uint32_t lod_vertex_count(GptmLod lod) noexcept;
    [[nodiscard]] static std::uint32_t lod_index_count(GptmLod lod) noexcept;

    /// Uploads CPU mesh to VMA-backed device buffers; leaves handles in `out_tile`.
    [[nodiscard]] static bool upload_tile(gw::render::hal::VulkanDevice& device,
                                          const GptmMeshCpuData&          mesh,
                                          GptmTile&                       out_tile) noexcept;

    static void destroy_tile_buffers(gw::render::hal::VulkanDevice& device, GptmTile& tile) noexcept;
};

} // namespace gw::world::planet
