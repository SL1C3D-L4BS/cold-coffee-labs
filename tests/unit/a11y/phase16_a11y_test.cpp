// Phase 16 Wave 16E — accessibility (ADR-0071, 0072).

#include <doctest/doctest.h>
#include <ostream>

#include "engine/a11y/a11y_cvars.hpp"
#include "engine/a11y/a11y_world.hpp"
#include "engine/a11y/color_matrices.hpp"
#include "engine/a11y/screen_reader.hpp"
#include "engine/a11y/selfcheck.hpp"
#include "engine/a11y/subtitles.hpp"
#include "engine/core/config/cvar_registry.hpp"

#include <string>
#include <vector>

using namespace gw::a11y;

TEST_CASE("phase16 — parse_color_mode covers every token") {
    CHECK(parse_color_mode("none")    == ColorMode::None);
    CHECK(parse_color_mode("protan")  == ColorMode::Protanopia);
    CHECK(parse_color_mode("deutan")  == ColorMode::Deuteranopia);
    CHECK(parse_color_mode("tritan")  == ColorMode::Tritanopia);
    CHECK(parse_color_mode("hc")      == ColorMode::HighContrast);
    CHECK(parse_color_mode("garbage") == ColorMode::None);
}

TEST_CASE("phase16 — color_matrix None is the identity") {
    const auto m = color_matrix_for(ColorMode::None);
    CHECK(m.m[0] == doctest::Approx(1.0f));
    CHECK(m.m[4] == doctest::Approx(1.0f));
    CHECK(m.m[8] == doctest::Approx(1.0f));
}

TEST_CASE("phase16 — color_matrix Protanopia matches ADR-0071 freeze") {
    const auto m = color_matrix_for(ColorMode::Protanopia);
    CHECK(m.m[0] == doctest::Approx(0.152f));
    CHECK(m.m[1] == doctest::Approx(1.053f));
    CHECK(m.m[2] == doctest::Approx(-0.205f));
}

TEST_CASE("phase16 — color_matrix HighContrast scales 1.5x") {
    const auto m = color_matrix_for(ColorMode::HighContrast);
    CHECK(m.m[0] == doctest::Approx(1.5f));
    CHECK(m.m[4] == doctest::Approx(1.5f));
    CHECK(m.m[8] == doctest::Approx(1.5f));
}

TEST_CASE("phase16 — color_matrix strength blends towards identity") {
    const auto m = color_matrix_for(ColorMode::Protanopia, 0.0f);
    CHECK(m.m[0] == doctest::Approx(1.0f));
    CHECK(m.m[4] == doctest::Approx(1.0f));
    const auto full = color_matrix_for(ColorMode::Protanopia, 1.0f);
    CHECK(full.m[0] == doctest::Approx(0.152f));
    const auto mid = color_matrix_for(ColorMode::Protanopia, 0.5f);
    CHECK(mid.m[0] == doctest::Approx((1.0f + 0.152f) * 0.5f));
}

TEST_CASE("phase16 — SubtitleQueue push/tick/snapshot roundtrip") {
    SubtitleQueue q;
    q.set_max_lines(3);
    SubtitleCue c{};
    c.cue_id = 1;
    c.speaker = "Mara";
    c.text_utf8 = "Stay sharp.";
    c.duration_ms = 1000;
    CHECK(q.push(std::move(c), 0));
    std::vector<SubtitleLine> lines;
    q.snapshot(lines, 100);
    REQUIRE(lines.size() == 1u);
    CHECK(lines[0].speaker == std::string{"Mara"});
    q.tick(2000);
    CHECK(q.active_count() == 0u);
}

TEST_CASE("phase16 — SubtitleQueue drops lowest priority on overflow") {
    SubtitleQueue q;
    q.set_max_lines(2);
    SubtitleCue a{}; a.priority = 10; a.cue_id = 1; a.duration_ms = 5000;
    SubtitleCue b{}; b.priority =  5; b.cue_id = 2; b.duration_ms = 5000;
    SubtitleCue c{}; c.priority =  1; c.cue_id = 3; c.duration_ms = 5000;
    CHECK(q.push(std::move(a), 0));
    CHECK(q.push(std::move(b), 100));
    CHECK_FALSE(q.push(std::move(c), 200));
    CHECK(q.active_count() == 2u);
}

