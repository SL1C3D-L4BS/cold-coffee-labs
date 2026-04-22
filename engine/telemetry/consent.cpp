// engine/telemetry/consent.cpp

#include "engine/telemetry/consent.hpp"
#include "engine/persist/local_store.hpp"

namespace gw::telemetry {

std::string_view to_string(ConsentTier t) noexcept {
    switch (t) {
    case ConsentTier::None: return "None";
    case ConsentTier::CrashOnly: return "CrashOnly";
    case ConsentTier::CoreTelemetry: return "CoreTelemetry";
    case ConsentTier::AnalyticsAllowed: return "AnalyticsAllowed";
    case ConsentTier::MarketingAllowed: return "MarketingAllowed";
    }
    return "?";
}

ConsentTier parse_consent_tier(std::string_view s) noexcept {
    if (s == "CrashOnly") return ConsentTier::CrashOnly;
    if (s == "CoreTelemetry") return ConsentTier::CoreTelemetry;
    if (s == "AnalyticsAllowed") return ConsentTier::AnalyticsAllowed;
    if (s == "MarketingAllowed") return ConsentTier::MarketingAllowed;
    return ConsentTier::None;
}

void consent_store(gw::persist::ILocalStore& store, ConsentTier tier, std::string_view version) {
    (void)store.set_setting("tele.consent", "tier", std::string{to_string(tier)});
    (void)store.set_setting("tele.consent", "version", std::string{version});
}

ConsentTier consent_load(gw::persist::ILocalStore& store) {
    const auto t = store.get_setting("tele.consent", "tier");
    if (!t) return ConsentTier::None;
    return parse_consent_tier(*t);
}

ConsentTier apply_age_gate(ConsentTier chosen, int age_years, int gate_years) noexcept {
    if (age_years >= 0 && age_years < gate_years) {
        if (chosen > ConsentTier::CrashOnly) return ConsentTier::CrashOnly;
    }
    return chosen;
}

} // namespace gw::telemetry
