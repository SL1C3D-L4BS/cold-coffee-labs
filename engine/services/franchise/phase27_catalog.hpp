#pragma once
// engine/services/franchise/phase27_catalog.hpp — seven franchise services (plan Track J / phase27).

#include <cstdint>

namespace gw::services::franchise {

enum class Phase27ServiceId : std::uint8_t {
    MaterialForge    = 0,
    LevelArchitect   = 1,
    CombatSimulator  = 2,
    Gore             = 3,
    AudioWeave       = 4,
    Director         = 5,
    EditorCopilot    = 6,
    Count            = 7,
};

} // namespace gw::services::franchise
