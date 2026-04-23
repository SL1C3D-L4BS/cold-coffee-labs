#include "cook_worker.hpp"
#include "content_signing.hpp"
#include "engine/jobs/scheduler.hpp"
#include "engine/jobs/reserved_worker.hpp"
#include <algorithm>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

namespace gw::tools::cook {

CookWorker::CookWorker(CookWorkerConfig cfg,
                       gw::assets::cook::CookRegistry registry,
                       ProgressFn progress)
    : cfg_(std::move(cfg))
    , registry_(std::move(registry))
    , progress_(std::move(progress))
{
    if (cfg_.thread_count == 0) {
        const uint32_t hw = static_cast<uint32_t>(std::thread::hardware_concurrency());
        cfg_.thread_count = std::max(1u, hw > 1 ? hw - 1 : 1);
    }
}

bool CookWorker::matches_filter(const std::filesystem::path& p) const {
    if (cfg_.filter_glob.empty()) return true;
    // Simple substring filter (full glob support is Phase 6 stretch).
    return p.string().find(cfg_.filter_glob) != std::string::npos;
}

CookStats CookWorker::execute(CookGraph&                            graph,
                               CookCache&                            cache,
                               gw::assets::cook::CookManifest&      manifest)
{
    const auto wall_start = std::chrono::steady_clock::now();

    auto order_result = graph.topological_order();
    if (!order_result) {
        CookStats s{};
        s.errors = 1;
        return s;
    }
    const auto& order = *order_result;

    const uint32_t total = static_cast<uint32_t>(order.size());
    std::atomic<uint32_t> done{0};
    std::atomic<uint32_t> errors{0};
    std::atomic<uint32_t> cached_count{0};
    std::atomic<uint32_t> cooked_count{0};

    std::mutex manifest_mutex;

    // Fork N reserved workers; NN #10 routes all raw std::thread ownership
    // through engine/jobs/. See audit-2026-04-20 (P2-2).
    const uint32_t n_threads = std::min(cfg_.thread_count, total);
    std::vector<gw::jobs::ReservedWorker> threads;
    threads.reserve(n_threads);

    std::atomic<uint32_t> next_idx{0};

    auto worker = [&]() {
        while (true) {
            const uint32_t i = next_idx.fetch_add(1u, std::memory_order_relaxed);
            if (i >= total) break;

            const std::size_t node_idx = order[i];
            auto& node = graph.nodes()[node_idx];

            if (!matches_filter(node.source)) {
                done.fetch_add(1u, std::memory_order_relaxed);
                continue;
            }

            // Check cache first (unless --force).
            if (!cfg_.force) {
                auto hit = cache.lookup(node.cache_key);
                if (hit && std::filesystem::exists(hit->output_path)) {
                    node.needs_cook = false;
                    cached_count.fetch_add(1u, std::memory_order_relaxed);
                    done.fetch_add(1u, std::memory_order_relaxed);
                    if (progress_) progress_(done.load(), total, node.source.string());
                    continue;
                }
            }

            // Find cooker.
            const auto ext = node.source.extension().string();
            const auto* cooker = registry_.find(ext);
            if (!cooker) {
                errors.fetch_add(1u, std::memory_order_relaxed);
                done.fetch_add(1u, std::memory_order_relaxed);
                continue;
            }

            gw::assets::cook::CookContext ctx{};
            ctx.source_path = node.source;
            ctx.output_path = node.output;
            ctx.cache_dir   = cfg_.cache_dir;
            ctx.platform    = cfg_.platform;
            ctx.config      = cfg_.config;
            ctx.verbose     = cfg_.verbose;

            const auto cook_result = cooker->cook(ctx);
            if (!cook_result) {
                errors.fetch_add(1u, std::memory_order_relaxed);
                if (cfg_.verbose) {
                    std::fprintf(stderr, "[COOK ERROR] %s: %s\n",
                                 node.source.string().c_str(),
                                 cook_result.error().message);
                }
            } else {
                cooked_count.fetch_add(1u, std::memory_order_relaxed);
                node.cache_key = cook_result->cook_key;

                // Update SQLite cache.
                CacheEntry ce{};
                ce.cook_key     = cook_result->cook_key;
                ce.output_path  = node.output.string();
                ce.cook_version = cooker->rule_version();
                ce.platform     = static_cast<uint8_t>(cfg_.platform);
                ce.config       = static_cast<uint8_t>(cfg_.config);
                (void)cache.insert(ce);

                // Append to manifest.
                gw::assets::cook::CookEntry entry{};
                entry.source       = node.source.string();
                entry.output       = node.output.string();
                entry.cook_key     = cook_result->cook_key;
                entry.cook_time_ms = cook_result->cook_time_ms;
                entry.cache_hit    = false;
                std::lock_guard lk{manifest_mutex};
                manifest.assets.push_back(std::move(entry));
            }

            done.fetch_add(1u, std::memory_order_relaxed);
            if (progress_) progress_(done.load(), total, node.source.string());
        }
    };

    for (uint32_t t = 0; t < n_threads; ++t) {
        threads.emplace_back(gw::jobs::Scheduler::reserve_worker(worker));
    }
    for (auto& t : threads) t.join();

    const auto wall_end = std::chrono::steady_clock::now();
    const uint32_t wall_ms = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(wall_end - wall_start).count());

    // pre-tc-content-signing — post-cook finalisation. When a signing key is
    // configured, sign the emitted manifest and stamp the signature into the
    // manifest blob for Release-build loader verification. Signing failures
    // are surfaced as errors so CI fails fast on sealed-secret misconfig.
    if (!cfg_.sign_key_path.empty() && errors.load() == 0u) {
        gw::cook::signing::Signature sig{};
        const auto ok = gw::cook::signing::sign_manifest(
            cfg_.output_root.string(),
            cfg_.sign_key_path.string(),
            sig);
        if (!ok) {
            errors.fetch_add(1u, std::memory_order_relaxed);
            if (cfg_.verbose) {
                std::fprintf(stderr, "[COOK ERROR] manifest signing failed\n");
            }
        } else if (cfg_.verbose) {
            std::fprintf(stdout, "[COOK] manifest signed (Ed25519)\n");
        }
    }

    CookStats stats{};
    stats.total   = total;
    stats.cooked  = cooked_count.load();
    stats.cached  = cached_count.load();
    stats.errors  = errors.load();
    stats.wall_ms = wall_ms;
    return stats;
}

} // namespace gw::tools::cook
