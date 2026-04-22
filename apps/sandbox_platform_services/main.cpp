// apps/sandbox_platform_services/main.cpp — Phase 16 SHIP READY exit gate (ADR-0073).

#include "runtime/engine.hpp"

#include "engine/a11y/a11y_world.hpp"
#include "engine/a11y/selfcheck.hpp"
#include "engine/i18n/i18n_world.hpp"
#include "engine/platform_services/platform_services_world.hpp"

#include <cstdio>
#include <string>
#include <utility>
#include <vector>

namespace {

std::vector<std::pair<std::string, std::string>> en_kv() {
    return {
        {"menu.continue", "Continue"},
        {"menu.newgame",  "New Game"},
        {"hud.fps.suffix","fps"},
        {"a11y.focus_announce", "focused: {0}"},
    };
}
std::vector<std::pair<std::string, std::string>> fr_kv() {
    return {
        {"menu.continue", "Continuer"},
        {"menu.newgame",  "Nouvelle partie"},
    };
}
std::vector<std::pair<std::string, std::string>> de_kv() {
    return {
        {"menu.continue", "Fortsetzen"},
        {"menu.newgame",  "Neues Spiel"},
    };
}
std::vector<std::pair<std::string, std::string>> ja_kv() {
    return {
        {"menu.continue", "\xe7\xb6\x9a\xe3\x81\x91\xe3\x82\x8b"},
        {"menu.newgame",  "\xe6\x96\xb0\xe3\x81\x97\xe3\x81\x84\xe3\x82\xb2\xe3\x83\xbc\xe3\x83\xa0"},
    };
}
std::vector<std::pair<std::string, std::string>> zh_kv() {
    return {
        {"menu.continue", "\xe7\xbb\xa7\xe7\xbb\xad"},
        {"menu.newgame",  "\xe6\x96\xb0\xe6\xb8\xb8\xe6\x88\x8f"},
    };
}

} // namespace

int main() {
    gw::runtime::EngineConfig ecfg{};
    ecfg.headless         = true;
    ecfg.deterministic    = true;
    ecfg.self_test_frames = 30;
    gw::runtime::Engine engine(ecfg);

    // ------ Wave 16A/B: Platform Services ------
    auto& ps = engine.platform();
    if (ps.initialized() && ps.backend()) {
        (void)ps.unlock_achievement("phase16_first_wake");
        (void)ps.submit_score("sandbox_board", 12345);
        (void)ps.set_rich_presence("status", "Sandbox 16");
        (void)ps.publish_event("session_open", "{}");
    }

    // ------ Wave 16C/D: i18n tables ------
    auto& lw = engine.i18n();
    lw.install_inline_table("en-US", en_kv());
    lw.install_inline_table("fr-FR", fr_kv());
    lw.install_inline_table("de-DE", de_kv());
    lw.install_inline_table("ja-JP", ja_kv());
    lw.install_inline_table("zh-CN", zh_kv());

    const std::size_t locales_loaded = lw.table_count();

    // Flip through locales once each to demonstrate MessageFormat usage.
    for (const char* tag : {"en-US", "fr-FR", "de-DE", "ja-JP", "zh-CN"}) {
        (void)lw.set_locale(tag);
        const auto greeting = lw.resolve("menu.continue");
        (void)greeting;
    }

    // ------ Wave 16E: a11y ------
    auto& aw = engine.a11y();
    aw.set_color_mode(gw::a11y::ColorMode::Deuteranopia, 0.85f);
    aw.set_text_scale(1.25f);

    gw::a11y::SubtitleCue cue{};
    cue.cue_id = 1;
    cue.speaker = "Mara";
    cue.text_utf8 = "Containment breach sealed.";
    cue.duration_ms = 3000;
    (void)aw.push_subtitle(std::move(cue), 0);

    gw::a11y::SelfCheckContext sc{};
    sc.min_text_contrast_ratio = 7.5f;
    sc.max_flash_rate_hz       = 0.0f;
    sc.focus_ring_visible      = true;
    sc.tab_order_visual        = true;
    sc.subtitle_coverage       = 1.0f;
    sc.effective_body_pt       = 16.0f;
    const auto report = aw.selfcheck(sc);

    engine.run_frames(ecfg.self_test_frames);

    const bool steam_on = false;       // gated by GW_ENABLE_STEAMWORKS
    const bool eos_on   = false;       // gated by GW_ENABLE_EOS
    const bool wcag_aa  = report.is_green();

    std::printf("phase16 locales_loaded=%zu selfcheck_pass=%u selfcheck_warn=%u selfcheck_fail=%u\n",
                 locales_loaded, report.pass_count, report.warn_count, report.fail_count);
    if (!wcag_aa || locales_loaded < 5) {
        std::printf("SHIP NOT READY — steam=%d eos=%d wcag22=%s i18n_locales=%zu a11y_selfcheck=%s\n",
                     static_cast<int>(steam_on), static_cast<int>(eos_on),
                     wcag_aa ? "AA" : "FAIL",
                     locales_loaded,
                     wcag_aa ? "green" : "red");
        return 1;
    }

    std::printf("SHIP READY - steam=%s eos=%s wcag22=AA i18n_locales=%zu a11y_selfcheck=green\n",
                 steam_on ? "\xe2\x9c\x93" : "skip",
                 eos_on ? "\xe2\x9c\x93" : "skip",
                 locales_loaded);
    return 0;
}
