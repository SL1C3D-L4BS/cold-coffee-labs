// engine/services/level_architect/level_architect.cpp — Phase 27 scaffold.

#include "engine/services/level_architect/level_architect.hpp"

namespace gw::services::level_architect {

LayoutResult generate(const LayoutRequest& /*req*/) noexcept {
    // Phase 27 will wire this to engine/world/wfc + GPTM streaming.
    return LayoutResult{0, nullptr};
}

} // namespace gw::services::level_architect
