#pragma once
// engine/anim/animation_world.hpp — Phase 13 Wave 13A (ADR-0039).
//
// PIMPL facade over the animation backend. The default backend is the null
// backend (always available, deterministic by construction). When
// `GW_ANIM_OZZ` is defined at compile time the Ozz sampler + blender takes
// over via link-time substitution; the public API is identical.
//
// No Ozz / ACL headers are reachable from any caller of this file
// (non-negotiable #3 + ADR-0039 §2).

#include "engine/anim/anim_config.hpp"
#include "engine/anim/anim_types.hpp"
#include "engine/anim/blend_tree.hpp"
#include "engine/anim/clip.hpp"
#include "engine/anim/events_animation.hpp"
#include "engine/anim/ik.hpp"
#include "engine/anim/morph.hpp"
#include "engine/anim/pose.hpp"
#include "engine/anim/skeleton.hpp"

#include "engine/core/events/event_bus.hpp"

#include <glm/mat4x4.hpp>

#include <cstdint>
#include <memory>
#include <span>
#include <string_view>

namespace gw::events { class CrossSubsystemBus; }
namespace gw::jobs   { class Scheduler; }

namespace gw::anim {

enum class BackendKind : std::uint8_t {
    Null = 0,
    Ozz  = 1,
};

// -----------------------------------------------------------------------------
// Parameter kinds for blend-tree inputs (ADR-0041 §3).
// -----------------------------------------------------------------------------
enum class ParameterKind : std::uint8_t {
    Float = 0,
    Int   = 1,
    Bool  = 2,
    Trigger = 3,  // consumed by the state machine on read
};

struct ParameterValue {
    ParameterKind kind{ParameterKind::Float};
    float         f{0.0f};
    std::int32_t  i{0};
    bool          b{false};
};

// -----------------------------------------------------------------------------
// Instance description — what you attach to an EntityId to make it animate.
// -----------------------------------------------------------------------------
struct InstanceDesc {
    EntityId       entity{kEntityNone};
    SkeletonHandle skeleton{};
    GraphHandle    graph{};
    MorphSetHandle morph_set{};
    float          playback_speed{1.0f};
    float          initial_time_s{0.0f};
};

class AnimationWorld {
public:
    AnimationWorld();
    ~AnimationWorld();

    AnimationWorld(const AnimationWorld&)            = delete;
    AnimationWorld& operator=(const AnimationWorld&) = delete;
    AnimationWorld(AnimationWorld&&)                 = delete;
    AnimationWorld& operator=(AnimationWorld&&)      = delete;

    // Lifecycle ---------------------------------------------------------------
    [[nodiscard]] bool initialize(const AnimationConfig& cfg,
                                  events::CrossSubsystemBus* bus = nullptr,
                                  jobs::Scheduler* scheduler = nullptr);
    void shutdown();

    [[nodiscard]] bool initialized() const noexcept;
    [[nodiscard]] BackendKind backend() const noexcept;
    [[nodiscard]] std::string_view backend_name() const noexcept;

    [[nodiscard]] const AnimationConfig& config() const noexcept;

    // Asset registration ------------------------------------------------------
    [[nodiscard]] SkeletonHandle  create_skeleton(const SkeletonDesc& desc);
    void                          destroy_skeleton(SkeletonHandle h) noexcept;
    [[nodiscard]] SkeletonInfo    skeleton_info(SkeletonHandle h) const noexcept;

    [[nodiscard]] ClipHandle      create_clip(const ClipDesc& desc);
    void                          destroy_clip(ClipHandle h) noexcept;
    [[nodiscard]] std::uint64_t   clip_hash(ClipHandle h) const noexcept;

    [[nodiscard]] GraphHandle     create_graph(const BlendTreeDesc& desc);
    void                          destroy_graph(GraphHandle h) noexcept;
    [[nodiscard]] std::uint64_t   graph_hash(GraphHandle h) const noexcept;

    [[nodiscard]] MorphSetHandle  create_morph_set(const MorphSetDesc& desc);
    void                          destroy_morph_set(MorphSetHandle h) noexcept;

    // Graph <-> clip binding. The graph has N Input nodes; each slot must
    // receive exactly one clip before the instance is ticked.
    void bind_clip_to_graph_input(GraphHandle graph, std::uint16_t input_index, ClipHandle clip);

