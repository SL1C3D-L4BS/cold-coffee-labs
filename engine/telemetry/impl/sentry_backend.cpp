// engine/telemetry/impl/sentry_backend.cpp — Sentry Native hook (ADR-0060).

#if defined(GW_ENABLE_SENTRY) && GW_ENABLE_SENTRY
// #include <sentry.h>
#endif

namespace gw::telemetry::impl {

void sentry_init_stub() {
#if defined(GW_ENABLE_SENTRY) && GW_ENABLE_SENTRY
    // Phase 15: pin sentry-native 0.7.x + crashpad; wire keyring DSN resolution.
#endif
}

} // namespace gw::telemetry::impl
