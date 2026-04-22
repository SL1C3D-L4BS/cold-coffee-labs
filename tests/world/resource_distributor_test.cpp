#include <doctest/doctest.h>

#include "engine/world/resources/resource_distributor.hpp"
#include "engine/world/resources/resource_types.hpp"
#include "engine/world/streaming/heightmap_synthesis.hpp"
#include "engine/world/universe/hec.hpp"

#include <array>
#include <cmath>
#include <cstdint>

using gw::universe::hec_derive;
using gw::universe::HecDomain;
using gw::universe::UniverseSeed;
using gw::world::resources::ResourceDistributor;
using gw::world::resources::ResourceDensityMap;
using gw::world::resources::ResourceType;
using gw::world::streaming::ChunkCoord;
using gw::world::streaming::HeightmapChunk;
using gw::world::streaming::fill_heightmap_chunk_full;

namespace {

[[nodiscard]] HeightmapChunk make_filled_chunk(const char* const seed_str) {
    const auto             world      = UniverseSeed(seed_str);
    const ChunkCoord       c{2, 3, 0};
    const auto             chunk_seed = hec_derive(world, HecDomain::Chunk, c.x, c.y, c.z);
    static std::array<float, 1024>     hbuf;
    static std::array<std::uint8_t, 1024> bbuf;
    // Serialised tests: single chunk buffer at a time.
    hbuf.fill(0.0F);
    bbuf.fill(0);
    HeightmapChunk hc{};
    hc.heights   = {hbuf.data(), hbuf.size()};
    hc.biome_ids = {bbuf.data(), bbuf.size()};
    hc.seed      = chunk_seed;
    hc.coord     = c;
    fill_heightmap_chunk_full(hc, chunk_seed);
    return hc;
}

} // namespace

TEST_CASE("resource distributor: count in [1, 64] for known chunk") {
    const auto       d  = ResourceDensityMap::make_uniform(0.35F);
    const HeightmapChunk hc = make_filled_chunk("RDI_TEST_LOT");
    const auto       b  = ResourceDistributor::generate(hc, d);
    REQUIRE(b.count >= 1u);
    REQUIRE(b.count <= 64u);
}

TEST_CASE("resource distributor: 50x identical batch") {
    const auto       d  = ResourceDensityMap::make_uniform(0.35F);
    const HeightmapChunk hc = make_filled_chunk("DETERM_LOT_50");
    const auto        first = ResourceDistributor::generate(hc, d);
    for (int r = 0; r < 50; ++r) {
        const auto g = ResourceDistributor::generate(hc, d);
        REQUIRE(g.count == first.count);
        for (std::uint32_t i = 0; i < first.count; ++i) {
            REQUIRE(g.nodes[i].type == first.nodes[i].type);
            REQUIRE(g.nodes[i].world_pos.x() == first.nodes[i].world_pos.x());
            REQUIRE(g.nodes[i].world_pos.y() == first.nodes[i].world_pos.y());
            REQUIRE(g.nodes[i].world_pos.z() == first.nodes[i].world_pos.z());
        }
    }
}

TEST_CASE("resource distributor: types occupy distinct 2D positions in chunk plane") {
    const auto       d  = ResourceDensityMap::make_uniform(0.4F);
    const HeightmapChunk hc = make_filled_chunk("DISTINCT_XY");
    const auto       b  = ResourceDistributor::generate(hc, d);
    for (std::uint32_t i = 0; i < b.count; ++i) {
        for (std::uint32_t j = i + 1; j < b.count; ++j) {
            if (b.nodes[i].type != b.nodes[j].type) {
                const double a = static_cast<double>(b.nodes[i].world_pos.x());
                const double c = static_cast<double>(b.nodes[j].world_pos.x());
                const double s = static_cast<double>(b.nodes[i].world_pos.y());
                const double t = static_cast<double>(b.nodes[j].world_pos.y());
                // Same X/Y in metres → reject (2D “overlap” for distinct resource kinds).
                REQUIRE((std::fabs(a - c) + std::fabs(s - t)) > 1.0e-2);
            }
        }
    }
}

TEST_CASE("resource distributor: pairwise separation > kPairwiseMinSeparationM") {
    const auto         d  = ResourceDensityMap::make_uniform(0.35F);
    const HeightmapChunk   hc = make_filled_chunk("SEPARATION");
    const auto         b  = ResourceDistributor::generate(hc, d);
    const double       m  = static_cast<double>(ResourceDistributor::kPairwiseMinSeparationM);
    const double       m2 = m * m;
    for (std::uint32_t i = 0; i < b.count; ++i) {
        for (std::uint32_t j = i + 1; j < b.count; ++j) {
            const double a = static_cast<double>(b.nodes[i].world_pos.x() - b.nodes[j].world_pos.x());
            const double s = static_cast<double>(b.nodes[i].world_pos.y() - b.nodes[j].world_pos.y());
            REQUIRE((a * a + s * s) > m2);
        }
    }
}

TEST_CASE("resource type enum count matches density column contract") {
    const auto d   = ResourceDensityMap::make_uniform(0.1F);
    const auto row = d.density[0];
    REQUIRE(row.size() == 7u);
    REQUIRE(row.size() == 7u);
    REQUIRE(7u == static_cast<std::uint32_t>(ResourceType::COUNT));
}
