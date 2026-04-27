// engine/anim/animation_world.cpp — Phase 13 (ADR-0039..0042).
//
// Null-backend implementation. The backend samples clips uniformly
// between keyframes, evaluates the blend tree depth-first, then applies
// queued IK goals. Full public API parity with the future Ozz/ACL bridge.

#include "engine/anim/animation_world.hpp"

#include "engine/core/events/event_bus.hpp"
#include "engine/jobs/scheduler.hpp"
#include "engine/physics/determinism_hash.hpp"

#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace gw::anim {

namespace {

[[nodiscard]] glm::mat4 joint_trs_to_mat4(const JointTransform& j) noexcept {
    glm::mat4 m = glm::translate(glm::mat4(1.0f), j.translation);
    m *= glm::mat4_cast(j.rotation);
    m = glm::scale(m, j.scale);
    return m;
}

// Short-arc NLERP (local helper — matches pose.cpp::nlerp_short).
inline glm::quat nlerp_short(glm::quat a, glm::quat b, float w) noexcept {
    const float d = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    if (d < 0.0f) { b.x = -b.x; b.y = -b.y; b.z = -b.z; b.w = -b.w; }
    glm::quat q{
        a.w + (b.w - a.w) * w,
        a.x + (b.x - a.x) * w,
        a.y + (b.y - a.y) * w,
        a.z + (b.z - a.z) * w,
    };
    const float len2 = q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z;
    if (len2 > 0.0f) {
        const float inv = 1.0f / std::sqrt(len2);
        q.w *= inv; q.x *= inv; q.y *= inv; q.z *= inv;
    }
    return q;
}

} // namespace

// ---------------------------------------------------------------------------
// Internal records.
// ---------------------------------------------------------------------------
struct NullSkeleton {
    SkeletonDesc   desc{};
    bool           alive{false};
};

struct NullClip {
    ClipDesc       desc{};
    bool           alive{false};
};

struct NullGraph {
    BlendTreeDesc  desc{};
    // Input-slot bindings: graph input_index → ClipHandle. `input_clips[i]`
    // is the clip bound to the node at index `i` when that node is an Input.
    std::unordered_map<std::uint16_t, ClipHandle> input_bindings{};
    bool           alive{false};
};

struct NullMorph {
    MorphSetDesc   desc{};
    bool           alive{false};
};

struct NullInstance {
    InstanceDesc   desc{};
    float          playback_time_s{0.0f};
    float          playback_speed{1.0f};
    Pose           local_pose{};     // filled each fixed step
    // Per-input per-clip playback time (each Input node can have its own
    // phase so e.g. left/right foot clips sync correctly).
    std::unordered_map<std::uint16_t, float>      per_input_time{};
    std::unordered_map<std::string, ParameterValue> parameters{};
    // Pending trigger names (consumed by state machine on next tick).
    std::vector<std::string>                        pending_triggers{};

    // State-machine bookkeeping.
    std::uint16_t  active_state{0};
    std::uint16_t  blend_state{0};
    float          blend_t{0.0f};
    float          blend_duration{0.0f};
    bool           blending{false};

    // IK goals (one-shot; consumed per fixed step).
    std::optional<TwoBoneIKInput> pending_two_bone{};
    std::optional<FabrikInput>    pending_fabrik{};
    std::optional<CcdInput>       pending_ccd{};

    // Morph weights.
    std::vector<float>           morph_weights{};

    bool           alive{false};
    std::uint32_t  generation{0};
};

// ---------------------------------------------------------------------------
// Impl — null backend state.
// ---------------------------------------------------------------------------
struct AnimationWorld::Impl {
    AnimationConfig                cfg{};
    events::CrossSubsystemBus*     cross_bus{nullptr};
    jobs::Scheduler*               scheduler{nullptr};

    std::vector<NullSkeleton>  skeletons{1};
    std::vector<NullClip>      clips{1};
    std::vector<NullGraph>     graphs{1};
    std::vector<NullMorph>     morphs{1};
    std::vector<NullInstance>  instances{1};

    std::vector<std::uint32_t> free_skeletons{};
    std::vector<std::uint32_t> free_clips{};
    std::vector<std::uint32_t> free_graphs{};
    std::vector<std::uint32_t> free_morphs{};
    std::vector<std::uint32_t> free_instances{};

