// gameplay/gameplay.cpp — Phase 1 hot-reload skeleton.
// The engine's scripting loader (Phase 2) discovers this library and calls
// gameplay_tick() each simulation step. Phase 1 provides only the ABI
// entry points; real gameplay code lives in gameplay_module/<subsystem>.

#include "engine/core/version.hpp"
// pre-gp-logos-phase4 stub-wire — reference the Logos selector from the
// gameplay DLL entry so the free function is no longer an orphan symbol.
// Real ownership (weak_ptr in the director) lands Phase 23.
#include "gameplay/boss/logos/phase4_selector.hpp"
// pre-gp-martyrdom-ecs / pre-gp-acts-tick / pre-gp-a11y-init stub-wires —
// reach the gameplay modules so they link and survive DCE until P21 W140
// (a11y HUD) and P22 W143-W146 promote them into ECS systems.
#include "gameplay/a11y/gameplay_a11y.hpp"
#include "gameplay/acts/act_state_component.hpp"
#include "gameplay/martyrdom/blasphemy_state.hpp"
#include "gameplay/martyrdom/god_mode.hpp"
#include "gameplay/martyrdom/grace_meter.hpp"
#include "gameplay/characters/malakor_niccolo/character_state.hpp"
#include "engine/narrative/grace_meter.hpp"

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

    // pre-gp-logos-phase4 stub-wire — exercise the selector free function.
    (void)gw::gameplay::boss::logos::select_phase4_branch(grace);

    // pre-gp-malakor-owner stub-wire — keep Malakor/Niccolò character state
    // referenced until P22 W146 promotes it into the VoiceDirector owner.
    static gw::gameplay::characters::malakor_niccolo::CharacterState malakor_state{};
    (void)malakor_state;
}

GW_GAMEPLAY_API void gameplay_tick(double /*dt_seconds*/) noexcept {
    // Phase 2+ will fill this in.
}

GW_GAMEPLAY_API void gameplay_shutdown() noexcept {
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
GW_GAMEPLAY_API void gw_gameplay_init(void* /*ctx*/) noexcept {
    gameplay_init();
}

GW_GAMEPLAY_API void gw_gameplay_update(void* /*ctx*/, float dt_seconds) noexcept {
    gameplay_tick(static_cast<double>(dt_seconds));
}

GW_GAMEPLAY_API void gw_gameplay_shutdown() noexcept {
    gameplay_shutdown();
}

}  // extern "C"
