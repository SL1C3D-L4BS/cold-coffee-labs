#pragma once
// engine/physics/physics_config.hpp — Phase 12 Wave 12A (ADR-0031).
//
// Static (per-run) configuration for the physics world. Values here are
// bootstrapped from `config::StandardCVars` but once `PhysicsWorld` is
// running the fixed-step, layer matrix, and max-body caps are immutable.

#include "engine/physics/physics_types.hpp"

#include <glm/vec3.hpp>

#include <cstdint>

namespace gw::physics {

struct PhysicsConfig {
    // Fixed-step determinism inputs.
    float         fixed_dt{1.0f / 60.0f};
    std::uint32_t max_substeps{4};

    // Solver knobs.
    std::uint32_t solver_vel_iters{10};
    std::uint32_t solver_pos_iters{2};

    // World constants.
    glm::vec3     gravity{0.0f, -9.81f, 0.0f};
    float         contact_tolerance{0.001f};
    float         sleep_linear_threshold{0.01f};
    float         sleep_angular_threshold{0.02f};

    // Capacities (backend sizing hints).
    std::uint32_t max_bodies{16384};
    std::uint32_t max_body_pairs{16384};
    std::uint32_t max_contact_constraints{8192};
    std::uint32_t query_batch_cap{4096};

    // Debug draw cap.
    std::uint32_t debug_max_lines{32768};

    // Determinism cadence (1 = every frame). Folded into the replay header.
    std::uint32_t hash_every_n{30};

    // Deterministic seed folded into body-id generation.
    std::uint64_t world_seed{0x9E3779B97F4A7C15ULL};
};

} // namespace gw::physics
