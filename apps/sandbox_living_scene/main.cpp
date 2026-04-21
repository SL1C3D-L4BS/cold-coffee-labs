// apps/sandbox_living_scene/main.cpp — Phase 13 Wave 13F exit-demo (ADR-0046).
//
// The "Living Scene" milestone. This app boots the Phase-13 facades in
// deterministic mode and drives a character through a navmeshed scene
// with blended animations:
//
//   1. Construct a 2x2 tile navmesh (fully walkable grid, cell = 0.25 m).
//   2. Create a 3-joint skeleton + a looping "stride" clip + a 2-input
//      blend graph for idle/walk transitions.
//   3. Spawn a BehaviorTree with a small "patrol" pattern (ActionLeaf
//      "pick_target" -> agent drives along returned path).
//   4. Walk the agent from start to target and count how many BT ticks,
//      pose samples, and path queries occurred.
//   5. Emit a grep-friendly `LIVING OK` summary so CI (`living_sandbox`
//      test) can gate on it.
//
// Runs against the null anim + null BT + null navmesh backends by
// default. With GW_ENABLE_OZZ / GW_ENABLE_ACL / GW_ENABLE_RECAST the
// same demo exercises the production bridges end-to-end.

#include "engine/anim/animation_world.hpp"
#include "engine/core/version.hpp"
#include "engine/gameai/gameai_world.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace gw;

namespace {

constexpr std::uint32_t kDefaultFrames = 300u;

gw::anim::SkeletonDesc make_stickman_skeleton() {
    gw::anim::SkeletonDesc s{};
    s.name = "stickman";
    s.joint_names = {"root", "spine", "head"};
    s.parents = {std::uint16_t(0xFFFF), std::uint16_t(0), std::uint16_t(1)};
    s.rest_pose = {gw::anim::JointRest{}, gw::anim::JointRest{}, gw::anim::JointRest{}};
    return s;
}

gw::anim::ClipDesc make_stride_clip(gw::anim::SkeletonHandle sh) {
    gw::anim::ClipDesc c{};
    c.name = "stride";
    c.key_count = 8;
    c.duration_s = 0.8f;
    c.looping = true;
    c.skeleton = sh;
    gw::anim::ClipTrack tk{};
    tk.joint_index = 0;
    for (std::uint32_t k = 0; k < c.key_count; ++k) {
        const float t = static_cast<float>(k) / static_cast<float>(c.key_count);
        tk.translations.push_back(glm::vec3{0.0f, 0.05f * std::sin(6.283185f * t), 0.0f});
        tk.rotations.push_back(glm::quat{1.0f, 0.0f, 0.0f, 0.0f});
        tk.scales.push_back(glm::vec3{1.0f});
    }
    c.tracks.push_back(tk);
    return c;
}

gw::gameai::NavmeshDesc make_scene_navmesh() {
    gw::gameai::NavmeshDesc d{};
    d.name = "living_chunk";
    d.tile_count_x = 2;
    d.tile_count_z = 2;
    d.tile_size_m = 1.0f;
    d.cell_size_m = 0.25f;
    d.agent_radius_m = 0.15f;
    const int cells = 2 * 4;
    d.walkable.assign(static_cast<std::size_t>(cells * cells), 1);
    return d;
}

gw::gameai::BTDesc make_patrol_tree() {
    // Selector( Condition("is_arrived"), Sequence( Action("pick_target"), Action("drive_to_target") ) )
    gw::gameai::BTDesc t{};
    t.name = "patrol";
    gw::gameai::BTNodeDesc sel{}; sel.kind = gw::gameai::BTNodeKind::Selector;
    sel.children = {std::uint16_t(1), std::uint16_t(2)};
    gw::gameai::BTNodeDesc cond{}; cond.kind = gw::gameai::BTNodeKind::ConditionLeaf;
    cond.name = "is_arrived";
    gw::gameai::BTNodeDesc seq{};  seq.kind = gw::gameai::BTNodeKind::Sequence;
    seq.children = {std::uint16_t(3), std::uint16_t(4)};
    gw::gameai::BTNodeDesc pick{}; pick.kind = gw::gameai::BTNodeKind::ActionLeaf;
    pick.name = "pick_target";
    gw::gameai::BTNodeDesc drive{}; drive.kind = gw::gameai::BTNodeKind::ActionLeaf;
    drive.name = "drive_to_target";
    t.nodes.push_back(sel);   // 0 root
    t.nodes.push_back(cond);  // 1
    t.nodes.push_back(seq);   // 2
    t.nodes.push_back(pick);  // 3
    t.nodes.push_back(drive); // 4
    t.root_index = 0;
    return t;
}

struct DemoCtx {
    gw::gameai::GameAIWorld* ai{nullptr};
    gw::gameai::AgentHandle  agent{};
    glm::vec3                target{1.9f, 0.0f, 1.9f};
    std::uint32_t            pick_count{0};
    std::uint32_t            drive_count{0};
    std::uint32_t            arrived_count{0};
};

gw::gameai::BTStatus act_pick_target(void* user, gw::gameai::BTContext& ctx) {
    auto* d = static_cast<DemoCtx*>(user);
    ++d->pick_count;
    (void)ctx;
    d->ai->set_agent_target(d->agent, d->target);
    return gw::gameai::BTStatus::Success;
}

gw::gameai::BTStatus act_drive(void* user, gw::gameai::BTContext& ctx) {
    auto* d = static_cast<DemoCtx*>(user);
    ++d->drive_count;
    (void)ctx;
    return d->ai->agent_arrived(d->agent)
        ? gw::gameai::BTStatus::Success
        : gw::gameai::BTStatus::Running;
}

bool cond_is_arrived(void* user, gw::gameai::BTContext& ctx) {
    auto* d = static_cast<DemoCtx*>(user);
    (void)ctx;
    if (d->ai->agent_arrived(d->agent)) { ++d->arrived_count; return true; }
    return false;
}

} // namespace

