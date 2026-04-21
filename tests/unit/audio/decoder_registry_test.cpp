// tests/unit/audio/decoder_registry_test.cpp — ADR-0019 decoder plug-ins.

#include <doctest/doctest.h>

#include "engine/audio/decoder_registry.hpp"
#include "engine/audio/kaudio_format.hpp"

#include <cmath>
#include <vector>

using namespace gw::audio;

namespace {

std::vector<uint8_t> make_sine_raw(int frames = 512) {
    std::vector<float> pcm(frames * 2);
    for (int i = 0; i < frames; ++i) {
        const float t = static_cast<float>(i) / frames;
        pcm[2 * i + 0] = std::sin(6.2831853f * 220.0f * t);
        pcm[2 * i + 1] = std::sin(6.2831853f * 440.0f * t);
    }
    return kaudio::build_raw_f32(48'000, 2, pcm);
}

} // namespace

TEST_CASE("DecoderRegistry: raw-f32 is registered by default") {
    DecoderRegistry r;
    CHECK(r.size() >= 1);
    CHECK(r.find(kaudio::Codec::RawF32) != nullptr);
}

TEST_CASE("DecoderRegistry: decode_file round-trips raw_f32") {
    DecoderRegistry r;
    auto file = make_sine_raw(64);
    kaudio::Header h;
    std::vector<float> pcm;
    REQUIRE(r.decode_file(file, h, pcm));
    CHECK(pcm.size() == 64 * 2);
}

TEST_CASE("raw_f32 stream: pull advances and loops via rewind") {
    auto file = make_sine_raw(32);
    auto stream = make_raw_f32_stream(file);
    REQUIRE(stream);
    CHECK(stream->sample_rate() == 48'000);
    CHECK(stream->channels() == 2);
    CHECK(stream->total_frames() == 32);
    std::vector<float> buf(16 * 2, 0.0f);
    auto n = stream->pull(buf, 16);
    CHECK(n == 16);
    n = stream->pull(buf, 32);
    CHECK(n <= 16);  // only 16 frames left
    stream->rewind();
    n = stream->pull(buf, 16);
    CHECK(n == 16);
}

TEST_CASE("raw_f32 stream: seek to mid-file") {
    auto file = make_sine_raw(64);
    auto stream = make_raw_f32_stream(file);
    REQUIRE(stream);
    REQUIRE(stream->seek(32));
    std::vector<float> buf(16 * 2, 0.0f);
    auto n = stream->pull(buf, 16);
    CHECK(n == 16);
}
