// tests/unit/render/post/reality_warp_test.cpp — Phase 22 W147 (ADR-0108).

#include "engine/render/post/reality_warp.hpp"

#include <doctest/doctest.h>

#include <cstring>

using gw::render::post::RealityWarpConfig;
using gw::render::post::reality_warp_uv;
using gw::render::post::reality_warp_budget_ms;
using gw::render::post::reality_warp_cvar_names;

TEST_CASE("reality_warp: identity when all weights are zero") {
    RealityWarpConfig cfg{};
    const auto uv = reality_warp_uv(cfg, {0.3f, 0.7f});
    CHECK(uv[0] == doctest::Approx(0.3f));
    CHECK(uv[1] == doctest::Approx(0.7f));
}

TEST_CASE("reality_warp: mirror step flips x, invert flips y") {
    RealityWarpConfig cfg{};
    cfg.mirror_weight = 1.0f;
    cfg.invert_weight = 0.0f;
    const auto m = reality_warp_uv(cfg, {0.3f, 0.7f});
    CHECK(m[0] == doctest::Approx(0.7f).epsilon(1e-3));

    cfg.mirror_weight = 0.0f;
    cfg.invert_weight = 1.0f;
    const auto i = reality_warp_uv(cfg, {0.3f, 0.7f});
    CHECK(i[0] == doctest::Approx(0.3f));
    CHECK(i[1] < 0.5f);
}

TEST_CASE("reality_warp: scalar oracle budget is finite + bounded") {
    RealityWarpConfig cfg{};
    cfg.mirror_weight    = 1.0f;
    cfg.distortion_pulse = 1.0f;
    // The GPU kernel ships under the Phase 22 perf gate (≤ 0.15 ms on RX 580).
    // The scalar oracle runs ~20× slower in Debug; here we only guard the
    // kernel against runaway regressions.
    const double ms = reality_warp_budget_ms(1920u, 1080u, cfg);
    CHECK(ms > 0.0);
    CHECK(ms < 50.0);
}

TEST_CASE("reality_warp: CVars named and stable") {
    const auto names = reality_warp_cvar_names();
    CHECK(names.mirror_weight != nullptr);
    CHECK(names.invert_weight != nullptr);
    CHECK(names.distortion_pulse != nullptr);
    CHECK(std::strstr(names.mirror_weight, "mirror") != nullptr);
}
