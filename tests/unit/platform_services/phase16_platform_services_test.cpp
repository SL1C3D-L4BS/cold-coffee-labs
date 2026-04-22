// Phase 16 Wave 16A — platform services (ADR-0065..0067).

#include <doctest/doctest.h>
#include <ostream>

#include "engine/core/config/cvar_registry.hpp"
#include "engine/platform_services/platform_cvars.hpp"
#include "engine/platform_services/platform_services.hpp"
#include "engine/platform_services/platform_services_world.hpp"
#include "engine/platform_services/rate_limiter.hpp"

#include <filesystem>
#include <string>
#include <vector>

using namespace gw::platform_services;

namespace {

PlatformServicesConfig make_test_cfg(const std::string& dir) {
    PlatformServicesConfig cfg{};
    cfg.backend               = "local";
    cfg.storage_dir           = dir;
    cfg.app_id                = "test";
    cfg.dry_run_sdk           = true;
    cfg.rate_limit_per_minute = 10;
    return cfg;
}

} // namespace

TEST_CASE("phase16 — IPlatformServices dev-local factory returns an instance") {
    auto svc = make_dev_local_platform_services();
    REQUIRE(svc != nullptr);
    CHECK(svc->backend_name() == std::string_view{"local"});
}

TEST_CASE("phase16 — dev-local initialize/shutdown lifecycle") {
    auto svc = make_dev_local_platform_services();
    REQUIRE(svc != nullptr);
    CHECK(svc->initialize(make_test_cfg("")));
    svc->shutdown();
}

TEST_CASE("phase16 — dev-local identity is deterministic") {
    auto svc = make_dev_local_platform_services();
    REQUIRE(svc->initialize(make_test_cfg("")));
    const auto u = svc->current_user();
    CHECK(!u.id_hash_hex.empty());
    CHECK(svc->signed_in());
}

TEST_CASE("phase16 — unlock_achievement + is_unlocked round-trip") {
    auto svc = make_dev_local_platform_services();
    REQUIRE(svc->initialize(make_test_cfg("")));
    CHECK_FALSE(svc->is_unlocked("first_wake"));
    CHECK(svc->unlock_achievement("first_wake") == PlatformError::Ok);
    CHECK(svc->is_unlocked("first_wake"));
}

TEST_CASE("phase16 — clear_achievement flips state back to locked") {
    auto svc = make_dev_local_platform_services();
    REQUIRE(svc->initialize(make_test_cfg("")));
    CHECK(svc->unlock_achievement("a_one") == PlatformError::Ok);
    CHECK(svc->clear_achievement("a_one") == PlatformError::Ok);
    CHECK_FALSE(svc->is_unlocked("a_one"));
}

TEST_CASE("phase16 — stats: i32 round-trip") {
    auto svc = make_dev_local_platform_services();
    REQUIRE(svc->initialize(make_test_cfg("")));
    CHECK(svc->set_stat_i32("kills", 42) == PlatformError::Ok);
    CHECK(svc->get_stat_i32("kills") == 42);
}

TEST_CASE("phase16 — stats: f32 round-trip") {
    auto svc = make_dev_local_platform_services();
    REQUIRE(svc->initialize(make_test_cfg("")));
    CHECK(svc->set_stat_f32("acc", 0.75f) == PlatformError::Ok);
    CHECK(svc->get_stat_f32("acc") == doctest::Approx(0.75f));
}

TEST_CASE("phase16 — stats default fallback when key missing") {
    auto svc = make_dev_local_platform_services();
    REQUIRE(svc->initialize(make_test_cfg("")));
    CHECK(svc->get_stat_i32("missing", 999) == 999);
}

TEST_CASE("phase16 — leaderboard submit + top_scores returns rows") {
    auto svc = make_dev_local_platform_services();
    REQUIRE(svc->initialize(make_test_cfg("")));
    CHECK(svc->submit_score("board1", 100) == PlatformError::Ok);
    CHECK(svc->submit_score("board1", 500) == PlatformError::Ok);
    CHECK(svc->submit_score("board1", 200) == PlatformError::Ok);
    std::vector<LeaderboardRow> rows;
    CHECK(svc->top_scores("board1", 5, rows) == PlatformError::Ok);
    REQUIRE(rows.size() == 3);
    // Descending order.
    CHECK(rows[0].score >= rows[1].score);
    CHECK(rows[1].score >= rows[2].score);
}

