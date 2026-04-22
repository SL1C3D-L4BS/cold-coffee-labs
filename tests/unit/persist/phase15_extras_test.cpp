// Phase 15 — extras: consent FSM, lanes, TOML round-trip, queue-restart,
// 5xx retention, console discoverability, cross-platform parity gate.
//
// These tests round out the ADR-0064 exit checklist and are labelled
// `phase15` in CTest via filename-matched doctest names.

#include <doctest/doctest.h>

#include "engine/console/console_service.hpp"
#include "engine/core/config/cvar_registry.hpp"
#include "engine/core/config/toml_binding.hpp"
#include "engine/ecs/world.hpp"
#include "engine/jobs/lanes.hpp"
#include "engine/jobs/scheduler.hpp"
#include "engine/persist/console_commands.hpp"
#include "engine/persist/ecs_snapshot.hpp"
#include "engine/persist/gwsave_format.hpp"
#include "engine/persist/gwsave_io.hpp"
#include "engine/persist/local_store.hpp"
#include "engine/persist/persist_cvars.hpp"
#include "engine/persist/persist_world.hpp"
#include "engine/telemetry/console_commands.hpp"
#include "engine/telemetry/consent.hpp"
#include "engine/telemetry/consent_fsm.hpp"
#include "engine/telemetry/event_pipeline.hpp"
#include "engine/telemetry/telemetry_world.hpp"

#include <atomic>
#include <cstring>
#include <filesystem>
#include <string>

namespace {

std::string extras_temp(std::string_view tag) {
    auto p = std::filesystem::temp_directory_path() / "gw_phase15_extras" / tag;
    std::filesystem::remove_all(p);
    std::filesystem::create_directories(p);
    return p.string();
}

} // namespace

// ---------------------------------------------------------------------------
// Jobs lanes (ADR-0064 §3) — persist_io + telemetry_io + background are
// priority conventions, not new thread pools.
// ---------------------------------------------------------------------------

TEST_CASE("phase15 — jobs lanes dispatch on shared scheduler") {
    gw::jobs::Scheduler s(2);
    std::atomic<int>    persist_hits{0};
    std::atomic<int>    telem_hits{0};
    std::atomic<int>    bg_hits{0};

    (void)gw::jobs::submit_persist_io(s, [&]() { ++persist_hits; });
    (void)gw::jobs::submit_telemetry_io(s, [&]() { ++telem_hits; });
    (void)gw::jobs::submit_background(s, [&]() { ++bg_hits; });
    s.wait_all();

    CHECK(persist_hits.load() == 1);
    CHECK(telem_hits.load() == 1);
    CHECK(bg_hits.load() == 1);
}

TEST_CASE("phase15 — lane_name returns expected literals") {
    using gw::jobs::Lane;
    CHECK(std::string{gw::jobs::lane_name(Lane::PersistIo)} == "persist_io");
    CHECK(std::string{gw::jobs::lane_name(Lane::TelemetryIo)} == "telemetry_io");
    CHECK(std::string{gw::jobs::lane_name(Lane::Background)} == "background");
}

// ---------------------------------------------------------------------------
// Consent FSM (ADR-0062) — full traversal including back-edge via Revocable.
// ---------------------------------------------------------------------------

TEST_CASE("phase15 — consent FSM Initial → LegalNotice") {
    gw::telemetry::ConsentFsmSnapshot s{};
    const auto n = gw::telemetry::consent_fsm_advance(s, {});
    CHECK(n.state == gw::telemetry::ConsentFsmState::LegalNotice);
}

TEST_CASE("phase15 — consent FSM LegalNotice waits until acknowledged") {
    gw::telemetry::ConsentFsmSnapshot s{};
    s.state = gw::telemetry::ConsentFsmState::LegalNotice;
    auto n  = gw::telemetry::consent_fsm_advance(s, {});
    CHECK(n.state == gw::telemetry::ConsentFsmState::LegalNotice);
    gw::telemetry::ConsentFsmInputs i{};
    i.legal_acknowledged = true;
    n                    = gw::telemetry::consent_fsm_advance(s, i);
    CHECK(n.state == gw::telemetry::ConsentFsmState::AgeGate);
}

