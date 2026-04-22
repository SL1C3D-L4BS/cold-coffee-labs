#include "engine/world/origin/floating_origin.hpp"

namespace gw::world::origin {

FloatingOrigin::FloatingOrigin(gw::events::EventBus<gw::events::OriginShiftPayload>* bus) noexcept
    : bus_(bus) {}

bool FloatingOrigin::maybe_recentre(const gw::world::streaming::Vec3_f64& camera_world_pos) noexcept {
    const gw::world::streaming::Vec3_f64 d(camera_world_pos.x() - origin_.x(),
                                           camera_world_pos.y() - origin_.y(),
                                           camera_world_pos.z() - origin_.z());
    if (d.length() <= kRecentreThresholdMeters) {
        return false;
    }
    const gw::world::streaming::Vec3_f64 delta = d;
    origin_                                    = camera_world_pos;
    if (bus_ != nullptr) {
        gw::events::OriginShiftPayload payload{};
        payload.delta = delta;
        bus_->publish(payload);
    }
    return true;
}

} // namespace gw::world::origin
