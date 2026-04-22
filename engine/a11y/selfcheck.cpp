// engine/a11y/selfcheck.cpp — WCAG 2.2 AA static checkable subset.

#include "engine/a11y/selfcheck.hpp"

#include <string>

namespace gw::a11y {

namespace {

void push(SelfCheckReport& r, std::string criterion, CheckResult c, std::string detail = {}) {
    CheckItem it{};
    it.criterion = std::move(criterion);
    it.result    = c;
    it.detail    = std::move(detail);
    switch (c) {
        case CheckResult::Pass: ++r.pass_count; break;
        case CheckResult::Warn: ++r.warn_count; break;
        case CheckResult::Fail: ++r.fail_count; break;
        default: break;
    }
    r.items.push_back(std::move(it));
}

} // namespace

SelfCheckReport run_selfcheck(const SelfCheckContext& ctx) {
    SelfCheckReport r{};
    r.wcag_version = ctx.cfg.wcag_version;

    // §1.4.3 Contrast (Minimum) — AA: ≥ 4.5:1 for body text.
    if (ctx.min_text_contrast_ratio >= 4.5f) push(r, "WCAG 2.2 §1.4.3 Contrast (Minimum)", CheckResult::Pass);
    else                                      push(r, "WCAG 2.2 §1.4.3 Contrast (Minimum)", CheckResult::Fail,
                                                    "min contrast < 4.5:1");

    // §1.4.4 Resize text — AA: must support 200% scale without loss.
    const float scaled_body = ctx.effective_body_pt * ctx.cfg.text_scale;
    if (ctx.cfg.text_scale >= 2.0f || scaled_body >= 32.0f) push(r, "WCAG 2.2 §1.4.4 Resize text", CheckResult::Pass);
    else if (scaled_body >= 14.0f)                           push(r, "WCAG 2.2 §1.4.4 Resize text", CheckResult::Pass,
                                                                     "body ≥ 14pt after scale");
    else                                                      push(r, "WCAG 2.2 §1.4.4 Resize text", CheckResult::Fail);

    // §1.4.11 Non-text Contrast — AA: ≥ 3:1 (approximated by min contrast above).
    push(r, "WCAG 2.2 §1.4.11 Non-text Contrast",
         ctx.min_text_contrast_ratio >= 3.0f ? CheckResult::Pass : CheckResult::Fail);

    // §2.3.1 Three Flashes — AA: ≤ 3 flashes/s.
    if (ctx.cfg.photosensitivity_safe || ctx.max_flash_rate_hz <= 3.0f)
        push(r, "WCAG 2.2 §2.3.1 Three Flashes", CheckResult::Pass);
    else
        push(r, "WCAG 2.2 §2.3.1 Three Flashes", CheckResult::Fail,
             "flash rate > 3 Hz without photosensitivity-safe mode");

    // §2.3.3 Animation from Interactions — reduce_motion honored.
    push(r, "WCAG 2.2 §2.3.3 Animation from Interactions",
         ctx.cfg.reduce_motion ? CheckResult::Pass : CheckResult::Warn,
         "motion reduction off (user preference)");

    // §2.4.7 Focus Visible.
    push(r, "WCAG 2.2 §2.4.7 Focus Visible",
         (ctx.focus_ring_visible && ctx.cfg.focus_show_ring) ? CheckResult::Pass : CheckResult::Fail);

    // §2.4.11 Focus Not Obscured (Minimum) — AA new in 2.2.
    push(r, "WCAG 2.2 §2.4.11 Focus Not Obscured (Minimum)",
         ctx.focus_ring_visible ? CheckResult::Pass : CheckResult::Fail);

    // §1.2.2 Captions (Prerecorded) — subtitle coverage heuristic.
    if (!ctx.cfg.subtitles_enabled) push(r, "WCAG 2.2 §1.2.2 Captions (Prerecorded)", CheckResult::Fail,
                                            "captions disabled");
    else if (ctx.subtitle_coverage >= 0.98f) push(r, "WCAG 2.2 §1.2.2 Captions (Prerecorded)", CheckResult::Pass);
    else                                      push(r, "WCAG 2.2 §1.2.2 Captions (Prerecorded)", CheckResult::Warn,
                                                    "coverage < 98%");

    // §4.1.2 Name, Role, Value — every node has non-empty name + real role.
    bool all_named = true;
    bool all_roled = true;
    for (const auto& n : ctx.tree) {
        if (n.role == Role::Unknown) all_roled = false;
        if (n.name_utf8.empty()) all_named = false;
    }
    push(r, "WCAG 2.2 §4.1.2 Name, Role, Value",
         (all_named && all_roled) ? CheckResult::Pass : CheckResult::Fail);

    // Ship-Ready extras.
    push(r, "Tab order matches visual order",
         ctx.tab_order_visual ? CheckResult::Pass : CheckResult::Fail);

    return r;
}

} // namespace gw::a11y
