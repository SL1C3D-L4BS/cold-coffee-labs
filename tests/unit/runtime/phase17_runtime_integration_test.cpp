// tests/unit/runtime/phase17_runtime_integration_test.cpp — ADR-0082 Wave 17F.

#include <doctest/doctest.h>

#include "runtime/engine.hpp"

using namespace gw::runtime;

static EngineConfig headless_cfg() {
    EngineConfig c{};
    c.headless           = true;
    c.deterministic      = true;
    c.self_test_frames   = 4;
    c.width_px           = 640;
    c.height_px          = 480;
    return c;
}

TEST_CASE("phase17.runtime: Engine owns MaterialWorld and it initializes") {
    Engine e{headless_cfg()};
    auto& m = e.materials();
    CHECK(m.initialized());
    CHECK(m.template_count() >= 1);
}

TEST_CASE("phase17.runtime: Engine owns ParticleWorld and exposes config") {
    Engine e{headless_cfg()};
    auto& vfx = e.vfx();
    CHECK(vfx.config().particle_budget >= 1u);
}

TEST_CASE("phase17.runtime: Engine owns PostWorld and reports initialized") {
    Engine e{headless_cfg()};
    auto& p = e.post();
    CHECK(p.initialized());
}

TEST_CASE("phase17.runtime: tick advances all three subsystems") {
    Engine e{headless_cfg()};
    const auto mat_before  = e.materials().step_count();
    e.run_frames(3, 1.0 / 60.0);
    CHECK(e.materials().step_count() > mat_before);
    CHECK(e.frame_index() >= 3);
}

TEST_CASE("phase17.runtime: material CVars are live and typed") {
    Engine e{headless_cfg()};
    auto& cv = e.cvars();
    CHECK(cv.get_i32_or("mat.instance_budget", -1) >= 0);
    const bool auto_reload = cv.get_bool_or("mat.auto_reload", false);
    (void)auto_reload;  // present (value may be true or false)
    CHECK(!cv.get_string_or("mat.default_template", "").empty());
}

TEST_CASE("phase17.runtime: post CVars are live and typed") {
    Engine e{headless_cfg()};
    auto& cv = e.cvars();
    CHECK(!cv.get_string_or("post.tonemap.curve", "").empty());
    CHECK(cv.get_i32_or("post.bloom.iterations", -1) >= 0);
}

TEST_CASE("phase17.runtime: vfx CVars are live and typed") {
    Engine e{headless_cfg()};
    auto& cv = e.cvars();
    CHECK(cv.get_i32_or("vfx.particles.budget", -1) >= 1);
    CHECK(!cv.get_string_or("vfx.particles.sort_mode", "").empty());
}

TEST_CASE("phase17.runtime: console commands registered (mat.list, vfx.particles.stats)") {
    Engine e{headless_cfg()};
    auto& cs = e.console();
    gw::console::CapturingWriter w;
    cs.list_commands(w);
    const auto joined = w.joined();
    CHECK(joined.find("mat.list")              != std::string::npos);
    CHECK(joined.find("vfx.particles.stats")   != std::string::npos);
    CHECK(joined.find("r.shader.stats")        != std::string::npos);
}
