// tests/unit/render/shader/phase17_permutation_test.cpp — ADR-0074 Wave 17A.

#include <doctest/doctest.h>

#include "engine/render/shader/permutation.hpp"
#include "engine/render/shader/permutation_cache.hpp"
#include "engine/render/shader/reflect.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

using namespace gw::render::shader;

TEST_CASE("phase17.permutation: bool axes count correctly") {
    PermutationTable t;
    t.add_bool_axis("GW_HAS_CLEARCOAT");
    t.add_bool_axis("GW_HAS_SHEEN");
    CHECK(t.axis_count() == 2);
    CHECK(t.permutation_count() == 4);
}

TEST_CASE("phase17.permutation: enum axis contributes correct radix") {
    PermutationTable t;
    t.add_bool_axis("GW_HAS_CLEARCOAT");
    t.add_enum_axis("GW_QUALITY", {"LOW", "MEDIUM", "HIGH"});
    CHECK(t.axis_count() == 2);
    CHECK(t.permutation_count() == 2 * 3);
}

TEST_CASE("phase17.permutation: encode/decode roundtrip stable") {
    PermutationTable t;
    t.add_bool_axis("A");
    t.add_bool_axis("B");
    t.add_enum_axis("Q", {"LO", "HI", "ULTRA"});
    std::array<VariantIndex, 3> idx{1, 0, 2};
    const auto k = t.encode(idx);
    const auto back = t.decode(k);
    REQUIRE(back.size() == 3);
    CHECK(back[0] == 1);
    CHECK(back[1] == 0);
    CHECK(back[2] == 2);
    CHECK(t.valid(k));
}

TEST_CASE("phase17.permutation: invalid index rejected") {
    PermutationTable t;
    t.add_bool_axis("A");
    std::array<VariantIndex, 1> bad{7};
    const auto k = t.encode(bad);
    CHECK(k.bits == ~std::uint32_t{0});
    CHECK_FALSE(t.valid(k));
}

TEST_CASE("phase17.permutation: 64-ceiling budget check") {
    PermutationTable t;
    for (int i = 0; i < 6; ++i) t.add_bool_axis("GW_BIT_" + std::to_string(i));
    PermutationBudget b{64};
    CHECK(b.within(t));
    t.add_bool_axis("GW_BIT_7");
    CHECK_FALSE(b.within(t));
}

TEST_CASE("phase17.permutation: HLSL pragma parse extracts axes") {
    const char* src =
        "// gw_axis: GW_HAS_CLEARCOAT = bool\n"
        "// gw_axis: GW_QUALITY = low|med|high\n"
        "void main() {}\n";
    auto t = extract_axes_from_hlsl(src);
    CHECK(t.axis_count() == 2);
    CHECK(t.permutation_count() == 2 * 3);
}

TEST_CASE("phase17.permutation: defines_for emits -D switches") {
    PermutationTable t;
    t.add_bool_axis("GW_HAS_CLEARCOAT");
    t.add_enum_axis("QUALITY", {"LOW", "HIGH"});
    std::array<VariantIndex, 2> idx{1, 1};
    const auto k = t.encode(idx);
    const auto defs = t.defines_for(k);
    CHECK(defs.find("GW_HAS_CLEARCOAT=1") != std::string::npos);
    CHECK(defs.find("QUALITY_HIGH=1") != std::string::npos);
}

TEST_CASE("phase17.cache: content hash is deterministic") {
    const auto h1 = PermutationCache::compute_hash("void main(){}", "-DX=1", "dxc-1.9.2602");
    const auto h2 = PermutationCache::compute_hash("void main(){}", "-DX=1", "dxc-1.9.2602");
    CHECK(h1 == h2);
    const auto h3 = PermutationCache::compute_hash("void main(){}", "-DX=2", "dxc-1.9.2602");
    CHECK(h1 != h3);
}

TEST_CASE("phase17.cache: hit/miss accounting") {
    PermutationCache c{1024 * 1024};
    CacheKey k{"pbr_opaque", PermutationKey{1}, 0x1111u};
    CHECK(c.get(k) == nullptr);
    CachedPermutation v{};
    v.spirv_words = {0x07230203u, 0x00010600u, 1, 1, 0};
    v.content_hash = 0x1111u;
    REQUIRE(c.insert(k, std::move(v)));
    CHECK(c.get(k) != nullptr);
    const auto s = c.stats();
    CHECK(s.hits == 1);
    CHECK(s.misses == 1);
}

TEST_CASE("phase17.cache: LRU eviction under capacity pressure") {
    PermutationCache c{256};   // tiny cap → insert bumps evictions
    for (int i = 0; i < 8; ++i) {
        CacheKey k{"t", PermutationKey{static_cast<std::uint32_t>(i)}, static_cast<std::uint64_t>(i)};
        CachedPermutation v{};
        v.spirv_words.assign(20, 0x07230203u);
        (void)c.insert(k, std::move(v));
    }
    CHECK(c.stats().evictions > 0);
}

TEST_CASE("phase17.reflect: rejects non-SPIR-V header") {
    std::array<std::uint32_t, 5> junk{0xDEADBEEFu, 1, 0, 0, 0};
    const auto r = reflect_spirv(junk.data(), junk.size());
    CHECK_FALSE(r.ok());
}

TEST_CASE("phase17.reflect: header-only path picks up entry point + stage") {
    // Minimal: magic, version, generator, bound=1, schema=0, then an
    // OpEntryPoint Vertex with name "main\0\0\0\0" (two words).
    std::vector<std::uint32_t> words = {
        0x07230203u, 0x00010600u, 0u, 1u, 0u,
        // OpEntryPoint: opcode=15, wc=5 → 0x00050000 | 0x000F == 0x0005000F
        (5u << 16) | 15u,
        0u,           // Vertex
        1u,           // entry point id
        0x6E69616Du,  // 'main'
        0u,           // NUL
    };
    const auto r = reflect_spirv(words.data(), words.size());
    CHECK(r.ok());
    CHECK((r.stage_mask & static_cast<std::uint32_t>(ShaderStageBit::Vertex)) != 0);
    CHECK(r.entry_point == "main");
}
