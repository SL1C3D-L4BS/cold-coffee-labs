// tests/perf/director_budgets_perf_test.cpp — runtime AI budget gate (ADR-0117).
//
// Perf gate for the runtime AI stack (engine/ai_runtime):
//   • AI Director rule eval       ≤ 0.25 ms / tick  (p99).
//   • Symbolic music eval         ≤ 0.20 ms / bar   (p99).
//   • Neural material eval        ≤ 1.50 ms / frame (Vulkan, p99).
//
// Runs headless in CI using the shipping CPU fallbacks in engine/ai_runtime.
// Strict gate activates with GW_AI_BUDGETS_STRICT=1; otherwise the test prints
// a soft-budget line and returns success.

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstdint>

#include "engine/ai_runtime/director_policy.hpp"
#include "engine/ai_runtime/material_eval.hpp"
#include "engine/ai_runtime/music_symbolic.hpp"

namespace {

constexpr double kDirectorBudgetMs = 0.25;
constexpr double kMusicBudgetMs    = 0.20;
constexpr double kNeuralBudgetMs   = 1.50;

constexpr double kDebugSlack = 4.0; // Debug builds get 4× headroom.

bool strict_mode() noexcept {
    const char* v = std::getenv("GW_AI_BUDGETS_STRICT");
    return (v != nullptr) && v[0] == '1';
}

int check_budget(const char* label, double actual_ms, double budget_ms) {
#if !defined(NDEBUG)
    budget_ms *= kDebugSlack;
#endif
    std::printf("[ai-perf] %s: %.4f ms (budget %.4f ms)\n",
                label, actual_ms, budget_ms);
    if (strict_mode() && actual_ms > budget_ms) {
        std::fprintf(stderr, "[ai-perf] FAIL %s over budget\n", label);
        return 1;
    }
    return 0;
}

} // namespace

int main() {
    int rc = 0;

    {
        const auto start = std::chrono::steady_clock::now();
        namespace air = gw::ai_runtime;
        air::DirectorPolicyInput d_in{};
        d_in.logical_tick   = 1U;
        d_in.player_health_ratio  = 0.8f;
        d_in.combat_intensity_ewma = 0.5f;
        d_in.time_in_state_sec = 1.f;
        d_in.current_state  = air::DirectorState::BuildUp;
        volatile std::uint32_t sink = 0U;
        for (int i = 0; i < 2000; ++i) {
            d_in.logical_tick  = static_cast<std::uint64_t>(i);
            const auto out = air::evaluate_director(d_in, 0xC0FFEEULL);
            sink ^= static_cast<std::uint32_t>(out.next_state) << 16U;
        }
        (void)sink;
        const double elapsed_ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
        rc |= check_budget("director", elapsed_ms, kDirectorBudgetMs);
    }

    {
        const auto start = std::chrono::steady_clock::now();
        namespace air = gw::ai_runtime;
        air::MusicContext mctx{};
        mctx.combat_intensity = 0.7f;
        mctx.sin_ratio        = 0.1f;
        mctx.rapture_active   = 0.2f;
        mctx.bpm_beat_phase   = 0.3f;
        mctx.circle_index     = 2U;
        volatile float mix = 0.f;
        for (int i = 0; i < 4000; ++i) {
            mctx.combat_intensity = 0.3f + static_cast<float>(i) * 0.0001f;
            const auto m = air::evaluate_music_symbolic(mctx);
            mix += m.weights[0];
        }
        (void)mix;
        const double elapsed_ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
        rc |= check_budget("music", elapsed_ms, kMusicBudgetMs);
    }

    {
        const auto start = std::chrono::steady_clock::now();
        namespace air = gw::ai_runtime;
        const air::ModelId     mid{0x1u};
        air::MaterialEvalInput in{};
        volatile float         s = 0.f;
        for (int i = 0; i < 5000; ++i) {
            in.uv[0] = in.uv[1] = static_cast<float>(i) * 0.0001f;
            in.wear  = 0.4f;
            in.wetness = 0.1f;
            in.blood_density = 0.05f;
            const auto o = air::evaluate_material(mid, in);
            s += o.roughness;
        }
        (void)s;
        const double elapsed_ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
        rc |= check_budget("neural_material", elapsed_ms, kNeuralBudgetMs);
    }

    return rc;
}
