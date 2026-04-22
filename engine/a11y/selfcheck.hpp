#pragma once
// engine/a11y/selfcheck.hpp — ADR-0071 §5, ADR-0072 §6.

#include "engine/a11y/a11y_config.hpp"
#include "engine/a11y/a11y_types.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace gw::a11y {

enum class CheckResult : std::uint8_t {
    Pass = 0,
    Warn = 1,
    Fail = 2,
    NA   = 3,
};

[[nodiscard]] constexpr std::string_view to_string(CheckResult r) noexcept {
    switch (r) {
        case CheckResult::Pass: return "pass";
        case CheckResult::Warn: return "warn";
        case CheckResult::Fail: return "fail";
        case CheckResult::NA:   return "n/a";
    }
    return "?";
}

struct CheckItem {
    std::string criterion{};      // e.g. "WCAG 2.2 §1.4.4 Resize text"
    CheckResult result{CheckResult::NA};
    std::string detail{};
};

struct SelfCheckReport {
    std::string  wcag_version{"2.2"};
    std::vector<CheckItem> items{};
    std::uint32_t pass_count{0};
    std::uint32_t warn_count{0};
    std::uint32_t fail_count{0};

    [[nodiscard]] bool is_green() const noexcept { return fail_count == 0; }
};

struct SelfCheckContext {
    A11yConfig                    cfg{};
    std::span<const AccessibleNode> tree{};
    // Minimum body text size after text_scale — expressed in points.
    float                         effective_body_pt{16.0f};
    // Observed per-second flash rate in UI (0 when nothing flashing).
    float                         max_flash_rate_hz{0.0f};
    // Contrast ratio of worst UI text-on-background pair.
    float                         min_text_contrast_ratio{7.0f};
    // True when every interactive element passes `tab order == visual order`.
    bool                          tab_order_visual{true};
    // True when a visible focus ring is rendered when keyboard focus lands.
    bool                          focus_ring_visible{true};
    // Subtitle coverage for the last playback window (0..1).
    float                         subtitle_coverage{1.0f};
};

[[nodiscard]] SelfCheckReport run_selfcheck(const SelfCheckContext& ctx);

} // namespace gw::a11y
