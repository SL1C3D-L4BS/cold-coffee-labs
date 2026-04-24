// runtime/main.cpp — Phase 11 Wave 11F (ADR-0029).
//
// CLI: --self-test, --deterministic, --frames=N, viewport/locale, plus playable
// bootstrap: --scene=, --seed=, --cvars-toml= (and GW_PLAY_SCENE / GW_UNIVERSE_SEED /
// GW_CVARS_TOML). Optional scene load uses the same authoring codec as the editor.

#include "engine/core/version.hpp"
#include "runtime/engine.hpp"
#include "runtime/playable_bootstrap.hpp"
#include "runtime/playable_scene_host.hpp"

#include <cstdio>
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
        "  --scene=PATH                Load authoring .gwscene after boot\n"
        "  --seed=N                    Universe / rng.seed (optional)\n"
        "  --cvars-toml=PATH         Merge TOML into CVar registry (optional)\n"
        "  --help                      Show this message\n",
        gw::core::version_string(), argv0);
}

} // namespace

int main(int argc, char** argv) {
    gw::runtime::PlayableCliOptions play{};
    gw::runtime::parse_playable_cli(argc, argv, play);

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
        if (starts_with(a, "--scene="))   { continue; }
        if (starts_with(a, "--seed="))    { continue; }
        if (starts_with(a, "--cvars-toml=")) { continue; }
        std::fprintf(stderr, "[runtime] unknown argument: %.*s\n",
                     static_cast<int>(a.size()), a.data());
        print_usage(argv[0]);
        return 2;
    }

    std::fprintf(stdout, "[runtime] greywater %s boot (self_test=%d, deterministic=%d)\n",
                 gw::core::version_string(), self_test ? 1 : 0, cfg.deterministic ? 1 : 0);

    try {
        gw::runtime::Engine engine{cfg};
        gw::runtime::apply_playable_bootstrap(engine, play);

        if (!play.scene_path.empty()) {
            gw::runtime::PlayableSceneLoadSummary sum{};
            if (!gw::runtime::load_authoring_scene_into_physics(engine, play.scene_path, sum)) {
                std::fprintf(stderr, "[runtime] SCENE LOAD FAILED path=%s\n",
                             play.scene_path.c_str());
                return 3;
            }
            std::fprintf(stdout,
                         "[runtime] SCENE LOAD OK path=%s entities=%u blockout_bodies=%u\n",
                         play.scene_path.c_str(),
                         static_cast<unsigned>(sum.entities),
                         static_cast<unsigned>(sum.blockout_bodies));
        }

        if (self_test) {
            engine.run_frames(cfg.self_test_frames);
            std::fprintf(stdout, "[runtime] self-test OK — ticked %u frames\n", cfg.self_test_frames);
            return 0;
        }

        const std::uint32_t frames = cfg.self_test_frames ? cfg.self_test_frames : 600;
        engine.run_frames(frames);
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "[runtime] fatal: %s\n", ex.what());
        return 1;
    }

    return 0;
}
