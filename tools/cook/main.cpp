// tools/cook/main.cpp — gw_cook: Greywater content cooker CLI.
// Phase 6 spec §5.1.
//
// Usage:
//   gw_cook [options] <input-dir>
//
// Options:
//   --output <dir>         Cooked output root (default: ./assets/)
//   --cache <dir>          Cook cache root (default: .greywater/cook_cache/)
//   --jobs N               Parallel cook threads (default: hw_concurrency - 1)
//   --incremental          Skip assets whose source hash is unchanged (default: on)
//   --force                Re-cook all assets regardless of cache
//   --filter <glob>        Only cook matching assets
//   --platform <id>        Target platform: win64 | linux-x64
//   --config <debug|release> Quality preset
//   --manifest <file>      Write cook manifest JSON to file
//   --verbose              Per-asset progress to stdout
//   --version              Print version and exit

#include "cook_graph.hpp"
#include "cook_cache.hpp"
#include "cook_worker.hpp"
#include "engine/assets/cook/cook_registry.hpp"
#include "engine/assets/cook/cook_manifest.hpp"
#include "engine/core/crash_reporter.hpp"
#include "engine/core/version.hpp"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>

using namespace gw::tools::cook;
using gw::assets::CookPlatform;
using gw::assets::CookConfig;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static void print_usage() {
    std::fprintf(stdout,
        "gw_cook v%s — Greywater content cooker\n\n"
        "Usage:\n"
        "  gw_cook [options] <input-dir>\n\n"
        "Options:\n"
        "  --output <dir>         Cooked output root (default: ./assets/)\n"
        "  --cache <dir>          Cook cache root (default: .greywater/cook_cache/)\n"
        "  --jobs N               Parallel cook threads (default: hw_concurrency - 1)\n"
        "  --incremental          Skip unchanged assets (default: on)\n"
        "  --force                Re-cook all assets regardless of cache\n"
        "  --filter <glob>        Only cook matching assets\n"
        "  --platform <id>        win64 | linux-x64 (default: host)\n"
        "  --config <p>           debug | release (default: release)\n"
        "  --manifest <file>      Write cook manifest JSON to file\n"
        "  --verbose              Per-asset progress to stdout\n"
        "  --version              Print version and exit\n",
        gw::core::version_string()
    );
}

static bool arg_eq(const char* a, const char* b) {
    return std::strcmp(a, b) == 0;
}

