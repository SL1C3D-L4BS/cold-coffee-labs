// tests/unit/render/authoring_lighting_blob_test.cpp — lighting cook blob (plan Track C1).

#include <doctest/doctest.h>

#include "engine/render/authoring_lighting.hpp"
#include "engine/render/scene_surface_contract.hpp"

TEST_CASE("authoring_lighting — blob round-trip") {
    gw::render::authoring::LightingParams src{};
    src.ambient_intensity = 0.4f;
    src.light_count       = 3;
    src.lights[2].intensity = 2.5f;

    alignas(gw::render::authoring::LightingParams) std::byte buf[512]{};
    const auto n = gw::render::authoring::encode_lighting_blob(
        src, buf, sizeof(buf));
    REQUIRE(n == gw::render::authoring::lighting_blob_byte_size());

    gw::render::authoring::LightingParams dst{};
    REQUIRE(gw::render::authoring::decode_lighting_blob(&dst, buf, n));
    CHECK(dst.ambient_intensity == doctest::Approx(src.ambient_intensity));
    CHECK(dst.light_count == 3);
    CHECK(dst.lights[2].intensity == doctest::Approx(2.5f));
}

TEST_CASE("scene_surface_contract — draw token is POD") {
    gw::render::SceneMeshDrawToken t{.mesh_content_hash = 0x42, .material_id = 3};
    CHECK(t.mesh_content_hash == 0x42u);
}