    // Event buses.
    events::EventBus<events::AnimationClipPlay>         bus_clip_play{};
    events::EventBus<events::AnimationClipStop>         bus_clip_stop{};
    events::EventBus<events::AnimationStateEntered>     bus_state_entered{};
    events::EventBus<events::AnimationStateExited>      bus_state_exited{};
    events::EventBus<events::AnimationEvent>            bus_anim_event{};
    events::EventBus<events::AnimationBlendCompleted>   bus_blend_completed{};
    events::EventBus<events::AnimationIKApplied>        bus_ik_applied{};
    events::EventBus<events::AnimationRagdollActivated> bus_ragdoll_activated{};

    bool           initialized_flag{false};
    bool           paused_flag{false};
    float          time_accumulator_s{0.0f};
    std::uint64_t  frame_index{0};
    std::uint32_t  debug_flag_mask{0};

    // Scratch buffers (sized once per instance for hot-path reuse).
    std::vector<Pose> node_pose_scratch{};

    // ---- accessors ----
    NullSkeleton* skel_ptr(SkeletonHandle h) noexcept {
        if (!h.valid() || h.id >= skeletons.size()) return nullptr;
        auto& s = skeletons[h.id];
        return s.alive ? &s : nullptr;
    }
    const NullSkeleton* skel_ptr(SkeletonHandle h) const noexcept {
        if (!h.valid() || h.id >= skeletons.size()) return nullptr;
        const auto& s = skeletons[h.id];
        return s.alive ? &s : nullptr;
    }
    NullClip* clip_ptr(ClipHandle h) noexcept {
        if (!h.valid() || h.id >= clips.size()) return nullptr;
        auto& c = clips[h.id];
        return c.alive ? &c : nullptr;
    }
    const NullClip* clip_ptr(ClipHandle h) const noexcept {
        if (!h.valid() || h.id >= clips.size()) return nullptr;
        const auto& c = clips[h.id];
        return c.alive ? &c : nullptr;
    }
    NullGraph* graph_ptr(GraphHandle h) noexcept {
        if (!h.valid() || h.id >= graphs.size()) return nullptr;
        auto& g = graphs[h.id];
        return g.alive ? &g : nullptr;
    }
    const NullGraph* graph_ptr(GraphHandle h) const noexcept {
        if (!h.valid() || h.id >= graphs.size()) return nullptr;
        const auto& g = graphs[h.id];
        return g.alive ? &g : nullptr;
    }
    NullMorph* morph_ptr(MorphSetHandle h) noexcept {
        if (!h.valid() || h.id >= morphs.size()) return nullptr;
        auto& m = morphs[h.id];
        return m.alive ? &m : nullptr;
    }
    const NullMorph* morph_ptr(MorphSetHandle h) const noexcept {
        if (!h.valid() || h.id >= morphs.size()) return nullptr;
        const auto& m = morphs[h.id];
        return m.alive ? &m : nullptr;
    }
    NullInstance* inst_ptr(InstanceHandle h) noexcept {
        if (!h.valid() || h.id >= instances.size()) return nullptr;
        auto& i = instances[h.id];
        return i.alive ? &i : nullptr;
    }
    const NullInstance* inst_ptr(InstanceHandle h) const noexcept {
        if (!h.valid() || h.id >= instances.size()) return nullptr;
        const auto& i = instances[h.id];
        return i.alive ? &i : nullptr;
    }

