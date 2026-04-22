#pragma once

#include "engine/telemetry/consent.hpp"
#include "engine/telemetry/telemetry_types.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace gw::config {
class CVarRegistry;
}
namespace gw::persist {
class ILocalStore;
}

namespace gw::telemetry {

[[nodiscard]] TelemetryError pipeline_record(gw::persist::ILocalStore& store,
                                             config::CVarRegistry&     r,
                                             ConsentTier               tier,
                                             std::string_view          event_name,
                                             std::string_view          props_json);

[[nodiscard]] TelemetryError pipeline_flush_queue(gw::persist::ILocalStore& store,
                                                  config::CVarRegistry&       r);

/// Prune queue rows older than `now_ms - tele.queue.max_age_days * 86400_000`
/// (ADR-0061 storage minimisation). Returns the number of rows dropped, or -1 on error.
[[nodiscard]] std::int64_t pipeline_prune_aged(gw::persist::ILocalStore& store,
                                                config::CVarRegistry&    r,
                                                std::int64_t              now_ms) noexcept;

} // namespace gw::telemetry
