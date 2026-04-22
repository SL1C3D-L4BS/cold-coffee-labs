// engine/telemetry/event_pipeline.cpp

#include "engine/telemetry/event_pipeline.hpp"
#include "engine/persist/integrity.hpp"
#include "engine/persist/local_store.hpp"
#include "engine/core/config/cvar_registry.hpp"

#include <array>
#include <chrono>
#include <cstring>
#include <string>
#include <vector>

namespace gw::telemetry {

namespace {

std::string json_envelope(std::string_view name, std::string_view props) {
    std::string j = "{\"event\":\"";
    j.append(name);
    j += "\",\"props\":";
    j.append(props);
    j += "}";
    return j;
}

} // namespace

TelemetryError pipeline_record(gw::persist::ILocalStore& store,
                               config::CVarRegistry&     r,
                               ConsentTier               tier,
                               std::string_view          event_name,
                               std::string_view          props_json) {
#if !GW_TELEMETRY_COMPILED
    (void)store;
    (void)r;
    (void)tier;
    (void)event_name;
    (void)props_json;
    return TelemetryError::Disabled;
#else
    if (!r.get_bool_or("tele.enabled", false) || !r.get_bool_or("tele.events.enabled", false)) {
        return TelemetryError::Disabled;
    }
    if (tier < ConsentTier::CoreTelemetry) return TelemetryError::ConsentDenied;

    const auto j = json_envelope(event_name, props_json);
    std::vector<std::byte> payload(j.size());
    std::memcpy(payload.data(), j.data(), j.size());
    const auto digest =
        gw::persist::blake3_digest_256(std::span<const std::byte>(payload.data(), payload.size()));

    const std::int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 std::chrono::system_clock::now().time_since_epoch())
                                 .count();

    std::array<std::byte, 8> batch_id{};
    std::memcpy(batch_id.data(), &now, sizeof(now));

    std::array<std::byte, 16> d128{};
    std::memcpy(d128.data(), digest.data(), 16);

    if (!store.telemetry_enqueue(now, batch_id, payload, 0, now, d128)) return TelemetryError::IoError;
    return TelemetryError::Ok;
#endif
}

TelemetryError pipeline_flush_queue(gw::persist::ILocalStore& store, config::CVarRegistry& r) {
#if !GW_TELEMETRY_COMPILED
    (void)store;
    (void)r;
    return TelemetryError::Disabled;
#else
    if (store.telemetry_pending_count() < 0) return TelemetryError::IoError;
    if (store.telemetry_pending_count() == 0) return TelemetryError::Ok;
    // Without a studio ingest URL, treat flush as a successful no-op upload and drain the queue
    // (deterministic tests + offline-first policy, ADR-0061).
    if (r.get_string_or("tele.endpoint", {}).empty()) {
        return store.telemetry_delete_all_rows() ? TelemetryError::Ok : TelemetryError::IoError;
    }
    // Phase 15: HTTP POST lands behind GW_ENABLE_CPR; queue retained until wired.
    return TelemetryError::Ok;
#endif
}

std::int64_t pipeline_prune_aged(gw::persist::ILocalStore& store,
                                  config::CVarRegistry&    r,
                                  std::int64_t              now_ms) noexcept {
#if !GW_TELEMETRY_COMPILED
    (void)store; (void)r; (void)now_ms;
    return 0;
#else
    const auto days = r.get_i32_or("tele.queue.max_age_days", 30);
    if (days <= 0) return 0;
    const std::int64_t cutoff = now_ms - static_cast<std::int64_t>(days) * 86'400'000;
    if (cutoff <= 0) return 0;
    return store.telemetry_delete_older_than(cutoff);
#endif
}

} // namespace gw::telemetry