TEST_CASE("phase16 — SubtitleQueue clamps max_lines to [1,6]") {
    SubtitleQueue q;
    q.set_max_lines(0);
    CHECK(q.max_lines() == 1);
    q.set_max_lines(99);
    CHECK(q.max_lines() == 6);
}

TEST_CASE("phase16 — null screen reader receives announcements") {
    auto sr = make_null_screen_reader();
    REQUIRE(sr);
    sr->announce_text("Low oxygen", Politeness::Assertive);
    CHECK(sr->announce_count() == 1u);
    CHECK(sr->last_announcement() == std::string_view{"Low oxygen"});
}

TEST_CASE("phase16 — null screen reader tree update stores nodes") {
    auto sr = make_null_screen_reader();
    std::vector<AccessibleNode> nodes;
    nodes.push_back({1, Role::Button, "OK", "Confirm", 0, 0, 80, 32, true, true, 0, {}});
    sr->update_tree(nodes);
    CHECK(sr->tree_size() == 1u);
}

TEST_CASE("phase16 — screen_reader_aggregated returns a usable backend") {
    auto sr = make_screen_reader_aggregated(false);
    REQUIRE(sr);
    CHECK(!sr->backend_name().empty());
}

TEST_CASE("phase16 — A11yWorld initialize with CVars") {
    gw::config::CVarRegistry reg;
    (void)register_a11y_cvars(reg);
    A11yWorld w;
    REQUIRE(w.initialize(A11yConfig{}, &reg));
    CHECK(w.initialized());
    CHECK(w.text_scale() == doctest::Approx(1.0f));
}

TEST_CASE("phase16 — A11yWorld set_color_mode switches the transform") {
    A11yWorld w;
    REQUIRE(w.initialize(A11yConfig{}, nullptr));
    w.set_color_mode(ColorMode::Deuteranopia);
    const auto m = w.active_color_transform();
    CHECK(m.m[0] == doctest::Approx(0.367f));
}

TEST_CASE("phase16 — A11yWorld clamps text_scale to [0.5, 2.5]") {
    A11yWorld w;
    REQUIRE(w.initialize(A11yConfig{}, nullptr));
    w.set_text_scale(99.0f);
    CHECK(w.text_scale() == doctest::Approx(2.5f));
    w.set_text_scale(-1.0f);
    CHECK(w.text_scale() == doctest::Approx(0.5f));
}

TEST_CASE("phase16 — A11yWorld honors subtitle gate CVar") {
    A11yWorld w;
    A11yConfig cfg{};
    cfg.subtitles_enabled = false;
    REQUIRE(w.initialize(cfg, nullptr));
    SubtitleCue c{}; c.text_utf8 = "X"; c.duration_ms = 500;
    CHECK_FALSE(w.push_subtitle(std::move(c), 0));
}

TEST_CASE("phase16 — A11yWorld subtitles feed queue") {
    A11yWorld w;
    REQUIRE(w.initialize(A11yConfig{}, nullptr));
    for (int i = 0; i < 3; ++i) {
        SubtitleCue c{}; c.cue_id = static_cast<std::uint64_t>(i + 1);
        c.text_utf8 = "line"; c.duration_ms = 1000;
        w.push_subtitle(std::move(c), 0);
    }
    CHECK(w.subtitle_active_count() >= 1u);
    CHECK(w.subtitle_emitted_count() >= 1u);
}

TEST_CASE("phase16 — A11yWorld announce only when screen_reader_enabled") {
    A11yWorld w;
    A11yConfig cfg{};
    cfg.screen_reader_enabled = false;
    REQUIRE(w.initialize(cfg, nullptr));
    w.announce("silent", Politeness::Polite);
    CHECK(w.screen_reader()->announce_count() == 0u);
    cfg.screen_reader_enabled = true;
    REQUIRE(w.initialize(cfg, nullptr));
    w.announce("loud", Politeness::Assertive);
    CHECK(w.screen_reader()->announce_count() == 1u);
}

TEST_CASE("phase16 — A11yCVars registers 16 keys") {
    gw::config::CVarRegistry reg;
    const auto c = register_a11y_cvars(reg);
    CHECK(c.color_mode.valid());
    CHECK(c.text_scale.valid());
    CHECK(c.reduce_motion.valid());
    CHECK(c.subtitle_enabled.valid());
    CHECK(c.focus_show_ring.valid());
    CHECK(c.screen_reader_enabled.valid());
    CHECK(reg.count() >= 16u);
}

