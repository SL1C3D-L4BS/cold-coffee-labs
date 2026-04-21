// tests/integration/engine_boot_test.cpp — Phase 11 Wave 11F (ADR-0029).

#include <doctest/doctest.h>

#include "runtime/engine.hpp"

using namespace gw::runtime;

TEST_CASE("Engine: boots headless deterministically") {
    EngineConfig cfg{};
    cfg.headless = true;
    cfg.deterministic = true;
    cfg.self_test_frames = 10;
    Engine e(cfg);
    CHECK(e.frame_index() == 0);
    CHECK(e.cvars().count() > 20);
    CHECK(e.ui().document_count() == 0);
    CHECK(e.console().command_count() >= 6);  // register_builtin_commands
}

TEST_CASE("Engine: run_frames ticks deterministic frame counter") {
    EngineConfig cfg{};
    cfg.headless = true;
    cfg.deterministic = true;
    cfg.self_test_frames = 30;
    Engine e(cfg);
    e.run_frames(30);
    CHECK(e.frame_index() == 30);
    CHECK(e.ui().stats().frame_index == 30);
}

TEST_CASE("Engine: console quit command triggers shutdown flag") {
    EngineConfig cfg{};
    cfg.headless = true;
    cfg.deterministic = true;
    Engine e(cfg);
    gw::console::CapturingWriter out;
    (void)e.console().execute("quit", out);
    CHECK(e.should_quit());
}

TEST_CASE("Engine: settings binder write reflects in CVar registry") {
    EngineConfig cfg{};
    cfg.headless = true;
    cfg.deterministic = true;
    Engine e(cfg);
    CHECK(e.settings_binder().push_from_ui("settings-contrast-boost", "true"));
    CHECK(e.cvars().get_bool_or("ui.contrast.boost", false) == true);
}
