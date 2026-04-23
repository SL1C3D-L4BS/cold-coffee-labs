#pragma once
// engine/services/franchise_services_facade.hpp — pre-eng-franchise-services
//
// Single entry-point facade that reaches each of the six franchise services
// from one place. Owning subsystems (render init, world init, combat system,
// damage system, audio init, gameplay loop) each call one of the helpers
// below at startup so the service symbols link and are observable from
// instrumentation / CI lint.
//
// Phase 27 replaces the facade with per-owner direct dispatch and adds state.
// Until then, this module keeps the services out of the "orphan scaffold"
// bucket reported by `docs/prompts/00_WIRE_UP_AUDIT.md`.

#include "engine/services/audio_weave/audio_weave.hpp"
#include "engine/services/combat_simulator/combat_simulator.hpp"
#include "engine/services/director/director_service.hpp"
#include "engine/services/gore/gore_service.hpp"
#include "engine/services/level_architect/level_architect.hpp"
#include "engine/services/material_forge/material_forge.hpp"

namespace gw::services {

/// Evaluate one sample request on every service. Used by startup diagnostics
/// and by sandbox apps to prove the shim chain links end-to-end.
struct FacadeSmokeResult {
    director::DirectorResult           director{};
    audio_weave::MusicResult           music{};
    material_forge::MaterialEvalResult material{};
    level_architect::LayoutResult      layout{};
    combat_simulator::EncounterResult  encounter{};
    gore::GoreHitResult                gore{};
};

[[nodiscard]] FacadeSmokeResult franchise_smoke(std::uint64_t seed) noexcept;

} // namespace gw::services
