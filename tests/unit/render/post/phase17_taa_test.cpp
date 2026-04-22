// tests/unit/render/post/phase17_taa_test.cpp — ADR-0079 Wave 17F.

#include <doctest/doctest.h>

#include "engine/render/post/taa.hpp"
#include "engine/render/post/post_types.hpp"

#include <array>

using namespace gw::render::post;

static std::array<std::array<float, 3>, 9> uniform_neighborhood(float v) {
    std::array<std::array<float, 3>, 9> nb{};
    for (auto& s : nb) s = {v, v, v};
    return nb;
}

TEST_CASE("phase17.taa.jitter: Halton sequence is deterministic") {
    const auto a = halton23_jitter(42, 1.0f);
    const auto b = halton23_jitter(42, 1.0f);
    CHECK(a.x == doctest::Approx(b.x));
    CHECK(a.y == doctest::Approx(b.y));
}

TEST_CASE("phase17.taa.jitter: centered around zero, magnitude ≤ 0.5") {
    for (std::uint32_t i = 0; i < 64; ++i) {
        const auto j = halton23_jitter(i, 1.0f);
        CHECK(j.x >= -0.5f);
        CHECK(j.x <=  0.5f);
        CHECK(j.y >= -0.5f);
        CHECK(j.y <=  0.5f);
    }
}

TEST_CASE("phase17.taa.jitter: scale multiplier") {
    const auto j1 = halton23_jitter(5, 1.0f);
    const auto j2 = halton23_jitter(5, 2.0f);
    CHECK(j2.x == doctest::Approx(j1.x * 2.0f));
    CHECK(j2.y == doctest::Approx(j1.y * 2.0f));
}

TEST_CASE("phase17.taa.clip: AABB clamps out-of-box history") {
    const auto nb = uniform_neighborhood(0.5f);
    const auto out = clip_color(TaaClipMode::Aabb, {2.0f, 2.0f, 2.0f}, nb);
    CHECK(out[0] == doctest::Approx(0.5f));
    CHECK(out[2] == doctest::Approx(0.5f));
}

TEST_CASE("phase17.taa.clip: Variance mode clamps history to sigma-band around mean") {
    const auto nb = uniform_neighborhood(0.5f);
    const auto out = clip_color(TaaClipMode::Variance, {2.0f, 2.0f, 2.0f}, nb);
    CHECK(out[0] == doctest::Approx(0.5f));
}

TEST_CASE("phase17.taa.clip: k-DOP pulls history toward center along dir") {
    auto nb = uniform_neighborhood(0.5f);
    nb[4] = {0.5f, 0.5f, 0.5f};
    const auto out = clip_color(TaaClipMode::KDop, {5.0f, 5.0f, 5.0f}, nb);
    CHECK(out[0] <= 5.0f);
    CHECK(out[0] >= 0.5f);
}

TEST_CASE("phase17.taa: resolve blends current + clipped history") {
    Taa t;
    TaaConfig cfg;
    cfg.enabled      = true;
    cfg.clip_mode    = TaaClipMode::Aabb;
    cfg.blend_factor = 0.5f;
    t.configure(cfg);
    const auto nb = uniform_neighborhood(0.3f);
    const auto out = t.resolve({0.7f, 0.7f, 0.7f}, {0.3f, 0.3f, 0.3f}, nb);
    CHECK(out[0] == doctest::Approx(0.5f));
}

TEST_CASE("phase17.taa: invalidate drops the history") {
    Taa t;
    TaaConfig cfg;
    cfg.enabled = true;
    t.configure(cfg);
    t.invalidate();
    CHECK(t.invalidated());
    const auto nb = uniform_neighborhood(0.0f);
    const auto out = t.resolve({0.9f, 0.9f, 0.9f}, {0.1f, 0.1f, 0.1f}, nb);
    CHECK(out[0] == doctest::Approx(0.9f));
    t.clear_invalidated();
    CHECK_FALSE(t.invalidated());
}