    // ---- clip sampling ----
    void sample_clip(const NullClip& c, float time_s, Pose& out,
                     const NullSkeleton& s) noexcept {
        const std::size_t jn = s.desc.rest_pose.size();
        if (out.size() != jn) out.resize(jn);
        // Seed with rest pose.
        pose_set_from_rest(out, s.desc.rest_pose);
        if (c.desc.key_count == 0 || c.desc.duration_s <= 0.0f) return;

        // Compute key index + alpha.
        float t = time_s;
        if (c.desc.looping) {
            t = std::fmod(t, c.desc.duration_s);
            if (t < 0.0f) t += c.desc.duration_s;
        } else {
            t = std::clamp(t, 0.0f, c.desc.duration_s);
        }
        const std::uint32_t kn = c.desc.key_count;
        const float step = (kn > 1) ? (c.desc.duration_s / static_cast<float>(kn - 1)) : 1.0f;
        float frac_index = t / std::max(1e-6f, step);
        std::uint32_t k0 = static_cast<std::uint32_t>(frac_index);
        if (k0 >= kn - 1) k0 = kn - 1;
        std::uint32_t k1 = k0 + 1;
        if (k1 >= kn) k1 = c.desc.looping ? 0 : kn - 1;
        const float alpha = frac_index - static_cast<float>(k0);

        for (const auto& tr : c.desc.tracks) {
            if (tr.joint_index >= jn) continue;
            auto& j = out[tr.joint_index];
            if (tr.translations.size() > k1) {
                j.translation = tr.translations[k0] + (tr.translations[k1] - tr.translations[k0]) * alpha;
            }
            if (tr.rotations.size() > k1) {
                j.rotation = nlerp_short(tr.rotations[k0], tr.rotations[k1], alpha);
            }
            if (tr.scales.size() > k1) {
                j.scale = tr.scales[k0] + (tr.scales[k1] - tr.scales[k0]) * alpha;
            }
        }
    }

    // ---- blend graph evaluation ----
    void evaluate_graph(const NullGraph& g, NullInstance& inst,
                        const NullSkeleton& skel, Pose& out, float /*dt*/) {
        const auto& nodes = g.desc.nodes;
        if (nodes.empty()) {
            pose_set_from_rest(out, skel.desc.rest_pose);
            return;
        }
        if (node_pose_scratch.size() < nodes.size()) node_pose_scratch.resize(nodes.size());

        for (std::size_t i = 0; i < nodes.size(); ++i) {
            const auto& n = nodes[i];
            Pose& np = node_pose_scratch[i];
            np.resize(skel.desc.rest_pose.size());
            switch (n.kind) {
                case BlendKind::Input: {
                    const auto it = g.input_bindings.find(n.input_index);
                    if (it == g.input_bindings.end()) {
                        pose_set_from_rest(np, skel.desc.rest_pose);
                        break;
                    }
                    const NullClip* c = clip_ptr(it->second);
                    if (!c) {
                        pose_set_from_rest(np, skel.desc.rest_pose);
                        break;
                    }
                    // Use per-input time if populated, else global instance time.
                    const auto tit = inst.per_input_time.find(n.input_index);
                    const float tt = (tit != inst.per_input_time.end()) ? tit->second : inst.playback_time_s;
                    sample_clip(*c, tt, np, skel);
                } break;
                case BlendKind::Lerp: {
                    if (n.children.size() != 2) {
                        pose_set_from_rest(np, skel.desc.rest_pose);
                        break;
                    }
                    pose_lerp(np,
                              node_pose_scratch[n.children[0]],
                              node_pose_scratch[n.children[1]],
                              n.weight);
                } break;
                case BlendKind::OneMinus: {
                    if (n.children.size() != 1) {
                        pose_set_from_rest(np, skel.desc.rest_pose);
                        break;
                    }
                    Pose rest;
                    pose_set_from_rest(rest, skel.desc.rest_pose);
                    pose_lerp(np, node_pose_scratch[n.children[0]], rest, 1.0f - n.weight);
                } break;
                case BlendKind::Additive: {
                    if (n.children.size() != 2) {
                        pose_set_from_rest(np, skel.desc.rest_pose);
                        break;
                    }
                    pose_add(np,
                             node_pose_scratch[n.children[0]],
                             node_pose_scratch[n.children[1]],
                             skel.desc.rest_pose, n.weight);
                } break;
                case BlendKind::MaskLerp: {
                    if (n.children.size() != 2 ||
                        n.mask_index >= g.desc.masks.size()) {
                        pose_set_from_rest(np, skel.desc.rest_pose);
                        break;
                    }
                    pose_mask_lerp(np,
                                   node_pose_scratch[n.children[0]],
                                   node_pose_scratch[n.children[1]],
                                   n.weight,
                                   g.desc.masks[n.mask_index]);
                } break;
                case BlendKind::StateMachine: {
                    // Active state or crossfade.
                    const auto& states = g.desc.states;
                    if (states.empty()) {
                        pose_set_from_rest(np, skel.desc.rest_pose);
                        break;
                    }
                    const std::uint16_t act = std::min<std::uint16_t>(inst.active_state,
                                                                     static_cast<std::uint16_t>(states.size() - 1));
                    const Pose& active_pose = node_pose_scratch[states[act].node_index];
                    if (!inst.blending) {
                        np = active_pose;
                    } else {
                        const std::uint16_t dst = std::min<std::uint16_t>(inst.blend_state,
                                                                         static_cast<std::uint16_t>(states.size() - 1));
                        const Pose& dst_pose = node_pose_scratch[states[dst].node_index];
                        const float w = (inst.blend_duration > 0.0f)
                                            ? std::clamp(inst.blend_t / inst.blend_duration, 0.0f, 1.0f)
                                            : 1.0f;
                        pose_lerp(np, active_pose, dst_pose, w);
                    }
                } break;
            }
        }

        const std::uint16_t root = std::min<std::uint16_t>(g.desc.root_index,
                                                          static_cast<std::uint16_t>(nodes.size() - 1));
        out = node_pose_scratch[root];
    }

