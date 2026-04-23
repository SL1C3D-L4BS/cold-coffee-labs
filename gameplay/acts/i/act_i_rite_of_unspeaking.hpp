#pragma once

// gameplay/acts/i/ — Act I: The Rite of Unspeaking.
// See docs/07 §0.5 and docs/11 §5.1. Scaffold for Phase 21/22 wiring.

namespace gw::gameplay::acts::act_i {

/// Called once per frame during Act I. Drives tutorial-density dialogue,
/// first-rapture hook detection, and Blasphemy unlock gating.
void tick(float dt_sec) noexcept;

} // namespace gw::gameplay::acts::act_i
