#include <doctest/doctest.h>

#include "engine/memory/arena_allocator.hpp"
#include "engine/world/planet/gptm_lod_selector.hpp"
#include "engine/world/planet/gptm_mesh_builder.hpp"
#include "engine/world/streaming/chunk_data.hpp"
#include "engine/world/streaming/heightmap_synthesis.hpp"
#include "engine/world/universe/hec.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <vector>

using gw::universe::hec_derive;
using gw::universe::HecDomain;
using gw::universe::UniverseSeed;
using gw::world::planet::ArenaAllocatorRef;
using gw::world::planet::GptmLod;
using gw::world::planet::GptmMeshBuilder;
using gw::world::planet::GptmLodSelector;
using gw::world::streaming::ChunkCoord;
using gw::world::streaming::HeightmapChunk;
using gw::world::streaming::chunk_origin;
using gw::world::streaming::fill_heightmap_chunk_full;

namespace {

[[nodiscard]] HeightmapChunk make_test_chunk() {
    static std::array<float, 1024>       hbuf{};
    static std::array<std::uint8_t, 1024> bbuf{};

    const UniverseSeed   world      = UniverseSeed("GPTM_TEST_LOT");
    const ChunkCoord     c{0, 0, 0};
    const auto           chunk_seed = hec_derive(world, HecDomain::Chunk, c.x, c.y, c.z);
    HeightmapChunk      hc{};
    hc.heights   = {hbuf.data(), hbuf.size()};
    hc.biome_ids = {bbuf.data(), bbuf.size()};
    hc.seed      = chunk_seed;
    hc.coord     = c;
    fill_heightmap_chunk_full(hc, chunk_seed);
    return hc;
}

[[nodiscard]] double median_ms(std::vector<double>& samples) {
    if (samples.empty()) {
        return 0.0;
    }
    const std::size_t mid = samples.size() / 2;
    std::nth_element(samples.begin(), samples.begin() + static_cast<std::ptrdiff_t>(mid),
                     samples.end());
    return samples[mid];
}

} // namespace

TEST_CASE("gptm: lod0 mesh topology and bounds") {
    gw::memory::ArenaAllocator arena(1 << 22);
    ArenaAllocatorRef          ar(arena);
    const HeightmapChunk       hc = make_test_chunk();

    const auto mesh = GptmMeshBuilder::build_lod(hc, GptmLod::Lod0, ar);
    REQUIRE(mesh.vertices != nullptr);
    REQUIRE(mesh.indices != nullptr);
    REQUIRE(mesh.vertex_count == GptmMeshBuilder::lod_vertex_count(GptmLod::Lod0));
    REQUIRE(mesh.index_count == GptmMeshBuilder::lod_index_count(GptmLod::Lod0));
    REQUIRE(mesh.vertex_count == 32u * 32u * 4u);

    float min_h = hc.heights[0];
    float max_h = hc.heights[0];
    for (std::size_t i = 1; i < gw::world::streaming::kHeightmapSampleCount; ++i) {
        min_h = std::min(min_h, hc.heights[i]);
        max_h = std::max(max_h, hc.heights[i]);
    }

    const auto origin = chunk_origin(hc.coord);
    for (std::uint32_t i = 0; i < mesh.vertex_count; ++i) {
        const auto& v = mesh.vertices[i];
        const float wx = static_cast<float>(origin.x()) + v.local_pos[0];
        const float wy = static_cast<float>(origin.y()) + v.local_pos[1];
        const float wz = static_cast<float>(origin.z()) + v.local_pos[2];

        REQUIRE(wx >= static_cast<float>(origin.x()) - 1e-3f);
        REQUIRE(wx <= static_cast<float>(origin.x()) + 32.f + 1e-3f);
        REQUIRE(wz >= static_cast<float>(origin.z()) - 1e-3f);
        REQUIRE(wz <= static_cast<float>(origin.z()) + 32.f + 1e-3f);
        REQUIRE(wy >= min_h - 0.05f);
        REQUIRE(wy <= max_h + 0.05f);

        const auto n = gw::world::planet::gptm_decode_oct_normal(v.n_oct_x, v.n_oct_y);
        const float len = std::sqrt(n.dot(n));
        REQUIRE(len == doctest::Approx(1.0f).epsilon(1e-5f));
    }
}

TEST_CASE("gptm: lod selector distance bands") {
    const ChunkCoord c0{0, 0, 0};
    const auto       center = chunk_origin(c0)
                        + gw::world::streaming::Vec3_f64(16.0, 16.0, 16.0);
    const GptmLod near_lod =
        GptmLodSelector::select(c0, center, 1.0471975512f, 1080u, nullptr);
    REQUIRE(near_lod == GptmLod::Lod0);

    const gw::world::streaming::Vec3_f64 far_cam(3000.0, 0.0, 0.0);
    const GptmLod far_lod = GptmLodSelector::select(c0, far_cam, 1.0471975512f, 1080u, nullptr);
    REQUIRE(far_lod == GptmLod::Lod4);
}

TEST_CASE("gptm: lod0 mesh build median under 3 ms") {
    gw::memory::ArenaAllocator arena(1 << 24);
    ArenaAllocatorRef          ar(arena);
    const HeightmapChunk       hc = make_test_chunk();

    std::vector<double> samples;
    samples.reserve(100);
    using clock = std::chrono::high_resolution_clock;
    for (int i = 0; i < 100; ++i) {
        arena.reset();
        const auto t0 = clock::now();
        static_cast<void>(GptmMeshBuilder::build_lod(hc, GptmLod::Lod0, ar));
        const auto t1     = clock::now();
        const auto micros = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
        samples.push_back(static_cast<double>(micros) / 1000.0);
    }

    const double med = median_ms(samples);
    MESSAGE("Lod0 mesh build median ms: ", med);
    REQUIRE(med < 3.0);
}

TEST_CASE("gptm: lod4 mesh build median under 0.1 ms") {
    gw::memory::ArenaAllocator arena(1 << 20);
    ArenaAllocatorRef          ar(arena);
    const HeightmapChunk       hc = make_test_chunk();

    std::vector<double> samples;
    samples.reserve(100);
    using clock = std::chrono::high_resolution_clock;
    for (int i = 0; i < 100; ++i) {
        arena.reset();
        const auto t0 = clock::now();
        static_cast<void>(GptmMeshBuilder::build_lod(hc, GptmLod::Lod4, ar));
        const auto t1     = clock::now();
        const auto micros = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
        samples.push_back(static_cast<double>(micros) / 1000.0);
    }

    const double med = median_ms(samples);
    MESSAGE("Lod4 mesh build median ms: ", med);
    REQUIRE(med < 0.1);
}
