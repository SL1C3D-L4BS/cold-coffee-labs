// engine/telemetry/consent.cpp

#include "engine/telemetry/consent.hpp"
#include "engine/persist/local_store.hpp"

namespace gw::telemetry {

std::string_view to_string(ConsentTier tier) noexcept {
    switch (tier) {
    case ConsentTier::None: return "None";
    case ConsentTier::CrashOnly: return "CrashOnly";
    case ConsentTier::CoreTelemetry: return "CoreTelemetry";
    case ConsentTier::AnalyticsAllowed: return "AnalyticsAllowed";
    case ConsentTier::MarketingAllowed: return "MarketingAllowed";
    }
    return "?";
}

ConsentTier parse_consent_tier(std::string_view tier_text) noexcept {
    if (tier_text == "CrashOnly") {
        return ConsentTier::CrashOnly;
    }
    if (tier_text == "CoreTelemetry") {
        return ConsentTier::CoreTelemetry;
    }
    if (tier_text == "AnalyticsAllowed") {
        return ConsentTier::AnalyticsAllowed;
    }
    if (tier_text == "MarketingAllowed") {
        return ConsentTier::MarketingAllowed;
    }
    return ConsentTier::None;
}

void consent_store(gw::persist::ILocalStore& store, ConsentTier tier, std::string_view version) {
    (void)store.set_setting("tele.consent", "tier", std::string{to_string(tier)});
    (void)store.set_setting("tele.consent", "version", std::string{version});
}

ConsentTier consent_load(gw::persist::ILocalStore& store) {
    const auto tier_text = store.get_setting("tele.consent", "tier");
    if (!tier_text) {
        return ConsentTier::None;
    }
    return parse_consent_tier(*tier_text);
}

ConsentTier apply_age_gate(ConsentTier chosen, int age_years, int gate_years) noexcept {
    if (age_years >= 0 && age_years < gate_years) {
        // Allow None / CrashOnly / CoreTelemetry for minors; clamp higher tiers.
        if (chosen > ConsentTier::CoreTelemetry) {
            return ConsentTier::CrashOnly;
        }
    }
    return chosen;
}

} // namespace gw::telemetry
