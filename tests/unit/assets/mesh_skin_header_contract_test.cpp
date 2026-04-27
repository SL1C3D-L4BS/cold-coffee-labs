// tests/unit/assets/mesh_skin_header_contract_test.cpp — cook ↔ runtime skin contract.

#include <doctest/doctest.h>

#include "engine/assets/mesh_asset.hpp"

static_assert(sizeof(float) * 16 == gw::assets::kInverseBindMat4Bytes);

TEST_CASE("mesh skin — inverse bind matrix byte stride is 4×4 floats") {
    CHECK(gw::assets::kInverseBindMat4Bytes == 64u);
}

TEST_CASE("mesh skin — kMeshFlagSkinned is distinct power-of-two") {
    CHECK((gw::assets::kMeshFlagSkinned & (gw::assets::kMeshFlagSkinned - 1u)) == 0u);
}
