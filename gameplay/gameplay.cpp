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

}  // extern "C"
