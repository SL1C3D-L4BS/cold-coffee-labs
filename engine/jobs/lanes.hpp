#pragma once
// engine/jobs/lanes.hpp — Phase 15 named job lanes (ADR-0064 §3).
//
// ADR-0064 §3 mandates that `persist_io` / `telemetry_io` are *conventions*
// on `jobs::Scheduler::submit` priorities, not additional thread pools:
// the engine owns exactly one pool and every Phase-15 subsystem routes its
// IO through this lane-helper so thread-ownership stays auditable.
//
// Callers:
//   gw::jobs::submit_persist_io    (persist.step autosave, DSAR, migration)
//   gw::jobs::submit_telemetry_io  (tele.flush HTTP POST, Sentry upload)
//   gw::jobs::submit_background    (cloud sync poll, quota check)

#include "engine/jobs/scheduler.hpp"

namespace gw::jobs {

/// Phase-15 lane constants. Lanes are priority annotations — the scheduler's
/// priority queue already orders them; see `job_queue.hpp`.
enum class Lane : std::uint8_t {
    PersistIo   = static_cast<std::uint8_t>(JobPriority::Normal),
    TelemetryIo = static_cast<std::uint8_t>(JobPriority::Low),
    Background  = static_cast<std::uint8_t>(JobPriority::Background),
};

[[nodiscard]] inline JobHandle submit_persist_io(Scheduler& s, JobFn fn) {
    return s.submit(JobPriority::Normal, std::move(fn));
}

[[nodiscard]] inline JobHandle submit_telemetry_io(Scheduler& s, JobFn fn) {
    return s.submit(JobPriority::Low, std::move(fn));
}

[[nodiscard]] inline JobHandle submit_background(Scheduler& s, JobFn fn) {
    return s.submit(JobPriority::Background, std::move(fn));
}

/// Returns the string name of a lane — used by tests / console diagnostics.
[[nodiscard]] inline const char* lane_name(Lane l) noexcept {
    switch (l) {
    case Lane::PersistIo:   return "persist_io";
    case Lane::TelemetryIo: return "telemetry_io";
    case Lane::Background:  return "background";
    }
    return "?";
}

} // namespace gw::jobs
