#pragma once

#include "engine/narrative/dialogue_graph.hpp"
#include "engine/narrative/narrative_types.hpp"
#include "engine/narrative/sin_signature.hpp"

#include <cstdint>

namespace gw::narrative {

struct VoiceDirectorContext {
    const DialogueGraph* graph           = nullptr;
    Act                  act             = Act::I;
    std::uint32_t        circle_index    = 0;
    bool                 rapture_active  = false;
    bool                 ruin_active     = false;
    bool                 grace_window    = false;
    SinSignature         signature{};
    std::uint64_t        seed            = 0;
};

struct VoicePick {
    DialogueLineId line{};
    Speaker        speaker = Speaker::None;
};

/// Deterministic line selection. See docs/07 §15b.2.
///
/// Rules:
///   - `rapture_active` → Malakor only.
///   - `ruin_active`    → Niccolo only.
///   - `grace_window`   → the terminal "We forgive" line, both voices (returned as Speaker::None
///                         to signal combined delivery; renderer handles both).
///   - otherwise → weighted roll seeded by `seed`, biased by Sin-signature.
[[nodiscard]] VoicePick pick_voice_line(const VoiceDirectorContext& ctx) noexcept;

} // namespace gw::narrative
