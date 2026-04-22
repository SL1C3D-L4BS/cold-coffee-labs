// engine/world/streaming/chunk_streamer.cpp
//
// PATENT-PENDING PIPELINE (Greywater × Blacklake — Phase 19-B):
//   Σ₀ (studio session seed) → `UniverseSeedManager` → Σ_chunk = HEC(Chunk, cx, cy, cz).
//   Σ_chunk keys octave-stratified SDR height synthesis (`SdrNoise`, Feature domain 401) and four
//   orthogonal SDR scalar channels inside `BiomeClassifier` (Feature 301–304). The composed
//   height + biome lattice is deterministic, auditably replayable from Σ₀ alone, and is intentionally
//   isolated from gameplay mutation paths (jobs only, mutex-gated promotion to Ready).
//
// Cooperative time-slicing: each 32×32 tile is generated as **two horizontal bands**, each
// **16 rows × 32 columns** (512 samples), so two jobs cover all 1024 samples with no overlap.
// Literal “two 16×16” patches would only cover half the grid (512 samples) unless you schedule
// **four** quads; the row-band split matches the §4 job budget and keeps stripe size predictable.
// If a single band still exceeds ~10 ms wall-clock on reference hardware, subdivide further.

#include "engine/world/streaming/chunk_streamer.hpp"

#include "engine/core/vector.hpp"
#include "engine/world/streaming/heightmap_synthesis.hpp"
#include "engine/world/universe/hec.hpp"

#include <cstddef>
#include <cstdint>

