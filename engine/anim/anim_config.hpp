#pragma once
// engine/anim/anim_config.hpp — Phase 13 Wave 13A (ADR-0039).
//
// Static (per-run) configuration for the animation world. Values are
// bootstrapped from `anim.*` CVars at boot; once the AnimationWorld is
// running the fixed-step is immutable (ADR-0039 §3).

#include <cstdint>

namespace gw::anim {

struct AnimationConfig {
    // Fixed-step animation clock. ADR-0039 pins this to 60 Hz and aligns it
    // to the physics fixed-step clock (shared wave boundary).
    float         fixed_dt{1.0f / 60.0f};
    std::uint32_t max_substeps{4};

    // Capacities (used by the null backend for pre-reserve; Ozz/ACL can
    // ignore these in favour of its own pool sizes).
    std::uint32_t max_skeletons{4096};
    std::uint32_t max_clips{8192};
    std::uint32_t max_instances{8192};

    // Debug draw cap (animation_debug_pass).
    std::uint32_t debug_max_lines{16384};

    // Determinism cadence (1 = every frame). Matches physics contract.
    std::uint32_t hash_every_n{30};

    // Deterministic seed folded into instance id generation.
    std::uint64_t world_seed{0x9E3779B97F4A7C15ULL};
};

} // namespace gw::anim
