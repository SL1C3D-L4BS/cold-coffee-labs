// gameplay/simulation_tick.cpp — shared by greywater_gameplay DLL and unit tests.

#include "gameplay/simulation_tick.hpp"

#include "gameplay/boss/god_machine/god_machine.hpp"
#include "gameplay/combat/encounter_director_bridge.hpp"
#include "gameplay/enemies/enemy_bt.hpp"
#include "gameplay/enemies/enemy_types.hpp"
#include "gameplay/weapons/weapon_system.hpp"
#include "gameplay/weapons/weapon_types.hpp"
#include "engine/ecs/pie_sim_component.hpp"
#include "engine/ecs/world.hpp"
#include "engine/play/playable_paths.hpp"
#include "engine/services/director/schema/director.hpp"

#include <array>
#include <cmath>
#include <cstdint>

namespace gw::gameplay {
namespace {

struct Session {
    bool                                                          inited = false;
    std::uint64_t                                                 tick_index = 0;
    std::int64_t                                                  play_universe_seed =
        gw::play::kDefaultPlayUniverseSeed;
    std::uint64_t                                                 last_director_seed = 0;
    boss::god_machine::GodMachineState                            god_machine{};
    std::array<weapons::WeaponRuntimeState,
               static_cast<std::size_t>(weapons::WeaponId::Count)> weapon_rt{};
    enemies::EnemyCombatState sample_enemy{};
    services::director::IntensityState                            dir_phase =
        services::director::IntensityState::BuildUp;
};

Session& session() noexcept {
    static Session s{};
    return s;
}

void ensure_inited(Session& s) noexcept {
    if (s.inited) return;
    for (std::uint8_t i = 0;
         i < static_cast<std::uint8_t>(weapons::WeaponId::Count); ++i) {
        weapons::init_default_runtime(static_cast<weapons::WeaponId>(i),
                                      s.weapon_rt[static_cast<std::size_t>(i)]);
    }
    s.sample_enemy = enemies::EnemyCombatState{.hp             = 80.f,
                                                .max_hp         = 100.f,
                                                .dist_to_player = 12.f,
                                                .attack_range   = 2.5f,
                                                .dead           = false};
    s.inited       = true;
}

} // namespace

static gw::ecs::World* g_bound_ecs_world = nullptr;

void simulation_bind_ecs_world(gw::ecs::World* world) noexcept {
    g_bound_ecs_world = world;
}

void simulation_set_play_bootstrap(std::int64_t universe_seed,
                                   const char* play_cvars_toml_abs_utf8) noexcept {
    session().play_universe_seed = universe_seed;
    (void)play_cvars_toml_abs_utf8;
}

void simulation_reset() noexcept {
    g_bound_ecs_world = nullptr;
    session()         = Session{};
}

std::uint64_t simulation_session_tick_count() noexcept {
    return session().tick_index;
}

std::uint64_t simulation_last_director_seed() noexcept {
    return session().last_director_seed;
}

void simulation_step(double dt_seconds) noexcept {
    Session& s = session();
    ensure_inited(s);

    const float dt =
        (dt_seconds > 0.0 && std::isfinite(dt_seconds))
            ? static_cast<float>(dt_seconds)
            : 0.f;

    for (std::uint8_t i = 0; i < static_cast<std::uint8_t>(weapons::WeaponId::Count);
         ++i) {
        weapons::tick_cooldowns(s.weapon_rt[static_cast<std::size_t>(i)], dt);
    }

    boss::god_machine::tick_god_machine(s.god_machine, 0.f, dt);

    enemies::simulate_enemy_tick(s.sample_enemy, enemies::ArchetypeId::Martyr, 6.f, dt);

    if (gw::ecs::World* w = g_bound_ecs_world) {
        w->for_each<gw::ecs::PieSimTickComponent>(
            [](gw::ecs::Entity, gw::ecs::PieSimTickComponent& tag) { ++tag.accum; });
    }

    services::director::DirectorRequest dr{};
    dr.logical_tick   = s.tick_index;
    const auto seed_base =
        static_cast<std::uint64_t>(static_cast<std::int64_t>(s.play_universe_seed));
    dr.seed           = seed_base ^ 0xC0DA'C0DA'0000'0000u ^ s.tick_index;
    if (gw::ecs::World* w = g_bound_ecs_world) {
        dr.seed ^= static_cast<std::uint64_t>(w->entity_count()) * 0x9E3779B97F4A7C15llu;
    }
    s.last_director_seed = dr.seed;
    dr.normalized_sin = 0.5f
        + 0.49f * std::sin(static_cast<float>(s.tick_index) * 0.01f);
    dr.current_state = s.dir_phase;
    (void)combat::evaluate_encounter_director(dr);

    ++s.tick_index;
    if ((s.tick_index % 240u) == 0u) {
        using IS = services::director::IntensityState;
        switch (s.dir_phase) {
        case IS::BuildUp:       s.dir_phase = IS::SustainedPeak; break;
        case IS::SustainedPeak: s.dir_phase = IS::PeakFade; break;
        case IS::PeakFade:      s.dir_phase = IS::Relax; break;
        case IS::Relax:
        default:                s.dir_phase = IS::BuildUp; break;
        }
    }
}

} // namespace gw::gameplay