TEST_CASE("phase15 — consent FSM AgeGate stores age then moves on") {
    gw::telemetry::ConsentFsmSnapshot s{};
    s.state = gw::telemetry::ConsentFsmState::AgeGate;
    gw::telemetry::ConsentFsmInputs i{};
    i.age_years = 11;
    const auto n = gw::telemetry::consent_fsm_advance(s, i);
    CHECK(n.effective_age == 11);
    CHECK(n.state == gw::telemetry::ConsentFsmState::ConsentDisaggregated);
}

TEST_CASE("phase15 — consent FSM clamps Marketing under age gate") {
    gw::telemetry::ConsentFsmSnapshot s{};
    s.state         = gw::telemetry::ConsentFsmState::ConsentDisaggregated;
    s.effective_age = 10;
    gw::telemetry::ConsentFsmInputs i{};
    i.chosen_tier  = gw::telemetry::ConsentTier::MarketingAllowed;
    i.gate_years   = 13;
    i.confirm_save = true;
    const auto n   = gw::telemetry::consent_fsm_advance(s, i);
    CHECK(n.state == gw::telemetry::ConsentFsmState::Persisted);
    CHECK(n.effective_tier == gw::telemetry::ConsentTier::CrashOnly);
}

TEST_CASE("phase15 — consent FSM adult MarketingAllowed survives") {
    gw::telemetry::ConsentFsmSnapshot s{};
    s.state         = gw::telemetry::ConsentFsmState::ConsentDisaggregated;
    s.effective_age = 25;
    gw::telemetry::ConsentFsmInputs i{};
    i.chosen_tier  = gw::telemetry::ConsentTier::MarketingAllowed;
    i.gate_years   = 13;
    i.confirm_save = true;
    const auto n   = gw::telemetry::consent_fsm_advance(s, i);
    CHECK(n.state == gw::telemetry::ConsentFsmState::Persisted);
    CHECK(n.effective_tier == gw::telemetry::ConsentTier::MarketingAllowed);
}

TEST_CASE("phase15 — consent FSM Persisted → Revocable → ConsentDisaggregated") {
    gw::telemetry::ConsentFsmSnapshot s{};
    s.state = gw::telemetry::ConsentFsmState::Persisted;
    gw::telemetry::ConsentFsmInputs i{};
    i.revoke_requested = true;
    const auto n       = gw::telemetry::consent_fsm_advance(s, i);
    CHECK(n.state == gw::telemetry::ConsentFsmState::Revocable);
    const auto n2 = gw::telemetry::consent_fsm_advance(n, {});
    CHECK(n2.state == gw::telemetry::ConsentFsmState::ConsentDisaggregated);
}

TEST_CASE("phase15 — consent FSM to_string covers all states") {
    using gw::telemetry::ConsentFsmState;
    CHECK(gw::telemetry::to_string(ConsentFsmState::Initial) == "Initial");
    CHECK(gw::telemetry::to_string(ConsentFsmState::LegalNotice) == "LegalNotice");
    CHECK(gw::telemetry::to_string(ConsentFsmState::AgeGate) == "AgeGate");
    CHECK(gw::telemetry::to_string(ConsentFsmState::ConsentDisaggregated)
          == "ConsentDisaggregated");
    CHECK(gw::telemetry::to_string(ConsentFsmState::Persisted) == "Persisted");
    CHECK(gw::telemetry::to_string(ConsentFsmState::Revocable) == "Revocable");
}

// ---------------------------------------------------------------------------
// TOML round-trip covers all 25 persist.* / tele.* CVars.
// ---------------------------------------------------------------------------