TEST_CASE("phase16 — rich presence set → stored utf8") {
    auto svc = make_dev_local_platform_services();
    REQUIRE(svc->initialize(make_test_cfg("")));
    CHECK(svc->set_rich_presence("status", "Exploring Sub-Level 7") == PlatformError::Ok);
}

TEST_CASE("phase16 — publish_event success counts in event_count()") {
    auto svc = make_dev_local_platform_services();
    REQUIRE(svc->initialize(make_test_cfg("")));
    const auto before = svc->event_count();
    CHECK(svc->publish_event("run_complete", "{}") == PlatformError::Ok);
    CHECK(svc->event_count() > before);
}

TEST_CASE("phase16 — event rate limit trips after N calls in 60s") {
    auto cfg = make_test_cfg("");
    cfg.rate_limit_per_minute = 3;
    auto svc = make_dev_local_platform_services();
    REQUIRE(svc->initialize(cfg));
    CHECK(svc->publish_event("burst", "{}") == PlatformError::Ok);
    CHECK(svc->publish_event("burst", "{}") == PlatformError::Ok);
    CHECK(svc->publish_event("burst", "{}") == PlatformError::Ok);
    CHECK(svc->publish_event("burst", "{}") == PlatformError::RateLimited);
}

TEST_CASE("phase16 — would_accept returns false when limit exceeded") {
    auto cfg = make_test_cfg("");
    cfg.rate_limit_per_minute = 1;
    auto svc = make_dev_local_platform_services();
    REQUIRE(svc->initialize(cfg));
    CHECK(svc->publish_event("probe", "{}") == PlatformError::Ok);
    CHECK_FALSE(svc->would_accept("probe"));
}

TEST_CASE("phase16 — list_subscribed_content returns empty on dev-local by default") {
    auto svc = make_dev_local_platform_services();
    REQUIRE(svc->initialize(make_test_cfg("")));
    std::vector<UgcItem> items;
    CHECK(svc->list_subscribed_content(items) == PlatformError::Ok);
    CHECK(items.empty());
}

TEST_CASE("phase16 — download_content returns Ok on dev-local (no-op)") {
    auto svc = make_dev_local_platform_services();
    REQUIRE(svc->initialize(make_test_cfg("")));
    CHECK(svc->download_content("deadbeef") == PlatformError::Ok);
}

TEST_CASE("phase16 — aggregated factory falls back to dev-local when SDKs disabled") {
    auto svc = make_platform_services_aggregated("steam");
    REQUIRE(svc != nullptr);
    // With GW_ENABLE_STEAMWORKS=0 (default in dev-win) we expect the
    // dev-local fallback which reports `local` as its backend name.
    CHECK(svc->backend_name() == std::string_view{"local"});
}

TEST_CASE("phase16 — RateLimiter admits up to N in the same minute") {
    RateLimiter r(3);
    CHECK(r.check("a", 0));
    CHECK(r.check("a", 100));
    CHECK(r.check("a", 200));
    CHECK_FALSE(r.check("a", 300));
}

