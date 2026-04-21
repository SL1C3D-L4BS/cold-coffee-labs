// runtime/main.cpp — Phase 11 Wave 11F (ADR-0029).
//
// Minimal CLI: recognises --self-test (bounded headless run, exits with 0
// on full boot + tick loop), --deterministic (force null backends + pinned
// seed), --frames=N, --width=N, --height=N, --locale=TAG.
//
// Everything gameplay-side (scenes, levels, networking) arrives in later
// phases; Phase 11 proves the runtime boots cleanly and ticks the event /
// CVar / UI / console / audio / input wiring.

#include "engine/core/version.hpp"
#include "runtime/engine.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>

namespace {

bool starts_with(std::string_view s, std::string_view p) noexcept {
    return s.size() >= p.size() && std::memcmp(s.data(), p.data(), p.size()) == 0;
}

[[nodiscard]] std::uint32_t parse_u32(std::string_view s, std::uint32_t fallback) noexcept {
    try { return static_cast<std::uint32_t>(std::stoul(std::string{s})); }
    catch (...) { return fallback; }
}

void print_usage(const char* argv0) {
    std::fprintf(stdout,
        "greywater runtime — %s\n"
        "\nUsage: %s [options]\n"
        "Options:\n"
        "  --self-test                 Boot, tick N frames, exit 0\n"
        "  --deterministic             Force null backends + fixed seed\n"
        "  --frames=N                  Frames to tick in self-test (default 120)\n"
        "  --width=N  --height=N       Initial virtual viewport\n"
        "  --locale=TAG                BCP-47 tag (default en-US)\n"
        "  --help                      Show this message\n",
        gw::core::version_string(), argv0);
}

} // namespace

int main(int argc, char** argv) {
    gw::runtime::EngineConfig cfg{};
    bool self_test = false;

    for (int i = 1; i < argc; ++i) {
        std::string_view a{argv[i]};
        if (a == "--help" || a == "-h") { print_usage(argv[0]); return 0; }
        if (a == "--self-test")      { self_test = true; cfg.headless = true; continue; }
        if (a == "--deterministic")  { cfg.deterministic = true; continue; }
        if (starts_with(a, "--frames=")) { cfg.self_test_frames = parse_u32(a.substr(9), cfg.self_test_frames); continue; }
        if (starts_with(a, "--width="))  { cfg.width_px  = parse_u32(a.substr(8), cfg.width_px); continue; }
        if (starts_with(a, "--height=")) { cfg.height_px = parse_u32(a.substr(9), cfg.height_px); continue; }
        if (starts_with(a, "--locale=")) { cfg.locale = std::string{a.substr(9)}; continue; }
        std::fprintf(stderr, "[runtime] unknown argument: %.*s\n",
                     static_cast<int>(a.size()), a.data());
        print_usage(argv[0]);
        return 2;
    }

    std::fprintf(stdout, "[runtime] greywater %s boot (self_test=%d, deterministic=%d)\n",
                 gw::core::version_string(), self_test ? 1 : 0, cfg.deterministic ? 1 : 0);

    try {
        gw::runtime::Engine engine{cfg};

        if (self_test) {
            engine.run_frames(cfg.self_test_frames);
            std::fprintf(stdout, "[runtime] self-test OK — ticked %u frames\n", cfg.self_test_frames);
            return 0;
        }

        // Non-self-test path: run a bounded loop until quit is requested.
        // Gameplay path / window / render pump live in Phase 12+. For now
        // we run indefinitely but still respect a --frames cap for smoke.
        const std::uint32_t frames = cfg.self_test_frames ? cfg.self_test_frames : 600;
        engine.run_frames(frames);
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "[runtime] fatal: %s\n", ex.what());
        return 1;
    }

    return 0;
}
