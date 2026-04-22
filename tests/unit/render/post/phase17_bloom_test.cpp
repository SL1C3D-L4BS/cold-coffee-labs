// tests/unit/render/post/phase17_bloom_test.cpp — ADR-0079 Wave 17F.

#include <doctest/doctest.h>

#include "engine/render/post/bloom.hpp"

#include <cstring>
#include <vector>

using namespace gw::render::post;

static std::vector<float> make_hdr(std::uint32_t w, std::uint32_t h, float v) {
    std::vector<float> out(static_cast<std::size_t>(w) * h * 4, 0.0f);
    for (std::size_t i = 0; i < out.size(); i += 4) {
        out[i + 0] = v;
        out[i + 1] = v;
        out[i + 2] = v;
        out[i + 3] = 1.0f;
    }
    return out;
}

TEST_CASE("phase17.bloom: default config") {
    Bloom b;
    CHECK(b.config().enabled == true);
    CHECK(b.config().iterations == 5);
    CHECK(b.config().intensity == doctest::Approx(1.0f));
}

TEST_CASE("phase17.bloom: configure round-trip") {
    Bloom b;
    BloomConfig cfg;
    cfg.enabled    = false;
    cfg.iterations = 3;
    cfg.threshold  = 2.0f;
    cfg.intensity  = 0.5f;
    b.configure(cfg);
    CHECK(b.config().iterations == 3);
    CHECK(b.config().threshold  == doctest::Approx(2.0f));
}

TEST_CASE("phase17.bloom: deterministic output for fixed input") {
    Bloom b;
    b.configure({true, 3, 1.0f, 1.0f});
    const auto in = make_hdr(16, 16, 2.0f);
    std::vector<float> a, b2;
    b.run(in, 16, 16, a);
    b.run(in, 16, 16, b2);
    REQUIRE(a.size() == b2.size());
    for (std::size_t i = 0; i < a.size(); ++i) CHECK(a[i] == doctest::Approx(b2[i]));
}

TEST_CASE("phase17.bloom: zero iterations is a pass-through (no bloom)") {
    Bloom b;
    b.configure({true, 0, 1.0f, 1.0f});
    const auto in = make_hdr(8, 8, 3.0f);
    std::vector<float> out;
    b.run(in, 8, 8, out);
    CHECK(out.size() == in.size());
}

TEST_CASE("phase17.bloom: threshold above input leaves base HDR unchanged") {
    Bloom b;
    b.configure({true, 4, 10.0f, 1.0f});   // threshold above 1.0 input
    const auto in = make_hdr(8, 8, 1.0f);
    std::vector<float> out;
    b.run(in, 8, 8, out);
    REQUIRE(out.size() == in.size());
    for (std::size_t i = 0; i < out.size(); ++i) CHECK(out[i] == doctest::Approx(in[i]));
}

TEST_CASE("phase17.bloom: disabled returns input untouched") {
    Bloom b;
    b.configure({false, 5, 1.0f, 1.0f});
    const auto in = make_hdr(4, 4, 2.5f);
    std::vector<float> out;
    b.run(in, 4, 4, out);
    REQUIRE(out.size() == in.size());
    for (std::size_t i = 0; i < out.size(); ++i) CHECK(out[i] == doctest::Approx(in[i]));
}
