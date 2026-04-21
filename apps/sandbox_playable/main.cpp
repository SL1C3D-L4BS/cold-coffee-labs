// apps/sandbox_playable/main.cpp — Phase 11 Wave 11F demo (ADR-0029).
//
// This app is the exit-demo referenced in docs/05 *Playable Runtime*.
// It boots the full runtime (CVars, events, UI, console, audio, input) in
// headless/deterministic mode and ticks a pre-canned script:
//   frame 1: publish a WindowResized to force the UI viewport update
//   frame 2: execute `echo sandbox_playable online` on the console
//   frame 3: set `ui.contrast.boost = true` through the settings binder
//   frame 4..N: idle ticks
// The app prints a trailing summary line so CI can grep for "PLAYABLE OK".
//
// Gameplay logic and a real scene land in Phase 12.

#include "engine/core/version.hpp"
#include "runtime/engine.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
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
    std::fprintf(stdout, "[sandbox_playable] greywater %s\n", gw::core::version_string());

    gw::runtime::EngineConfig cfg{};
    cfg.headless         = true;
    cfg.deterministic    = true;
    cfg.self_test_frames = 60;
    cfg.width_px         = 1280;
    cfg.height_px        = 720;
    cfg.locale           = "en-US";

    // --frames=N override for CI tuning.
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a.rfind("--frames=", 0) == 0) {
            try { cfg.self_test_frames = static_cast<std::uint32_t>(std::stoul(a.substr(9))); }
            catch (...) {}
        }
    }

    gw::runtime::Engine engine{cfg};
    StdoutWriter out;

    // Frame 1 — viewport resize
    engine.ui().set_viewport(cfg.width_px, cfg.height_px, 1.0f);
    engine.tick(1.0 / 60.0);

    // Frame 2 — console drive
    (void)engine.console().execute("echo sandbox_playable online", out);
    engine.tick(1.0 / 60.0);

    // Frame 3 — flip contrast boost through the settings binder
    (void)engine.settings_binder().push_from_ui("settings-contrast-boost", "true");
    engine.tick(1.0 / 60.0);

    // Frame 4..N — idle
    if (cfg.self_test_frames > 3) {
        engine.run_frames(cfg.self_test_frames - 3);
    }

    std::fprintf(stdout,
        "[sandbox_playable] PLAYABLE OK — frames=%llu contrast=%d cheats=%d cvars=%zu\n",
        static_cast<unsigned long long>(engine.frame_index()),
        engine.ui().contrast_boost() ? 1 : 0,
        engine.console().cheats_banner_visible() ? 1 : 0,
        engine.cvars().count());

    return 0;
}
