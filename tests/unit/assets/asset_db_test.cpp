// tests/unit/assets/asset_db_test.cpp
// Phase 6 spec §13.1: AssetHandle and AssetDatabase tests.
// No Vulkan required — tests the handle / slot mechanics only.
#include <doctest/doctest.h>
#include "engine/assets/asset_handle.hpp"
#include "engine/assets/asset_types.hpp"
#include "engine/assets/mesh_asset.hpp"
#include "engine/assets/texture_asset.hpp"

using namespace gw::assets;

TEST_CASE("AssetHandle null() has zero bits") {
    auto h = AssetHandle::null();
    CHECK(h.bits == 0u);
    CHECK(!h.valid());
}

TEST_CASE("AssetHandle::make encodes index, generation, type_tag correctly") {
    const uint32_t idx = 42u;
    const uint16_t gen = 7u;
    const uint16_t tag = static_cast<uint16_t>(AssetType::Mesh);

    const auto h = AssetHandle::make(idx, gen, tag);
    CHECK(h.valid());
    CHECK(h.index()      == idx);
    CHECK(h.generation() == gen);
    CHECK(h.type_tag()   == tag);
}

TEST_CASE("TypedHandle kTypeTag matches AssetType") {
    CHECK(MeshHandle::kTypeTag    == static_cast<uint16_t>(AssetType::Mesh));
    CHECK(TextureHandle::kTypeTag == static_cast<uint16_t>(AssetType::Texture));
}

TEST_CASE("AssetHandle equality") {
    auto h1 = AssetHandle::make(1u, 0u, 1u);
    auto h2 = AssetHandle::make(1u, 0u, 1u);
    auto h3 = AssetHandle::make(2u, 0u, 1u);
    CHECK(h1 == h2);
    CHECK(h1 != h3);
}

TEST_CASE("TypedHandle construction from AssetHandle preserves bits") {
    const auto base = AssetHandle::make(99u, 3u, MeshHandle::kTypeTag);
    const MeshHandle th{base};
    CHECK(th.bits == base.bits);
    CHECK(th.index() == 99u);
    CHECK(th.generation() == 3u);
}

TEST_CASE("AssetHandle null() is not equal to a valid handle") {
    auto null_h = AssetHandle::null();
    auto valid  = AssetHandle::make(0u, 0u, 1u);
    // Even index=0 gen=0 tag=1 has non-zero bits.
    CHECK(null_h != valid);
}
