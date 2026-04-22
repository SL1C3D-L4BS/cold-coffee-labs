// tests/unit/render/post/phase17_tonemap_test.cpp — ADR-0080 Wave 17F.

#include <doctest/doctest.h>

#include "engine/render/post/tonemap.hpp"

#include <array>
#include <string>

using namespace gw::render::post;

TEST_CASE("phase17.tonemap: parse + to_cstring roundtrip") {
    CHECK(parse_tonemap_curve("linear")      == TonemapCurve::Linear);
    CHECK(parse_tonemap_curve("aces")        == TonemapCurve::ACES);
    CHECK(parse_tonemap_curve("pbr_neutral") == TonemapCurve::PbrNeutral);
    CHECK(parse_tonemap_curve("unknown")     == TonemapCurve::PbrNeutral);
    CHECK(std::string{to_cstring(TonemapCurve::Linear)}     == "linear");
    CHECK(std::string{to_cstring(TonemapCurve::ACES)}       == "aces");
    CHECK(std::string{to_cstring(TonemapCurve::PbrNeutral)} == "pbr_neutral");
}

TEST_CASE("phase17.tonemap.pbr_neutral: origin is fixed") {
    const auto out = tonemap_pbr_neutral({0, 0, 0});
    CHECK(out[0] == doctest::Approx(0.0f).epsilon(1e-4));
    CHECK(out[1] == doctest::Approx(0.0f).epsilon(1e-4));
    CHECK(out[2] == doctest::Approx(0.0f).epsilon(1e-4));
}

TEST_CASE("phase17.tonemap.pbr_neutral: monotonic on midtones") {
    const auto a = tonemap_pbr_neutral({0.2f, 0.2f, 0.2f})[0];
    const auto b = tonemap_pbr_neutral({0.4f, 0.4f, 0.4f})[0];
    const auto c = tonemap_pbr_neutral({0.6f, 0.6f, 0.6f})[0];
    CHECK(a < b);
    CHECK(b < c);
}

TEST_CASE("phase17.tonemap.pbr_neutral: highlights compress below 1.0") {
    const auto out = tonemap_pbr_neutral({10.0f, 10.0f, 10.0f});
    CHECK(out[0] < 1.05f);
    CHECK(out[1] < 1.05f);
    CHECK(out[2] < 1.05f);
}

TEST_CASE("phase17.tonemap.pbr_neutral: local invertibility for low-x") {
    const std::array<float, 3> x{0.05f, 0.05f, 0.05f};
    const auto y = tonemap_pbr_neutral(x);
    const auto inv = tonemap_pbr_neutral_inverse(y);
    CHECK(inv[0] == doctest::Approx(x[0]).epsilon(0.01));
}

TEST_CASE("phase17.tonemap.aces: monotonic + clamped to [0,1]") {
    const auto a = tonemap_aces({0.1f, 0.1f, 0.1f})[0];
    const auto b = tonemap_aces({1.0f, 1.0f, 1.0f})[0];
    const auto c = tonemap_aces({8.0f, 8.0f, 8.0f})[0];
    CHECK(a < b);
    CHECK(b < c);
    CHECK(c <= 1.0f);
    CHECK(a >= 0.0f);
}

TEST_CASE("phase17.tonemap: apply_tonemap dispatches by curve") {
    const std::array<float, 3> rgb{0.5f, 0.5f, 0.5f};
    const auto lin = apply_tonemap({TonemapCurve::Linear, 1.0f}, rgb);
    const auto pbr = apply_tonemap({TonemapCurve::PbrNeutral, 1.0f}, rgb);
    const auto aces = apply_tonemap({TonemapCurve::ACES, 1.0f}, rgb);
    // Not strictly equal; each curve is its own shape.
    CHECK(lin[0] == doctest::Approx(0.5f));
    CHECK(pbr[0] >  0.0f);
    CHECK(aces[0] > 0.0f);
}

TEST_CASE("phase17.tonemap: exposure scales input linearly pre-curve") {
    const auto a = apply_tonemap({TonemapCurve::Linear, 0.5f}, {1.0f, 1.0f, 1.0f});
    const auto b = apply_tonemap({TonemapCurve::Linear, 2.0f}, {0.25f, 0.25f, 0.25f});
    CHECK(a[0] == doctest::Approx(0.5f));
    CHECK(b[0] == doctest::Approx(0.5f));
}

TEST_CASE("phase17.tonemap.srgb: encode approximates gamma 2.2") {
    const auto out = srgb_encode({0.5f, 0.5f, 0.5f});
    CHECK(out[0] > 0.65f);
    CHECK(out[0] < 0.85f);
}