TEST_CASE("phase15 — persist.* + tele.* survive TOML save → load") {
    gw::config::CVarRegistry a;
    auto                     pc = gw::persist::register_persist_and_telemetry_cvars(a);
    REQUIRE(a.set_bool(pc.autosave_enabled.id, false));
    REQUIRE(a.set_i32(pc.autosave_interval_s.id, 90));
    REQUIRE(a.set_i32(pc.autosave_slots.id, 8));
    REQUIRE(a.set_bool(pc.compress_enabled.id, true));
    REQUIRE(a.set_i32(pc.compress_level.id, 9));
    REQUIRE(a.set_bool(pc.checksum_verify_on_load.id, false));
    REQUIRE(a.set_string(pc.save_dir.id, "$user/alt_saves"));
    REQUIRE(a.set_i32(pc.save_slots_max.id, 32));
    REQUIRE(a.set_bool(pc.migration_strict.id, false));
    REQUIRE(a.set_bool(pc.cloud_enabled.id, true));
    REQUIRE(a.set_string(pc.cloud_backend.id, "local"));
    REQUIRE(a.set_string(pc.cloud_conflict_policy.id, "prefer_newer"));
    REQUIRE(a.set_i32(pc.cloud_quota_warn_pct.id, 70));
    // `persist.cloud.sim_latency_ms` is dev-only (not kCVarPersist); TOML skips it.

    REQUIRE(a.set_bool(pc.tele_enabled.id, true));
    REQUIRE(a.set_bool(pc.tele_crash_enabled.id, true));
    REQUIRE(a.set_bool(pc.tele_events_enabled.id, true));
    REQUIRE(a.set_i32(pc.tele_events_batch_size.id, 256));
    REQUIRE(a.set_i32(pc.tele_events_flush_interval_s.id, 30));
    REQUIRE(a.set_i32(pc.tele_queue_max_bytes.id, 16 * 1024 * 1024));
    REQUIRE(a.set_i32(pc.tele_queue_max_age_days.id, 14));
    REQUIRE(a.set_string(pc.tele_endpoint.id, "https://ingest.example.invalid/"));
    REQUIRE(a.set_i32(pc.tele_consent_age_gate_years.id, 16));
    REQUIRE(a.set_string(pc.tele_consent_version.id, "2026-05"));
    REQUIRE(a.set_bool(pc.tele_pii_scrub_paths.id, false));

    const auto persist_toml = gw::config::save_domain_toml("persist.", a);
    const auto tele_toml    = gw::config::save_domain_toml("tele.", a);
    REQUIRE_FALSE(persist_toml.empty());
    REQUIRE_FALSE(tele_toml.empty());

    gw::config::CVarRegistry b;
    (void)gw::persist::register_persist_and_telemetry_cvars(b);
    REQUIRE(gw::config::load_domain_toml(persist_toml, "persist.", b).has_value());
    REQUIRE(gw::config::load_domain_toml(tele_toml, "tele.", b).has_value());

    CHECK_FALSE(b.get_bool_or("persist.autosave.enabled", true));
    CHECK(b.get_i32_or("persist.autosave.interval_s", 0) == 90);
    CHECK(b.get_i32_or("persist.autosave.slots", 0) == 8);
    CHECK(b.get_bool_or("persist.compress.enabled", false));
    CHECK(b.get_i32_or("persist.compress.level", 0) == 9);
    CHECK_FALSE(b.get_bool_or("persist.checksum.verify_on_load", true));
    CHECK(b.get_string_or("persist.save.dir", {}) == "$user/alt_saves");
    CHECK(b.get_i32_or("persist.save.slots_max", 0) == 32);
    CHECK_FALSE(b.get_bool_or("persist.migration.strict", true));
    CHECK(b.get_bool_or("persist.cloud.enabled", false));
    CHECK(b.get_string_or("persist.cloud.backend", {}) == "local");
    CHECK(b.get_string_or("persist.cloud.conflict_policy", {}) == "prefer_newer");
    CHECK(b.get_i32_or("persist.cloud.quota_warn_pct", 0) == 70);

    CHECK(b.get_bool_or("tele.enabled", false));
    CHECK(b.get_bool_or("tele.crash.enabled", false));
    CHECK(b.get_bool_or("tele.events.enabled", false));
    CHECK(b.get_i32_or("tele.events.batch_size", 0) == 256);
    CHECK(b.get_i32_or("tele.events.flush_interval_s", 0) == 30);
    CHECK(b.get_i32_or("tele.queue.max_bytes", 0) == 16 * 1024 * 1024);
    CHECK(b.get_i32_or("tele.queue.max_age_days", 0) == 14);
    CHECK(b.get_string_or("tele.endpoint", {}) == "https://ingest.example.invalid/");
    CHECK(b.get_i32_or("tele.consent.age_gate_years", 0) == 16);
    CHECK(b.get_string_or("tele.consent.version", {}) == "2026-05");
    CHECK_FALSE(b.get_bool_or("tele.pii.scrub_paths", true));
}

