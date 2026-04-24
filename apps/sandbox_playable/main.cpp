// apps/sandbox_playable/main.cpp — Phase 11 Wave 11F demo (ADR-0029).
//
// Boots runtime::Engine headless/deterministic and drives a short script.
// Optional: --scene=path.gwscene (+ GW_PLAY_SCENE), --seed=, --cvars-toml=.
// After load, syncs blockout box primitives into PhysicsWorld (Phase 24).

#include "engine/core/crash_reporter.hpp"
#include "engine/core/version.hpp"
#include "runtime/engine.hpp"
#include "runtime/playable_bootstrap.hpp"
#include "runtime/playable_scene_host.hpp"

#include <cstdarg>
#include <cstdio>
#include <string>

namespace {

class StdoutWriter final : public gw::console::ConsoleWriter {
public:
    void write_line(std::string_view line) override {
        std::fprintf(stdout, "[console] %.*s\n",
                     static_cast<int>(line.size()), line.data());
    }
    void writef(const char* fmt, ...) override {
        char buf[512];
        va_list ap;
        va_start(ap, fmt);
        std::vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        std::fprintf(stdout, "[console] %s\n", buf);
    }
};

} // namespace

int main(int argc, char** argv) {
    gw::core::crash::install_handlers();
    std::fprintf(stdout, "[sandbox_playable] greywater %s\n", gw::core::version_string());

    gw::runtime::PlayableCliOptions cli{};
    gw::runtime::parse_playable_cli(argc, argv, cli);

    gw::runtime::EngineConfig cfg{};
    cfg.headless         = true;
    cfg.deterministic    = true;
    cfg.self_test_frames = 60;
    cfg.width_px         = 1280;
    cfg.height_px        = 720;
    cfg.locale           = "en-US";

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a.rfind("--frames=", 0) == 0) {
            try { cfg.self_test_frames = static_cast<std::uint32_t>(std::stoul(a.substr(9))); }
            catch (...) {}
        }
    }

    gw::runtime::Engine engine{cfg};
    gw::runtime::apply_playable_bootstrap(engine, cli);

    StdoutWriter out;

    std::uint32_t scene_entities = 0;
    if (!cli.scene_path.empty()) {
        gw::runtime::PlayableSceneLoadSummary sum{};
        if (!gw::runtime::load_authoring_scene_into_physics(engine, cli.scene_path, sum)) {
            std::fprintf(stderr, "[sandbox_playable] SCENE LOAD FAILED path=%s\n",
                         cli.scene_path.c_str());
            return 3;
        }
        scene_entities = sum.entities;
        std::fprintf(stdout,
                     "[sandbox_playable] SCENE LOAD OK path=%s entities=%u blockout_bodies=%u\n",
                     cli.scene_path.c_str(),
                     static_cast<unsigned>(sum.entities),
                     static_cast<unsigned>(sum.blockout_bodies));
    }

    // Frame 1 — viewport resize
    engine.ui().set_viewport(cfg.width_px, cfg.height_px, 1.0f);
    engine.tick(1.0 / 60.0);

    // Frame 2 — console drive
    (void)engine.console().execute("echo sandbox_playable online", out);
    engine.tick(1.0 / 60.0);

    // Frame 3 — flip contrast boost through the settings binder
    (void)engine.settings_binder().push_from_ui("settings-contrast-boost", "true");
    engine.tick(1.0 / 60.0);

    if (cfg.self_test_frames > 3) {
        engine.run_frames(cfg.self_test_frames - 3);
    }

    std::fprintf(stdout,
                 "[sandbox_playable] PLAYABLE OK — frames=%llu contrast=%d cheats=%d cvars=%zu "
                 "scene_entities=%u\n",
                 static_cast<unsigned long long>(engine.frame_index()),
                 engine.ui().contrast_boost() ? 1 : 0,
                 engine.console().cheats_banner_visible() ? 1 : 0,
                 engine.cvars().count(),
                 static_cast<unsigned>(scene_entities));

    return 0;
}
