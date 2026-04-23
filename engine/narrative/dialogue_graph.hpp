#pragma once

#include "engine/narrative/narrative_types.hpp"

#include <cstdint>
#include <span>
#include <string_view>

namespace gw::narrative {

/// Compact `.gwdlg` in-memory view — cooked binary layout; see ADR-0103.
struct DialogueLine {
    DialogueLineId id{};
    Speaker        speaker   = Speaker::None;
    Act            act_gate  = Act::None;
    std::uint32_t  circle_gate_mask = 0;     // bit per Circle, 0 = any
    std::uint8_t   cruelty_min = 0;          // 0..255 Sin-signature cruelty_ratio * 255
    std::uint8_t   cruelty_max = 255;
    std::uint8_t   precision_min = 0;
    std::uint8_t   precision_max = 255;
    std::uint32_t  audio_guid_lo = 0;
    std::uint32_t  audio_guid_hi = 0;
    std::string_view text{};                 // UTF-8, pooled in cooked blob
};

class DialogueGraph {
public:
    DialogueGraph() noexcept = default;

    /// Attach a cooked `.gwdlg` blob (borrowed view; blob must outlive graph).
    void attach_blob(std::span<const DialogueLine> lines) noexcept { lines_ = lines; }

    [[nodiscard]] std::span<const DialogueLine> lines() const noexcept { return lines_; }

    [[nodiscard]] const DialogueLine* find(DialogueLineId id) const noexcept;

private:
    std::span<const DialogueLine> lines_{};
};

} // namespace gw::narrative
