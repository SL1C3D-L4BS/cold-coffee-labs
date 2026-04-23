#pragma once
// tools/cook/cook_worker.hpp
// Parallel cook job dispatch — maps the topologically-sorted CookGraph
// nodes onto a thread pool (std::execution::par_unseq semantics via
// a manual for_each over independent batches).
// Phase 6 spec §5.3.

#include "cook_graph.hpp"
#include "cook_cache.hpp"
#include "engine/assets/cook/cook_registry.hpp"
#include "engine/assets/cook/cook_manifest.hpp"
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace gw::tools::cook {

struct CookWorkerConfig {
    uint32_t              thread_count   = 0; // 0 → hw_concurrency - 1
    std::filesystem::path input_root;
    std::filesystem::path output_root;
    std::filesystem::path cache_dir;
    CookPlatform          platform       = CookPlatform::Host;
    CookConfig            config         = CookConfig::Release;
    bool                  force          = false; // ignore cache
    bool                  verbose        = false;
    std::string           filter_glob;           // empty = all
    // pre-tc-content-signing: path to the Ed25519 signing key. Empty =
    // skip signing (dev / sandbox builds). Release CI sets this from a
    // sealed secret. See docs/prompts/preflight/pre-tc-content-signing.md.
    std::filesystem::path sign_key_path;
};

// Progress callback invoked after each asset completes.
using ProgressFn = std::function<void(uint32_t done, uint32_t total, const std::string& path)>;

class CookWorker {
public:
    explicit CookWorker(CookWorkerConfig cfg,
                        gw::assets::cook::CookRegistry registry,
                        ProgressFn progress = {});

    // Execute all nodes in the graph.  Blocks until complete.
    [[nodiscard]] CookStats
    execute(CookGraph& graph, CookCache& cache,
            gw::assets::cook::CookManifest& manifest);

private:
    [[nodiscard]] bool matches_filter(const std::filesystem::path& p) const;

    CookWorkerConfig                  cfg_;
    gw::assets::cook::CookRegistry   registry_;
    ProgressFn                        progress_;
};

} // namespace gw::tools::cook
