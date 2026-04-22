// engine/a11y/subtitles.cpp — ADR-0071 §4.

#include "engine/a11y/subtitles.hpp"

#include <algorithm>

namespace gw::a11y {

void SubtitleQueue::set_max_lines(std::int32_t n) noexcept {
    if (n < 1) n = 1;
    if (n > 6) n = 6;
    max_lines_ = n;
}

bool SubtitleQueue::push(SubtitleCue cue, std::int64_t now_ms) {
    if (cue.duration_ms <= 0) cue.duration_ms = 2500;
    if (cue.start_unix_ms <= 0) cue.start_unix_ms = now_ms;
    Slot s{};
    s.cue    = std::move(cue);
    s.end_ms = s.cue.start_unix_ms + s.cue.duration_ms;
    active_.push_back(std::move(s));

    // Sort by priority desc, then start time asc — lowest priority drops.
    std::sort(active_.begin(), active_.end(), [](const Slot& a, const Slot& b) {
        if (a.cue.priority != b.cue.priority) return a.cue.priority > b.cue.priority;
        return a.cue.start_unix_ms < b.cue.start_unix_ms;
    });
    if (static_cast<std::int32_t>(active_.size()) > max_lines_) {
        active_.resize(static_cast<std::size_t>(max_lines_));
        return false; // indicates something was dropped
    }
    ++emitted_;
    return true;
}

void SubtitleQueue::tick(std::int64_t now_ms) {
    active_.erase(std::remove_if(active_.begin(), active_.end(),
                                   [&](const Slot& s) { return s.end_ms <= now_ms; }),
                     active_.end());
}

void SubtitleQueue::snapshot(std::vector<SubtitleLine>& out, std::int64_t now_ms) const {
    out.clear();
    out.reserve(active_.size());
    auto sorted = active_;
    std::sort(sorted.begin(), sorted.end(), [](const Slot& a, const Slot& b) {
        return a.cue.start_unix_ms < b.cue.start_unix_ms;
    });
    for (const auto& s : sorted) {
        SubtitleLine line{};
        line.cue_id       = s.cue.cue_id;
        line.speaker      = s.cue.speaker;
        line.text_utf8    = s.cue.text_utf8;
        line.priority     = s.cue.priority;
        line.remaining_ms = static_cast<std::int32_t>(std::max<std::int64_t>(0, s.end_ms - now_ms));
        out.push_back(std::move(line));
    }
}

void SubtitleQueue::clear() {
    active_.clear();
    emitted_ = 0;
}

} // namespace gw::a11y