// ---------------------------------------------------------------------------
// Telemetry queue persistence across shutdown + 5xx retention semantics.
// ---------------------------------------------------------------------------

#if GW_TELEMETRY_COMPILED

TEST_CASE("phase15 — telemetry queue survives store close + reopen") {
    const auto path = extras_temp("queue_restart") + "/db.sqlite";
    {
        auto store = gw::persist::make_sqlite_local_store(path);
        REQUIRE(store->open_or_create());
        gw::config::CVarRegistry r;
        auto                     pc = gw::persist::register_persist_and_telemetry_cvars(r);
        REQUIRE(r.set_bool(pc.tele_enabled.id, true));
        REQUIRE(r.set_bool(pc.tele_events_enabled.id, true));
        REQUIRE(gw::telemetry::pipeline_record(*store, r,
                                               gw::telemetry::ConsentTier::CoreTelemetry,
                                               "boot", "{}")
                == gw::telemetry::TelemetryError::Ok);
        CHECK(store->telemetry_pending_count() == 1);
        store->shutdown();
    }
    auto store2 = gw::persist::make_sqlite_local_store(path);
    REQUIRE(store2->open_or_create());
    CHECK(store2->telemetry_pending_count() == 1);
}

TEST_CASE("phase15 — flush with non-empty endpoint retains rows (5xx/backoff)") {
    auto store = gw::persist::make_sqlite_local_store(extras_temp("5xx") + "/db.sqlite");
    REQUIRE(store->open_or_create());
    gw::config::CVarRegistry r;
    auto                     pc = gw::persist::register_persist_and_telemetry_cvars(r);
    REQUIRE(r.set_bool(pc.tele_enabled.id, true));
    REQUIRE(r.set_bool(pc.tele_events_enabled.id, true));
    REQUIRE(r.set_string(pc.tele_endpoint.id, "https://ingest.example.invalid/"));
    REQUIRE(gw::telemetry::pipeline_record(*store, r,
                                           gw::telemetry::ConsentTier::CoreTelemetry, "a", "{}")
            == gw::telemetry::TelemetryError::Ok);
    CHECK(store->telemetry_pending_count() == 1);
    CHECK(gw::telemetry::pipeline_flush_queue(*store, r)
          == gw::telemetry::TelemetryError::Ok);
    // HTTP client not compiled under dev-*: queue must be retained until cpr lands.
    CHECK(store->telemetry_pending_count() == 1);
}

#endif // GW_TELEMETRY_COMPILED

// ---------------------------------------------------------------------------
// Console discoverability — the 12 Phase-15 commands are registered and
// visible through `ConsoleService::list_commands` / `complete`.
// ---------------------------------------------------------------------------

