// tests/unit/render/material/phase17_gwmat_test.cpp — ADR-0076 .gwmat v1.

#include <doctest/doctest.h>

#include "engine/render/material/gwmat.hpp"
#include "engine/render/material/gwmat_format.hpp"
#include "engine/render/material/material_template.hpp"
#include "engine/render/material/material_types.hpp"

#include <cstring>
#include <string>
#include <vector>

using namespace gw::render::material;

static MaterialTemplateDesc make_desc() {
    MaterialTemplateDesc d;
    d.name = "pbr_opaque/metal_rough";
    d.parameter_keys = {"base_color", "metallic_roughness", "emissive"};
    return d;
}

TEST_CASE("phase17.gwmat: encode emits 32 B header + footer") {
    MaterialTemplate tmpl{make_desc()};
    MaterialInstanceState s{};
    const auto blob = encode_gwmat(s, tmpl, tmpl.desc().name);
    CHECK(blob.bytes.size() >= kGwMatHeaderSize);
    GwMatHeader h{};
    CHECK(peek_gwmat(std::span<const std::byte>{blob.bytes}, h) == GwMatError::Ok);
    CHECK(h.magic == kGwMatMagic);
    CHECK(h.major == kGwMatMajor);
}

TEST_CASE("phase17.gwmat: deterministic — identical inputs yield identical bytes") {
    MaterialTemplate tmpl{make_desc()};
    MaterialInstanceState s{};
    const auto a = encode_gwmat(s, tmpl, tmpl.desc().name);
    const auto b = encode_gwmat(s, tmpl, tmpl.desc().name);
    CHECK(a.bytes.size() == b.bytes.size());
    CHECK(a.content_hash == b.content_hash);
    CHECK(std::memcmp(a.bytes.data(), b.bytes.data(), a.bytes.size()) == 0);
}

TEST_CASE("phase17.gwmat: roundtrip decode restores parameter block") {
    MaterialTemplate tmpl{make_desc()};
    MaterialInstanceState s{};
    ParameterValue v{}; v.kind = ParameterValue::Kind::Vec4; v.v = {0.9f, 0.1f, 0.1f, 1.0f};
    (void)tmpl.write_param(s.block, "base_color", v);
    const auto blob = encode_gwmat(s, tmpl, tmpl.desc().name);

    MaterialInstanceState out{};
    std::string name;
    CHECK(decode_gwmat(std::span<const std::byte>{blob.bytes}, out, name) == GwMatError::Ok);
    CHECK(name == tmpl.desc().name);
    ParameterValue rv{};
    CHECK(tmpl.read_param(out.block, "base_color", rv));
    CHECK(rv.v[0] == doctest::Approx(0.9f));
}

TEST_CASE("phase17.gwmat: hash mismatch detected on tamper") {
    MaterialTemplate tmpl{make_desc()};
    MaterialInstanceState s{};
    auto blob = encode_gwmat(s, tmpl, tmpl.desc().name);
    // Flip a byte in the payload region.
    blob.bytes[kGwMatHeaderSize + 1] = std::byte{0x7F};
    MaterialInstanceState out{};
    std::string name;
    CHECK(decode_gwmat(std::span<const std::byte>{blob.bytes}, out, name) == GwMatError::HashMismatch);
}

TEST_CASE("phase17.gwmat: magic mismatch rejected") {
    std::vector<std::byte> junk(kGwMatHeaderSize, std::byte{0xAB});
    MaterialInstanceState out{};
    std::string name;
    CHECK(decode_gwmat(junk, out, name) == GwMatError::MagicMismatch);
}

TEST_CASE("phase17.gwmat: truncated blob rejected") {
    std::vector<std::byte> tiny(8, std::byte{0});
    MaterialInstanceState out{};
    std::string name;
    CHECK(decode_gwmat(tiny, out, name) == GwMatError::Truncated);
}

TEST_CASE("phase17.gwmat: content-hash differs when payload differs") {
    MaterialTemplate tmpl{make_desc()};
    MaterialInstanceState a{};
    MaterialInstanceState b{};
    ParameterValue v{}; v.kind = ParameterValue::Kind::Vec4; v.v = {0.2f, 0.2f, 0.2f, 1.0f};
    (void)tmpl.write_param(b.block, "base_color", v);
    const auto ba = encode_gwmat(a, tmpl, tmpl.desc().name);
    const auto bb = encode_gwmat(b, tmpl, tmpl.desc().name);
    CHECK(ba.content_hash != bb.content_hash);
}

TEST_CASE("phase17.gwmat: gwmat_content_hash stable across calls") {
    const std::vector<std::byte> hdr(16, std::byte{0xAA});
    const std::vector<std::byte> pay(128, std::byte{0x55});
    CHECK(gwmat_content_hash(hdr, pay) == gwmat_content_hash(hdr, pay));
    const std::vector<std::byte> pay2(128, std::byte{0x56});
    CHECK(gwmat_content_hash(hdr, pay) != gwmat_content_hash(hdr, pay2));
}

TEST_CASE("phase17.gwmat: peek returns slot count + param block size") {
    MaterialTemplate tmpl{make_desc()};
    MaterialInstanceState s{};
    const auto blob = encode_gwmat(s, tmpl, tmpl.desc().name);
    GwMatHeader h{};
    CHECK(peek_gwmat(std::span<const std::byte>{blob.bytes}, h) == GwMatError::Ok);
    CHECK(h.param_block_size == 256u);
    CHECK(h.slot_count == 16u);
    CHECK(h.template_name_len == tmpl.desc().name.size());
}

TEST_CASE("phase17.gwmat: to_cstring maps every error") {
    CHECK(std::string{to_cstring(GwMatError::Ok)}                  == "ok");
    CHECK(std::string{to_cstring(GwMatError::MagicMismatch)}       == "magic mismatch");
    CHECK(std::string{to_cstring(GwMatError::HashMismatch)}        == "hash mismatch");
    CHECK(std::string{to_cstring(GwMatError::Truncated)}           == "truncated");
    CHECK(std::string{to_cstring(GwMatError::ParamLayoutMismatch)} == "param layout mismatch");
}