    // ---- state-machine advance ----
    void advance_state_machine(NullInstance& inst, const NullGraph& g, float dt) {
        if (g.desc.states.empty()) return;

        // Consume any pending triggers.
        for (const auto& trig : inst.pending_triggers) {
            for (const auto& t : g.desc.transitions) {
                if (t.from_state == inst.active_state && t.trigger == trig && !inst.blending) {
                    inst.blending       = true;
                    inst.blend_state    = t.to_state;
                    inst.blend_t        = 0.0f;
                    inst.blend_duration = std::max(t.duration_s, 0.0f);
                    events::AnimationStateExited ev{inst.desc.entity, InstanceHandle{}, g.desc.states[inst.active_state].name};
                    bus_state_exited.publish(ev);
                    break;
                }
            }
        }
        inst.pending_triggers.clear();

        if (inst.blending) {
            inst.blend_t += dt;
            if (inst.blend_t >= inst.blend_duration) {
                const std::uint16_t old_state = inst.active_state;
                inst.active_state = inst.blend_state;
                inst.blending = false;
                events::AnimationStateEntered sev{inst.desc.entity, InstanceHandle{}, g.desc.states[inst.active_state].name};
                bus_state_entered.publish(sev);
                events::AnimationBlendCompleted bc{inst.desc.entity, InstanceHandle{}, old_state, inst.active_state};
                bus_blend_completed.publish(bc);
            }
        }
    }

    // ---- IK application ----
    void apply_ik(NullInstance& inst, const NullSkeleton& skel) {
        if (inst.pending_two_bone.has_value()) {
            const auto& g = *inst.pending_two_bone;
            solve_two_bone_ik(inst.local_pose, skel.desc, g);
            events::AnimationIKApplied ev{inst.desc.entity, InstanceHandle{}, 3, 0.0f, 1};
            bus_ik_applied.publish(ev);
            inst.pending_two_bone.reset();
        }
        if (inst.pending_fabrik.has_value()) {
            const auto& g = *inst.pending_fabrik;
            solve_fabrik(inst.local_pose, skel.desc, g);
            events::AnimationIKApplied ev{inst.desc.entity, InstanceHandle{},
                                          static_cast<std::uint16_t>(g.chain_joints.size()),
                                          0.0f, g.max_iterations};
            bus_ik_applied.publish(ev);
            inst.pending_fabrik.reset();
        }
        if (inst.pending_ccd.has_value()) {
            const auto& g = *inst.pending_ccd;
            solve_ccd(inst.local_pose, skel.desc, g);
            events::AnimationIKApplied ev{inst.desc.entity, InstanceHandle{},
                                          static_cast<std::uint16_t>(g.chain_joints.size()),
                                          0.0f, g.max_iterations};
            bus_ik_applied.publish(ev);
            inst.pending_ccd.reset();
        }
    }

    // ---- collect hash ----
    std::uint64_t compute_hash(const PoseHashOptions& opt) const {
        // Deterministic order: iterate instances by id ascending.
        std::uint64_t h = gw::physics::kFnvOffset64;
        std::uint8_t count_bytes[8];
        std::uint64_t alive = 0;
        for (const auto& i : instances) if (i.alive) ++alive;
        std::memcpy(count_bytes, &alive, sizeof(alive));
        h = gw::physics::fnv1a64_combine(h, {count_bytes, 8});
        for (std::uint32_t id = 1; id < instances.size(); ++id) {
            const auto& i = instances[id];
            if (!i.alive) continue;
            std::uint8_t id_bytes[4];
            std::memcpy(id_bytes, &id, sizeof(id));
            h = gw::physics::fnv1a64_combine(h, {id_bytes, 4});
            const std::uint64_t ph = pose_hash(i.local_pose, opt);
            std::uint8_t ph_bytes[8];
            std::memcpy(ph_bytes, &ph, sizeof(ph));
            h = gw::physics::fnv1a64_combine(h, {ph_bytes, 8});
        }
        return h;
    }
};

