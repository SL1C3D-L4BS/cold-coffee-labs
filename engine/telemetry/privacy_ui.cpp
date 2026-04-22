// engine/telemetry/privacy_ui.cpp

#include "engine/telemetry/privacy_ui.hpp"

#include "engine/ui/locale_bridge.hpp"

namespace gw::telemetry {

namespace {

std::string_view trim(std::string_view s) noexcept {
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r')) s.remove_prefix(1);
    while (!s.empty() && (s.back()  == ' ' || s.back()  == '\t' || s.back()  == '\r')) s.remove_suffix(1);
    return s;
}

std::string resolve_key(const gw::ui::LocaleBridge& bridge, std::string_view key) {
    return bridge.resolve(gw::ui::LocaleBridge::make_id(key), key);
}

} // namespace

std::size_t register_privacy_locale(gw::ui::LocaleBridge& bridge,
                                     std::string_view      locale_tag,
                                     std::string_view      locale_txt) {
    std::size_t registered = 0;
    std::size_t i = 0;
    while (i < locale_txt.size()) {
        const auto eol = locale_txt.find('\n', i);
        const auto end = (eol == std::string_view::npos) ? locale_txt.size() : eol;
        auto line      = trim(locale_txt.substr(i, end - i));
        i = end + 1;
        if (line.empty() || line.front() == '#') continue;
        const auto eq = line.find('=');
        if (eq == std::string_view::npos) continue;
        const auto key = trim(line.substr(0, eq));
        const auto val = trim(line.substr(eq + 1));
        if (key.empty()) continue;
        bridge.register_string(locale_tag, key, val);
        ++registered;
    }
    return registered;
}

PrivacyStrings resolve_privacy_strings(const gw::ui::LocaleBridge& bridge) {
    PrivacyStrings s;
    s.title           = resolve_key(bridge, "privacy.title");
    s.crash           = resolve_key(bridge, "privacy.crash");
    s.core            = resolve_key(bridge, "privacy.core");
    s.analytics       = resolve_key(bridge, "privacy.analytics");
    s.marketing       = resolve_key(bridge, "privacy.marketing");
    s.button_continue = resolve_key(bridge, "privacy.continue");
    s.age_title       = resolve_key(bridge, "privacy.age.title");
    s.age_body        = resolve_key(bridge, "privacy.age.body");
    s.dsar_title      = resolve_key(bridge, "privacy.dsar.title");
    s.dsar_body       = resolve_key(bridge, "privacy.dsar.body");
    return s;
}

PrivacyViewModel build_privacy_view_model(const gw::ui::LocaleBridge& bridge,
                                           const ConsentFsmSnapshot&    fsm) {
    PrivacyViewModel vm{};
    vm.fsm     = fsm;
    vm.strings = resolve_privacy_strings(bridge);
    // COPPA clamp: when under the age gate, disaggregated tiers above
    // CrashOnly are disabled (ADR-0062). The FSM's `effective_tier` is the
    // source of truth; we surface which rows a host may render as enabled.
    const bool coppa_clamp = (fsm.effective_age > 0) && (fsm.effective_age < 13);
    auto push = [&](ConsentTier t, std::string_view code, const std::string& label) {
        PrivacyViewModel::TierRow row{};
        row.tier     = t;
        row.code     = code;
        row.label    = &label;
        row.disabled = coppa_clamp && (t > ConsentTier::CrashOnly);
        vm.tier_rows.push_back(row);
    };
    push(ConsentTier::CrashOnly,        "crash",     vm.strings.crash);
    push(ConsentTier::CoreTelemetry,    "core",      vm.strings.core);
    push(ConsentTier::AnalyticsAllowed, "analytics", vm.strings.analytics);
    push(ConsentTier::MarketingAllowed, "marketing", vm.strings.marketing);
    return vm;
}

} // namespace gw::telemetry
