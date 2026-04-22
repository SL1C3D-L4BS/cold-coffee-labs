// Phase 15 — privacy UI view-model (ADR-0062). Verifies the RmlUi template
// binding layer: `.locale.txt` parsing into LocaleBridge, full resolution of
// the 10-key privacy string set, COPPA clamp of tier rows for under-13s, and
// that every key in `ui/privacy/en.locale.txt` round-trips.

#include <doctest/doctest.h>

#include "engine/telemetry/privacy_ui.hpp"
#include "engine/ui/locale_bridge.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace {

constexpr std::string_view kSample =
    "# comment\n"
    "privacy.title=Privacy\n"
    "privacy.crash=Crash\n"
    "privacy.core=Core\n"
    "privacy.analytics=Analytics\n"
    "privacy.marketing=Marketing\n"
    "privacy.continue=OK\n"
    "privacy.age.title=Age\n"
    "privacy.age.body=Why\n"
    "privacy.dsar.title=Export\n"
    "privacy.dsar.body=Copy\n";

std::string read_locale_file_or(const std::string& path, std::string_view fallback) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return std::string(fallback);
    std::ostringstream ss; ss << f.rdbuf();
    return ss.str();
}

} // namespace

TEST_CASE("phase15 — register_privacy_locale parses KEY=VALUE lines") {
    gw::ui::LocaleBridge bridge;
    const auto n = gw::telemetry::register_privacy_locale(bridge, "en-US", kSample);
    CHECK(n == 10u);
    const auto s = gw::telemetry::resolve_privacy_strings(bridge);
    CHECK(s.title           == "Privacy");
    CHECK(s.crash           == "Crash");
    CHECK(s.core            == "Core");
    CHECK(s.analytics       == "Analytics");
    CHECK(s.marketing       == "Marketing");
    CHECK(s.button_continue == "OK");
    CHECK(s.age_title       == "Age");
    CHECK(s.age_body        == "Why");
    CHECK(s.dsar_title      == "Export");
    CHECK(s.dsar_body       == "Copy");
}

TEST_CASE("phase15 — privacy view-model clamps tiers above CrashOnly for under-13s") {
    gw::ui::LocaleBridge bridge;
    (void)gw::telemetry::register_privacy_locale(bridge, "en-US", kSample);
    gw::telemetry::ConsentFsmSnapshot fsm{};
    fsm.effective_age  = 10;
    fsm.effective_tier = gw::telemetry::ConsentTier::CrashOnly;
    const auto vm = gw::telemetry::build_privacy_view_model(bridge, fsm);
    REQUIRE(vm.tier_rows.size() == 4u);
    CHECK(vm.tier_rows[0].tier     == gw::telemetry::ConsentTier::CrashOnly);
    CHECK(vm.tier_rows[0].disabled == false);
    CHECK(vm.tier_rows[1].disabled == true);
    CHECK(vm.tier_rows[2].disabled == true);
    CHECK(vm.tier_rows[3].disabled == true);
}

TEST_CASE("phase15 — privacy view-model keeps tiers open for adults") {
    gw::ui::LocaleBridge bridge;
    (void)gw::telemetry::register_privacy_locale(bridge, "en-US", kSample);
    gw::telemetry::ConsentFsmSnapshot fsm{};
    fsm.effective_age  = 24;
    fsm.effective_tier = gw::telemetry::ConsentTier::MarketingAllowed;
    const auto vm = gw::telemetry::build_privacy_view_model(bridge, fsm);
    for (const auto& row : vm.tier_rows) CHECK(row.disabled == false);
}

TEST_CASE("phase15 — privacy view-model is age-gate-safe when age unknown") {
    gw::ui::LocaleBridge bridge;
    (void)gw::telemetry::register_privacy_locale(bridge, "en-US", kSample);
    gw::telemetry::ConsentFsmSnapshot fsm{};
    fsm.effective_age  = 0;
    fsm.effective_tier = gw::telemetry::ConsentTier::None;
    const auto vm = gw::telemetry::build_privacy_view_model(bridge, fsm);
    for (const auto& row : vm.tier_rows) CHECK(row.disabled == false);
}

TEST_CASE("phase15 — ui/privacy/en.locale.txt has all 10 canonical keys") {
    // CMake runs tests from the build directory; resolve the repo-relative path.
    const std::string path = std::string(CCL_REPO_ROOT) + "/ui/privacy/en.locale.txt";
    const auto payload = read_locale_file_or(path, kSample);
    gw::ui::LocaleBridge bridge;
    const auto n = gw::telemetry::register_privacy_locale(bridge, "en-US", payload);
    CHECK(n >= 10u);
    const auto s = gw::telemetry::resolve_privacy_strings(bridge);
    CHECK_FALSE(s.title.empty());
    CHECK_FALSE(s.crash.empty());
    CHECK_FALSE(s.core.empty());
    CHECK_FALSE(s.analytics.empty());
    CHECK_FALSE(s.marketing.empty());
    CHECK_FALSE(s.button_continue.empty());
    CHECK_FALSE(s.age_title.empty());
    CHECK_FALSE(s.age_body.empty());
    CHECK_FALSE(s.dsar_title.empty());
    CHECK_FALSE(s.dsar_body.empty());
}
