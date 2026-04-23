#pragma once
// engine/core/crash_reporter.hpp — Part C §22 scaffold (ADR-0114).
//
// Wires the existing GW_ENABLE_SENTRY option into the engine. Stack
// minidumps include seed + build SHA + scene hash so support can reproduce
// crashes deterministically.

#include <cstdint>

namespace gw::core::crash {

struct CrashContext {
    std::uint64_t seed_lo        = 0;
    std::uint64_t seed_hi        = 0;
    std::uint64_t scene_hash     = 0;
    const char*   build_sha      = nullptr;
    const char*   playtest_uuid  = nullptr;
};

/// Initialise the crash reporter. No-op when GW_ENABLE_SENTRY=OFF. Safe to
/// call multiple times.
void install_handlers() noexcept;

/// Update the per-frame context attached to any minidump written after this
/// call. Frequency: call once per simulation tick.
void update_context(const CrashContext& ctx) noexcept;

/// Force a user-initiated report (non-crashing). Returns the report id.
std::uint64_t submit_user_report(const char* message) noexcept;

} // namespace gw::core::crash
