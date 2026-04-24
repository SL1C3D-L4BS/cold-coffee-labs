// gameplay/gameplay.cpp — Phase 1 hot-reload skeleton.
// The engine's scripting loader (Phase 2) discovers this library and calls
// gameplay_tick() each simulation step. Phase 1 provides only the ABI
// entry points; real gameplay code lives in gameplay_module/<subsystem>.

#include "engine/core/version.hpp"
// pre-gp-logos-phase4 stub-wire — reference the Logos selector from the
// gameplay DLL entry so the free function is no longer an orphan symbol.
// Real ownership (weak_ptr in the director) lands Phase 23.
#include "gameplay/boss/god_machine/god_machine.hpp"
#include "gameplay/boss/logos/phase4_selector.hpp"
#include "gameplay/enemies/abyssal/abyssal.hpp"
#include "gameplay/enemies/cherubim/cherubim.hpp"
#include "gameplay/enemies/deacon/deacon.hpp"
#include "gameplay/enemies/hell_knight/hell_knight.hpp"
#include "gameplay/enemies/leviathan/leviathan.hpp"
#include "gameplay/enemies/martyr/martyr.hpp"
#include "gameplay/enemies/painweaver/painweaver.hpp"
#include "gameplay/enemies/warden/warden.hpp"
// pre-gp-martyrdom-ecs / pre-gp-acts-tick / pre-gp-a11y-init stub-wires —
// reach the gameplay modules so they link and survive DCE until P21 W140
// (a11y HUD) and P22 W143-W146 promote them into ECS systems.
#include "gameplay/a11y/gameplay_a11y.hpp"
#include "gameplay/acts/act_state_component.hpp"
#include "gameplay/martyrdom/blasphemy_state.hpp"
#include "gameplay/martyrdom/god_mode.hpp"
#include "gameplay/martyrdom/grace_meter.hpp"
// P22 W143 — canonical Martyrdom ECS components. Imported here so the
// gameplay DLL links every component the ECS registrar references.
#include "gameplay/martyrdom/martyrdom_components.hpp"
#include "gameplay/martyrdom/stats.hpp"
#include "gameplay/characters/malakor_niccolo/character_state.hpp"
#include "engine/narrative/grace_meter.hpp"
#include "engine/narrative/sin_signature.hpp"
#include "engine/services/director/schema/director.hpp"
#include "engine/services/gore/schema/gore.hpp"
#include "gameplay/combat/encounter_director_bridge.hpp"
#include "gameplay/combat/gore_system.hpp"
#include "gameplay/simulation_tick.hpp"
#include "gameplay/weapons/weapon_system.hpp"
#include "engine/ecs/world.hpp"
#include "engine/play/gameplay_context_abi.hpp"
#include "engine/play/playable_paths.hpp"

#include <cstdio>

#if defined(_WIN32)
    #define GW_GAMEPLAY_API __declspec(dllexport)
#else
    #define GW_GAMEPLAY_API __attribute__((visibility("default")))
#endif

