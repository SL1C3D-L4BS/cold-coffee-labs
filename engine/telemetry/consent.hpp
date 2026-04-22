#pragma once

#include <string>
#include <string_view>

namespace gw::persist {
class ILocalStore;
}

namespace gw::telemetry {

enum class ConsentTier : std::uint8_t {
    None = 0,
    CrashOnly,
    CoreTelemetry,
    AnalyticsAllowed,
    MarketingAllowed,
};

[[nodiscard]] std::string_view to_string(ConsentTier t) noexcept;
[[nodiscard]] ConsentTier     parse_consent_tier(std::string_view s) noexcept;

void consent_store(gw::persist::ILocalStore& store, ConsentTier tier, std::string_view version);
[[nodiscard]] ConsentTier consent_load(gw::persist::ILocalStore& store);

/// COPPA-style clamp: if age < gate, max tier is CrashOnly.
[[nodiscard]] ConsentTier apply_age_gate(ConsentTier chosen, int age_years, int gate_years) noexcept;

} // namespace gw::telemetry
