#pragma once
// engine/physics/physics_cvars.hpp — Phase 12 (ADR-0031, ADR-0036, ADR-0037).
//
// 18 physics CVars registered into the engine-wide CVarRegistry at boot.
// Every tuneable lives here: gravity, solver iters, substep cap, sleep
// thresholds, contact tolerance, debug-draw flags, determinism cadence.

#include "engine/core/config/cvar_registry.hpp"
#include "engine/physics/physics_config.hpp"

namespace gw::physics {

struct PhysicsCVars {
    config::CVarRef<float>        gravity_y;
    config::CVarRef<float>        fixed_dt;
    config::CVarRef<std::int32_t> substeps_max;
    config::CVarRef<std::int32_t> solver_vel_iters;
    config::CVarRef<std::int32_t> solver_pos_iters;
    config::CVarRef<float>        sleep_linear;
    config::CVarRef<float>        sleep_angular;
    config::CVarRef<float>        contact_tolerance;
    config::CVarRef<std::int32_t> query_batch_cap;

    config::CVarRef<bool>         debug_enabled;
    config::CVarRef<bool>         debug_bodies;
    config::CVarRef<bool>         debug_contacts;
    config::CVarRef<bool>         debug_broadphase;
    config::CVarRef<bool>         debug_character;
    config::CVarRef<bool>         debug_constraints;
    config::CVarRef<std::int32_t> debug_max_lines;

    config::CVarRef<std::int32_t> determinism_hash_every_n;
    config::CVarRef<bool>         determinism_enforce_hash;
};

[[nodiscard]] PhysicsCVars register_physics_cvars(config::CVarRegistry& r);

// Bridge CVar state into a mutable PhysicsConfig. Read-only: the config
// struct becomes the single source of truth inside the physics world.
void apply_cvars_to_config(const config::CVarRegistry& r, PhysicsConfig& cfg);

} // namespace gw::physics
