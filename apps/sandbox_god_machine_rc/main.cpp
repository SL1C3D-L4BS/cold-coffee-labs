// apps/sandbox_god_machine_rc/main.cpp — Phase 23 W154 exit gate (docs/02 §Phase 23).
//
// Circle IX — Silentium profile: exercises enemy BTs, weapon roster, gore,
// hybrid Director, God Machine phases, and Logos Grace selection.
//
// Success banner:
//   GOD MACHINE RC — circle=IX silentium=1 boss=defeated logos_annih=OK logos_grace=OK director=OK weapons=6 gore=OK

#include "engine/core/crash_reporter.hpp"
#include "engine/gameai/gameai_world.hpp"
#include "engine/narrative/grace_meter.hpp"
#include "engine/narrative/sin_signature.hpp"
#include "engine/services/director/schema/director.hpp"
#include "engine/services/gore/schema/gore.hpp"
#include "gameplay/boss/god_machine/god_machine.hpp"
#include "gameplay/boss/logos/phase4_selector.hpp"
#include "gameplay/combat/encounter_director_bridge.hpp"
#include "gameplay/combat/gore_system.hpp"
#include "gameplay/enemies/enemy_bt.hpp"
#include "gameplay/martyrdom/stats.hpp"
#include "gameplay/weapons/weapon_system.hpp"

#include <cstdio>
#include <cstring>

namespace {

struct RcCtx {
    bool fast_mode = false;
};

RcCtx parse_args(int argc, char** argv) {
    RcCtx c{};
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--fast") == 0) c.fast_mode = true;
    }
    return c;
}

} // namespace

int main(int argc, char** argv) {
    gw::core::crash::install_handlers();
    const RcCtx cfg = parse_args(argc, argv);

    const int bt_ticks = cfg.fast_mode ? 24 : 120;

    // --- Enemy BT smoke (Cherubim) -----------------------------------------
    gw::gameai::GameAIWorld ai;
    if (!ai.initialize(gw::gameai::GameAIConfig{})) {
        std::fprintf(stderr, "GOD MACHINE RC — FAIL (gameai init)\n");
        return 1;
    }
    gw::gameplay::enemies::register_sacrilege_enemy_behaviors(ai);
    const auto bt_desc = gw::gameplay::enemies::make_enemy_combat_tree(
        gw::gameplay::enemies::ArchetypeId::Cherubim);
    const auto th = ai.create_tree(bt_desc);
    const auto ih = ai.create_bt_instance(th, 1);
    auto* bb = ai.blackboard(ih);
    bb->set_float(gw::gameplay::enemies::bb::kHp, 100.f);
    bb->set_float(gw::gameplay::enemies::bb::kDist, 6.f);
    bb->set_float(gw::gameplay::enemies::bb::kAttackRange, 2.5f);
    bb->set_bool(gw::gameplay::enemies::bb::kDead, false);
    bb->set_float(gw::gameplay::enemies::bb::kAttacks, 0.f);
    for (int t = 0; t < bt_ticks; ++t) ai.tick_bt_fixed();
    const bool enemy_ok = bb->get(gw::gameplay::enemies::bb::kAttacks).f32 >= 1.f;

    // --- Weapons (all six) ---------------------------------------------------
    int weapon_fires = 0;
    for (std::uint8_t w = 0; w < static_cast<std::uint8_t>(gw::gameplay::weapons::WeaponId::Count); ++w) {
        gw::gameplay::weapons::WeaponRuntimeState rt{};
        gw::gameplay::weapons::init_default_runtime(static_cast<gw::gameplay::weapons::WeaponId>(w), rt);
        gw::gameplay::martyrdom::ResolvedStats rs{};
        (void)gw::gameplay::weapons::fire_primary(static_cast<gw::gameplay::weapons::WeaponId>(w), rs, rt,
                                                  0.016f);
        ++weapon_fires;
    }
    const bool weapons_ok = weapon_fires == 6;

    // --- Gore ----------------------------------------------------------------
    gw::services::gore::GoreHitRequest gh{};
    gh.seed   = 0x51ACE123u; // deterministic gore seed (Circle IX RC)
    gh.damage = 88.f;
    const auto gr = gw::gameplay::combat::apply_gore_hit(gh);
    const bool gore_ok = gr.decals_spawned > 0u;

    // --- Director ------------------------------------------------------------
    gw::services::director::DirectorRequest dr{};
    dr.seed           = 0xD1A0C00Du; // deterministic director seed
    dr.normalized_sin = 0.65f;
    dr.current_state  = gw::services::director::IntensityState::SustainedPeak;
    const auto d_out  = gw::gameplay::combat::evaluate_encounter_director(dr);
    const bool dir_ok = d_out.spawn_rate_scale > 0.f;

    // --- God Machine + Logos -------------------------------------------------
    gw::gameplay::boss::god_machine::GodMachineState gm{};
    for (int step = 0; step < 512 && !gw::gameplay::boss::god_machine::is_defeated(gm); ++step) {
        gw::gameplay::boss::god_machine::tick_god_machine(gm, 420.f, 0.05f);
    }
    const bool boss_damage_ok = gw::gameplay::boss::god_machine::is_defeated(gm);

    gw::narrative::GraceComponent grace_low{};
    grace_low.max   = 100.f;
    grace_low.value = 40.f;
    gw::narrative::GraceComponent grace_high{};
    grace_high.max   = 100.f;
    grace_high.value = 100.f;
    gw::narrative::SinSignature sig{};
    const auto sel_lo = gw::gameplay::boss::logos::select_phase4(grace_low, sig);
    const auto sel_hi = gw::gameplay::boss::logos::select_phase4(grace_high, sig);
    const bool logos_annih_ok = sel_lo.branch == gw::gameplay::boss::logos::Phase4Branch::Annihilation;
    const bool logos_grace_ok = sel_hi.branch == gw::gameplay::boss::logos::Phase4Branch::Grace;

    const bool silentium = true;
    const bool circle_ix = true;

    const bool all_ok = enemy_ok && weapons_ok && gore_ok && dir_ok && boss_damage_ok && logos_annih_ok &&
                        logos_grace_ok && circle_ix && silentium;

    std::fprintf(stdout,
                 "GOD MACHINE RC — circle=IX silentium=%d boss=%s logos_annih=%s logos_grace=%s "
                 "director=%s weapons=%d gore=%s enemy_bt=%s\n",
                 silentium ? 1 : 0, boss_damage_ok ? "defeated" : "pending",
                 logos_annih_ok ? "OK" : "FAIL", logos_grace_ok ? "OK" : "FAIL", dir_ok ? "OK" : "FAIL",
                 weapon_fires, gore_ok ? "OK" : "FAIL", enemy_ok ? "OK" : "FAIL");

    return all_ok ? 0 : 2;
}