extern "C" {

GW_GAMEPLAY_API int gameplay_api_version() noexcept {
    return 1;
}

GW_GAMEPLAY_API const char* gameplay_engine_version() noexcept {
    return gw::core::version_string();
}

GW_GAMEPLAY_API void gameplay_init() noexcept {
    std::fprintf(stdout, "[gameplay] init (Phase 1 skeleton)\n");

    gw::gameplay::simulation_reset();

    // pre-gp-a11y-init stub-wire — seed gameplay a11y defaults and apply once.
    static gw::gameplay::a11y::GameplayA11yConfig a11y_cfg{};
    gw::gameplay::a11y::initialize_defaults(a11y_cfg);
    gw::gameplay::a11y::apply(a11y_cfg);

    // pre-gp-martyrdom-ecs / pre-gp-acts-tick stub-wires — keep the Martyrdom,
    // GodMode, Blasphemy, Grace-bookkeeping, and Act-state components
    // referenced so the objects link until P22 W143-W145 register them as
    // real ECS components.
    static gw::gameplay::acts::ActStateComponent       act_state{};
    static gw::gameplay::martyrdom::GodModePresentation god_mode{};
    static gw::gameplay::martyrdom::BlasphemyState      blasphemy{};
    static gw::gameplay::martyrdom::GraceBookkeeping    grace_book{};
    static gw::narrative::GraceComponent                grace{};
    (void)act_state;
    (void)god_mode;
    (void)blasphemy.active();
    gw::narrative::GraceTransaction tx{};
    gw::gameplay::martyrdom::try_apply_grace(grace, grace_book, tx);

    // P22 W143 — canonical component instantiation keeps the registrar types
    // linked until the gameplay world ECS takes ownership in P22 W148.
    static gw::gameplay::martyrdom::SinComponent           sin_c{};
    static gw::gameplay::martyrdom::MantraComponent        mantra_c{};
    static gw::gameplay::martyrdom::RaptureState           rapture_c{};
    static gw::gameplay::martyrdom::RuinState              ruin_c{};
    static gw::gameplay::martyrdom::ResolvedStats          resolved_c{};
    static gw::gameplay::martyrdom::ActiveBlasphemies      active_b{};
    static gw::gameplay::martyrdom::RaptureTriggerCounter  trigger_counter{};
    (void)sin_c;
    (void)mantra_c;
    (void)rapture_c;
    (void)ruin_c;
    (void)resolved_c;
    (void)active_b;
    (void)trigger_counter;

    // pre-gp-logos-phase4 stub-wire — exercise the selector free function.
    (void)gw::gameplay::boss::logos::select_phase4_branch(grace);
    gw::narrative::SinSignature sin_fingerprint{};
    sin_fingerprint.precision_ratio = 0.42f;
    (void)gw::gameplay::boss::logos::select_phase4(grace, sin_fingerprint);
    (void)gw::narrative::grace_mechanic_unlocked_for_act(gw::narrative::Act::III);

    gw::gameplay::boss::god_machine::GodMachineState gm{};
    gw::gameplay::boss::god_machine::tick_god_machine(gm, 500.f, 0.05f);
    (void)gw::gameplay::boss::god_machine::is_defeated(gm);

    // Phase 23 — enemy BT factories linked into the gameplay DLL.
    (void)gw::gameplay::enemies::abyssal::behavior_tree();
    (void)gw::gameplay::enemies::cherubim::behavior_tree();
    (void)gw::gameplay::enemies::deacon::behavior_tree();
    (void)gw::gameplay::enemies::hell_knight::behavior_tree();
    (void)gw::gameplay::enemies::leviathan::behavior_tree();
    (void)gw::gameplay::enemies::martyr::behavior_tree();
    (void)gw::gameplay::enemies::painweaver::behavior_tree();
    (void)gw::gameplay::enemies::warden::behavior_tree();

    // Phase 23 — gore + director bridge (franchise services).
    gw::services::gore::GoreHitRequest gh{};
    gh.seed      = 0xC0FFEEu;
    gh.damage    = 80.f;
    gh.archetype = 3;
    (void)gw::gameplay::combat::apply_gore_hit(gh);
    gw::services::director::DirectorRequest dr{};
    dr.seed                = 0xD00Du;
    dr.normalized_sin      = 0.7f;
    dr.current_state       = gw::services::director::IntensityState::BuildUp;
    (void)gw::gameplay::combat::evaluate_encounter_director(dr);

    // Phase 23 — weapon roster (docs/07 §4).
    for (std::uint8_t wi = 0; wi < static_cast<std::uint8_t>(gw::gameplay::weapons::WeaponId::Count);
         ++wi) {
        gw::gameplay::weapons::WeaponRuntimeState wrt{};
        gw::gameplay::weapons::init_default_runtime(static_cast<gw::gameplay::weapons::WeaponId>(wi),
                                                    wrt);
        gw::gameplay::martyrdom::ResolvedStats wrs{};
        (void)gw::gameplay::weapons::fire_primary(static_cast<gw::gameplay::weapons::WeaponId>(wi),
                                                  wrs, wrt, 0.016f);
    }

    // pre-gp-malakor-owner stub-wire — keep Malakor/Niccolò character state
    // referenced until P22 W146 promotes it into the VoiceDirector owner.
    static gw::gameplay::characters::malakor_niccolo::CharacterState malakor_state{};
    (void)malakor_state;
}

GW_GAMEPLAY_API void gameplay_tick(double dt_seconds) noexcept {
    gw::gameplay::simulation_step(dt_seconds);
}

GW_GAMEPLAY_API void gameplay_shutdown() noexcept {
    gw::gameplay::simulation_reset();
    std::fprintf(stdout, "[gameplay] shutdown\n");
}

// ---------------------------------------------------------------------------
// Editor Play-in-Editor ABI (gw_gameplay_*) — Phase 7 §12.
// These wrap the Phase-1 `gameplay_*` exports with the signatures the editor
// host expects:
//     gw_gameplay_init     (GameplayContext*)
//     gw_gameplay_update   (GameplayContext*, float dt)
//     gw_gameplay_shutdown ()
// Until Phase 8 wires a real `GameplayContext`, the context pointer is
// accepted as opaque void* and ignored. This preserves Phase 1 ABI for
// standalone callers while letting PIE resolve its symbols.
// ---------------------------------------------------------------------------
GW_GAMEPLAY_API void gw_gameplay_init(void* ctx_v) noexcept {
    gameplay_init();
    if (!ctx_v) {
        gw::gameplay::simulation_set_play_bootstrap(gw::play::kDefaultPlayUniverseSeed,
                                                     nullptr);
        return;
    }
    const auto* ctx = static_cast<const gw::play::GameplayContext*>(ctx_v);
    if (ctx->version >= 2u) {
        gw::gameplay::simulation_set_play_bootstrap(ctx->play_universe_seed,
                                                     ctx->play_cvars_toml_abs_utf8);
    } else {
        gw::gameplay::simulation_set_play_bootstrap(gw::play::kDefaultPlayUniverseSeed,
                                                     nullptr);
    }
}

GW_GAMEPLAY_API void gw_gameplay_update(void* /*ctx*/, float dt_seconds) noexcept {
    gameplay_tick(static_cast<double>(dt_seconds));
}

GW_GAMEPLAY_API void gw_gameplay_shutdown() noexcept {
    gameplay_shutdown();
}

/// Optional PIE hook — binds the live authoring `World*` for ECS-driven sim (plan Track D1).
GW_GAMEPLAY_API void gameplay_bind_ecs_world(void* world) noexcept {
    gw::gameplay::simulation_bind_ecs_world(static_cast<gw::ecs::World*>(world));
}

}  // extern "C"