// ---------------------------------------------------------------------------
// AnimationWorld public API.
// ---------------------------------------------------------------------------
AnimationWorld::AnimationWorld() : impl_(std::make_unique<Impl>()) {}
AnimationWorld::~AnimationWorld() = default;

bool AnimationWorld::initialize(const AnimationConfig& cfg,
                                events::CrossSubsystemBus* bus,
                                jobs::Scheduler* scheduler) {
    if (impl_->initialized_flag) return true;
    impl_->cfg = cfg;
    impl_->cross_bus = bus;
    impl_->scheduler = scheduler;
    impl_->initialized_flag = true;
    impl_->frame_index = 0;
    impl_->debug_flag_mask = 0;
    return true;
}

void AnimationWorld::shutdown() {
    if (!impl_->initialized_flag) return;
    impl_->skeletons.assign(1, NullSkeleton{});
    impl_->clips.assign(1, NullClip{});
    impl_->graphs.assign(1, NullGraph{});
    impl_->morphs.assign(1, NullMorph{});
    impl_->instances.assign(1, NullInstance{});
    impl_->free_skeletons.clear();
    impl_->free_clips.clear();
    impl_->free_graphs.clear();
    impl_->free_morphs.clear();
    impl_->free_instances.clear();
    impl_->time_accumulator_s = 0.0f;
    impl_->frame_index = 0;
    impl_->paused_flag = false;
    impl_->initialized_flag = false;
}

bool AnimationWorld::initialized() const noexcept { return impl_->initialized_flag; }
BackendKind AnimationWorld::backend() const noexcept { return BackendKind::Null; }
std::string_view AnimationWorld::backend_name() const noexcept { return "null"; }
const AnimationConfig& AnimationWorld::config() const noexcept { return impl_->cfg; }

// ---- assets ----
SkeletonHandle AnimationWorld::create_skeleton(const SkeletonDesc& desc) {
    std::uint32_t id;
    if (!impl_->free_skeletons.empty()) {
        id = impl_->free_skeletons.back();
        impl_->free_skeletons.pop_back();
    } else {
        id = static_cast<std::uint32_t>(impl_->skeletons.size());
        impl_->skeletons.push_back(NullSkeleton{});
    }
    auto& s = impl_->skeletons[id];
    s = NullSkeleton{};
    s.desc = desc;
    if (s.desc.content_hash == 0) s.desc.content_hash = skeleton_content_hash(s.desc);
    s.alive = true;
    return SkeletonHandle{id};
}

void AnimationWorld::destroy_skeleton(SkeletonHandle h) noexcept {
    auto* s = impl_->skel_ptr(h);
    if (!s) return;
    s->alive = false;
    impl_->free_skeletons.push_back(h.id);
}

SkeletonInfo AnimationWorld::skeleton_info(SkeletonHandle h) const noexcept {
    const auto* s = impl_->skel_ptr(h);
    if (!s) return {};
    return SkeletonInfo{
        static_cast<std::uint32_t>(s->desc.rest_pose.size()),
        s->desc.content_hash,
    };
}

ClipHandle AnimationWorld::create_clip(const ClipDesc& desc) {
    std::uint32_t id;
    if (!impl_->free_clips.empty()) {
        id = impl_->free_clips.back();
        impl_->free_clips.pop_back();
    } else {
        id = static_cast<std::uint32_t>(impl_->clips.size());
        impl_->clips.push_back(NullClip{});
    }
    auto& c = impl_->clips[id];
    c = NullClip{};
    c.desc = desc;
    c.alive = true;
    return ClipHandle{id};
}

void AnimationWorld::destroy_clip(ClipHandle h) noexcept {
    auto* c = impl_->clip_ptr(h);
    if (!c) return;
    c->alive = false;
    impl_->free_clips.push_back(h.id);
}

