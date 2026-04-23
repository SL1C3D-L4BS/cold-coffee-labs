#pragma once

#include <cstdint>

namespace gw::services::combat_simulator {

struct EncounterId { std::uint32_t value = 0; };

struct EncounterRequest {
    std::uint64_t seed              = 0;
    std::uint32_t profile_index     = 0;
    float         intensity_target  = 0.5f;
    std::uint32_t spawn_budget      = 16;
    std::uint32_t variety_mask      = 0xFFFFFFFFu;
};

struct EncounterSpawn {
    std::uint32_t archetype = 0;      // consumer-defined archetype id
    float         pos[3]{0.f, 0.f, 0.f};
    std::uint32_t wave_index = 0;
};

struct EncounterResult {
    std::uint32_t spawn_count  = 0;
    const EncounterSpawn* spawns = nullptr;
};

} // namespace gw::services::combat_simulator
