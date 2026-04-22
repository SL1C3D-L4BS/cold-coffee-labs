#pragma once
// engine/core/events/events_world.hpp — world-space lifecycle events (Phase 19-B floating origin).
//
// Published on `gw::events::EventBus<gw::events::OriginShiftPayload>` so subsystems that cache
// absolute f64 positions can translate by `delta` without touching the frozen `events_core.hpp`
// catalogue (ADR-0023).

#include "engine/math/vec.hpp"

namespace gw::events {

/// Emitted when the engine recentres its floating origin; all cached world f64 positions must
/// subtract this delta to stay coherent with the new frame.
struct OriginShiftPayload {
    gw::math::WorldVec3d delta{};
};

} // namespace gw::events
