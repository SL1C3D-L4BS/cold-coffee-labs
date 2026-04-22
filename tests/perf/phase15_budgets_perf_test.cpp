// tests/perf/phase15_budgets_perf_test.cpp — Phase 15 perf gate (ADR-0064 §2).
//
// Runs a set of persistence / telemetry micro-benchmarks against the budgets in
// `docs/perf/phase15_budgets.md`. Under Debug builds we intentionally multiply
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

#include <chrono>
#include <cstdio>
#include <cstring>
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
    const char* name;
    double      ms_ceiling_release;
    // Optional extra Debug-only multiplier for pathologically slow-in-Debug
    // operations (e.g. BLAKE3 without SIMD optimizations). Applied on top of
    // kDebugSlack in Debug builds only; Release ceilings are untouched.
    double      extra_debug_slack = 1.0;
};

double elapsed_ms(std::chrono::steady_clock::time_point t0) {
    const auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

bool run_budget(const Budget& b, double measured_ms) {
#ifndef NDEBUG
    const double ceiling = b.ms_ceiling_release * kDebugSlack * b.extra_debug_slack;
#else
    const double ceiling = b.ms_ceiling_release * kDebugSlack;
#endif
    std::printf("phase15.perf %-32s measured=%.3f ms ceiling=%.3f ms %s\n",
                b.name, measured_ms, ceiling,
                measured_ms <= ceiling ? "OK" : "FAIL");
    return measured_ms <= ceiling;
}

} // namespace

int main() {
    gw::ecs::World w;
    (void)w.component_registry().id_of<Pos>();
    for (int i = 0; i < 2048; ++i) {
        const auto e = w.create_entity();
        w.add_component(e, Pos{static_cast<float>(i), 0.0f, 0.0f});
    }

    auto chunks = gw::persist::build_chunk_grid_demo(w);
    gw::persist::gwsave::HeaderPrefix hdr{};
    hdr.schema_version = 1;
    hdr.engine_version = 1;
    hdr.chunk_count    = static_cast<std::uint32_t>(chunks.size());
    const auto c       = gw::persist::centre_chunk_payload(chunks);
    hdr.determinism_hash = gw::persist::ecs_blob_determinism_hash(
        std::span<const std::byte>(reinterpret_cast<const std::byte*>(c.data()), c.size()));

    bool ok = true;

    // 1. gwsave write (2048 entities) — budget 18 ms release.
    {
        auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < 4; ++i) {
            auto bytes = gw::persist::write_gwsave_container(0, hdr, chunks);
            (void)bytes;
        }
        const double per_iter = elapsed_ms(t0) / 4.0;
        ok &= run_budget({"gwsave write 2048 ent", 18.0}, per_iter);
    }

    auto file = gw::persist::write_gwsave_container(0, hdr, chunks);

    // 2. gwsave header peek — budget 0.6 ms.
    {
        auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < 32; ++i) {
            auto peek = gw::persist::peek_gwsave(
                std::span<const std::byte>(file.data(), file.size()), true);
            (void)peek;
        }
        const double per_iter = elapsed_ms(t0) / 32.0;
        ok &= run_budget({"gwsave header peek", 0.6}, per_iter);
    }

    // 3. gwsave full load (2048 entities) — budget 14 ms.
    {
        auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < 4; ++i) {
            auto peek = gw::persist::peek_gwsave(
                std::span<const std::byte>(file.data(), file.size()), true);
            std::vector<gw::persist::gwsave::ChunkPayload> loaded;
            (void)gw::persist::load_gwsave_all_chunks(
                std::span<const std::byte>(file.data(), file.size()), peek, loaded);
        }
        const double per_iter = elapsed_ms(t0) / 4.0;
        ok &= run_budget({"gwsave full load 2048 ent", 14.0}, per_iter);
    }

    // 4. BLAKE3-256 over a 2 MiB buffer — budget 2.5 ms Release.
    //    BLAKE3 reference (non-SIMD) is pathologically slow in Debug builds —
    //    we take the *best-of-N* iteration timing to reject CPU-contention
    //    spikes from CI/antivirus/other jobs, and apply a 4× extra Debug
    //    slack on top of kDebugSlack so the Debug ceiling becomes 200 ms
    //    (≥ 10 MiB/s sustained; Release ceiling stays at 2.5 ms → 800 MiB/s).
    {
        std::vector<std::byte> big(2ull * 1024 * 1024);
        std::memset(big.data(), 0xAB, big.size());
        // Warm-up.
        {
            auto d = gw::persist::blake3_digest_256(
                std::span<const std::byte>(big.data(), big.size()));
            (void)d;
        }
        constexpr int kIters = 8;
        double best_ms = 1e9;
        for (int i = 0; i < kIters; ++i) {
            auto t0 = std::chrono::steady_clock::now();
            auto d  = gw::persist::blake3_digest_256(
                std::span<const std::byte>(big.data(), big.size()));
            (void)d;
            const double iter_ms = elapsed_ms(t0);
            if (iter_ms < best_ms) best_ms = iter_ms;
        }
        ok &= run_budget({"BLAKE3-256 2 MiB", 2.5, 4.0}, best_ms);
    }

    std::printf("phase15.perf RESULT %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