static const char* next_arg(int argc, char** argv, int& i) {
    if (++i >= argc) {
        std::fprintf(stderr, "Error: missing argument after '%s'\n", argv[i - 1]);
        return nullptr;
    }
    return argv[i];
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
    // pre-eng-crash-reporter-init: install before discovery so crashes in the
    // cook graph are captured with build-SHA context.
    gw::core::crash::install_handlers();

    if (argc <= 1) { print_usage(); return 0; }

    // Defaults
    std::string input_dir;
    std::string output_dir = "assets";
    std::string cache_dir  = ".greywater/cook_cache";
    std::string manifest_path;
    std::string filter_glob;
    uint32_t    jobs     = 0; // 0 = auto
    bool        force    = false;
    bool        verbose  = false;
    CookPlatform platform = CookPlatform::Host;
    CookConfig  config   = CookConfig::Release;

    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];
        if (arg_eq(arg, "--version")) {
            std::fprintf(stdout, "%s\n", gw::core::version_string());
            return 0;
        } else if (arg_eq(arg, "--help") || arg_eq(arg, "-h")) {
            print_usage(); return 0;
        } else if (arg_eq(arg, "--output")) {
            const char* v = next_arg(argc, argv, i);
            if (!v) return 1;
            output_dir = v;
        } else if (arg_eq(arg, "--cache")) {
            const char* v = next_arg(argc, argv, i);
            if (!v) return 1;
            cache_dir = v;
        } else if (arg_eq(arg, "--jobs")) {
            const char* v = next_arg(argc, argv, i);
            if (!v) return 1;
            jobs = static_cast<uint32_t>(std::stoul(v));
        } else if (arg_eq(arg, "--force")) {
            force = true;
        } else if (arg_eq(arg, "--incremental")) {
            // already the default; accept for scripts
        } else if (arg_eq(arg, "--filter")) {
            const char* v = next_arg(argc, argv, i);
            if (!v) return 1;
            filter_glob = v;
        } else if (arg_eq(arg, "--platform")) {
            const char* v = next_arg(argc, argv, i);
            if (!v) return 1;
            if (std::strcmp(v, "win64")    == 0) platform = CookPlatform::Win64;
            if (std::strcmp(v, "linux-x64")== 0) platform = CookPlatform::LinuxX64;
        } else if (arg_eq(arg, "--config")) {
            const char* v = next_arg(argc, argv, i);
            if (!v) return 1;
            if (std::strcmp(v, "debug")   == 0) config = CookConfig::Debug;
            if (std::strcmp(v, "release") == 0) config = CookConfig::Release;
        } else if (arg_eq(arg, "--manifest")) {
            const char* v = next_arg(argc, argv, i);
            if (!v) return 1;
            manifest_path = v;
        } else if (arg_eq(arg, "--verbose")) {
            verbose = true;
        } else if (arg[0] != '-') {
            input_dir = arg;
        } else {
            std::fprintf(stderr, "Unknown option: %s\n", arg);
            return 1;
        }
    }

    if (input_dir.empty()) {
        std::fprintf(stderr, "Error: no input directory specified.\n");
        print_usage();
        return 1;
    }

    if (!std::filesystem::exists(input_dir)) {
        std::fprintf(stderr, "Error: input directory '%s' does not exist.\n",
                     input_dir.c_str());
        return 1;
    }

    // Build graph.
    CookGraph graph(input_dir, output_dir, platform, config);
    graph.discover();

    if (graph.nodes().empty()) {
        std::fprintf(stdout, "No cookable assets found in '%s'.\n", input_dir.c_str());
        return 0;
    }

    // Open cache.
    CookCache cache(std::filesystem::path(cache_dir) / "cook_cache.db");
    if (!force) graph.resolve_cache(nullptr);

    // Prepare manifest.
    gw::assets::cook::CookManifest manifest{};
    manifest.cook_version   = 1;
    manifest.platform       = (platform == CookPlatform::Win64) ? "win64" : "linux-x64";
    manifest.config         = (config == CookConfig::Release) ? "release" : "debug";
    manifest.engine_version = gw::core::version_string();
    manifest.timestamp      = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    // Run worker.
    CookWorkerConfig worker_cfg{};
    worker_cfg.thread_count = jobs;
    worker_cfg.input_root   = input_dir;
    worker_cfg.output_root  = output_dir;
    worker_cfg.cache_dir    = cache_dir;
    worker_cfg.platform     = platform;
    worker_cfg.config       = config;
    worker_cfg.force        = force;
    worker_cfg.verbose      = verbose;
    worker_cfg.filter_glob  = filter_glob;

    ProgressFn progress;
    if (verbose) {
        progress = [](uint32_t done, uint32_t total, const std::string& path) {
            std::fprintf(stdout, "[%u/%u] %s\n", done, total, path.c_str());
        };
    }

    CookWorker worker(worker_cfg, gw::assets::cook::make_default_registry(),
                       std::move(progress));
    const CookStats stats = worker.execute(graph, cache, manifest);

    // Report.
    std::fprintf(stdout,
        "\ngw_cook done: %u total, %u cooked, %u cached, %u errors, %u ms wall.\n",
        stats.total, stats.cooked, stats.cached, stats.errors, stats.wall_ms);

    // Write manifest.
    if (!manifest_path.empty()) {
        if (manifest.write_file(manifest_path)) {
            if (verbose) std::fprintf(stdout, "Manifest written: %s\n",
                                       manifest_path.c_str());
        } else {
            std::fprintf(stderr, "Warning: failed to write manifest to '%s'\n",
                         manifest_path.c_str());
        }
    }

    return (stats.errors > 0) ? 1 : 0;
}
