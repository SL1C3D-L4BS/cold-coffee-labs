// engine/persist/persist_cvars.cpp — 25 persist.* + tele.* CVars (ADR-0064).

#include "engine/persist/persist_cvars.hpp"

namespace gw::persist {

namespace {
constexpr std::uint32_t kP = config::kCVarPersist;
constexpr std::uint32_t kD = config::kCVarDevOnly;
} // namespace

PersistCVars register_persist_and_telemetry_cvars(config::CVarRegistry& r) {
    PersistCVars c{};
    c.autosave_enabled = r.register_bool({
        "persist.autosave.enabled", true, kP, {}, {}, false,
        "Enable periodic autosave.",
    });
    c.autosave_interval_s = r.register_i32({
        "persist.autosave.interval_s", 120, kP, 10, 3600, true,
        "Seconds between autosave attempts.",
    });
    c.autosave_slots = r.register_i32({
        "persist.autosave.slots", 4, kP, 1, 16, true,
        "Circular autosave slot count.",
    });
    c.compress_enabled = r.register_bool({
        "persist.compress.enabled", false, kP, {}, {}, false,
        "Enable zstd chunk compression (requires GW_ENABLE_ZSTD).",
    });
    c.compress_level = r.register_i32({
        "persist.compress.level", 3, kP, 1, 22, true,
        "zstd level when compression enabled.",
    });
    c.checksum_verify_on_load = r.register_bool({
        "persist.checksum.verify_on_load", true, kP, {}, {}, false,
        "Verify BLAKE3 footer on load.",
    });
    c.save_dir = r.register_string({
        "persist.save.dir", std::string{"$user/saves"}, kP, {}, {}, false,
        "Directory root for slot files (expand $user).",
    });
    c.save_slots_max = r.register_i32({
        "persist.save.slots_max", 16, kP, 1, 64, true,
        "Maximum numbered save slots.",
    });
    c.migration_strict = r.register_bool({
        "persist.migration.strict", true, kP, {}, {}, false,
        "Refuse forward-incompatible engine versions.",
    });
    c.cloud_enabled = r.register_bool({
        "persist.cloud.enabled", false, kP, {}, {}, false,
        "Enable cloud sync backends.",
    });
    c.cloud_backend = r.register_string({
        "persist.cloud.backend", std::string{"local"}, kP, {}, {}, false,
        "Cloud backend id: local | steam | eos | s3.",
    });
    c.cloud_conflict_policy = r.register_string({
        "persist.cloud.conflict_policy", std::string{"prompt"}, kP, {}, {}, false,
        "Conflict policy: prompt | prefer_local | prefer_cloud | prefer_newer | preserve_both.",
    });
    c.cloud_quota_warn_pct = r.register_i32({
        "persist.cloud.quota_warn_pct", 80, kP, 1, 99, true,
        "Emit QuotaWarning when used quota crosses this percent.",
    });
    c.cloud_sim_latency_ms = r.register_f32({
        "persist.cloud.sim_latency_ms", 0.0f, kD, 0.0f, 500.0f, true,
        "Artificial latency for dev-local cloud (tests).",
    });

    c.tele_enabled = r.register_bool({
        "tele.enabled", false, kP, {}, {}, false,
        "Master enable for telemetry subsystems (opt-in).",
    });
    c.tele_crash_enabled = r.register_bool({
        "tele.crash.enabled", false, kP, {}, {}, false,
        "Enable crash reporter (Sentry when compiled).",
    });
    c.tele_events_enabled = r.register_bool({
        "tele.events.enabled", false, kP, {}, {}, false,
        "Enable custom analytics event pipeline.",
    });
    c.tele_events_batch_size = r.register_i32({
        "tele.events.batch_size", 64, kP, 1, 4096, true,
        "Events per batch before SQLite enqueue / flush.",
    });
    c.tele_events_flush_interval_s = r.register_i32({
        "tele.events.flush_interval_s", 60, kP, 1, 86400, true,
        "Max seconds before a partial batch flushes.",
    });
    c.tele_queue_max_bytes = r.register_i32({
        "tele.queue.max_bytes", 32 * 1024 * 1024, kP, 1024, 256 * 1024 * 1024, true,
        "Telemetry SQLite queue soft cap (bytes).",
    });
    c.tele_queue_max_age_days = r.register_i32({
        "tele.queue.max_age_days", 30, kP, 1, 365, true,
        "Drop queued telemetry older than this many days.",
    });
    c.tele_endpoint = r.register_string({
        "tele.endpoint", std::string{}, kP, {}, {}, false,
        "HTTPS ingest URL for custom events (studio-owned).",
    });
    c.tele_consent_age_gate_years = r.register_i32({
        "tele.consent.age_gate_years", 13, kP, 0, 21, true,
        "Default COPPA/GDPR age gate (locale may override).",
    });
    c.tele_consent_version = r.register_string({
        "tele.consent.version", std::string{"2026-04"}, kP, {}, {}, false,
        "Consent text version; mismatch forces re-prompt.",
    });
    c.tele_pii_scrub_paths = r.register_bool({
        "tele.pii.scrub_paths", true, kP, {}, {}, false,
        "Scrub file paths from telemetry payloads.",
    });
    return c;
}

} // namespace gw::persist