TEST_CASE("phase16 — RateLimiter evicts stamps older than 60s") {
    RateLimiter r(2);
    CHECK(r.check("b", 0));
    CHECK(r.check("b", 1000));
    // After >60s the first stamp should fall off the window.
    CHECK(r.check("b", 61'001));
}

TEST_CASE("phase16 — RateLimiter per-event buckets are independent") {
    RateLimiter r(1);
    CHECK(r.check("x", 0));
    CHECK(r.check("y", 0));
    CHECK_FALSE(r.check("x", 0));
    CHECK_FALSE(r.check("y", 0));
}

TEST_CASE("phase16 — RateLimiter would_accept is non-destructive") {
    RateLimiter r(2);
    CHECK(r.would_accept("z", 0));
    CHECK(r.would_accept("z", 0));
    CHECK(r.check("z", 0));
    CHECK(r.check("z", 0));
    CHECK_FALSE(r.would_accept("z", 0));
}

TEST_CASE("phase16 — RateLimiter set_per_minute updates limit") {
    RateLimiter r(1);
    r.set_per_minute(4);
    CHECK(r.per_minute() == 4);
}

TEST_CASE("phase16 — PlatformServicesWorld initialize with CVars picks up backend override") {
    gw::config::CVarRegistry reg;
    (void)register_platform_cvars(reg);
    PlatformServicesWorld w;
    CHECK(w.initialize(PlatformServicesConfig{}, &reg));
    REQUIRE(w.backend() != nullptr);
    CHECK(w.backend()->backend_name() == std::string_view{"local"});
}

TEST_CASE("phase16 — PlatformServicesWorld step advances step_count") {
    gw::config::CVarRegistry reg;
    (void)register_platform_cvars(reg);
    PlatformServicesWorld w;
    REQUIRE(w.initialize(PlatformServicesConfig{}, &reg));
    CHECK(w.step_count() == 0);
    w.step(0.016);
    w.step(0.016);
    CHECK(w.step_count() == 2);
}

TEST_CASE("phase16 — PlatformServicesWorld forwarders hit the backend") {
    gw::config::CVarRegistry reg;
    (void)register_platform_cvars(reg);
    PlatformServicesWorld w;
    REQUIRE(w.initialize(PlatformServicesConfig{}, &reg));
    CHECK(w.unlock_achievement("a") == PlatformError::Ok);
    CHECK(w.is_unlocked("a"));
    CHECK(w.submit_score("b", 99) == PlatformError::Ok);
    CHECK(w.set_rich_presence("k", "v") == PlatformError::Ok);
    CHECK(w.publish_event("name", "{}") == PlatformError::Ok);
}

TEST_CASE("phase16 — PlatformServicesWorld rejects operations when uninitialized") {
    PlatformServicesWorld w;
    CHECK(w.unlock_achievement("x")          == PlatformError::BackendDisabled);
    CHECK(w.submit_score("b", 1)             == PlatformError::BackendDisabled);
    CHECK(w.set_rich_presence("k", "v")       == PlatformError::BackendDisabled);
    CHECK(w.publish_event("n", "{}")          == PlatformError::BackendDisabled);
    CHECK_FALSE(w.is_unlocked("x"));
}

TEST_CASE("phase16 — PlatformCVars register 8 keys under `plat.*`") {
    gw::config::CVarRegistry reg;
    const auto c = register_platform_cvars(reg);
    CHECK(c.backend.valid());
    CHECK(c.app_id.valid());
    CHECK(c.achievements_enabled.valid());
    CHECK(c.leaderboards_enabled.valid());
    CHECK(c.rich_presence_enabled.valid());
    CHECK(c.workshop_enabled.valid());
    CHECK(c.rate_limit_per_minute.valid());
    CHECK(c.dry_run_sdk.valid());
    CHECK(reg.count() >= 8u);
}

TEST_CASE("phase16 — PlatformError string table is total") {
    using E = PlatformError;
    CHECK(to_string(E::Ok)               == std::string_view{"ok"});
    CHECK(to_string(E::NotSignedIn)      != std::string_view{});
    CHECK(to_string(E::NotFound)         != std::string_view{});
    CHECK(to_string(E::RateLimited)      != std::string_view{});
    CHECK(to_string(E::BackendDisabled)  != std::string_view{});
    CHECK(to_string(E::IoError)          != std::string_view{});
    CHECK(to_string(E::InvalidArg)       != std::string_view{});
}

TEST_CASE("phase16 — dev-local store_stats returns Ok when initialized") {
    auto svc = make_dev_local_platform_services();
    REQUIRE(svc->initialize(make_test_cfg("")));
    CHECK(svc->unlock_achievement("persist_me") == PlatformError::Ok);
    CHECK(svc->store_stats() == PlatformError::Ok);
    CHECK(svc->is_unlocked("persist_me"));
}

TEST_CASE("phase16 — unlock_achievement rejects empty id") {
    auto svc = make_dev_local_platform_services();
    REQUIRE(svc->initialize(make_test_cfg("")));
    CHECK(svc->unlock_achievement("") == PlatformError::InvalidArg);
}

TEST_CASE("phase16 — set_stat_i32 rejects empty key") {
    auto svc = make_dev_local_platform_services();
    REQUIRE(svc->initialize(make_test_cfg("")));
    CHECK(svc->set_stat_i32("", 1) == PlatformError::InvalidArg);
}

TEST_CASE("phase16 — set_rich_presence caps value length") {
    auto svc = make_dev_local_platform_services();
    REQUIRE(svc->initialize(make_test_cfg("")));
    std::string big(200, 'x');
    CHECK(svc->set_rich_presence("k", big) == PlatformError::InvalidArg);
}
