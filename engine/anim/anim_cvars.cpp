// engine/anim/anim_cvars.cpp — Phase 13 (ADR-0039).

#include "engine/anim/anim_cvars.hpp"

namespace gw::anim {

namespace {
constexpr std::uint32_t kPersist = config::kCVarPersist;
constexpr std::uint32_t kDevOnly = config::kCVarDevOnly;
} // namespace

AnimationCVars register_anim_cvars(config::CVarRegistry& r) {
    AnimationCVars c;

    c.fixed_dt = r.register_f32({
        "anim.fixed_dt", 1.0f / 60.0f, kPersist, 1.0f / 240.0f, 1.0f / 30.0f, true,
        "Fixed-step animation delta (seconds). Determinism-sensitive.",
    });
    c.substeps_max = r.register_i32({
        "anim.substeps.max", 4, kPersist, 1, 16, true,
        "Max substeps per AnimationWorld::step() call.",
    });
    c.max_skeletons = r.register_i32({
        "anim.caps.max_skeletons", 4096, kDevOnly, 32, 65536, true,
        "Reserved skeleton pool size.",
    });
    c.max_clips = r.register_i32({
        "anim.caps.max_clips", 8192, kDevOnly, 32, 131072, true,
        "Reserved clip pool size.",
    });
    c.max_instances = r.register_i32({
        "anim.caps.max_instances", 8192, kDevOnly, 32, 65536, true,
        "Reserved graph-instance pool size.",
    });

    c.debug_enabled = r.register_bool({
        "anim.debug.enabled", false, kDevOnly, {}, {}, false,
        "Master animation debug draw toggle.",
    });
    c.debug_skeletons = r.register_bool({
        "anim.debug.skeletons", false, kDevOnly, {}, {}, false,
        "Render debug bones for each animated instance.",
    });
    c.debug_ik_goals = r.register_bool({
        "anim.debug.ik_goals", false, kDevOnly, {}, {}, false,
        "Render active IK goal spheres.",
    });
    c.debug_root_motion = r.register_bool({
        "anim.debug.root_motion", false, kDevOnly, {}, {}, false,
        "Render root-motion trails.",
    });
    c.debug_morph = r.register_bool({
        "anim.debug.morph", false, kDevOnly, {}, {}, false,
        "Dump morph weight histogram overlay.",
    });
    c.debug_max_lines = r.register_i32({
        "anim.debug.max_lines", 16384, kDevOnly, 1024, 262144, true,
        "Cap on animation debug lines per frame.",
    });

    c.determinism_hash_every_n = r.register_i32({
        "anim.determinism.hash_every_n", 30, kDevOnly, 1, 600, true,
        "Hash the animation world every Nth frame (1 = every frame).",
    });
    c.determinism_enforce_hash = r.register_bool({
        "anim.determinism.enforce_hash", false, kDevOnly, {}, {}, false,
        "Assert on hash mismatch against the active replay.",
    });

    return c;
}

void apply_cvars_to_config(const config::CVarRegistry& r, AnimationConfig& cfg) {
    cfg.fixed_dt        = r.get_f32_or("anim.fixed_dt",              cfg.fixed_dt);
    cfg.max_substeps    = static_cast<std::uint32_t>(
                              r.get_i32_or("anim.substeps.max",      static_cast<std::int32_t>(cfg.max_substeps)));
    cfg.max_skeletons   = static_cast<std::uint32_t>(
                              r.get_i32_or("anim.caps.max_skeletons",static_cast<std::int32_t>(cfg.max_skeletons)));
    cfg.max_clips       = static_cast<std::uint32_t>(
                              r.get_i32_or("anim.caps.max_clips",    static_cast<std::int32_t>(cfg.max_clips)));
    cfg.max_instances   = static_cast<std::uint32_t>(
                              r.get_i32_or("anim.caps.max_instances",static_cast<std::int32_t>(cfg.max_instances)));
    cfg.debug_max_lines = static_cast<std::uint32_t>(
                              r.get_i32_or("anim.debug.max_lines",   static_cast<std::int32_t>(cfg.debug_max_lines)));
    cfg.hash_every_n    = static_cast<std::uint32_t>(
                              r.get_i32_or("anim.determinism.hash_every_n", static_cast<std::int32_t>(cfg.hash_every_n)));
}

} // namespace gw::anim
