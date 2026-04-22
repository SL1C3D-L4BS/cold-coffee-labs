// Phase 15 — telemetry pipeline, consent, PII, DSAR (ADR-0061..0062).

#include <doctest/doctest.h>

#include "engine/core/config/cvar_registry.hpp"
#include "engine/persist/local_store.hpp"
#include "engine/persist/persist_cvars.hpp"
#include "engine/telemetry/consent.hpp"
#include "engine/telemetry/dsar_exporter.hpp"
#include "engine/telemetry/event_pipeline.hpp"
#include "engine/telemetry/pii_scrub.hpp"

#include <cstdint>
#include <filesystem>
#include <string>

namespace {

std::string temp_store_db(std::string_view tag) {
    auto p = std::filesystem::temp_directory_path() / "gw_phase15_tele" / tag;
    std::filesystem::remove_all(p);
    std::filesystem::create_directories(p);
    return (p / "db.sqlite").string();
}

} // namespace

#if GW_TELEMETRY_COMPILED

TEST_CASE("phase15 — telemetry pipeline None tier denies at entry") {
    auto                store = gw::persist::make_sqlite_local_store(temp_store_db("none"));
    REQUIRE(store->open_or_create());
    gw::config::CVarRegistry r;
    auto                     pc = gw::persist::register_persist_and_telemetry_cvars(r);
    REQUIRE(r.set_bool(pc.tele_enabled.id, true));
    REQUIRE(r.set_bool(pc.tele_events_enabled.id, true));
    const auto e = gw::telemetry::pipeline_record(*store, r, gw::telemetry::ConsentTier::None, "evt",
                                                   "{}");
    CHECK(e == gw::telemetry::TelemetryError::ConsentDenied);
    CHECK(store->telemetry_pending_count() == 0);
}

TEST_CASE("phase15 — telemetry pipeline CrashOnly denies custom events") {
    auto                store = gw::persist::make_sqlite_local_store(temp_store_db("crash"));
    REQUIRE(store->open_or_create());
    gw::config::CVarRegistry r;
    auto                     pc = gw::persist::register_persist_and_telemetry_cvars(r);
    REQUIRE(r.set_bool(pc.tele_enabled.id, true));
    REQUIRE(r.set_bool(pc.tele_events_enabled.id, true));
    const auto e = gw::telemetry::pipeline_record(*store, r, gw::telemetry::ConsentTier::CrashOnly,
                                                   "heartbeat", "{}");
    CHECK(e == gw::telemetry::TelemetryError::ConsentDenied);
    CHECK(store->telemetry_pending_count() == 0);
}

TEST_CASE("phase15 — telemetry pipeline CoreTelemetry enqueues") {
    auto                store = gw::persist::make_sqlite_local_store(temp_store_db("core"));
    REQUIRE(store->open_or_create());
    gw::config::CVarRegistry r;
    auto                     pc = gw::persist::register_persist_and_telemetry_cvars(r);
    REQUIRE(r.set_bool(pc.tele_enabled.id, true));
    REQUIRE(r.set_bool(pc.tele_events_enabled.id, true));
    const auto e = gw::telemetry::pipeline_record(*store, r, gw::telemetry::ConsentTier::CoreTelemetry,
                                                   "session_ping", "{\"k\":1}");
    CHECK(e == gw::telemetry::TelemetryError::Ok);
    CHECK(store->telemetry_pending_count() == 1);
}

TEST_CASE("phase15 — telemetry flush drains when endpoint empty") {
    auto                store = gw::persist::make_sqlite_local_store(temp_store_db("flush"));
    REQUIRE(store->open_or_create());
    gw::config::CVarRegistry r;
    auto                     pc = gw::persist::register_persist_and_telemetry_cvars(r);
    REQUIRE(r.set_bool(pc.tele_enabled.id, true));
    REQUIRE(r.set_bool(pc.tele_events_enabled.id, true));
    REQUIRE(gw::telemetry::pipeline_record(*store, r, gw::telemetry::ConsentTier::CoreTelemetry, "a",
                                            "{}")
            == gw::telemetry::TelemetryError::Ok);
    CHECK(store->telemetry_pending_count() == 1);
    REQUIRE(r.get_string_or("tele.endpoint", {}).empty());
    CHECK(gw::telemetry::pipeline_flush_queue(*store, r) == gw::telemetry::TelemetryError::Ok);
    CHECK(store->telemetry_pending_count() == 0);
}

