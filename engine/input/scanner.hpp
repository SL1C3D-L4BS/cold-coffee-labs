#pragma once
// engine/input/scanner.hpp — single-switch scanner (ADR-0021 §2.8).
//
// The scanner lets a user with a single button (or breath switch, or
// Adaptive Controller pad) cycle through eligible actions and activate
// the highlighted one. Scan candidates are actions with
// `accessibility.scan_index >= 0`, ordered by that index (stable).
//
// Timing:
//   - scan_rate_ms: time the highlight dwells on each action.
//   - activation mode: press the scan key to activate the currently
//     highlighted action. Hold-to-select (dwell) is supported when
//     `hold_to_select_ms > 0`.

#include "engine/input/action.hpp"

#include <string>
#include <vector>

namespace gw::input {

struct ScannerConfig {
    float  scan_rate_ms{900.0f};
    float  hold_to_select_ms{0.0f};
    // Whether to auto-scan forward (true) or require user to advance with
    // a second switch (false, two-switch scanning).
    bool   auto_scan{true};
};

class SingleSwitchScanner {
public:
    SingleSwitchScanner() = default;
    ~SingleSwitchScanner() = default;

    void configure(ScannerConfig cfg) noexcept { cfg_ = cfg; }
    [[nodiscard]] const ScannerConfig& config() const noexcept { return cfg_; }

    // Collect eligible candidates from the set(s). Stable order: by scan_index.
    void set_candidates(std::vector<Action*> candidates) noexcept;
    [[nodiscard]] std::size_t candidate_count() const noexcept { return candidates_.size(); }
    [[nodiscard]] Action* highlighted() const noexcept;
    [[nodiscard]] std::size_t highlight_index() const noexcept { return index_; }

    // Per-frame tick. `now_ms` must be monotonic. Returns true if the
    // highlight moved this frame (UI can ding/flash).
    bool tick(float now_ms) noexcept;

    // Activate the currently highlighted action. Returns a pointer to the
    // action that was "pressed" this frame (so the host can dispatch events).
    [[nodiscard]] Action* activate(float now_ms) noexcept;

    // Reset scan to the first candidate.
    void reset(float now_ms) noexcept;

private:
    ScannerConfig         cfg_{};
    std::vector<Action*>  candidates_{};
    std::size_t           index_{0};
    float                 last_advance_ms_{-1000.0f};
};

// Utility: gather all scan-eligible actions from a context stack (top→bottom)
// in order of their `scan_index` flag.
[[nodiscard]] std::vector<Action*> gather_scan_candidates(const ContextStack& stack) noexcept;

} // namespace gw::input
