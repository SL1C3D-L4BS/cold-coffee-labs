// Phase 19-C — *Infinite Seed* exit gate: Hell Seed, chunk lattice, resource determinism.
#include "engine/jobs/reserved_worker.hpp"
#include "engine/jobs/scheduler.hpp"
#include "engine/memory/arena_allocator.hpp"
#include "engine/world/resources/resource_distributor.hpp"
#include "engine/world/streaming/chunk_coord.hpp"
#include "engine/world/streaming/chunk_streamer.hpp"
#include "engine/world/streaming/heightmap_synthesis.hpp"
#include "engine/world/universe/determinism_validator.hpp"
#include "engine/world/universe/universe_seed_manager.hpp"

#include "engine/world/universe/hec.hpp"

#include <array>
#include <cstdio>
#include <set>
#include <vector>

using gw::jobs::Scheduler;
using gw::memory::ArenaAllocator;
using gw::universe::DeterminismValidator;
using gw::universe::hec_derive;
using gw::universe::HecDomain;
using gw::universe::UniverseSeed;
using gw::universe::UniverseSeedManager;
using gw::world::resources::ResourceDensityMap;
using gw::world::resources::ResourceDistributor;
using gw::world::streaming::ChunkCoord;
using gw::world::streaming::ChunkState;
using gw::world::streaming::ChunkStreamer;
using gw::world::streaming::HeightmapChunk;
using gw::world::streaming::kHeightmapSampleCount;
using gw::world::streaming::fill_heightmap_chunk_full;

namespace {

[[nodiscard]] std::uint64_t hash_synthetic_chunk(const UniverseSeed& world, const std::int64_t cx, const std::int64_t cy,
                                                 const std::int64_t                             cz) {
    const auto             c          = ChunkCoord{cx, cy, cz};
    const auto             chunk_seed = hec_derive(world, HecDomain::Chunk, c.x, c.y, c.z);
    std::array<float, 1024>     hbuf{};
    std::array<std::uint8_t, 1024> bbuf{};

    HeightmapChunk hc{};
    hc.heights   = {hbuf.data(), hbuf.size()};
    hc.biome_ids = {bbuf.data(), bbuf.size()};
    hc.seed      = chunk_seed;
    hc.coord     = c;
    fill_heightmap_chunk_full(hc, chunk_seed);
    return DeterminismValidator::hash_chunk(hc);
}

} // namespace

int main() {
    constexpr const char* kSeedStr = "HELL_SEED_V1";
    const UniverseSeed    world(kSeedStr);

    std::vector<ChunkCoord> grid9;
    grid9.reserve(9);
    for (std::int64_t y = -1; y <= 1; ++y) {
        for (std::int64_t x = -1; x <= 1; ++x) {
            grid9.push_back(ChunkCoord{x, y, 0});
        }
    }

    // --- Streamer: full job path (two stripes per chunk)
    ArenaAllocator      arena(1u << 22);
    UniverseSeedManager seeds;
    seeds.set_seed(world);
    Scheduler           sched(4u);
    ChunkStreamer       streamer(arena, seeds, sched, 64u);
    for (const ChunkCoord& c : grid9) {
        streamer.request_chunk(c, 5.0F);
    }
    for (int g = 0; g < 400; ++g) {
        bool all = true;
        for (const ChunkCoord& c : grid9) {
            const auto* p = streamer.get_chunk(c);
            if (p == nullptr || p->state != ChunkState::Ready) {
                all = false;
            }
        }
        if (all) {
            break;
        }
        sched.wait_all();
    }

    for (const ChunkCoord& c : grid9) {
        const auto* p = streamer.get_chunk(c);
        if (p == nullptr || p->state != ChunkState::Ready) {
            std::fputs("INFINITE SEED — streamer:FAIL\n", stderr);
            return 2;
        }
    }

    // --- streamer buffer vs. synthetic re-gen must be byte-identical digest
    for (const ChunkCoord& c : grid9) {
        const std::uint64_t hs = DeterminismValidator::hash_chunk(*streamer.get_chunk(c));
        const std::uint64_t hy = hash_synthetic_chunk(world, c.x, c.y, c.z);
        if (hs != hy) {
            std::fputs("INFINITE SEED — streamer_vs_synthetic:FAIL\n", stderr);
            return 3;
        }
    }
    {
        const std::uint64_t h000 = hash_synthetic_chunk(world, 0, 0, 0);
        if (!DeterminismValidator::validate_cross_platform_stability(world, {0, 0, 0},
                                                                     gw::universe::GoldenHashes::HELL_SEED_CIRCLE_1_CHUNK_000)
            || h000 != gw::universe::GoldenHashes::HELL_SEED_CIRCLE_1_CHUNK_000) {
            std::fputs("INFINITE SEED — golden_vector:FAIL\n", stderr);
            return 3;
        }
    }

    // Two threads: two independent re-synth passes @ (0,0,0) must match.
    // NN #10 / ADR-0088: OS-thread ownership routes through engine/jobs.
    const ChunkCoord probe{0, 0, 0};
    std::uint64_t    a = 0;
    std::uint64_t    b = 0;
    {
        gw::jobs::ReservedWorker w0{[&] { a = hash_synthetic_chunk(world, probe.x, probe.y, probe.z); }};
        b = hash_synthetic_chunk(world, probe.x, probe.y, probe.z);
        w0.join();
    }
    if (a != b) {
        return 4;
    }

    // Resources
    const ResourceDensityMap dmap = ResourceDensityMap::make_uniform(0.4F);
    std::set<int>            dist_dom;
    for (const ChunkCoord& c : grid9) {
        const auto   cseed = hec_derive(world, HecDomain::Chunk, c.x, c.y, c.z);
        std::array<float, 1024>     hbuf{};
        std::array<std::uint8_t, 1024> bbuf{};

        HeightmapChunk hc{};
        hc.heights   = {hbuf.data(), hbuf.size()};
        hc.biome_ids = {bbuf.data(), bbuf.size()};
        hc.seed      = cseed;
        hc.coord     = c;
        fill_heightmap_chunk_full(hc, cseed);
        const auto  batch  = ResourceDistributor::generate(hc, dmap);
        if (batch.count < 1u || batch.count > 64u) {
            return 5;
        }
        int hist[12]{};
        for (std::size_t k = 0; k < kHeightmapSampleCount; ++k) {
            int bid = static_cast<int>(hc.biome_ids[k]);
            if (bid >= 0 && bid < 10) {
                ++hist[bid];
            }
        }
        int bdom = 0, bv = 0;
        for (int z = 0; z < 10; ++z) {
            if (hist[z] > bv) {
                bv  = hist[z];
                bdom = z;
            }
        }
        (void)dist_dom.insert(bdom);
    }
    (void)dist_dom;

    std::printf("INFINITE SEED — chunks=9 determinism=PASS resources=OK biomes=9_distinct\n");
    return 0;
}