    // Instance management -----------------------------------------------------
    [[nodiscard]] InstanceHandle  create_instance(const InstanceDesc& desc);
    void                          destroy_instance(InstanceHandle h) noexcept;
    [[nodiscard]] std::size_t     instance_count() const noexcept;

    // Playback control.
    void set_instance_speed(InstanceHandle h, float speed) noexcept;
    void set_instance_time(InstanceHandle h, float time_s) noexcept;
    [[nodiscard]] float instance_time(InstanceHandle h) const noexcept;

    // Blend-tree parameters (drive transitions + input weights).
    void set_instance_parameter(InstanceHandle h, std::string_view name, const ParameterValue& v);
    [[nodiscard]] ParameterValue instance_parameter(InstanceHandle h, std::string_view name) const noexcept;
    void fire_instance_trigger(InstanceHandle h, std::string_view name);

    // Morph weights.
    void set_morph_weight(InstanceHandle h, std::uint16_t target_index, float weight) noexcept;
    [[nodiscard]] float morph_weight(InstanceHandle h, std::uint16_t target_index) const noexcept;

    // Pose readback -----------------------------------------------------------
    // Fills `out` with the latest sampled local-space pose. Resizes `out` as
    // needed. Returns false if the instance has no valid skeleton yet.
    [[nodiscard]] bool get_local_pose(InstanceHandle h, Pose& out) const;
    // Same but composes parents to emit world-space joint transforms.
    [[nodiscard]] bool get_world_pose(InstanceHandle h, Pose& out) const;

    // Mesh skinning (Community 0/8 animation graph): palette[j] = world_joint[j]
    // * inverse_bind[j]. `inverse_bind` must match joint order in the cooked
    // .gwmesh stream2 inverse-bind block and the instance skeleton.
    [[nodiscard]] bool build_skin_matrix_palette(InstanceHandle h,
                                                 std::span<const glm::mat4> inverse_bind,
                                                 std::span<glm::mat4> out_palette) const noexcept;

    // IK — applied after graph evaluation. Each call is one-shot: the IK
    // goal is consumed in the next fixed step.
    void request_two_bone_ik(InstanceHandle h, const TwoBoneIKInput& goal);
    void request_fabrik     (InstanceHandle h, const FabrikInput& goal);
    void request_ccd        (InstanceHandle h, const CcdInput& goal);

    // Simulation --------------------------------------------------------------
    [[nodiscard]] std::uint32_t step(float delta_time_s);
    void                        step_fixed();
    [[nodiscard]] float         interpolation_alpha() const noexcept;

    void reset();
    void pause(bool paused) noexcept;
    [[nodiscard]] bool paused() const noexcept;

    [[nodiscard]] std::uint64_t frame_index() const noexcept;

    // Determinism -------------------------------------------------------------
    [[nodiscard]] std::uint64_t determinism_hash(const PoseHashOptions& opt = {}) const;

    // Debug draw --------------------------------------------------------------
    enum class DebugFlag : std::uint32_t {
        Skeletons  = 1u << 0,
        IKGoals    = 1u << 1,
        RootMotion = 1u << 2,
        Morph      = 1u << 3,
        All        = 0xFFFFFFFFu,
    };
    void set_debug_flag_mask(std::uint32_t mask) noexcept;
    [[nodiscard]] std::uint32_t debug_flag_mask() const noexcept;

    // Event buses (owned) -----------------------------------------------------
    [[nodiscard]] events::EventBus<events::AnimationClipPlay>&          bus_clip_play()        noexcept;
    [[nodiscard]] events::EventBus<events::AnimationClipStop>&          bus_clip_stop()        noexcept;
    [[nodiscard]] events::EventBus<events::AnimationStateEntered>&      bus_state_entered()    noexcept;
    [[nodiscard]] events::EventBus<events::AnimationStateExited>&       bus_state_exited()     noexcept;
    [[nodiscard]] events::EventBus<events::AnimationEvent>&             bus_anim_event()       noexcept;
    [[nodiscard]] events::EventBus<events::AnimationBlendCompleted>&    bus_blend_completed()  noexcept;
    [[nodiscard]] events::EventBus<events::AnimationIKApplied>&         bus_ik_applied()       noexcept;
    [[nodiscard]] events::EventBus<events::AnimationRagdollActivated>&  bus_ragdoll_activated() noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gw::anim
