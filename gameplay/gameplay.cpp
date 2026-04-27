// gameplay/gameplay.cpp — Gameplay DLL ABI (hot-reload entry points).
// The host loads this library and calls gameplay_init / gameplay_tick /
// gameplay_shutdown each session. Heavy subsystem linkage and VM touch
// surfaces live in gameplay_dll_linkage.cpp so symbols are not orphaned.

#include "engine/core/version.hpp"
#include "gameplay/gameplay_dll_linkage.hpp"
#include "gameplay/simulation_tick.hpp"
#include "engine/ecs/world.hpp"
#include "engine/play/gameplay_context_abi.hpp"
#include "engine/play/playable_paths.hpp"
#include "engine/runtime/progs_vm.hpp"

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

    gw_gameplay_touch_dll_linkage();
}

GW_GAMEPLAY_API void gameplay_tick(double dt_seconds) noexcept {
    (void)gw::runtime::progs::tick_vm(0u);
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
