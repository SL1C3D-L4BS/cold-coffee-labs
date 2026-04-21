// engine/physics/physics_cvars.cpp — Phase 12.

#include "engine/physics/physics_cvars.hpp"

namespace gw::physics {

namespace {
constexpr std::uint32_t kPersist = config::kCVarPersist;
constexpr std::uint32_t kDevOnly = config::kCVarDevOnly;
} // namespace

PhysicsCVars register_physics_cvars(config::CVarRegistry& r) {
    PhysicsCVars c;

    c.gravity_y = r.register_f32({
        "physics.gravity_y", -9.81f, kPersist, -30.0f, 30.0f, true,
        "World gravity along Y (m/s^2).",
    });
    c.fixed_dt = r.register_f32({
        "physics.fixed_dt", 1.0f / 60.0f, kPersist, 1.0f / 240.0f, 1.0f / 30.0f, true,
        "Fixed-step simulation delta (seconds). Determinism-sensitive.",
    });
    c.substeps_max = r.register_i32({
        "physics.substeps.max", 4, kPersist, 1, 16, true,
        "Max substeps per step() call (catch-up clamp).",
    });
    c.solver_vel_iters = r.register_i32({
        "physics.solver.vel_iters", 10, kDevOnly, 1, 64, true,
        "Solver velocity iterations (dev only).",
    });
    c.solver_pos_iters = r.register_i32({
        "physics.solver.pos_iters", 2, kDevOnly, 1, 64, true,
        "Solver position iterations (dev only).",
    });
    c.sleep_linear = r.register_f32({
        "physics.sleep.linear", 0.01f, kDevOnly, 0.0f, 10.0f, true,
        "Linear velocity threshold below which a body may sleep.",
    });
    c.sleep_angular = r.register_f32({
        "physics.sleep.angular", 0.02f, kDevOnly, 0.0f, 10.0f, true,
        "Angular velocity threshold below which a body may sleep.",
    });
    c.contact_tolerance = r.register_f32({
        "physics.contact.tolerance", 0.001f, kDevOnly, 0.0f, 1.0f, true,
        "Contact tolerance (meters).",
    });
    c.query_batch_cap = r.register_i32({
        "physics.query.batch_cap", 4096, kDevOnly, 64, 65536, true,
        "Max batched physics queries fanned out per job.",
    });

    c.debug_enabled = r.register_bool({
        "physics.debug.enabled", false, kDevOnly, {}, {}, false,
        "Master physics debug draw toggle.",
    });
    c.debug_bodies = r.register_bool({
        "physics.debug.bodies", false, kDevOnly, {}, {}, false,
        "Render debug AABBs for each active body.",
    });
    c.debug_contacts = r.register_bool({
        "physics.debug.contacts", false, kDevOnly, {}, {}, false,
        "Render contact points + normals.",
    });
    c.debug_broadphase = r.register_bool({
        "physics.debug.broadphase", false, kDevOnly, {}, {}, false,
        "Render broadphase pairs.",
    });
    c.debug_character = r.register_bool({
        "physics.debug.character", false, kDevOnly, {}, {}, false,
        "Render character controller capsule + ground normal.",
    });
    c.debug_constraints = r.register_bool({
        "physics.debug.constraints", false, kDevOnly, {}, {}, false,
        "Render constraint axes + limits.",
    });
    c.debug_max_lines = r.register_i32({
        "physics.debug.max_lines", 32768, kDevOnly, 1024, 262144, true,
        "Cap on physics debug lines per frame.",
    });

    c.determinism_hash_every_n = r.register_i32({
        "physics.determinism.hash_every_n", 30, kDevOnly, 1, 600, true,
        "Hash the physics world every Nth frame (1 = every frame).",
    });
    c.determinism_enforce_hash = r.register_bool({
        "physics.determinism.enforce_hash", false, kDevOnly, {}, {}, false,
        "Assert on hash mismatch against the active replay (release playtest flag).",
    });

    return c;
}

void apply_cvars_to_config(const config::CVarRegistry& r, PhysicsConfig& cfg) {
    cfg.gravity.y              = r.get_f32_or("physics.gravity_y",       cfg.gravity.y);
    cfg.fixed_dt               = r.get_f32_or("physics.fixed_dt",        cfg.fixed_dt);
    cfg.max_substeps           = static_cast<std::uint32_t>(
                                     r.get_i32_or("physics.substeps.max", static_cast<std::int32_t>(cfg.max_substeps)));
    cfg.solver_vel_iters       = static_cast<std::uint32_t>(
                                     r.get_i32_or("physics.solver.vel_iters", static_cast<std::int32_t>(cfg.solver_vel_iters)));
    cfg.solver_pos_iters       = static_cast<std::uint32_t>(
                                     r.get_i32_or("physics.solver.pos_iters", static_cast<std::int32_t>(cfg.solver_pos_iters)));
    cfg.sleep_linear_threshold = r.get_f32_or("physics.sleep.linear",      cfg.sleep_linear_threshold);
    cfg.sleep_angular_threshold= r.get_f32_or("physics.sleep.angular",     cfg.sleep_angular_threshold);
    cfg.contact_tolerance      = r.get_f32_or("physics.contact.tolerance", cfg.contact_tolerance);
    cfg.query_batch_cap        = static_cast<std::uint32_t>(
                                     r.get_i32_or("physics.query.batch_cap", static_cast<std::int32_t>(cfg.query_batch_cap)));
    cfg.debug_max_lines        = static_cast<std::uint32_t>(
                                     r.get_i32_or("physics.debug.max_lines", static_cast<std::int32_t>(cfg.debug_max_lines)));
    cfg.hash_every_n           = static_cast<std::uint32_t>(
                                     r.get_i32_or("physics.determinism.hash_every_n", static_cast<std::int32_t>(cfg.hash_every_n)));
}

} // namespace gw::physics
