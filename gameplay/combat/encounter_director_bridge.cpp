// gameplay/combat/encounter_director_bridge.cpp — Phase 23 W151

#include "gameplay/combat/encounter_director_bridge.hpp"

#include "engine/services/director/director_service.hpp"

namespace gw::gameplay::combat {

gw::services::director::DirectorResult evaluate_encounter_director(
    const gw::services::director::DirectorRequest& req) noexcept {
    return gw::services::director::evaluate(req);
}

} // namespace gw::gameplay::combat
