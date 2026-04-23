// tests/unit/render/post/horror_post_test.cpp — Phase 21 W135 (ADR-0108).
//
// Scalar-oracle tests for the horror post stack. The HLSL compute shader
// mirrors this logic; golden-hash parity is asserted by
// `tests/render/horror_post_visual_test` on GPU-enabled CI runs.

#include "engine/render/post/horror_post.hpp"

#include <doctest/doctest.h>

#include <cstring>

using gw::render::post::HorrorPostConfig;
using gw::render::post::horror_post_sample;
using gw::render::post::horror_post_budget_ms_per_frame;
using gw::render::post::horror_post_cvar_names;

TEST_CASE("horror_post: deterministic — same frame/seed yields same value") {
    HorrorPostConfig cfg{};
    cfg.chromatic_aberration = 0.50f;
    cfg.film_grain           = 0.30f;
    cfg.screen_shake         = 0.20f;
    cfg.frame_index          = 42u;
    cfg.seed                 = 0xFEEDFACEULL;

    const auto a = horror_post_sample(cfg, {0.50f, 0.50f}, {0.3f, 0.4f, 0.5f});
    const auto b = horror_post_sample(cfg, {0.50f, 0.50f}, {0.3f, 0.4f, 0.5f});
    CHECK(a[0] == doctest::Approx(b[0]));
    CHECK(a[1] == doctest::Approx(b[1]));
    CHECK(a[2] == doctest::Approx(b[2]));
}

TEST_CASE("horror_post: zero CVars leave colour effectively untouched") {
    HorrorPostConfig cfg{};
    cfg.chromatic_aberration = 0.f;
    cfg.film_grain           = 0.f;
    cfg.screen_shake         = 0.f;

    const auto p = horror_post_sample(cfg, {0.25f, 0.75f}, {0.1f, 0.2f, 0.9f});
    CHECK(p[0] == doctest::Approx(0.1f).epsilon(1e-4));
    CHECK(p[1] == doctest::Approx(0.2f).epsilon(1e-4));
    CHECK(p[2] == doctest::Approx(0.9f).epsilon(1e-4));
}

TEST_CASE("horror_post: scalar oracle budget is finite + bounded") {
    HorrorPostConfig cfg{};
    cfg.chromatic_aberration = 1.0f;
    cfg.film_grain           = 1.0f;
    cfg.screen_shake         = 1.0f;
    // The HLSL twin must hit the Phase 21 GPU budget (≤ 0.25 ms @ 1080p on
    // RX 580 reference HW) — that's asserted by the hell-frame perf gate in
    // tests/perf/. Here we only guard the scalar oracle against runaway
    // regressions; the CPU SIMT projection is O(10 ms) in Debug, O(1 ms) in
    // Release — give both sides headroom.
    const double ms = horror_post_budget_ms_per_frame(1920u, 1080u, cfg);
    CHECK(ms > 0.0);
    CHECK(ms < 50.0);
}

TEST_CASE("horror_post: cvar names are stable and non-empty") {
    const auto names = horror_post_cvar_names();
    CHECK(names.chromatic_aberration != nullptr);
    CHECK(names.film_grain != nullptr);
    CHECK(names.screen_shake != nullptr);
    CHECK(std::strcmp(names.chromatic_aberration, "r.horror.chromatic_aberration") == 0);
    CHECK(std::strcmp(names.film_grain, "r.horror.film_grain") == 0);
    CHECK(std::strcmp(names.screen_shake, "r.horror.screen_shake") == 0);
}
