// engine/input/scanner.cpp
#include "engine/input/scanner.hpp"

#include <algorithm>

namespace gw::input {

void SingleSwitchScanner::set_candidates(std::vector<Action*> candidates) noexcept {
    candidates_ = std::move(candidates);
    // Stable sort by scan_index (negative values filtered out by gatherer).
    std::sort(candidates_.begin(), candidates_.end(),
              [](const Action* a, const Action* b) {
                  return a->accessibility.scan_index < b->accessibility.scan_index;
              });
    if (index_ >= candidates_.size()) index_ = 0;
}

Action* SingleSwitchScanner::highlighted() const noexcept {
    if (candidates_.empty()) return nullptr;
    return candidates_[index_];
}

bool SingleSwitchScanner::tick(float now_ms) noexcept {
    if (!cfg_.auto_scan || candidates_.empty()) return false;
    if (last_advance_ms_ < 0.0f) { last_advance_ms_ = now_ms; return false; }
    if ((now_ms - last_advance_ms_) < cfg_.scan_rate_ms) return false;
    last_advance_ms_ = now_ms;
    index_ = (index_ + 1) % candidates_.size();
    return true;
}

Action* SingleSwitchScanner::activate(float now_ms) noexcept {
    (void)now_ms;
    auto* a = highlighted();
    if (a) {
        a->value.kind = ActionOutputKind::Bool;
        a->value.b = true;
        a->state.last_activation_time_ms = now_ms;
    }
    return a;
}

void SingleSwitchScanner::reset(float now_ms) noexcept {
    index_ = 0;
    last_advance_ms_ = now_ms;
}

std::vector<Action*> gather_scan_candidates(const ContextStack& stack) noexcept {
    std::vector<Action*> out;
    for (auto* set : stack.stack()) {
        if (!set) continue;
        for (auto& a : set->actions()) {
            if (a.accessibility.scan_index >= 0) out.push_back(&a);
        }
    }
    std::sort(out.begin(), out.end(), [](const Action* a, const Action* b) {
        return a->accessibility.scan_index < b->accessibility.scan_index;
    });
    return out;
}

} // namespace gw::input
