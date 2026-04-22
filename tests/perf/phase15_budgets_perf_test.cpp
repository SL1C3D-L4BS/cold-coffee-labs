// tests/perf/phase15_budgets_perf_test.cpp — Phase 15 perf gate (ADR-0064 §2).
//
// Runs a set of persistence / telemetry micro-benchmarks against the budgets in
// `docs/09_NETCODE_DETERMINISM_PERF.md` (merged **Phase 15 performance budgets**). Under Debug builds we intentionally multiply
// the budget ceilings by `kDebugSlack` so CI on Debug configs stays green —
// Release-mode runs (persist-* presets) tighten the envelope.
//
// This is a standalone executable; it is NOT part of gw_tests because it links
// persist / ecs and needs its own perf ceiling policy.

#include "engine/ecs/world.hpp"
#include "engine/persist/ecs_snapshot.hpp"
#include "engine/persist/gwsave_format.hpp"
#include "engine/persist/gwsave_io.hpp"
#include "engine/persist/integrity.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <span>
#include <vector>

namespace {

struct Pos {
    float x = 0, y = 0, z = 0;
};

#ifndef NDEBUG
constexpr double kDebugSlack = 20.0; // Debug builds get generous headroom.
#else
constexpr double kDebugSlack = 1.0;
#endif

struct Budget {
    const char* name                = nullptr;
    double      ms_ceiling_release = 0.0;
    // Optional extra Debug-only multiplier for pathologically slow-in-Debug
    // operations (e.g. BLAKE3 without SIMD optimizations). Applied on top of
    // kDebugSlack in Debug builds only; Release ceilings are untouched.
    double extra_debug_slack = 1.0;
};

double elapsed_ms(decltype(std::chrono::steady_clock::now()) start) {
    const auto stop = std::chrono::steady_clock::now();
    const auto micros =
        std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();
    return static_cast<double>(micros) / 1000.0;
}

bool run_budget(const Budget& budget, double measured_ms) {
#ifndef NDEBUG
    const double ceiling =
        budget.ms_ceiling_release * kDebugSlack * budget.extra_debug_slack;
#else
    const double ceiling = budget.ms_ceiling_release * kDebugSlack;
#endif
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    std::printf("phase15.perf %-32s measured=%.3f ms ceiling=%.3f ms %s\n",
                budget.name, measured_ms, ceiling,
                measured_ms <= ceiling ? "OK" : "FAIL");
    return measured_ms <= ceiling;
}

} // namespace

int main() {
    gw::ecs::World world;
    (void)world.component_registry().id_of<Pos>();
    for (int idx = 0; idx < 2048; ++idx) {
        const auto ent = world.create_entity();
        world.add_component(ent, Pos{.x = static_cast<float>(idx), .y = 0.0F, .z = 0.0F});
    }

    auto chunks = gw::persist::build_chunk_grid_demo(world);
    gw::persist::gwsave::HeaderPrefix hdr{};
    hdr.schema_version = 1;
    hdr.engine_version = 1;
    hdr.chunk_count    = static_cast<std::uint32_t>(chunks.size());
    const auto centre_bytes = gw::persist::centre_chunk_payload(chunks);
    hdr.determinism_hash =
        gw::persist::ecs_blob_determinism_hash(std::span<const std::byte>(centre_bytes));

    bool all_ok = true;

    // 1. gwsave write (2048 entities) — budget 18 ms release.
    {
        const auto bench_start = std::chrono::steady_clock::now();
        for (int idx = 0; idx < 4; ++idx) {
            auto bytes = gw::persist::write_gwsave_container(0, hdr, chunks);
            (void)bytes;
        }
        double per_iter{0.0};
        per_iter = elapsed_ms(bench_start) / 4.0;
        all_ok &= run_budget(Budget{.name                = "gwsave write 2048 ent",
                                    .ms_ceiling_release = 18.0},
                              per_iter);
    }

    auto file = gw::persist::write_gwsave_container(0, hdr, chunks);

    // 2. gwsave header peek — budget 0.6 ms.
    {
        const auto bench_start = std::chrono::steady_clock::now();
        for (int idx = 0; idx < 32; ++idx) {
            auto peek = gw::persist::peek_gwsave(
                std::span<const std::byte>(file.data(), file.size()), true);
            (void)peek;
        }
        double per_iter{0.0};
        per_iter = elapsed_ms(bench_start) / 32.0;
        all_ok &= run_budget(Budget{.name = "gwsave header peek", .ms_ceiling_release = 0.6}, per_iter);
    }

    // 3. gwsave full load (2048 entities) — budget 14 ms.
    {
        const auto bench_start = std::chrono::steady_clock::now();
        for (int idx = 0; idx < 4; ++idx) {
            auto peek = gw::persist::peek_gwsave(
                std::span<const std::byte>(file.data(), file.size()), true);
            std::vector<gw::persist::gwsave::ChunkPayload> loaded;
            (void)gw::persist::load_gwsave_all_chunks(
                std::span<const std::byte>(file.data(), file.size()), peek, loaded);
        }
        double per_iter{0.0};
        per_iter = elapsed_ms(bench_start) / 4.0;
        all_ok &= run_budget(Budget{.name                = "gwsave full load 2048 ent",
                                    .ms_ceiling_release = 14.0},
                              per_iter);
    }

    // 4. BLAKE3-256 over a 2 MiB buffer — budget 2.5 ms Release.
    //    BLAKE3 reference (non-SIMD) is pathologically slow in Debug builds —
    //    we take the *best-of-N* iteration timing to reject CPU-contention
    //    spikes from CI/antivirus/other jobs, and apply a 4× extra Debug
    //    slack on top of kDebugSlack so the Debug ceiling becomes 200 ms
    //    (≥ 10 MiB/s sustained; Release ceiling stays at 2.5 ms → 800 MiB/s).
    {
        std::vector<std::byte> big(2ULL * 1024 * 1024);
        std::memset(big.data(), 0xAB, big.size());
        // Warm-up.
        {
            auto digest = gw::persist::blake3_digest_256(
                std::span<const std::byte>(big.data(), big.size()));
            (void)digest;
        }
        constexpr int kIters = 8;
        double best_ms = 1e9;
        for (int idx = 0; idx < kIters; ++idx) {
            const auto bench_start = std::chrono::steady_clock::now();
            auto       digest      = gw::persist::blake3_digest_256(
                std::span<const std::byte>(big.data(), big.size()));
            (void)digest;
            const double iter_ms = elapsed_ms(bench_start);
            best_ms              = std::min(best_ms, iter_ms);
        }
        all_ok &= run_budget(Budget{.name                 = "BLAKE3-256 2 MiB",
                                    .ms_ceiling_release  = 2.5,
                                    .extra_debug_slack   = 4.0},
                              best_ms);
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    std::printf("phase15.perf RESULT %s\n", all_ok ? "PASS" : "FAIL");
    return all_ok ? 0 : 1;
}