std::uint64_t AnimationWorld::clip_hash(ClipHandle h) const noexcept {
    const auto* c = impl_->clip_ptr(h);
    if (!c) return 0;
    return clip_content_hash(c->desc);
}

GraphHandle AnimationWorld::create_graph(const BlendTreeDesc& desc) {
    std::uint32_t id;
    if (!impl_->free_graphs.empty()) {
        id = impl_->free_graphs.back();
        impl_->free_graphs.pop_back();
    } else {
        id = static_cast<std::uint32_t>(impl_->graphs.size());
        impl_->graphs.push_back(NullGraph{});
    }
    auto& g = impl_->graphs[id];
    g = NullGraph{};
    g.desc = desc;
    g.alive = true;
    return GraphHandle{id};
}

void AnimationWorld::destroy_graph(GraphHandle h) noexcept {
    auto* g = impl_->graph_ptr(h);
    if (!g) return;
    g->alive = false;
    impl_->free_graphs.push_back(h.id);
}

std::uint64_t AnimationWorld::graph_hash(GraphHandle h) const noexcept {
    const auto* g = impl_->graph_ptr(h);
    if (!g) return 0;
    return blend_tree_content_hash(g->desc);
}

MorphSetHandle AnimationWorld::create_morph_set(const MorphSetDesc& desc) {
    std::uint32_t id;
    if (!impl_->free_morphs.empty()) {
        id = impl_->free_morphs.back();
        impl_->free_morphs.pop_back();
    } else {
        id = static_cast<std::uint32_t>(impl_->morphs.size());
        impl_->morphs.push_back(NullMorph{});
    }
    auto& m = impl_->morphs[id];
    m = NullMorph{};
    m.desc = desc;
    m.alive = true;
    return MorphSetHandle{id};
}

void AnimationWorld::destroy_morph_set(MorphSetHandle h) noexcept {
    auto* m = impl_->morph_ptr(h);
    if (!m) return;
    m->alive = false;
    impl_->free_morphs.push_back(h.id);
}

void AnimationWorld::bind_clip_to_graph_input(GraphHandle graph, std::uint16_t input_index, ClipHandle clip) {
    auto* g = impl_->graph_ptr(graph);
    if (!g) return;
    g->input_bindings[input_index] = clip;
}

// ---- instances ----
InstanceHandle AnimationWorld::create_instance(const InstanceDesc& desc) {
    std::uint32_t id;
    if (!impl_->free_instances.empty()) {
        id = impl_->free_instances.back();
        impl_->free_instances.pop_back();
    } else {
        id = static_cast<std::uint32_t>(impl_->instances.size());
        impl_->instances.push_back(NullInstance{});
    }
    auto& i = impl_->instances[id];
    i = NullInstance{};
    i.alive = true;
    i.desc = desc;
    i.playback_speed = desc.playback_speed;
    i.playback_time_s = desc.initial_time_s;
    ++i.generation;

    if (const auto* s = impl_->skel_ptr(desc.skeleton)) {
        pose_set_from_rest(i.local_pose, s->desc.rest_pose);
    }
    if (const auto* m = impl_->morph_ptr(desc.morph_set)) {
        i.morph_weights.assign(m->desc.targets.size(), 0.0f);
        for (std::size_t k = 0; k < m->desc.targets.size(); ++k) {
            i.morph_weights[k] = m->desc.targets[k].default_weight;
        }
    }
    if (const auto* g = impl_->graph_ptr(desc.graph)) {
        i.active_state = g->desc.default_state;
    }
    return InstanceHandle{id};
}

void AnimationWorld::destroy_instance(InstanceHandle h) noexcept {
    auto* i = impl_->inst_ptr(h);
    if (!i) return;
    i->alive = false;
    impl_->free_instances.push_back(h.id);
}

std::size_t AnimationWorld::instance_count() const noexcept {
    std::size_t n = 0;
    for (const auto& i : impl_->instances) if (i.alive) ++n;
    return n;
}

void AnimationWorld::set_instance_speed(InstanceHandle h, float speed) noexcept {
    if (auto* i = impl_->inst_ptr(h)) i->playback_speed = speed;
}

void AnimationWorld::set_instance_time(InstanceHandle h, float time_s) noexcept {
    if (auto* i = impl_->inst_ptr(h)) i->playback_time_s = time_s;
}

