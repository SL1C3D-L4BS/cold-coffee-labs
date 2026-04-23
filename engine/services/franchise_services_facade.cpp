// engine/services/franchise_services_facade.cpp — pre-eng-franchise-services.

#include "engine/services/franchise_services_facade.hpp"

namespace gw::services {

FacadeSmokeResult franchise_smoke(std::uint64_t seed) noexcept {
    FacadeSmokeResult r{};
    {
        director::DirectorRequest req{};
        req.seed = seed;
        req.intensity_ewma = 0.2f;
        r.director = director::evaluate(req);
    }
    {
        audio_weave::MusicRequest req{};
        req.seed      = seed;
        req.intensity = 0.2f;
        r.music = audio_weave::mix(req);
    }
    {
        material_forge::MaterialEvalRequest req{};
        req.seed = seed;
        r.material = material_forge::evaluate(req);
    }
    {
        level_architect::LayoutRequest req{};
        req.seed = seed;
        r.layout = level_architect::generate(req);
    }
    {
        combat_simulator::EncounterRequest req{};
        req.seed = seed;
        r.encounter = combat_simulator::plan(req);
    }
    {
        gore::GoreHitRequest req{};
        req.seed = seed;
        r.gore = gore::apply_hit(req);
    }
    return r;
}

} // namespace gw::services
