#pragma once
// engine/anim/morph.hpp — Phase 13 Wave 13D (ADR-0042 §5).
//
// Morph targets (blend shapes) operate on mesh vertex deltas. The engine
// stores morph weights alongside each AnimationGraphInstance; the render
// module consumes the active weight table to compose vertex positions.

#include "engine/anim/anim_types.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace gw::anim {

// -----------------------------------------------------------------------------
// MorphTarget — a named blend-shape slot.
// -----------------------------------------------------------------------------
struct MorphTarget {
    std::string   name{};
    float         default_weight{0.0f};
};

// -----------------------------------------------------------------------------
// MorphSetDesc — registered against a skeleton/mesh at author time. Runtime
// weights are written/read through AnimationWorld::set_morph_weight and
// AnimationWorld::morph_weight respectively.
// -----------------------------------------------------------------------------
struct MorphSetDesc {
    std::string              name{};
    SkeletonHandle           skeleton{};
    std::vector<MorphTarget> targets{};
};

// Deterministic content hash over (name, target names + default weights).
[[nodiscard]] std::uint64_t morph_set_content_hash(const MorphSetDesc& desc) noexcept;

} // namespace gw::anim
