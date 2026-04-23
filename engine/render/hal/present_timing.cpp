// engine/render/hal/present_timing.cpp — Part C §21 scaffold.

#include "engine/render/hal/present_timing.hpp"

namespace gw::render::hal {

bool sample_present_timing(PresentTiming& /*out*/) noexcept {
    // Phase 22: bind VK_KHR_present_wait; until then return false so the
    // telemetry layer falls back to the existing frame-time counters.
    return false;
}

} // namespace gw::render::hal