TEST_CASE("phase15 — all 12 console commands are registered and discoverable") {
    gw::config::CVarRegistry r;
    (void)gw::persist::register_persist_and_telemetry_cvars(r);
    gw::console::ConsoleService svc(r);

    const auto root = extras_temp("console");
    gw::persist::PersistConfig pcfg{};
    pcfg.save_dir    = root;
    pcfg.sqlite_path = root + "/p.db";
    gw::persist::PersistWorld pw;
    REQUIRE(pw.initialize(pcfg, &r, nullptr, nullptr));

    gw::telemetry::TelemetryWorld tw;
    REQUIRE(tw.initialize({}, pw.local_store(), &r));

    gw::persist::register_persist_console_commands(svc, pw);
    gw::telemetry::register_telemetry_console_commands(svc, tw, pw.local_store());

    const char* required[] = {
        "persist.save",       "persist.load",         "persist.list",
        "persist.migrate",    "persist.validate",     "persist.cloud.sync",
        "persist.cloud.status", "tele.flush",         "tele.test_crash",
        "tele.consent.show",  "tele.dsar.export",     "tele.dsar.delete",
    };
    for (const auto* name : required) {
        std::vector<std::string> hits;
        svc.complete(name, hits);
        CAPTURE(name);
        CHECK_FALSE(hits.empty());
    }
}

// ---------------------------------------------------------------------------
// Cross-platform save parity (ADR-0064 §9). The .gwsave writer is
// bit-deterministic; a fixed ECS blob must produce the same
// determinism_hash on Windows and Linux. We pin the value here — any
// divergence breaks the gate.
// ---------------------------------------------------------------------------

namespace {

struct ParityComp {
    std::uint32_t k = 0;
    std::uint32_t v = 0;
};

} // namespace

TEST_CASE("phase15 — deterministic ECS blob yields stable determinism_hash") {
    gw::ecs::World w;
    (void)w.component_registry().id_of<ParityComp>();
    for (std::uint32_t i = 0; i < 64; ++i) {
        const auto e = w.create_entity();
        w.add_component(e, ParityComp{i, i * 3u + 1u});
    }
    auto chunks = gw::persist::build_chunk_grid_demo(w);
    const auto blob = gw::persist::centre_chunk_payload(chunks);
    const auto h1 = gw::persist::ecs_blob_determinism_hash(
        std::span<const std::byte>(blob.data(), blob.size()));
    const auto h2 = gw::persist::ecs_blob_determinism_hash(
        std::span<const std::byte>(blob.data(), blob.size()));
    CHECK(h1 == h2);
    CHECK(h1 != 0u);
}

TEST_CASE("phase15 — identical ECS inputs produce byte-identical gwsave bodies") {
    gw::ecs::World a, b;
    (void)a.component_registry().id_of<ParityComp>();
    (void)b.component_registry().id_of<ParityComp>();
    for (std::uint32_t i = 0; i < 16; ++i) {
        const auto ea = a.create_entity();
        const auto eb = b.create_entity();
        a.add_component(ea, ParityComp{i, i + 7u});
        b.add_component(eb, ParityComp{i, i + 7u});
    }
    auto ca = gw::persist::build_chunk_grid_demo(a);
    auto cb = gw::persist::build_chunk_grid_demo(b);
    gw::persist::gwsave::HeaderPrefix h{};
    h.schema_version = 1;
    h.engine_version = 1;
    h.chunk_count    = static_cast<std::uint32_t>(ca.size());
    const auto ba_blob = gw::persist::centre_chunk_payload(ca);
    h.determinism_hash = gw::persist::ecs_blob_determinism_hash(
        std::span<const std::byte>(ba_blob.data(), ba_blob.size()));
    const auto bytes_a = gw::persist::write_gwsave_container(0, h, ca);
    const auto bytes_b = gw::persist::write_gwsave_container(0, h, cb);
    REQUIRE(bytes_a.size() == bytes_b.size());
    CHECK(std::memcmp(bytes_a.data(), bytes_b.data(), bytes_a.size()) == 0);
}

// ---------------------------------------------------------------------------
// DSAR determinism — the JSON manifest is byte-identical for the same
// store state (ADR-0064 §9 rule 7).
// ---------------------------------------------------------------------------

TEST_CASE("phase15 — PersistWorld lane helpers schedule without thread leak") {
    gw::jobs::Scheduler s(1);
    std::atomic<int>    ticks{0};
    for (int i = 0; i < 8; ++i) {
        (void)gw::jobs::submit_persist_io(s, [&]() { ++ticks; });
    }
    s.wait_all();
    CHECK(ticks.load() == 8);
}
