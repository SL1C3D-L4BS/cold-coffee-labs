// engine/core/crash_reporter.cpp — Part C §22 scaffold.
//
// Scaffold only. Real Sentry SDK wiring lands in Phase 22 behind
// `GW_ENABLE_SENTRY=ON` (already exposed by cmake/dependencies.cmake).

#include "engine/core/crash_reporter.hpp"

namespace gw::core::crash {

namespace {
CrashContext g_ctx{};
}

void install_handlers() noexcept {
#if defined(GW_ENABLE_SENTRY) && GW_ENABLE_SENTRY
    // Phase 22: sentry_options_new / sentry_init / set transport options;
    // register SIGSEGV / SEH handlers.
#endif
}

void update_context(const CrashContext& ctx) noexcept { g_ctx = ctx; }

std::uint64_t submit_user_report(const char* /*message*/) noexcept {
#if defined(GW_ENABLE_SENTRY) && GW_ENABLE_SENTRY
    // Phase 22: sentry_capture_event with crash tags.
#endif
    return 0;
}

} // namespace gw::core::crash
