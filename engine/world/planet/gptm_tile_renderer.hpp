#pragma once
// engine/world/planet/gptm_tile_renderer.hpp — Frame-graph terrain pass for GPTM tiles.

#include "engine/math/mat.hpp"
#include "engine/world/planet/gptm_types.hpp"
#include "engine/world/streaming/chunk_coord.hpp"

#include <cstddef>
#include <cstdint>

namespace gw::config {
class CVarRegistry;
}

namespace gw::ecs {
class World;
}

namespace gw::memory {
class FrameAllocator;
}

namespace gw::render::frame_graph {
class FrameGraph;
}

namespace gw::world::planet {

/// Minimal view packet for terrain (caller fills each frame before `tick`).
struct RenderView {
    gw::math::Mat4f                view{};
    gw::math::Mat4f               projection{};
    gw::world::streaming::Vec3_f64 camera_world_pos{};
    float                         fov_y_rad{1.0471975512f}; ///< Default ~60°; radians.
    std::uint32_t                 screen_width_px{1920};
    std::uint32_t                 screen_height_px{1080};
};

/// Batches GPTM draws into five frame-graph passes (one per LOD band).
class GptmTileRenderer {
public:
    static constexpr std::uint32_t kMaxTilesPerLod = 2048;

    /// Rebuilds the per-LOD tile pointer bins from `cache` using `GptmLodSelector`.
    void begin_frame(GptmMeshCache& cache, const gw::world::streaming::Vec3_f64& camera_world,
                     float fov_y_rad, std::uint32_t screen_height_px,
                     const gw::config::CVarRegistry* cvars = nullptr) noexcept;

    /// Injects five `gptm_lod*` graphics passes. `FrameGraph` must not yet be compiled.
    /// Uses `frame_alloc` for a packed `GptmTileInstance` blob (SSBO-style layout) — zero heap traffic.
    void tick(gw::ecs::World& world, const RenderView& view, gw::render::frame_graph::FrameGraph& fg,
              gw::memory::FrameAllocator& frame_alloc) noexcept;

    [[nodiscard]] std::uint32_t visible_count(GptmLod lod) const noexcept {
        return counts_[static_cast<std::size_t>(lod)];
    }

private:
    const GptmTile* tiles_by_lod_[5][kMaxTilesPerLod]{};
    std::uint32_t   instance_ids_[5][kMaxTilesPerLod]{};
    std::uint32_t   counts_[5]{};
    std::uint32_t   total_instances_{0};
};

} // namespace gw::world::planet
