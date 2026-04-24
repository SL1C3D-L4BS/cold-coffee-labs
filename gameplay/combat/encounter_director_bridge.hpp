#pragma once
// gameplay/combat/encounter_director_bridge.hpp — Phase 23 W151

#include "engine/services/director/schema/director.hpp"

namespace gw::gameplay::combat {

[[nodiscard]] gw::services::director::DirectorResult evaluate_encounter_director(
    const gw::services::director::DirectorRequest& req) noexcept;

} // namespace gw::gameplay::combat
