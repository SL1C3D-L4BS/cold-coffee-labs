#pragma once
// engine/telemetry/privacy_ui.hpp — headless view-model for the RmlUi privacy
// flow (ADR-0062). Binds the `ConsentFsm` state machine to the
// `ui::LocaleBridge` so the consent / age-gate / DSAR templates in
// `ui/privacy/*.rml` can render localised strings without the telemetry layer
// depending on RmlUi directly (CLAUDE.md #12 header quarantine).

#include "engine/telemetry/consent.hpp"
#include "engine/telemetry/consent_fsm.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace gw::ui { class LocaleBridge; }

namespace gw::telemetry {

struct PrivacyStrings {
    std::string title;
    std::string crash;
    std::string core;
    std::string analytics;
    std::string marketing;
    std::string button_continue;
    std::string age_title;
    std::string age_body;
    std::string dsar_title;
    std::string dsar_body;
};

/// Parse a `.locale.txt` payload (KEY=VALUE per line, '#' comments, blank
/// lines ignored) into a LocaleBridge under `locale_tag`. Returns the number
/// of rows registered. Malformed lines are skipped silently.
std::size_t register_privacy_locale(gw::ui::LocaleBridge& bridge,
                                     std::string_view      locale_tag,
                                     std::string_view      locale_txt);

/// Resolve the canonical 10-key privacy string set for the current locale.
[[nodiscard]] PrivacyStrings resolve_privacy_strings(const gw::ui::LocaleBridge& bridge);

/// Rendered view-model that an RmlUi host binds to the `.rml` templates.
struct PrivacyViewModel {
    ConsentFsmSnapshot fsm{};
    PrivacyStrings     strings{};
    /// List of tier rows the disaggregated consent screen renders. Under a
    /// COPPA-clamp (age < gate), rows > `CrashOnly` are marked disabled.
    struct TierRow {
        ConsentTier      tier;
        std::string_view code;
        const std::string* label; // points into `strings`
        bool              disabled;
    };
    std::vector<TierRow> tier_rows{};
};

[[nodiscard]] PrivacyViewModel build_privacy_view_model(const gw::ui::LocaleBridge& bridge,
                                                         const ConsentFsmSnapshot&    fsm);

} // namespace gw::telemetry
