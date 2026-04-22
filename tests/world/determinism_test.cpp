#include <doctest/doctest.h>

#include "engine/world/streaming/heightmap_synthesis.hpp"
#include "engine/world/universe/determinism_validator.hpp"

#include <array>
#include <thread>
#include <vector>

#include "engine/world/universe/hec.hpp"

using gw::universe::DeterminismValidator;
using gw::universe::HecDomain;
using gw::universe::hec_derive;
using gw::universe::UniverseSeed;
using gw::world::streaming::ChunkCoord;
using gw::world::streaming::HeightmapChunk;
using gw::world::streaming::fill_heightmap_chunk_full;
using gw::world::streaming::kHeightmapSampleCount;

namespace {

[[nodiscard]] std::uint64_t hash_chunk_000(const UniverseSeed& world) {
    const ChunkCoord c{0, 0, 0};
    const auto       chunk_seed = hec_derive(world, HecDomain::Chunk, c.x, c.y, c.z);
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

TEST_CASE("determinism: parallel chunk(0,0,0) generation — identical proof hash") {
    const UniverseSeed  world = UniverseSeed("TEST_SEED");
    constexpr int       n     = 100;
    std::vector<std::uint64_t>       hashes;
    std::vector<std::thread>          threads;
    hashes.resize(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        threads.emplace_back(
            [i, &world, &hashes] { hashes[static_cast<std::size_t>(i)] = hash_chunk_000(world); });
    }
    for (auto& t : threads) {
        t.join();
    }
    for (int i = 1; i < n; ++i) {
        REQUIRE(hashes[static_cast<std::size_t>(i)] == hashes[0]);
    }
}

TEST_CASE("determinism: different world seeds differ at (0,0,0)") {
    const std::uint64_t a = hash_chunk_000(UniverseSeed("TEST_SEED"));
    const std::uint64_t b = hash_chunk_000(UniverseSeed("OTHER_SEED"));
    REQUIRE(a != b);
}

TEST_CASE("determinism: TEST_SEED @ (0,0,0) matches proof vector") {
    REQUIRE(DeterminismValidator::validate_cross_platform_stability(UniverseSeed("TEST_SEED"), {0, 0, 0},
                                                                    gw::universe::GoldenHashes::TEST_SEED_CHUNK_000));
}

TEST_CASE("determinism: HELL_SEED_V1 @ (0,0,0) matches proof vector (Hell Seed share digest)") {
    REQUIRE(DeterminismValidator::validate_cross_platform_stability(UniverseSeed("HELL_SEED_V1"), {0, 0, 0},
                                                                    gw::universe::GoldenHashes::HELL_SEED_CIRCLE_1_CHUNK_000));
}
