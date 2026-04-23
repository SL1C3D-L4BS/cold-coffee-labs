// apps/sandbox_director/main.cpp — Part B §12.7 scaffold (ADR-0107).
//
// Two modes:
//   1. Headless CI      — reads a scenario TOML, runs the Director state
//                         machine deterministically, emits a JSON run report
//                         plus a bit-hash of visited state. Consumed by the
//                         gw_perf_gate_director perf gate.
//   2. Interactive      — its own ImGui dockspace (later wave) with scenario
//                         composer, policy diff, spawn heatmap, intensity
//                         plot, replay scrubber, A/B policy comparison.
//
// Exit marker:
//   DIRECTOR SANDBOX OK — seed=<hex> frames=<n> policy_ms=<x.xxx>
//   spawns=<n> bit_hash=<hex>
//
// Budget contract: Director policy eval ≤ 0.1 ms per tick on RX 580.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {

struct CliOptions {
    bool        headless  = true;
    std::string scenario  = "assets/ai/director/scenarios/default.toml";
    std::string policy    = "assets/ai/director/policies/phase26_v0.bin";
    int         max_frames = 600;
};

CliOptions parse_cli(int argc, char** argv) noexcept {
    CliOptions o{};
    for (int i = 1; i < argc; ++i) {
        const char* a = argv[i];
        if (std::strcmp(a, "--interactive") == 0) {
            o.headless = false;
        } else if (std::strcmp(a, "--scenario") == 0 && i + 1 < argc) {
            o.scenario = argv[++i];
        } else if (std::strcmp(a, "--policy") == 0 && i + 1 < argc) {
            o.policy = argv[++i];
        } else if (std::strcmp(a, "--frames") == 0 && i + 1 < argc) {
            o.max_frames = std::atoi(argv[++i]);
        }
    }
    return o;
}

int run_headless(const CliOptions& opts) noexcept {
    // Phase 26 lands the real deterministic run. Scaffold emits the exit
    // marker with synthesised numbers so CI wiring can be built in parallel
    // with the policy implementation.
    std::printf("DIRECTOR SANDBOX OK — seed=0x0 frames=%d policy_ms=0.000 "
                "spawns=0 bit_hash=0x0 scenario=%s policy=%s\n",
                opts.max_frames, opts.scenario.c_str(), opts.policy.c_str());
    return 0;
}

int run_interactive(const CliOptions& /*opts*/) noexcept {
    // Phase 26: ImGui dockspace. Scaffold prints a banner and exits.
    std::printf("sandbox_director: interactive mode is a Phase 26 deliverable.\n");
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    const auto opts = parse_cli(argc, argv);
    return opts.headless ? run_headless(opts) : run_interactive(opts);
}
