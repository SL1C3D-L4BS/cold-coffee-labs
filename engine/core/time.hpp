#pragma once
// engine/core/time.hpp
// TimeState — shared clock snapshot handed to gameplay every tick.
// Spec ref: Phase 7 §12 (PIE lifecycle), Phase 8 (gameplay tick loop).
//
// Intentionally POD: this type crosses the gameplay C-ABI boundary via the
// `GameplayContext::time` void*, so it must be trivially copyable and stable
// across compilers. Keep fields monotonically appended — never reorder.

#include <cstdint>

namespace gw {

struct TimeState {
    // Seconds since PIE entered (Editor → Playing transition zeroes this).
    double total_s = 0.0;
    // Delta since the previous gameplay tick. Clamped by the host when the
    // editor was paused so gameplay doesn't see huge dt spikes.
    float  dt_s    = 0.f;
    // Monotonic frame counter bumped every tick while in Play.
    std::uint64_t frame = 0;
    // 1.0 = realtime; editor may push 0.0 when Paused. Gameplay is free to
    // ignore this (the host already short-circuits tick()) but simulations
    // that want scrub/slow-mo can consume it.
    float  time_scale = 1.f;
};

}  // namespace gw
