#pragma once

#include "engine/core/config/cvar_registry.hpp"

namespace gw::persist {

struct PersistCVars {
    config::CVarRef<bool>         autosave_enabled;
    config::CVarRef<std::int32_t> autosave_interval_s;
    config::CVarRef<std::int32_t> autosave_slots;
    config::CVarRef<bool>         compress_enabled;
    config::CVarRef<std::int32_t> compress_level;
    config::CVarRef<bool>         checksum_verify_on_load;
    config::CVarRef<std::string>  save_dir;
    config::CVarRef<std::int32_t> save_slots_max;
    config::CVarRef<bool>         migration_strict;
    config::CVarRef<bool>         cloud_enabled;
    config::CVarRef<std::string>  cloud_backend;
    config::CVarRef<std::string>  cloud_conflict_policy;
    config::CVarRef<std::int32_t> cloud_quota_warn_pct;
    config::CVarRef<float>        cloud_sim_latency_ms;

    config::CVarRef<bool>         tele_enabled;
    config::CVarRef<bool>         tele_crash_enabled;
    config::CVarRef<bool>         tele_events_enabled;
    config::CVarRef<std::int32_t> tele_events_batch_size;
    config::CVarRef<std::int32_t> tele_events_flush_interval_s;
    config::CVarRef<std::int32_t> tele_queue_max_bytes;
    config::CVarRef<std::int32_t> tele_queue_max_age_days;
    config::CVarRef<std::string>  tele_endpoint;
    config::CVarRef<std::int32_t> tele_consent_age_gate_years;
    config::CVarRef<std::string>  tele_consent_version;
    config::CVarRef<bool>         tele_pii_scrub_paths;
};

[[nodiscard]] PersistCVars register_persist_and_telemetry_cvars(config::CVarRegistry& r);

} // namespace gw::persist
