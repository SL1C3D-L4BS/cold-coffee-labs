// engine/services/gore/gore_service.cpp — Phase 27 scaffold.

#include "engine/services/gore/gore_service.hpp"

namespace gw::services::gore {

GoreHitResult apply_hit(const GoreHitRequest& /*req*/) noexcept {
    // Phase 27: wires into engine/physics (ragdoll), engine/vfx (decals),
    // and gameplay/enemies (limb state).
    return GoreHitResult{};
}

} // namespace gw::services::gore