namespace gw::world::streaming {

void ChunkStreamer::fill_height_strip(ChunkStreamer*          self,
                                      const std::size_t       slot_index,
                                      const ChunkCoord        coord,
                                      const gw::universe::UniverseSeed chunk_seed,
                                      const std::uint64_t     generation_frame,
                                      const int               y_begin,
                                      const int               y_end) noexcept {
    Slot&           slot = self->slots_[slot_index];
    HeightmapChunk& hc   = slot.chunk;

    fill_heightmap_row_range(hc, chunk_seed, y_begin, y_end);

    {
        std::lock_guard<std::mutex> lk(self->mutex_);
        ++self->halves_done_[slot_index];
        if (self->halves_done_[slot_index] == 2 && hc.coord == coord && hc.state == ChunkState::Generating) {
            hc.state             = ChunkState::Ready;
            hc.generation_frame = generation_frame;
        }
    }
}

ChunkStreamer::ChunkStreamer(gw::memory::ArenaAllocator&          arena,
                             gw::universe::UniverseSeedManager& seeds,
                             gw::jobs::Scheduler&                scheduler,
                             std::uint32_t                        max_loaded_chunks)
    : max_loaded_chunks_(max_loaded_chunks)
    , arena_(arena)
    , seeds_(seeds)
    , scheduler_(scheduler) {
    slots_.resize(static_cast<std::size_t>(max_loaded_chunks_));
    halves_done_.assign(static_cast<std::size_t>(max_loaded_chunks_), 0);
    free_slot_indices_.reserve(static_cast<std::size_t>(max_loaded_chunks_));
    for (std::size_t i = 0; i < static_cast<std::size_t>(max_loaded_chunks_); ++i) {
        free_slot_indices_.push_back(static_cast<std::size_t>(max_loaded_chunks_ - 1u - static_cast<std::uint32_t>(i)));
    }

    const std::size_t bytes_per_slot =
        kHeightmapSampleCount * sizeof(float) + kHeightmapSampleCount * sizeof(std::uint8_t);
    slab_ = arena_.allocate(static_cast<std::size_t>(max_loaded_chunks_) * bytes_per_slot, alignof(float));

    auto* const base = static_cast<std::byte*>(slab_);
    for (std::size_t i = 0; i < static_cast<std::size_t>(max_loaded_chunks_); ++i) {
        auto* h_ptr = reinterpret_cast<float*>(base + i * bytes_per_slot);
        auto* b_ptr = reinterpret_cast<std::uint8_t*>(h_ptr + kHeightmapSampleCount);
        slots_[i].chunk.heights    = gw::core::Span<float>(h_ptr, kHeightmapSampleCount);
        slots_[i].chunk.biome_ids = gw::core::Span<std::uint8_t>(b_ptr, kHeightmapSampleCount);
    }
}

gw::jobs::JobPriority ChunkStreamer::map_priority(const float p) noexcept {
    if (p >= 10.0F) {
        return gw::jobs::JobPriority::Critical;
    }
    if (p >= 5.0F) {
        return gw::jobs::JobPriority::High;
    }
    if (p >= 1.0F) {
        return gw::jobs::JobPriority::Normal;
    }
    return gw::jobs::JobPriority::Background;
}

void ChunkStreamer::touch_lru(const ChunkCoord c) {
    const auto it = coord_lru_it_.find(c);
    if (it == coord_lru_it_.end()) {
        return;
    }
    lru_.splice(lru_.begin(), lru_, it->second);
    coord_lru_it_[c] = lru_.begin();
}

void ChunkStreamer::evict_lru_tail() {
    for (auto rit = lru_.rbegin(); rit != lru_.rend(); ++rit) {
        const std::size_t si = *rit;
        if (!slots_[si].in_use) {
            continue;
        }
        if (slots_[si].chunk.state != ChunkState::Ready) {
            continue;
        }
        const ChunkCoord victim = slots_[si].chunk.coord;
        const auto       lit    = coord_lru_it_.find(victim);
        if (lit != coord_lru_it_.end()) {
            lru_.erase(lit->second);
            coord_lru_it_.erase(lit);
        }
        coord_to_slot_.erase(victim);
        slots_[si].in_use            = false;
        slots_[si].chunk.state       = ChunkState::Unloaded;
        slots_[si].chunk.coord       = ChunkCoord{};
        slots_[si].chunk.generation_frame = 0;
        halves_done_[si] = 0;
        free_slot_indices_.push_back(si);
        return;
    }
}

void ChunkStreamer::evict_out_of_radius(const ChunkCoord camera_chunk, const std::int32_t radius) {
    gw::core::Vector<ChunkCoord> victims;
    victims.reserve(64);
    for (const std::size_t si : lru_) {
        if (!slots_[si].in_use) {
            continue;
        }
        if (slots_[si].chunk.state != ChunkState::Ready) {
            continue;
        }
        const ChunkCoord c = slots_[si].chunk.coord;
        if (chebyshev_distance(c, camera_chunk) > radius) {
            victims.push_back(c);
        }
    }
    for (const ChunkCoord v : victims) {
        const auto sit = coord_to_slot_.find(v);
        if (sit == coord_to_slot_.end()) {
            continue;
        }
        const std::size_t si = sit->second;
        const auto        lit = coord_lru_it_.find(v);
        if (lit != coord_lru_it_.end()) {
            lru_.erase(lit->second);
            coord_lru_it_.erase(lit);
        }
        coord_to_slot_.erase(v);
        slots_[si].in_use            = false;
        slots_[si].chunk.state       = ChunkState::Unloaded;
        slots_[si].chunk.coord       = ChunkCoord{};
        slots_[si].chunk.generation_frame = 0;
        halves_done_[si] = 0;
        free_slot_indices_.push_back(si);
    }
}

void ChunkStreamer::submit_generation_jobs(const std::size_t slot_index,
                                           const ChunkCoord coord,
                                           const float      priority) {
    gw::universe::UniverseSeed chunk_seed{};
    gw::jobs::JobPriority jp{};
    std::uint64_t         fc = 0;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        chunk_seed = seeds_.get_chunk_seed(coord.x, coord.y, coord.z);
        HeightmapChunk& hc = slots_[slot_index].chunk;
        hc.seed             = chunk_seed;
        hc.coord            = coord;
        hc.state            = ChunkState::Generating;
        halves_done_[slot_index] = 0;
        jp                  = map_priority(priority);
        fc                  = frame_counter_;
    }

