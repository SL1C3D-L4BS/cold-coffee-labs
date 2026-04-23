#pragma once

#include <cstdint>

namespace gw::gameplay::martyrdom {

/// God Mode is the narrative name for Rapture (docs/07 §2.6). It is NOT a
/// new mechanical state — the `RaptureState` in martyrdom_components.hpp is
/// authoritative. This component carries presentation/scripting side-channel
/// data only.
struct GodModePresentation {
    std::uint16_t trigger_count      = 0;        // lifetime total this run
    bool          first_trigger_done = false;    // Act I first-trigger hook
    float         distortion_pulse   = 0.f;      // 0..1 presentation strength
};

} // namespace gw::gameplay::martyrdom
