#include <doctest/doctest.h>

#include "engine/core/span.hpp"
#include "engine/world/biome/biome_classifier.hpp"
#include "engine/world/streaming/chunk_coord.hpp"
#include "engine/world/streaming/chunk_data.hpp"
#include "engine/world/universe/hec.hpp"

#include <bitset>
#include <cstddef>
#include <vector>

using gw::universe::UniverseSeed;
using gw::universe::hec_to_rng;
using gw::world::biome::BiomeClassifier;
using gw::world::biome::BiomeParams;
using gw::world::streaming::BiomeId;
using gw::world::streaming::Vec3_f64;

TEST_CASE("biome classify is stable across repeated calls") {
    const BiomeClassifier cls(UniverseSeed("Stability"));
    const Vec3_f64        p(12.25, -3.5, 900.125);
    const BiomeId         a = cls.classify(p);
    for (int i = 0; i < 100; ++i) {
        CHECK(cls.classify(p) == a);
    }
}

TEST_CASE("biome classify_params covers every BiomeId") {
    const BiomeClassifier cls(UniverseSeed("Coverage"));

    struct Case {
        BiomeParams p{};
        BiomeId     expect{};
    };
    const Case table[] = {
        {{0.0F, 0.0F, -0.5F, 0.75F}, BiomeId::FleshTunnel},
        {{-0.6F, 0.4F, 0.0F, 0.2F}, BiomeId::FrozenCavern},
        {{0.5F, 0.0F, 0.0F, 0.85F}, BiomeId::IndustrialForge},
        {{0.0F, 0.0F, 0.65F, 0.1F}, BiomeId::LimboIsland},
        {{0.0F, -0.4F, 0.0F, 0.65F}, BiomeId::Battlefield},
        {{0.3F, 0.0F, 0.15F, 0.55F}, BiomeId::HereticLibrary},
        {{0.0F, 0.0F, 0.0F, 0.02F}, BiomeId::Void},
        {{0.2F, 0.21F, 0.1F, 0.42F}, BiomeId::DeceptiveMaze},
        {{-0.1F, 0.0F, -0.2F, 0.3F}, BiomeId::AbyssVoid},
        {{0.1F, 0.0F, 0.2F, 0.3F}, BiomeId::Arena},
    };
    for (const Case& c : table) {
        CHECK(cls.classify_params(c.p) == c.expect);
    }

    std::bitset<32> seen;
    auto            mark = [&seen](BiomeId id) {
        const auto u = static_cast<std::size_t>(id);
        REQUIRE(u < 32u);
        seen.set(u);
    };

    for (const Case& c : table) {
        mark(cls.classify_params(c.p));
    }

    auto rng = hec_to_rng(UniverseSeed("MonteCarlo"));
    for (int i = 0; i < 100'000; ++i) {
        BiomeParams p{};
        p.temperature = static_cast<float>(rng.next_f64_open() * 2.0 - 1.0);
        p.humidity    = static_cast<float>(rng.next_f64_open() * 2.0 - 1.0);
        p.elevation   = static_cast<float>(rng.next_f64_open() * 2.0 - 1.0);
        p.chaos       = static_cast<float>(rng.next_f64_open());
        mark(cls.classify_params(p));
    }

    for (std::size_t i = 0; i <= static_cast<std::size_t>(BiomeId::AbyssVoid); ++i) {
        INFO("missing biome index " << i);
        CHECK(seen.test(i));
    }
}

TEST_CASE("biome classify_bulk matches classify") {
    const BiomeClassifier cls(UniverseSeed("Bulk"));

    std::vector<Vec3_f64> pos;
    pos.reserve(64);
    auto rng = hec_to_rng(UniverseSeed("BulkPositions"));
    for (int i = 0; i < 64; ++i) {
        pos.emplace_back(static_cast<double>(rng.next_f64_open() * 2000.0 - 1000.0),
                         static_cast<double>(rng.next_f64_open() * 2000.0 - 1000.0),
                         static_cast<double>(rng.next_f64_open() * 200.0 - 100.0));
    }

    std::vector<BiomeId> bulk(64);
    cls.classify_bulk(gw::core::Span<const Vec3_f64>(pos.data(), pos.size()),
                      gw::core::Span<BiomeId>(bulk.data(), bulk.size()));

    for (std::size_t i = 0; i < pos.size(); ++i) {
        CHECK(cls.classify(pos[i]) == bulk[i]);
    }
}
