// tests/unit/net/snapshot_test.cpp — Phase 14 Wave 14C.

#include <doctest/doctest.h>

#include "engine/net/snapshot.hpp"

#include <cmath>

using namespace gw::net;

TEST_CASE("BitWriter / BitReader round-trip packed integers") {
    BitWriter w;
    w.write_bits(0xABu, 8);
    w.write_bits(0x1234u, 16);
    w.write_u64(0x0123456789ABCDEFULL);
    w.write_f32(3.14159f);

    BitReader r{w.view()};
    CHECK(r.read_bits(8) == 0xABu);
    CHECK(r.read_bits(16) == 0x1234u);
    CHECK(r.read_u64() == 0x0123456789ABCDEFULL);
    CHECK(r.read_f32() == doctest::Approx(3.14159f));
}

TEST_CASE("signed_quant round-trip preserves sign and precision within epsilon") {
    BitWriter w;
    w.write_signed_quant(123.456, 1024.0, 24);
    w.write_signed_quant(-987.654, 1024.0, 24);
    w.write_signed_quant(0.0, 1024.0, 24);
    BitReader r{w.view()};
    CHECK(r.read_signed_quant(1024.0, 24) == doctest::Approx(123.456).epsilon(1e-3));
    CHECK(r.read_signed_quant(1024.0, 24) == doctest::Approx(-987.654).epsilon(1e-3));
    CHECK(r.read_signed_quant(1024.0, 24) == doctest::Approx(0.0).epsilon(1e-3));
}

TEST_CASE("AnchorPosCodec round-trips world position through quantization") {
    AnchorPosCodec codec{};
    codec.anchor = glm::dvec3{100.0, 0.0, 200.0};
    codec.range_m = 512.0;
    codec.bits_per_axis = 24;
    const glm::dvec3 world{110.5, 3.0, 199.25};

    BitWriter w;
    codec.write(w, world);
    BitReader r{w.view()};
    const glm::dvec3 out = codec.read(r);
    CHECK(out.x == doctest::Approx(world.x).epsilon(1e-3));
    CHECK(out.y == doctest::Approx(world.y).epsilon(1e-3));
    CHECK(out.z == doctest::Approx(world.z).epsilon(1e-3));
}

TEST_CASE("QuatSmallest3 round-trip preserves orientation within tolerance") {
    QuatSmallest3Codec codec{};
    codec.bits_per_axis = 10;
    const glm::quat q = glm::normalize(glm::quat{0.5f, 0.3f, -0.2f, 0.7f});

    BitWriter w;
    codec.write(w, q);
    BitReader r{w.view()};
    const glm::quat out = codec.read(r);

    const float dot = std::fabs(q.x*out.x + q.y*out.y + q.z*out.z + q.w*out.w);
    CHECK(dot == doctest::Approx(1.0f).epsilon(0.02f));
}

TEST_CASE("serialize_snapshot / deserialize_snapshot round-trip") {
    Snapshot s{};
    s.tick = 42;
    s.baseline_id = 7;
    s.determinism_hash = 0xDEADBEEFCAFEBABEULL;
    EntityDelta d{};
    d.id = EntityReplicationId{0x123456u};
    d.field_bitmap = 0x5;
    d.payload = {1, 2, 3, 4, 5};
    s.deltas.push_back(d);

    const auto bytes = serialize_snapshot(s);
    Snapshot out;
    REQUIRE(deserialize_snapshot(std::span<const std::uint8_t>(bytes), out));
    CHECK(out.tick == 42);
    CHECK(out.baseline_id == 7);
    CHECK(out.determinism_hash == 0xDEADBEEFCAFEBABEULL);
    REQUIRE(out.deltas.size() == 1);
    CHECK(out.deltas[0].id.value == 0x123456u);
    CHECK(out.deltas[0].field_bitmap == 0x5);
    CHECK(out.deltas[0].payload == std::vector<std::uint8_t>{1,2,3,4,5});
}

TEST_CASE("deserialize_snapshot rejects corrupt input") {
    std::vector<std::uint8_t> bad = {0x00, 0x01, 0x02};
    Snapshot out;
    CHECK_FALSE(deserialize_snapshot(std::span<const std::uint8_t>(bad), out));
}
