#pragma once
// engine/anim/blend_tree.hpp — Phase 13 Wave 13C (ADR-0041).
//
// A BlendTree is a directed acyclic graph of blend nodes. Each node is a
// typed struct with deterministic semantics (see ADR-0041 §2). The null
// backend evaluates the tree via a depth-first traversal; the Ozz/ACL
// backend lowers the same tree into ozz::SamplingJob + ozz::BlendingJob
// plans but produces a bit-identical pose at the root.

#include "engine/anim/anim_types.hpp"
#include "engine/anim/pose.hpp"

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace gw::anim {

// -----------------------------------------------------------------------------
// Node kinds. `Input` is a leaf that pulls pose #i from the current
// AnimationGraphInstance's clip binding table; all others are internal.
// -----------------------------------------------------------------------------
enum class BlendKind : std::uint8_t {
    Input      = 0,  // reference to a bound clip
    Lerp       = 1,  // lerp(a, b, weight)
    OneMinus   = 2,  // out = rest + (1-w) * (in - rest) — "fade to rest"
    Additive   = 3,  // out = a + w * (b - rest)
    MaskLerp   = 4,  // out = mix(a, b, w * mask[i])
    StateMachine = 5,// single active state; weighted blend across transitions
};

// -----------------------------------------------------------------------------
// Node descriptors — plain data. `inputs` are indices into the same node
// array; indices must be strictly less than the current node's index
// (topological order), so evaluation is a single forward sweep.
// -----------------------------------------------------------------------------
struct BlendNodeDesc {
    BlendKind                 kind{BlendKind::Input};
    std::uint16_t             input_index{0};       // used when kind == Input
    std::uint16_t             mask_index{0xFFFFu};  // optional joint mask (index into masks[])
    float                     weight{1.0f};
    std::vector<std::uint16_t> children{};           // child node indices
};

// -----------------------------------------------------------------------------
// State-machine transition. A state IS a single node (its sub-graph is
// built with the other BlendNodeDescs). On transition we crossfade over
// `duration_s` using pose_lerp across the two active poses.
// -----------------------------------------------------------------------------
struct SMState {
    std::string          name{};
    std::uint16_t        node_index{0};   // entry node of this state
};

struct SMTransition {
    std::uint16_t from_state{0};
    std::uint16_t to_state{0};
    std::string   trigger{};    // consumed by `AnimationWorld::set_instance_parameter`
    float         duration_s{0.25f};
};

// -----------------------------------------------------------------------------
// BlendTreeDesc — full graph description.
// -----------------------------------------------------------------------------
struct BlendTreeDesc {
    std::string                  name{};
    SkeletonHandle               skeleton{};
    std::uint64_t                target_skeleton_hash{0};
    std::vector<BlendNodeDesc>   nodes{};           // topologically sorted
    std::uint16_t                root_index{0};     // typically nodes.size()-1
    // Joint masks — each is a {name, per-joint weight in [0,1]} pair. Nodes
    // with kind == MaskLerp pick a mask by `mask_index`.
    std::vector<std::vector<float>> masks{};
    // State-machine overlay (optional).
    std::vector<SMState>          states{};
    std::vector<SMTransition>     transitions{};
    std::uint16_t                 default_state{0};
};

// -----------------------------------------------------------------------------
// Compile-time validator. Returns 0 on success; otherwise a bitmask of the
// first N failed invariants so the caller can print a stable, stable-
// ordered error list. Intentionally does NOT allocate.
// -----------------------------------------------------------------------------
enum BlendTreeValidationError : std::uint32_t {
    kBT_OK                       = 0,
    kBT_InputIndexOutOfRange     = 1u << 0,
    kBT_ChildAfterSelf           = 1u << 1,
    kBT_MaskIndexOutOfRange      = 1u << 2,
    kBT_ChildCountMismatch       = 1u << 3,
    kBT_StateNodeOutOfRange      = 1u << 4,
    kBT_TransitionStateOutOfRange= 1u << 5,
    kBT_DefaultStateOutOfRange   = 1u << 6,
    kBT_WeightOutOfRange         = 1u << 7,
};
[[nodiscard]] std::uint32_t validate_blend_tree(const BlendTreeDesc& desc) noexcept;

// Deterministic content hash over the full tree.
[[nodiscard]] std::uint64_t blend_tree_content_hash(const BlendTreeDesc& desc) noexcept;

} // namespace gw::anim
