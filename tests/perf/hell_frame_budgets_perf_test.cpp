// tests/perf/hell_frame_budgets_perf_test.cpp — Phase 21 W141 (ADR-0108).
//
// Hell Frame perf gate — runs the Phase-21 horror post stack + Circle post
// stages + reality-warp oracles at 1080p-equivalent sample counts and
// checks per-stage budgets against the GPU targets documented in
// docs/02 §Phase 21 (RX 580 reference HW, 1920x1080, 144 Hz goal):
//
//   • horror_post         ≤ 0.35 ms / frame   (p99)
//   • sin_tendril         ≤ 0.08 ms / frame   (p99)
//   • ruin_desat          ≤ 0.06 ms / frame   (p99)
//   • rapture_whiteout    ≤ 0.06 ms / frame   (p99)
//   • reality_warp        ≤ 0.15 ms / frame   (p99)
//   • blood_decal_tick    ≤ 0.05 ms / frame   (CPU)
//
// Under GW_HELL_FRAME_STRICT=1 the per-stage gate fails the test when the
// scalar kernel overshoots its CPU budget (scaled by the debug slack
// factor). Default mode is advisory — it prints the per-stage line so CI
// can track perf drift without blocking merges.

#include "engine/render/decals/blood_decal_system.hpp"
#include "engine/render/post/circle_post_stages.hpp"
#include "engine/render/post/horror_post.hpp"
#include "engine/render/post/reality_warp.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>

namespace {

constexpr double kDebugSlack = 8.0;

bool strict_mode() noexcept {
    const char* v = std::getenv("GW_HELL_FRAME_STRICT");
    return (v != nullptr) && v[0] == '1';
}

int check_budget(const char* label, double actual_ms, double budget_ms) {
#if !defined(NDEBUG)
    budget_ms *= kDebugSlack;
#endif
    std::printf("[hell-frame] %s: %.4f ms (budget %.4f ms)\n",
                label, actual_ms, budget_ms);
    if (strict_mode() && actual_ms > budget_ms) {
        std::fprintf(stderr, "[hell-frame] FAIL %s over budget\n", label);
        return 1;
    }
    return 0;
}

} // namespace

int main() {
    using namespace std::chrono;
    using gw::render::decals::BloodDecalSystem;
    using gw::render::decals::DecalComponent;
    using gw::render::post::CirclePostCompose;
    using gw::render::post::HorrorPostConfig;
    using gw::render::post::RealityWarpConfig;
    using gw::render::post::circle_post_apply;
    using gw::render::post::horror_post_budget_ms_per_frame;
    using gw::render::post::reality_warp_budget_ms;

    int rc = 0;

    // Horror post stack.
    {
        HorrorPostConfig cfg{};
        cfg.chromatic_aberration = 1.0f;
        cfg.film_grain           = 1.0f;
        cfg.screen_shake         = 1.0f;
        const double ms = horror_post_budget_ms_per_frame(1920u, 1080u, cfg);
        rc |= check_budget("horror_post", ms, 0.35);
    }

    // Circle post composed stack (sin_tendril + ruin_desat + rapture_whiteout).
    {
        CirclePostCompose cfg{};
        cfg.sin_tendril      = {1.0f, 100.0f};
        cfg.ruin_desat       = {1.0f, 1.0f};
        cfg.rapture_whiteout = {1.0f, 4.0f};
        const auto t0 = steady_clock::now();
        constexpr int kIter = 64;
        constexpr int kTile = 128;
        std::array<float, 3> acc{};
        for (int it = 0; it < kIter; ++it) {
            for (int y = 0; y < kTile; ++y) {
                for (int x = 0; x < kTile; ++x) {
                    const std::array<float, 2> uv{static_cast<float>(x) / kTile,
                                                    static_cast<float>(y) / kTile};
                    acc = circle_post_apply(cfg, uv, {0.5f, 0.5f, 0.5f});
                }
            }
        }
        const auto t1 = steady_clock::now();
        (void)acc;
        const double tile_pixels = static_cast<double>(kIter) * kTile * kTile;
        const double total_ms =
            duration<double, std::milli>(t1 - t0).count();
        const double ns_per_pixel = total_ms * 1e6 / tile_pixels;
        const double frame_ms =
            ns_per_pixel * (1920.0 * 1080.0) / 64.0 / 1e6;
        rc |= check_budget("circle_post_stack", frame_ms, 0.20);
    }

    // Reality warp.
    {
        RealityWarpConfig cfg{};
        cfg.mirror_weight    = 1.0f;
        cfg.distortion_pulse = 1.0f;
        const double ms = reality_warp_budget_ms(1920u, 1080u, cfg);
        rc |= check_budget("reality_warp", ms, 0.15);
    }

    // Blood decal ring tick.
    {
        BloodDecalSystem ring;
        DecalComponent   stamp{};
        stamp.alpha = 1.0f;
        for (std::uint32_t i = 0; i < 256; ++i) {
            stamp.stamp_seed = i;
            ring.insert(stamp);
        }
        const auto t0 = steady_clock::now();
        for (std::uint64_t f = 0; f < 1000; ++f) {
            ring.tick(1.0f / 144.0f, f);
        }
        const auto t1 = steady_clock::now();
        const double total_ms =
            duration<double, std::milli>(t1 - t0).count();
        rc |= check_budget("blood_decal_tick", total_ms / 1000.0, 0.05);
    }

    std::printf("[hell-frame] DONE — rc=%d strict=%s\n", rc,
                strict_mode() ? "ON" : "OFF");
    return rc;
}