float AnimationWorld::instance_time(InstanceHandle h) const noexcept {
    if (const auto* i = impl_->inst_ptr(h)) return i->playback_time_s;
    return 0.0f;
}

void AnimationWorld::set_instance_parameter(InstanceHandle h, std::string_view name, const ParameterValue& v) {
    if (auto* i = impl_->inst_ptr(h)) {
        i->parameters[std::string{name}] = v;
    }
}

ParameterValue AnimationWorld::instance_parameter(InstanceHandle h, std::string_view name) const noexcept {
    if (const auto* i = impl_->inst_ptr(h)) {
        auto it = i->parameters.find(std::string{name});
        if (it != i->parameters.end()) return it->second;
    }
    return ParameterValue{};
}

void AnimationWorld::fire_instance_trigger(InstanceHandle h, std::string_view name) {
    if (auto* i = impl_->inst_ptr(h)) {
        i->pending_triggers.emplace_back(name);
    }
}

void AnimationWorld::set_morph_weight(InstanceHandle h, std::uint16_t target_index, float weight) noexcept {
    auto* i = impl_->inst_ptr(h);
    if (!i) return;
    if (target_index >= i->morph_weights.size()) i->morph_weights.resize(target_index + 1u, 0.0f);
    i->morph_weights[target_index] = weight;
}

float AnimationWorld::morph_weight(InstanceHandle h, std::uint16_t target_index) const noexcept {
    const auto* i = impl_->inst_ptr(h);
    if (!i) return 0.0f;
    if (target_index >= i->morph_weights.size()) return 0.0f;
    return i->morph_weights[target_index];
}

bool AnimationWorld::get_local_pose(InstanceHandle h, Pose& out) const {
    const auto* i = impl_->inst_ptr(h);
    if (!i) return false;
    out = i->local_pose;
    return true;
}

bool AnimationWorld::get_world_pose(InstanceHandle h, Pose& out) const {
    const auto* i = impl_->inst_ptr(h);
    if (!i) return false;
    const auto* s = impl_->skel_ptr(i->desc.skeleton);
    if (!s) {
        out = i->local_pose;
        return true;
    }
    out.resize(i->local_pose.size());
    for (std::size_t k = 0; k < i->local_pose.size(); ++k) {
        const auto& loc = i->local_pose[k];
        const std::uint16_t p = (k < s->desc.parents.size()) ? s->desc.parents[k] : kInvalidJoint;
        if (p == kInvalidJoint) {
            out[k] = loc;
        } else {
            const auto& pw = out[p];
            out[k].translation = pw.translation + pw.rotation * (loc.translation * pw.scale);
            out[k].rotation    = pw.rotation * loc.rotation;
            out[k].scale       = pw.scale * loc.scale;
        }
    }
    return true;
}

bool AnimationWorld::build_skin_matrix_palette(InstanceHandle h,
                                               std::span<const glm::mat4> inverse_bind,
                                               std::span<glm::mat4> out_palette) const noexcept {
    if (inverse_bind.empty()) return false;
    if (out_palette.size() < inverse_bind.size()) return false;
    const auto* i = impl_->inst_ptr(h);
    if (!i) return false;
    Pose world;
    if (!get_world_pose(h, world)) return false;
    if (world.size() < inverse_bind.size()) return false;
    const std::size_t n = inverse_bind.size();
    for (std::size_t j = 0; j < n; ++j) {
        out_palette[j] = joint_trs_to_mat4(world[j]) * inverse_bind[j];
    }
    return true;
}

void AnimationWorld::request_two_bone_ik(InstanceHandle h, const TwoBoneIKInput& goal) {
    if (auto* i = impl_->inst_ptr(h)) i->pending_two_bone = goal;
}
void AnimationWorld::request_fabrik(InstanceHandle h, const FabrikInput& goal) {
    if (auto* i = impl_->inst_ptr(h)) i->pending_fabrik = goal;
}
void AnimationWorld::request_ccd(InstanceHandle h, const CcdInput& goal) {
    if (auto* i = impl_->inst_ptr(h)) i->pending_ccd = goal;
}

