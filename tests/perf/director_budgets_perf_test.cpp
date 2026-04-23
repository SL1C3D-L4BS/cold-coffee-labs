// tests/perf/director_budgets_perf_test.cpp — Part C §25 scaffold (ADR-0117).
//
// Perf gate for the runtime AI stack (engine/ai_runtime):
//   • AI Director rule eval       ≤ 0.25 ms / tick  (p99).
//   • Symbolic music eval         ≤ 0.20 ms / bar   (p99).
//   • Neural material eval        ≤ 1.50 ms / frame (Vulkan, p99).
//
// Runs headless in CI. Scaffold today — wires to real inference paths once
// engine/ai_runtime lands the ggml backend (ADR-0100, ADR-0117). Strict gate
// activates with GW_AI_BUDGETS_STRICT=1; otherwise the test prints a
// soft-budget line and returns success.

#include <chrono>
#include <cstdio>
#include <cstdlib>

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
        // TODO(#gw-ai-25): AIDirectorPolicy::tick() once ggml lands.
        volatile int x = 0;
        for (int i = 0; i < 1000; ++i) { x += i; }
        const double elapsed_ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
        rc |= check_budget("director", elapsed_ms, kDirectorBudgetMs);
    }

    {
        const auto start = std::chrono::steady_clock::now();
        // TODO(#gw-ai-25): symbolic-music tiny transformer once it lands.
        volatile int x = 0;
        for (int i = 0; i < 500; ++i) { x += i; }
        const double elapsed_ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
        rc |= check_budget("music", elapsed_ms, kMusicBudgetMs);
    }

    {
        const auto start = std::chrono::steady_clock::now();
        // TODO(#gw-ai-25): neural-material eval once weights cook path lands.
        volatile int x = 0;
        for (int i = 0; i < 2000; ++i) { x += i; }
        const double elapsed_ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
        rc |= check_budget("neural_material", elapsed_ms, kNeuralBudgetMs);
    }

    return rc;
}
