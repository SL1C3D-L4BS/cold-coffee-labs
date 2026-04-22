// tests/unit/render/material/phase17_material_template_test.cpp — ADR-0075.

#include <doctest/doctest.h>

#include "engine/render/material/material_template.hpp"
#include "engine/render/material/material_types.hpp"

using namespace gw::render::material;

static MaterialTemplateDesc make_desc(PbrExtension ext = PbrExtension::None,
                                       MaterialQualityTier q = MaterialQualityTier::High) {
    MaterialTemplateDesc d;
    d.name = "pbr_opaque/test";
    d.extensions = ext;
    d.quality = q;
    d.parameter_keys = {"base_color", "metallic_roughness", "emissive"};
    return d;
}

TEST_CASE("phase17.mat.template: five frozen permutation axes seeded") {
    MaterialTemplate t{make_desc()};
    CHECK(t.permutations().axis_count() == 5);
}

TEST_CASE("phase17.mat.template: permutation count == 2^4 * 3 (48)") {
    MaterialTemplate t{make_desc()};
    CHECK(t.permutation_count() == 48u);
}

TEST_CASE("phase17.mat.template: permutation count fits per-template ceiling of 64") {
    MaterialTemplate t{make_desc()};
    CHECK(t.permutation_count() <= 64u);
}

TEST_CASE("phase17.mat.template: clearcoat gate flips first axis") {
    MaterialTemplate t{make_desc()};
    const auto k_off = t.select_permutation(PbrExtension::None,        MaterialQualityTier::High);
    const auto k_on  = t.select_permutation(PbrExtension::Clearcoat,   MaterialQualityTier::High);
    CHECK(k_off.bits != k_on.bits);
}

TEST_CASE("phase17.mat.template: specular gate independent of clearcoat gate") {
    MaterialTemplate t{make_desc()};
    const auto a = t.select_permutation(PbrExtension::Clearcoat | PbrExtension::Specular, MaterialQualityTier::Low);
    const auto b = t.select_permutation(PbrExtension::Clearcoat,                          MaterialQualityTier::Low);
    CHECK(a.bits != b.bits);
}

TEST_CASE("phase17.mat.template: sheen + transmission gates independent") {
    MaterialTemplate t{make_desc()};
    const auto a = t.select_permutation(PbrExtension::Sheen,        MaterialQualityTier::Medium);
    const auto b = t.select_permutation(PbrExtension::Transmission, MaterialQualityTier::Medium);
    CHECK(a.bits != b.bits);
}

TEST_CASE("phase17.mat.template: quality tier advances permutation by 16") {
    MaterialTemplate t{make_desc()};
    const auto low  = t.select_permutation(PbrExtension::None, MaterialQualityTier::Low);
    const auto med  = t.select_permutation(PbrExtension::None, MaterialQualityTier::Medium);
    const auto high = t.select_permutation(PbrExtension::None, MaterialQualityTier::High);
    CHECK(low.bits  == 0u);
    CHECK(med.bits  == 16u);
    CHECK(high.bits == 32u);
}

TEST_CASE("phase17.mat.template: register_param lays out contiguous bytes") {
    MaterialTemplate t{make_desc()};
    const auto* a = t.find_param("base_color");
    const auto* b = t.find_param("metallic_roughness");
    const auto* c = t.find_param("emissive");
    REQUIRE(a);
    REQUIRE(b);
    REQUIRE(c);
    CHECK(a->offset == 0);
    CHECK(b->offset == 16);
    CHECK(c->offset == 32);
    CHECK(a->size == 16);
}

TEST_CASE("phase17.mat.template: register_param refuses duplicate key") {
    MaterialTemplate t{make_desc()};
    CHECK_FALSE(t.register_param("base_color", ParameterValue::Kind::Vec4));
}

TEST_CASE("phase17.mat.template: register_param honors 256 B cap") {
    MaterialTemplate t{make_desc()};
    for (int i = 0; i < 20; ++i) {
        (void)t.register_param("extra_" + std::to_string(i), ParameterValue::Kind::Vec4);
    }
    CHECK(t.next_offset() <= 256);
}

TEST_CASE("phase17.mat.template: write/read roundtrip for Vec4 param") {
    MaterialTemplate t{make_desc()};
    ParameterBlock pb{};
    ParameterValue v{};
    v.kind = ParameterValue::Kind::Vec4;
    v.v = {0.5f, 0.25f, 1.0f, 0.125f};
    CHECK(t.write_param(pb, "base_color", v));
    ParameterValue out{};
    CHECK(t.read_param(pb, "base_color", out));
    CHECK(out.v[0] == doctest::Approx(0.5f));
    CHECK(out.v[3] == doctest::Approx(0.125f));
}

TEST_CASE("phase17.mat.template: populate_default copies default block") {
    MaterialTemplate t{make_desc()};
    ParameterValue v{};
    v.kind = ParameterValue::Kind::Vec4;
    v.v = {1, 1, 1, 1};
    t.set_default_param("base_color", v);
    ParameterBlock dst{};
    t.populate_default(dst);
    ParameterValue out{};
    CHECK(t.read_param(dst, "base_color", out));
    CHECK(out.v[0] == doctest::Approx(1.0f));
}

TEST_CASE("phase17.mat.template: kind mismatch write rejected") {
    MaterialTemplate t{make_desc()};
    ParameterBlock pb{};
    ParameterValue v{};
    v.kind = ParameterValue::Kind::F32;
    v.f    = 1.0f;
    CHECK_FALSE(t.write_param(pb, "base_color", v));  // base_color is Vec4
}

TEST_CASE("phase17.mat.template: quality parse + to_string roundtrip") {
    CHECK(parse_quality("low")    == MaterialQualityTier::Low);
    CHECK(parse_quality("med")    == MaterialQualityTier::Medium);
    CHECK(parse_quality("medium") == MaterialQualityTier::Medium);
    CHECK(parse_quality("high")   == MaterialQualityTier::High);
    CHECK(std::string{to_string(MaterialQualityTier::Low)}    == "low");
    CHECK(std::string{to_string(MaterialQualityTier::High)}   == "high");
}
