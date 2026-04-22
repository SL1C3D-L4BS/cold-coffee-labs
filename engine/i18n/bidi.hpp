#pragma once
// engine/i18n/bidi.hpp — UBA-lite bidirectional segmentation (ADR-0070).
//
// The null build implements a "strong-LTR / strong-RTL" runpartitioning
// that is good enough for every Phase-16 locale we ship (en, fr, de,
// ja, zh).  The ICU build (`GW_ENABLE_ICU=1`) replaces this with
// `ubidi_countRuns` / `ubidi_getVisualMap` in `impl/icu_bidi.cpp`.

#include "engine/ui/text_shaper.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

namespace gw::i18n {

enum class BaseDirection : std::uint8_t { LTR = 0, RTL = 1, Auto = 2 };

struct BidiRun {
    std::uint32_t         start_byte{0};
    std::uint32_t         size_bytes{0};
    gw::ui::TextDirection direction{gw::ui::TextDirection::LTR};
    gw::ui::TextScript    script{gw::ui::TextScript::Unknown};
};

// Partition the UTF-8 string into runs of homogeneous direction + script.
// Returns the count written into `out` (resized to fit).
std::size_t bidi_segments(std::string_view utf8,
                           BaseDirection    base,
                           std::vector<BidiRun>& out);

// Returns the effective base direction given `Auto` input (first strong
// character wins, fallback LTR per UBA §P2).
[[nodiscard]] BaseDirection resolve_base_direction(std::string_view utf8,
                                                    BaseDirection hint) noexcept;

} // namespace gw::i18n
