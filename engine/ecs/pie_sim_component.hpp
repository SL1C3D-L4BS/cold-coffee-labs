#pragma once
// engine/ecs/pie_sim_component.hpp — tiny ECS tag incremented by gameplay PIE tick (Phase 24).

#include <cstdint>

namespace gw::ecs {

struct PieSimTickComponent {
    std::uint32_t accum = 0;
};

} // namespace gw::ecs