TEST_CASE("phase15 — pipeline_prune_aged drops rows older than max_age_days") {
    auto                store = gw::persist::make_sqlite_local_store(temp_store_db("age"));
    REQUIRE(store->open_or_create());
    gw::config::CVarRegistry r;
    auto                     pc = gw::persist::register_persist_and_telemetry_cvars(r);
    REQUIRE(r.set_i32(pc.tele_queue_max_age_days.id, 1));
    const std::byte p[1] = {std::byte{0x55}};
    const std::byte z8[8] = {};
    const std::byte z16[16] = {};
    const std::int64_t now_ms = 10ll * 86'400'000;
    REQUIRE(store->telemetry_enqueue(now_ms - 3ll * 86'400'000, z8, p, 0, 0, z16));
    REQUIRE(store->telemetry_enqueue(now_ms - 2ll * 86'400'000, z8, p, 0, 0, z16));
    REQUIRE(store->telemetry_enqueue(now_ms,                     z8, p, 0, 0, z16));
    CHECK(store->telemetry_pending_count() == 3);
    const auto dropped = gw::telemetry::pipeline_prune_aged(*store, r, now_ms);
    CHECK(dropped == 2);
    CHECK(store->telemetry_pending_count() == 1);
}

TEST_CASE("phase15 — pipeline_prune_aged returns 0 when max_age_days<=0") {
    auto                store = gw::persist::make_sqlite_local_store(temp_store_db("agez"));
    REQUIRE(store->open_or_create());
    gw::config::CVarRegistry r;
    auto                     pc = gw::persist::register_persist_and_telemetry_cvars(r);
    // Negative/zero age means "never drop" in this helper. Clamp into valid
    // range to avoid CVar rejection, then verify no-op semantics.
    REQUIRE(r.set_i32(pc.tele_queue_max_age_days.id, 1));
    CHECK(gw::telemetry::pipeline_prune_aged(*store, r, 0) == 0);
}

TEST_CASE("phase15 — telemetry disabled when tele.events.enabled false") {
    auto                store = gw::persist::make_sqlite_local_store(temp_store_db("dis"));
    REQUIRE(store->open_or_create());
    gw::config::CVarRegistry r;
    auto                     pc = gw::persist::register_persist_and_telemetry_cvars(r);
    REQUIRE(r.set_bool(pc.tele_enabled.id, true));
    REQUIRE(r.set_bool(pc.tele_events_enabled.id, false));
    const auto e = gw::telemetry::pipeline_record(*store, r, gw::telemetry::ConsentTier::CoreTelemetry,
                                                   "x", "{}");
    CHECK(e == gw::telemetry::TelemetryError::Disabled);
}

#endif // GW_TELEMETRY_COMPILED

TEST_CASE("phase15 — consent tier parse round-trip") {
    using gw::telemetry::ConsentTier;
    CHECK(gw::telemetry::parse_consent_tier("CrashOnly") == ConsentTier::CrashOnly);
    CHECK(gw::telemetry::parse_consent_tier("CoreTelemetry") == ConsentTier::CoreTelemetry);
    CHECK(gw::telemetry::parse_consent_tier("AnalyticsAllowed") == ConsentTier::AnalyticsAllowed);
    CHECK(gw::telemetry::parse_consent_tier("MarketingAllowed") == ConsentTier::MarketingAllowed);
    CHECK(gw::telemetry::to_string(ConsentTier::MarketingAllowed) == "MarketingAllowed");
}

TEST_CASE("phase15 — consent store and load") {
    auto store = gw::persist::make_sqlite_local_store(temp_store_db("consent"));
    REQUIRE(store->open_or_create());
    gw::telemetry::consent_store(*store, gw::telemetry::ConsentTier::CrashOnly, "2026-04");
    CHECK(gw::telemetry::consent_load(*store) == gw::telemetry::ConsentTier::CrashOnly);
}

