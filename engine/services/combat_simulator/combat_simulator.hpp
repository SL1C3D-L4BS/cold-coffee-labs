#pragma once

#include "engine/services/combat_simulator/schema/encounter.hpp"

namespace gw::services::combat_simulator {

[[nodiscard]] EncounterResult plan(const EncounterRequest& req) noexcept;

} // namespace gw::services::combat_simulator
