// tests/unit/audio/kaudio_format_test.cpp — ADR-0019 .kaudio format round-trip.

#include <doctest/doctest.h>

#include "engine/audio/kaudio_format.hpp"

#include <vector>

using namespace gw::audio::kaudio;

TEST_CASE("Header: round-trip") {
    Header h;
    h.codec = Codec::RawF32;
    h.channels = 2;
    h.sample_rate = 44100;
    h.num_frames = 1024;
    h.flags = Flag_Loop | Flag_Seekable;
    h.loop_start = 64;
    h.loop_end = 1024;
    h.body_offset = kHeaderBytes;
    h.body_bytes = 1024u * 2u * sizeof(float);
    h.content_hash.fill(0xAB);

    std::array<uint8_t, kHeaderBytes> buf{};
    REQUIRE(write_header(h, buf));

    Header parsed;
    Error err;
    REQUIRE(read_header(buf, parsed, err));
    CHECK(parsed.sample_rate == 44100);
    CHECK(parsed.channels == 2);
    CHECK(parsed.num_frames == 1024);
    CHECK(parsed.codec == Codec::RawF32);
    CHECK(parsed.flags == (Flag_Loop | Flag_Seekable));
    CHECK(parsed.has_flag(Flag_Loop));
    CHECK(parsed.loop_start == 64);
    CHECK(parsed.content_hash[0] == 0xAB);
}

TEST_CASE("read_header: rejects short input") {
    std::vector<uint8_t> short_buf(10, 0);
    Header h;
    Error err;
    CHECK_FALSE(read_header(short_buf, h, err));
    CHECK(err.kind == ParseError::TooShort);
}

TEST_CASE("read_header: rejects bad magic") {
    std::array<uint8_t, kHeaderBytes> buf{};
    buf[0] = 'X';
    Header h;
    Error err;
    CHECK_FALSE(read_header(buf, h, err));
    CHECK(err.kind == ParseError::BadMagic);
}

TEST_CASE("build_raw_f32 / decode_raw_f32: round-trip pcm data") {
    std::vector<float> pcm = {0.1f, -0.2f, 0.3f, -0.4f, 0.5f, -0.6f};
    auto image = build_raw_f32(48'000, /*channels*/2, pcm);
    REQUIRE(image.size() >= kHeaderBytes);

    Header h;
    Error err;
    REQUIRE(read_header(std::span(image.data(), kHeaderBytes), h, err));
    CHECK(h.sample_rate == 48'000);
    CHECK(h.channels == 2);
    CHECK(h.codec == Codec::RawF32);

    std::vector<float> decoded;
    std::span<const uint8_t> body(image.data() + h.body_offset, h.body_bytes);
    REQUIRE(decode_raw_f32(h, body, decoded));
    REQUIRE(decoded.size() == pcm.size());
    for (std::size_t i = 0; i < pcm.size(); ++i) {
        CHECK(decoded[i] == doctest::Approx(pcm[i]));
    }
}

TEST_CASE("Seek table: round-trip") {
    std::vector<SeekEntry> in = {
        {0, 64}, {1024, 8256}, {2048, 16448},
    };
    std::vector<uint8_t> buf;
    REQUIRE(write_seek_table(in, buf));
    std::vector<SeekEntry> out;
    REQUIRE(read_seek_table(buf, out));
    REQUIRE(out.size() == in.size());
    for (std::size_t i = 0; i < in.size(); ++i) {
        CHECK(out[i].frame_index == in[i].frame_index);
        CHECK(out[i].file_offset == in[i].file_offset);
    }
}
