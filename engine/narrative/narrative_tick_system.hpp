#pragma once
// engine/narrative/narrative_tick_system.hpp — pre-eng-narrative-tick wire-up.
//
// Advances the five scaffolded narrative modules (Act state, Sin signature,
// Dialogue graph lookup, Voice director, Grace meter) in a single deterministic
// step. Sandboxes and the gameplay world tick call `advance()` once per
// simulation step to keep the narrative substrate alive.
//
// Ordering (fixed):
//   1. Sin signature accumulator  — observes this-frame events.
//   2. Act state                   — may gate on sin signature + grace.
//   3. Grace meter                 — applies at most one transaction per call.
//   4. Voice director              — selects one line id per tick (may be none).
//
// All inputs are value types so the system is trivially unit-testable.

#include "engine/narrative/act_state.hpp"
#include "engine/narrative/dialogue_graph.hpp"
#include "engine/narrative/grace_meter.hpp"
#include "engine/narrative/narrative_types.hpp"
#include "engine/narrative/sin_signature.hpp"
#include "engine/narrative/voice_director.hpp"

#include <cstdint>

namespace gw::narrative {

struct NarrativeTickState {
    ActState                 act{};
    SinSignatureAccumulator  sin_acc{};
    SinSignature             signature{};
    GraceComponent           grace{};
    const DialogueGraph*     graph = nullptr;
};

struct NarrativeTickInput {
    float                         dt_sec = 0.f;
    SinSignatureStepInput         sin{};
    ActTransitionInput            act{};
    // Optional grace transaction. `delta == 0` signals "no transaction".
    GraceTransaction              grace{};
    // Voice-director inputs read directly from state above; these toggles
    // let the caller override for scripted beats.
    bool                          rapture_active = false;
    bool                          ruin_active    = false;
    bool                          grace_window   = false;
    std::uint32_t                 circle_index   = 0;
    std::uint64_t                 seed           = 0;
};

struct NarrativeTickOutput {
    VoicePick voice{};
};

[[nodiscard]] NarrativeTickOutput advance(NarrativeTickState& state,
                                          const NarrativeTickInput& in) noexcept;

} // namespace gw::narrative