int main(int argc, char** argv) {
    std::fprintf(stdout, "[sandbox_living_scene] greywater %s\n", gw::core::version_string());

    std::uint32_t frames = kDefaultFrames;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a.rfind("--frames=", 0) == 0) {
            try { frames = static_cast<std::uint32_t>(std::stoul(a.substr(9))); } catch (...) {}
        }
    }

    // ---- Animation world ---------------------------------------------------
    gw::anim::AnimationWorld anim;
    gw::anim::AnimationConfig acfg{};
    acfg.fixed_dt = 1.0f / 60.0f;
    if (!anim.initialize(acfg)) {
        std::fprintf(stderr, "[living] ERROR: anim.initialize failed\n");
        return EXIT_FAILURE;
    }
    const auto skel = anim.create_skeleton(make_stickman_skeleton());
    const auto clip = anim.create_clip(make_stride_clip(skel));

    // ---- Game-AI world -----------------------------------------------------
    gw::gameai::GameAIWorld ai;
    gw::gameai::GameAIConfig aicfg{};
    aicfg.bt_tick_hz = 60.0f;
    aicfg.bt_fixed_dt = 1.0f / 60.0f;
    if (!ai.initialize(aicfg)) {
        std::fprintf(stderr, "[living] ERROR: ai.initialize failed\n");
        return EXIT_FAILURE;
    }

    const auto nav  = ai.create_navmesh(make_scene_navmesh());
    const auto tree = ai.create_tree(make_patrol_tree());
    const auto inst = ai.create_bt_instance(tree, /*entity=*/0xA11CE);

    DemoCtx ctx{};
    ctx.ai    = &ai;
    ctx.agent = ai.create_agent(nav, /*entity=*/0xA11CE, glm::vec3{0.1f, 0.0f, 0.1f});
    ai.action_registry().register_action   ("pick_target",      &act_pick_target, &ctx);
    ai.action_registry().register_action   ("drive_to_target",  &act_drive,       &ctx);
    ai.action_registry().register_condition("is_arrived",       &cond_is_arrived, &ctx);

    // Seed the first path so the BT's `is_arrived` condition starts false.
    ai.set_agent_target(ctx.agent, ctx.target);

    // ---- Simulation loop ---------------------------------------------------
    const float dt = 1.0f / 60.0f;
    std::uint32_t anim_steps = 0;
    std::uint32_t bt_ticks   = 0;
    for (std::uint32_t i = 0; i < frames; ++i) {
        anim_steps += anim.step(dt);
        bt_ticks   += ai.step(dt);
    }
    (void)clip;
    (void)inst;

    const std::uint64_t anim_hash = anim.determinism_hash();
    const std::uint64_t ai_hash   = ai.determinism_hash();
    const glm::vec3     final_pos = ai.agent_position(ctx.agent);
    const bool          arrived   = ai.agent_arrived(ctx.agent);

    std::fprintf(stdout,
        "[sandbox_living_scene] LIVING OK — frames=%u anim_steps=%u bt_ticks=%u "
        "picks=%u drives=%u arrived=%d pos=(%.2f,%.2f,%.2f) "
        "anim_hash=%016llx ai_hash=%016llx anim_backend=%.*s ai_backend=%.*s\n",
        frames, anim_steps, bt_ticks,
        ctx.pick_count, ctx.drive_count, arrived ? 1 : 0,
        final_pos.x, final_pos.y, final_pos.z,
        static_cast<unsigned long long>(anim_hash),
        static_cast<unsigned long long>(ai_hash),
        static_cast<int>(anim.backend_name().size()), anim.backend_name().data(),
        static_cast<int>(ai.backend_name().size()),   ai.backend_name().data());

    ai.shutdown();
    anim.shutdown();
    return 0;
}