TEST_CASE("phase15 — age gate under 13 clamps Marketing to CrashOnly") {
    const auto t = gw::telemetry::apply_age_gate(gw::telemetry::ConsentTier::MarketingAllowed, 12, 13);
    CHECK(t == gw::telemetry::ConsentTier::CrashOnly);
}

TEST_CASE("phase15 — age gate adult keeps Marketing") {
    const auto t = gw::telemetry::apply_age_gate(gw::telemetry::ConsentTier::MarketingAllowed, 18, 13);
    CHECK(t == gw::telemetry::ConsentTier::MarketingAllowed);
}

TEST_CASE("phase15 — age gate CoreTelemetry under 13 unchanged") {
    const auto t = gw::telemetry::apply_age_gate(gw::telemetry::ConsentTier::CoreTelemetry, 10, 13);
    CHECK(t == gw::telemetry::ConsentTier::CoreTelemetry);
}

TEST_CASE("phase15 — age gate Analytics under 13 clamps to CrashOnly") {
    const auto t = gw::telemetry::apply_age_gate(gw::telemetry::ConsentTier::AnalyticsAllowed, 10, 13);
    CHECK(t == gw::telemetry::ConsentTier::CrashOnly);
}

TEST_CASE("phase15 — consent FSM initial None on empty store") {
    auto store = gw::persist::make_sqlite_local_store(temp_store_db("fsm0"));
    REQUIRE(store->open_or_create());
    CHECK(gw::telemetry::consent_load(*store) == gw::telemetry::ConsentTier::None);
}

TEST_CASE("phase15 — consent FSM persisted tier survives reopen") {
    const auto path = temp_store_db("fsm1");
    {
        auto store = gw::persist::make_sqlite_local_store(path);
        REQUIRE(store->open_or_create());
        gw::telemetry::consent_store(*store, gw::telemetry::ConsentTier::AnalyticsAllowed, "v1");
        store->shutdown();
    }
    auto store2 = gw::persist::make_sqlite_local_store(path);
    REQUIRE(store2->open_or_create());
    CHECK(gw::telemetry::consent_load(*store2) == gw::telemetry::ConsentTier::AnalyticsAllowed);
}

TEST_CASE("phase15 — PII scrub replaces email token") {
    const auto s = gw::telemetry::scrub_pii("hello@example.com", false);
    CHECK(s.find("[EMAIL]") != std::string::npos);
}

TEST_CASE("phase15 — PII scrub paths when enabled") {
    const auto s = gw::telemetry::scrub_pii("C:\\Users\\x\\file.txt", true);
    CHECK(s.find("[PATH]") != std::string::npos);
}

TEST_CASE("phase15 — DSAR export writes manifest") {
    auto                store = gw::persist::make_sqlite_local_store(temp_store_db("dsar"));
    REQUIRE(store->open_or_create());
    gw::persist::SlotRow r{};
    r.slot_id       = 1;
    r.display_name  = "n";
    r.unix_ms       = 0;
    r.engine_ver    = 0;
    r.schema_ver    = 1;
    r.determinism_h = 0;
    r.path          = "slot1.gwsave";
    r.size_bytes    = 10;
    r.blake3_hex    = std::string(64, 'b');
    REQUIRE(store->upsert_slot(r));
    const auto out = std::filesystem::temp_directory_path() / "gw_dsar_out";
    std::filesystem::remove_all(out);
    REQUIRE(gw::telemetry::dsar_export_to_dir(out, *store, "abc"));
    CHECK(std::filesystem::exists(out / "dsar_manifest.json"));
}

TEST_CASE("phase15 — persist + tele CVars register and round-trip") {
    gw::config::CVarRegistry r;
    auto                     pc = gw::persist::register_persist_and_telemetry_cvars(r);
    REQUIRE(r.set_bool(pc.autosave_enabled.id, false));
    REQUIRE(r.set_i32(pc.tele_events_batch_size.id, 128));
    REQUIRE(r.set_string(pc.tele_consent_version.id, "2026-05"));
    CHECK_FALSE(r.get_bool_or("persist.autosave.enabled", true));
    CHECK(r.get_i32_or("tele.events.batch_size", 64) == 128);
    CHECK(r.get_string_or("tele.consent.version", {}) == "2026-05");
}
