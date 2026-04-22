#pragma once
// engine/world/origin/floating_origin.hpp — Phase 19-B f64 floating origin (2048 m recentre threshold).

#include "engine/core/events/event_bus.hpp"
#include "engine/core/events/events_world.hpp"
#include "engine/world/streaming/chunk_coord.hpp"

namespace gw::world::origin {

/// Maintains the active translation between absolute world metres and simulation-local coordinates.
/// When the camera drifts farther than `kRecentreThresholdMeters` from the stored origin, a
/// `gw::events::OriginShiftPayload` is published so caches subtract `delta`.
class FloatingOrigin {
public:
    explicit FloatingOrigin(gw::events::EventBus<gw::events::OriginShiftPayload>* bus = nullptr) noexcept;

    [[nodiscard]] gw::world::streaming::Vec3_f64 origin() const noexcept { return origin_; }

    /// Returns `true` when a shift was applied (and an event emitted, if `bus_` is non-null).
    [[nodiscard]] bool maybe_recentre(const gw::world::streaming::Vec3_f64& camera_world_pos) noexcept;

    static constexpr double kRecentreThresholdMeters = 2048.0;

private:
    gw::events::EventBus<gw::events::OriginShiftPayload>* bus_{nullptr};
    gw::world::streaming::Vec3_f64                       origin_{};
};

} // namespace gw::world::origin
