#pragma once
// gameplay/simulation_tick.hpp — deterministic PIE/runtime simulation step (Phase 24 loop hardening).

#include <cstdint>

namespace gw::ecs {
class World;
}

namespace gw::gameplay {

/// Reset session scalars (called from gameplay_init / hot-reload entry).
void simulation_reset() noexcept;

/// One simulation step: weapons, boss stub, enemy sim, director heartbeat.
void simulation_step(double dt_seconds) noexcept;

/// Monotonic step counter since last `simulation_reset` (for tests + debug HUD).
[[nodiscard]] std::uint64_t simulation_session_tick_count() noexcept;

/// Optional ECS world for PIE/runtime (plan Track D1). Null clears the binding.
void simulation_bind_ecs_world(gw::ecs::World* world) noexcept;

/// PIE / `sandbox_playable` parity — seed (and optional cvars path for future merge).
void simulation_set_play_bootstrap(std::int64_t universe_seed,
                                   const char* play_cvars_toml_abs_utf8) noexcept;

/// Last `DirectorRequest::seed` from the previous `simulation_step` (tests / diagnostics).
[[nodiscard]] std::uint64_t simulation_last_director_seed() noexcept;

} // namespace gw::gameplay
