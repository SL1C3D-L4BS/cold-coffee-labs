#pragma once
// engine/a11y/subtitles.hpp — ADR-0071 §4.

#include "engine/a11y/events_a11y.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace gw::a11y {

struct SubtitleLine {
    std::uint64_t cue_id{0};
    std::string   speaker{};
    std::string   text_utf8{};
    std::int32_t  priority{0};
    std::int32_t  remaining_ms{0};
};

class SubtitleQueue {
public:
    SubtitleQueue() = default;

    void set_max_lines(std::int32_t n) noexcept;
    [[nodiscard]] std::int32_t max_lines() const noexcept { return max_lines_; }

    // Insert a cue.  Returns true when accepted; false when dropped due to
    // max-line overflow (after resolving priority ties).
    bool push(SubtitleCue cue, std::int64_t now_unix_ms);

    // Advance the clock; expires finished cues.
    void tick(std::int64_t now_unix_ms);

    // Snapshot currently active cues (ordered by start time asc).
    void snapshot(std::vector<SubtitleLine>& out, std::int64_t now_unix_ms) const;

    [[nodiscard]] std::size_t active_count() const noexcept { return active_.size(); }
    [[nodiscard]] std::size_t emitted_count() const noexcept { return emitted_; }

    void clear();

private:
    struct Slot {
        SubtitleCue   cue{};
        std::int64_t  end_ms{0};
    };
    std::vector<Slot> active_{};
    std::int32_t      max_lines_{3};
    std::size_t       emitted_{0};
};

} // namespace gw::a11y
