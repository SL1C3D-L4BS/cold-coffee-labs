// tests/unit/render/post/circle_post_stages_test.cpp — Phase 21 W136 (ADR-0108).

#include "engine/render/post/circle_post_stages.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstring>

using gw::render::post::CirclePostCompose;
using gw::render::post::RaptureWhiteoutConfig;
using gw::render::post::RuinDesatConfig;
using gw::render::post::SinTendrilConfig;
using gw::render::post::circle_post_apply;
using gw::render::post::circle_post_cvar_names;
using gw::render::post::rapture_whiteout_apply;
using gw::render::post::ruin_desat_apply;
using gw::render::post::sin_tendril_apply;

TEST_CASE("sin_tendril: zero intensity is identity, full vignettes corners") {
    const std::array<float, 3> rgb{0.5f, 0.6f, 0.7f};

    const auto id = sin_tendril_apply({0.0f, 100.0f}, {0.5f, 0.5f}, rgb);
    CHECK(id[0] == doctest::Approx(rgb[0]).epsilon(1e-4));
    CHECK(id[1] == doctest::Approx(rgb[1]).epsilon(1e-4));
    CHECK(id[2] == doctest::Approx(rgb[2]).epsilon(1e-4));

    const auto centre = sin_tendril_apply({1.0f, 100.0f}, {0.5f, 0.5f}, rgb);
    const auto corner = sin_tendril_apply({1.0f, 100.0f}, {0.0f, 0.0f}, rgb);
    // Corner should be darker than centre when sin+intensity are at max.
    const float lum_centre = centre[0] + centre[1] + centre[2];
    const float lum_corner = corner[0] + corner[1] + corner[2];
    CHECK(lum_corner < lum_centre);
}

TEST_CASE("ruin_desat: pulls colour toward grayscale as intensity rises") {
    const std::array<float, 3> rgb{1.0f, 0.0f, 0.0f};
    const auto half = ruin_desat_apply({1.0f, 0.5f}, rgb);
    // R channel should have dropped, G/B risen (toward mid-gray).
    CHECK(half[0] < rgb[0]);
    CHECK(half[1] >= rgb[1]);
    CHECK(half[2] >= rgb[2]);

    const auto full = ruin_desat_apply({1.0f, 1.0f}, rgb);
    const float spread =
        std::max({full[0], full[1], full[2]}) - std::min({full[0], full[1], full[2]});
    CHECK(spread <= 0.5f);
}

TEST_CASE("rapture_whiteout: monotonically climbs toward 1,1,1") {
    const std::array<float, 3> rgb{0.2f, 0.3f, 0.4f};
    const auto t0 = rapture_whiteout_apply({1.0f, 0.0f}, rgb);
    const auto t2 = rapture_whiteout_apply({1.0f, 2.0f}, rgb);
    const auto t6 = rapture_whiteout_apply({1.0f, 6.0f}, rgb);

    CHECK(t0[0] <= t2[0]);
    CHECK(t2[0] <= t6[0]);
    CHECK(t6[0] > 0.9f);
    CHECK(t6[1] > 0.9f);
    CHECK(t6[2] > 0.9f);
}

TEST_CASE("circle_post_apply: composed pipeline is deterministic") {
    CirclePostCompose a{};
    a.sin_tendril     = {0.5f, 40.0f};
    a.ruin_desat      = {0.3f, 0.25f};
    a.rapture_whiteout = {0.0f, 0.0f};

    const std::array<float, 3> rgb{0.5f, 0.5f, 0.5f};
    const auto r1 = circle_post_apply(a, {0.2f, 0.8f}, rgb);
    const auto r2 = circle_post_apply(a, {0.2f, 0.8f}, rgb);
    CHECK(r1[0] == doctest::Approx(r2[0]));
    CHECK(r1[1] == doctest::Approx(r2[1]));
    CHECK(r1[2] == doctest::Approx(r2[2]));
}

TEST_CASE("circle_post: cvar names are stable") {
    const auto names = circle_post_cvar_names();
    CHECK(std::strcmp(names.sin_tendril, "r.sin_tendril") == 0);
    CHECK(std::strcmp(names.ruin_desat, "r.ruin_desat") == 0);
    CHECK(std::strcmp(names.rapture_whiteout, "r.rapture_whiteout") == 0);
}