// ---- Simulation ----
std::uint32_t AnimationWorld::step(float delta_time_s) {
    if (!impl_->initialized_flag || impl_->paused_flag) return 0;
    impl_->time_accumulator_s += delta_time_s;
    const float dt = impl_->cfg.fixed_dt;
    const std::uint32_t max_sub = impl_->cfg.max_substeps;
    std::uint32_t n = 0;
    while (impl_->time_accumulator_s >= dt && n < max_sub) {
        step_fixed();
        impl_->time_accumulator_s -= dt;
        ++n;
    }
    if (impl_->time_accumulator_s > dt * static_cast<float>(max_sub)) {
        impl_->time_accumulator_s = dt;
    }
    return n;
}

void AnimationWorld::step_fixed() {
    if (!impl_->initialized_flag) return;
    const float dt = impl_->cfg.fixed_dt;

    for (auto& inst : impl_->instances) {
        if (!inst.alive) continue;
        const auto* g = impl_->graph_ptr(inst.desc.graph);
        const auto* s = impl_->skel_ptr(inst.desc.skeleton);
        if (!s) continue;

        // Advance clock.
        inst.playback_time_s += dt * inst.playback_speed;
        // Also advance per-input clocks proportionally (null backend: same rate).
        for (auto& kv : inst.per_input_time) kv.second += dt * inst.playback_speed;

        if (g) {
            impl_->advance_state_machine(inst, *g, dt);
            impl_->evaluate_graph(*g, inst, *s, inst.local_pose, dt);
        } else {
            pose_set_from_rest(inst.local_pose, s->desc.rest_pose);
        }

        impl_->apply_ik(inst, *s);
    }

    ++impl_->frame_index;
}

float AnimationWorld::interpolation_alpha() const noexcept {
    if (!impl_->initialized_flag) return 0.0f;
    const float dt = impl_->cfg.fixed_dt;
    return std::clamp(impl_->time_accumulator_s / dt, 0.0f, 1.0f);
}

void AnimationWorld::reset() {
    if (!impl_->initialized_flag) return;
    impl_->skeletons.assign(1, NullSkeleton{});
    impl_->clips.assign(1, NullClip{});
    impl_->graphs.assign(1, NullGraph{});
    impl_->morphs.assign(1, NullMorph{});
    impl_->instances.assign(1, NullInstance{});
    impl_->free_skeletons.clear();
    impl_->free_clips.clear();
    impl_->free_graphs.clear();
    impl_->free_morphs.clear();
    impl_->free_instances.clear();
    impl_->time_accumulator_s = 0.0f;
    impl_->frame_index = 0;
}

void AnimationWorld::pause(bool paused) noexcept { impl_->paused_flag = paused; }
bool AnimationWorld::paused() const noexcept    { return impl_->paused_flag; }
std::uint64_t AnimationWorld::frame_index() const noexcept { return impl_->frame_index; }

std::uint64_t AnimationWorld::determinism_hash(const PoseHashOptions& opt) const {
    return impl_->compute_hash(opt);
}

void AnimationWorld::set_debug_flag_mask(std::uint32_t mask) noexcept { impl_->debug_flag_mask = mask; }
std::uint32_t AnimationWorld::debug_flag_mask() const noexcept       { return impl_->debug_flag_mask; }

events::EventBus<events::AnimationClipPlay>&         AnimationWorld::bus_clip_play()         noexcept { return impl_->bus_clip_play; }
events::EventBus<events::AnimationClipStop>&         AnimationWorld::bus_clip_stop()         noexcept { return impl_->bus_clip_stop; }
events::EventBus<events::AnimationStateEntered>&     AnimationWorld::bus_state_entered()     noexcept { return impl_->bus_state_entered; }
events::EventBus<events::AnimationStateExited>&      AnimationWorld::bus_state_exited()      noexcept { return impl_->bus_state_exited; }
events::EventBus<events::AnimationEvent>&            AnimationWorld::bus_anim_event()        noexcept { return impl_->bus_anim_event; }
events::EventBus<events::AnimationBlendCompleted>&   AnimationWorld::bus_blend_completed()   noexcept { return impl_->bus_blend_completed; }
events::EventBus<events::AnimationIKApplied>&        AnimationWorld::bus_ik_applied()        noexcept { return impl_->bus_ik_applied; }
events::EventBus<events::AnimationRagdollActivated>& AnimationWorld::bus_ragdoll_activated() noexcept { return impl_->bus_ragdoll_activated; }

} // namespace gw::anim
