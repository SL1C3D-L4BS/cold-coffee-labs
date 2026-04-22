#pragma once
// engine/world/streaming/chunk_streamer.hpp — Phase 19-B LRU heightmap cache + job-queue generation.

#include "engine/core/hash_map.hpp"
#include "engine/jobs/scheduler.hpp"
#include "engine/memory/arena_allocator.hpp"
#include "engine/world/streaming/chunk_coord.hpp"
#include "engine/world/streaming/chunk_data.hpp"
#include "engine/world/universe/universe_seed_manager.hpp"

#include <cstddef>
#include <cstdint>
#include <list>
#include <mutex>
#include <vector>

namespace gw::world::streaming {

/// Owns an LRU cache of `HeightmapChunk` views backed by a single arena slab; generation runs on
/// `gw::jobs::Scheduler` workers (never the main thread inside the job body).
///
/// Pipeline (patent-pending when composed with HEC Σ_chunk — see `chunk_streamer.cpp`):
/// 1. `UniverseSeedManager::get_chunk_seed(cx,cy,cz)` → Σ_chunk (HEC domain `Chunk`).
/// 2. Σ_chunk drives `SdrNoise` octave packets for scalar height + four independent biome SDR
///    fields (`BiomeClassifier`, itself HEC-stratified per field channel).
/// 3. Each lattice site writes `heights[]` (f32) and `biome_ids[]` (`BiomeId`) into arena spans;
///    state transitions `Queued → Generating → Ready` under mutex so simulation never reads partial
///    tiles.
class ChunkStreamer {
public:
    /// `max_loaded_chunks` defaults to 512 — LRU footprint for a single streaming shell.
    ChunkStreamer(gw::memory::ArenaAllocator&          arena,
                  gw::universe::UniverseSeedManager& seeds,
                  gw::jobs::Scheduler&                scheduler,
                  std::uint32_t max_loaded_chunks = 512);

    /// Enqueues background synthesis if the chunk is absent or was evicted; LRU-touch if present.
    void request_chunk(ChunkCoord coord, float priority);

    /// Returns a live pointer only while `state == Ready`; nullptr otherwise (caller must treat as
    /// ephemeral — mutex protects internal tables, not per-chunk data races after Ready).
    [[nodiscard]] const HeightmapChunk* get_chunk(ChunkCoord coord) const;

    /// Increments the generation frame counter, evicts chunks outside Chebyshev `load_radius_chunks`
    /// of the camera chunk (LRU order among cold entries), and requests missing in-radius tiles.
    void tick(Vec3_f64 camera_world_pos, std::int32_t load_radius_chunks);

    [[nodiscard]] std::uint64_t generation_frame_counter() const noexcept { return frame_counter_; }

private:
    struct Slot {
        bool           in_use{false};
        HeightmapChunk chunk{};
    };

    void touch_lru(ChunkCoord coord);
    void evict_lru_tail();
    void evict_out_of_radius(ChunkCoord camera_chunk, std::int32_t radius);
    void submit_generation_jobs(std::size_t slot_index, ChunkCoord coord, float priority);

    [[nodiscard]] static gw::jobs::JobPriority map_priority(float p) noexcept;

    static void fill_height_strip(ChunkStreamer*               self,
                                  std::size_t                slot_index,
                                  ChunkCoord                 coord,
                                  gw::universe::UniverseSeed chunk_seed,
                                  std::uint64_t              generation_frame,
                                  int                        y_begin,
                                  int                        y_end) noexcept;

    std::uint32_t                     max_loaded_chunks_{512};
    gw::memory::ArenaAllocator&       arena_;
    gw::universe::UniverseSeedManager& seeds_;
    gw::jobs::Scheduler&              scheduler_;

    std::vector<Slot>  slots_{};
    /// Stripe completion (two stripes per chunk); guarded by `mutex_` together with LRU tables.
    std::vector<int>   halves_done_{};
    std::vector<std::size_t>         free_slot_indices_{};
    void*                            slab_{nullptr};

    gw::core::HashMap<ChunkCoord, std::size_t, ChunkCoordHash> coord_to_slot_{};
    std::list<std::size_t>           lru_{}; ///< slot indices, front = MRU
    gw::core::HashMap<ChunkCoord, std::list<std::size_t>::iterator, ChunkCoordHash> coord_lru_it_{};

    mutable std::mutex mutex_{};
    std::uint64_t                    frame_counter_{0};
};

} // namespace gw::world::streaming
