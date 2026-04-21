// gameplay/gameplay.cpp — Phase 1 hot-reload skeleton.
// The engine's scripting loader (Phase 2) discovers this library and calls
// gameplay_tick() each simulation step. Phase 1 provides only the ABI
// entry points; real gameplay code lives in gameplay_module/<subsystem>.

#include "engine/core/version.hpp"

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