TEST_CASE("phase16 — selfcheck passes the default context") {
    A11yWorld w;
    REQUIRE(w.initialize(A11yConfig{}, nullptr));
    SelfCheckContext ctx{};
    ctx.min_text_contrast_ratio = 7.5f;
    ctx.max_flash_rate_hz       = 0.5f;
    ctx.focus_ring_visible      = true;
    ctx.tab_order_visual        = true;
    ctx.subtitle_coverage       = 1.0f;
    const auto r = w.selfcheck(ctx);
    CHECK(r.is_green());
    CHECK(r.pass_count > 0u);
}

TEST_CASE("phase16 — selfcheck fails on low contrast") {
    A11yWorld w;
    REQUIRE(w.initialize(A11yConfig{}, nullptr));
    SelfCheckContext ctx{};
    ctx.min_text_contrast_ratio = 2.0f;
    ctx.max_flash_rate_hz       = 0.0f;
    ctx.focus_ring_visible      = true;
    ctx.tab_order_visual        = true;
    ctx.subtitle_coverage       = 1.0f;
    const auto r = w.selfcheck(ctx);
    CHECK_FALSE(r.is_green());
}

TEST_CASE("phase16 — selfcheck fails when focus not visible") {
    A11yWorld w;
    REQUIRE(w.initialize(A11yConfig{}, nullptr));
    SelfCheckContext ctx{};
    ctx.min_text_contrast_ratio = 7.5f;
    ctx.focus_ring_visible      = false;
    ctx.tab_order_visual        = true;
    ctx.subtitle_coverage       = 1.0f;
    const auto r = w.selfcheck(ctx);
    CHECK_FALSE(r.is_green());
}

TEST_CASE("phase16 — selfcheck fails on flash > 3 Hz without photosafe") {
    A11yWorld w;
    REQUIRE(w.initialize(A11yConfig{}, nullptr));
    SelfCheckContext ctx{};
    ctx.min_text_contrast_ratio = 7.5f;
    ctx.max_flash_rate_hz       = 5.0f;
    ctx.focus_ring_visible      = true;
    ctx.tab_order_visual        = true;
    ctx.subtitle_coverage       = 1.0f;
    const auto r = w.selfcheck(ctx);
    CHECK_FALSE(r.is_green());
}

TEST_CASE("phase16 — selfcheck detects unnamed nodes") {
    A11yWorld w;
    REQUIRE(w.initialize(A11yConfig{}, nullptr));
    std::vector<AccessibleNode> nodes;
    nodes.push_back({1, Role::Unknown, {}, {}, 0,0,10,10, false, true, 0, {}});
    SelfCheckContext ctx{};
    ctx.tree = std::span<const AccessibleNode>(nodes);
    ctx.min_text_contrast_ratio = 7.5f;
    ctx.focus_ring_visible = true;
    ctx.tab_order_visual = true;
    ctx.subtitle_coverage = 1.0f;
    const auto r = w.selfcheck(ctx);
    CHECK_FALSE(r.is_green());
}

TEST_CASE("phase16 — A11yWorld step increments step_count") {
    A11yWorld w;
    REQUIRE(w.initialize(A11yConfig{}, nullptr));
    w.step(0.016, 0);
    w.step(0.016, 100);
    CHECK(w.step_count() == 2u);
}

TEST_CASE("phase16 — Politeness and Role to_string cover every enum") {
    CHECK(to_string(Politeness::Off)       == std::string_view{"off"});
    CHECK(to_string(Politeness::Polite)    == std::string_view{"polite"});
    CHECK(to_string(Politeness::Assertive) == std::string_view{"assertive"});
    CHECK(to_string(Role::Unknown)   == std::string_view{"unknown"});
    CHECK(to_string(Role::Button)    == std::string_view{"button"});
    CHECK(to_string(Role::CheckBox)  == std::string_view{"checkbox"});
}

TEST_CASE("phase16 — null screen reader announces focus by id") {
    auto sr = make_null_screen_reader();
    std::vector<AccessibleNode> nodes;
    nodes.push_back({42, Role::Button, "Start", {}, 0,0,0,0, false, true, 0, {}});
    sr->update_tree(nodes);
    sr->announce_focus(42);
    CHECK(sr->last_announcement() == std::string_view{"Start"});
}
