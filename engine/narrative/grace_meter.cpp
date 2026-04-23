// engine/narrative/grace_meter.cpp — Phase 22 scaffold (ADR-0101).

#include "engine/narrative/grace_meter.hpp"

#include <algorithm>

namespace gw::narrative {

float grace_source_default_delta(GraceSource src) noexcept {
    switch (src) {
    case GraceSource::UnwrittenSpared:       return 15.f;
    case GraceSource::CircleNoRapture:       return 25.f;
    case GraceSource::GloryKillsOnlyHostile: return 10.f;
    case GraceSource::ParryLogosPhase1:      return 50.f;
    case GraceSource::BlasphemyNpcSave:      return  5.f;
    }
    return 0.f;
}

void apply_grace_transaction(GraceComponent& grace, const GraceTransaction& tx) noexcept {
    grace.value = std::clamp(grace.value + tx.delta, 0.f, grace.max);
}

} // namespace gw::narrative