    scheduler_.submit(jp, [this, slot_index, coord, chunk_seed, fc]() {
        ChunkStreamer::fill_height_strip(this, slot_index, coord, chunk_seed, fc, 0, 16);
    });
    scheduler_.submit(jp, [this, slot_index, coord, chunk_seed, fc]() {
        ChunkStreamer::fill_height_strip(this, slot_index, coord, chunk_seed, fc, 16, 32);
    });
}

void ChunkStreamer::request_chunk(const ChunkCoord coord, const float priority) {
    std::size_t slot_index = 0;
    bool        schedule   = false;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        if (const auto existing = coord_to_slot_.find(coord); existing != coord_to_slot_.end()) {
            touch_lru(coord);
            return;
        }

        while (free_slot_indices_.empty()) {
            const std::size_t in_flight = coord_to_slot_.size();
            evict_lru_tail();
            if (free_slot_indices_.empty() && coord_to_slot_.size() == in_flight) {
                return;
            }
        }

        if (coord_to_slot_.size() >= static_cast<std::size_t>(max_loaded_chunks_)) {
            evict_lru_tail();
        }
        if (free_slot_indices_.empty()) {
            return;
        }

        slot_index = free_slot_indices_.back();
        free_slot_indices_.pop_back();

        slots_[slot_index].in_use      = true;
        slots_[slot_index].chunk.coord = coord;
        slots_[slot_index].chunk.state = ChunkState::Queued;
        coord_to_slot_[coord]          = slot_index;
        lru_.push_front(slot_index);
        coord_lru_it_[coord] = lru_.begin();
        schedule             = true;
    }
    if (schedule) {
        submit_generation_jobs(slot_index, coord, priority);
    }
}

const HeightmapChunk* ChunkStreamer::get_chunk(const ChunkCoord coord) const {
    std::lock_guard<std::mutex> lk(mutex_);
    const auto                    it = coord_to_slot_.find(coord);
    if (it == coord_to_slot_.end()) {
        return nullptr;
    }
    const HeightmapChunk& ch = slots_[it->second].chunk;
    if (ch.state != ChunkState::Ready) {
        return nullptr;
    }
    return &ch;
}

void ChunkStreamer::tick(const Vec3_f64 camera_world_pos, const std::int32_t load_radius_chunks) {
    const ChunkCoord cc = world_to_chunk(camera_world_pos);
    gw::core::Vector<ChunkCoord> pending;
    pending.reserve(static_cast<std::size_t>((2 * load_radius_chunks + 1) * (2 * load_radius_chunks + 1)
                                              * (2 * load_radius_chunks + 1)));

    {
        std::lock_guard<std::mutex> lk(mutex_);
        ++frame_counter_;
        evict_out_of_radius(cc, load_radius_chunks);

        for (std::int32_t dz = -load_radius_chunks; dz <= load_radius_chunks; ++dz) {
            for (std::int32_t dy = -load_radius_chunks; dy <= load_radius_chunks; ++dy) {
                for (std::int32_t dx = -load_radius_chunks; dx <= load_radius_chunks; ++dx) {
                    const ChunkCoord c{cc.x + static_cast<std::int64_t>(dx),
                                         cc.y + static_cast<std::int64_t>(dy),
                                         cc.z + static_cast<std::int64_t>(dz)};
                    if (chebyshev_distance(c, cc) > load_radius_chunks) {
                        continue;
                    }
                    if (!coord_to_slot_.contains(c)) {
                        pending.push_back(c);
                    }
                }
            }
        }
    }

    for (const ChunkCoord c : pending) {
        request_chunk(c, 2.0F);
    }
}

} // namespace gw::world::streaming
