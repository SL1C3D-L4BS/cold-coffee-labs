#pragma once
// engine/a11y/events_a11y.hpp — ADR-0071/0072.

#include "engine/a11y/a11y_types.hpp"

#include <cstdint>
#include <string>

namespace gw::a11y {

struct ColorModeChanged {
    ColorMode from{ColorMode::None};
    ColorMode to{ColorMode::None};
    float     strength{1.0f};
};

struct SubtitleCue {
    std::uint64_t cue_id{0};
    std::string   speaker{};
    std::string   text_utf8{};
    std::int64_t  start_unix_ms{0};
    std::int32_t  duration_ms{0};
    std::int32_t  priority{0};
    bool          is_caption{false};
};

struct SubtitleEmitted {
    std::uint64_t cue_id{0};
    std::string   speaker{};
    std::int32_t  duration_ms{0};
};

struct A11yAnnounce {
    std::string text_utf8{};
    Politeness  politeness{Politeness::Polite};
};

struct FocusChanged {
    std::uint64_t node_id{0};
    std::string   name_utf8{};
};

} // namespace gw::a11y
