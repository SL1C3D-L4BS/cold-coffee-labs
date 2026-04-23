// apps/sandbox_physics/main.cpp — Phase 12 Wave 12F exit-demo (ADR-0038).
//
// This app is the Phase-12 exit-demo referenced in docs/05. It boots the
// runtime in headless / deterministic mode, then builds a small physics
// scene directly against the PhysicsWorld facade:
//   - a static 20x20 m floor box
//   - a 2x2x2 stack of dynamic crates
//   - a single character spawned 3 m above the floor
//   - a hinge-linked door for the constraint surface
//   - a 1024-ray raycast probe fan against the floor
// It ticks a fixed number of frames, prints the post-run determinism hash,
// and emits a grep-friendly summary so CI (`physics_sandbox`) can gate.
//
// The demo runs against the null backend by default; with
// `GW_ENABLE_JOLT=ON` (the `physics-*` preset) it exercises the real Jolt
// path end-to-end.

#include "engine/core/crash_reporter.hpp"
#include "engine/core/version.hpp"
#include "engine/physics/character_controller.hpp"
#include "engine/physics/collider.hpp"
#include "engine/physics/constraint.hpp"
#include "engine/physics/physics_config.hpp"
#include "engine/physics/physics_types.hpp"
#include "engine/physics/physics_world.hpp"
#include "engine/physics/queries.hpp"
#include "engine/physics/rigid_body.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace gw::physics;

namespace {

constexpr std::uint32_t kDefaultFrames = 120u;

struct RaycastProbeResult {
    std::uint32_t hits = 0;
    std::uint32_t total = 0;
};

RaycastProbeResult probe_floor(PhysicsWorld& w) {
    constexpr std::uint32_t kRays = 1024u;
    std::vector<RaycastInput>  inputs;
    std::vector<RaycastHit>    hits(kRays);
    std::vector<std::uint8_t>  flags(kRays, 0);
    inputs.reserve(kRays);
    for (std::uint32_t i = 0; i < kRays; ++i) {
        const float gx = static_cast<float>(i % 32) - 16.0f;
        const float gz = static_cast<float>(i / 32) - 16.0f;
        RaycastInput in{};
        in.origin_ws     = {gx * 0.25f, 5.0f, gz * 0.25f};
        in.direction_ws  = {0.0f, -1.0f, 0.0f};
        in.max_distance_m = 12.0f;
        inputs.push_back(in);
    }
    RaycastBatch batch{};
    batch.inputs    = inputs;
    batch.hits      = hits;
    batch.hit_flags = flags;
    w.raycast_batch(batch, QueryFilter{});
    RaycastProbeResult r{};
    r.total = kRays;
    for (auto f : flags) if (f != 0) ++r.hits;
    return r;
}

} // namespace

int main(int argc, char** argv) {
    gw::core::crash::install_handlers();
    std::fprintf(stdout, "[sandbox_physics] greywater %s\n", gw::core::version_string());

    std::uint32_t frames = kDefaultFrames;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a.rfind("--frames=", 0) == 0) {
            try { frames = static_cast<std::uint32_t>(std::stoul(a.substr(9))); } catch (...) {}
        }
    }

    PhysicsConfig cfg{};
    cfg.gravity = {0.0f, -9.81f, 0.0f};
    cfg.fixed_dt = 1.0f / 60.0f;

    PhysicsWorld world;
    if (!world.initialize(cfg)) {
        std::fprintf(stderr, "[sandbox_physics] ERROR: world initialize failed\n");
        return EXIT_FAILURE;
    }

    // Floor.
    const ShapeHandle floor_shape = world.create_shape(BoxShapeDesc{glm::vec3{10.0f, 0.1f, 10.0f}});
    RigidBodyDesc floor_desc{};
    floor_desc.shape = floor_shape;
    floor_desc.motion_type = BodyMotionType::Static;
    floor_desc.layer = ObjectLayer::Static;
    floor_desc.position_ws = {0.0, -0.1, 0.0};
    const BodyHandle floor_body = world.create_body(floor_desc);
    (void)floor_body;

    // 2x2x2 stack of crates.
    const ShapeHandle box_shape = world.create_shape(BoxShapeDesc{glm::vec3{0.5f, 0.5f, 0.5f}});
    std::vector<BodyHandle> stack;
    stack.reserve(8);
    for (int y = 0; y < 2; ++y) {
        for (int x = 0; x < 2; ++x) {
            for (int z = 0; z < 2; ++z) {
                RigidBodyDesc d{};
                d.shape = box_shape;
                d.motion_type = BodyMotionType::Dynamic;
                d.layer = ObjectLayer::Dynamic;
                d.position_ws = glm::dvec3{
                    1.2 * static_cast<double>(x) - 0.6,
                    2.0 + 1.2 * static_cast<double>(y),
                    1.2 * static_cast<double>(z) - 0.6,
                };
                d.mass_kg = 10.0f;
                stack.push_back(world.create_body(d));
            }
        }
    }

    // Hinge door (two dynamic bodies joined).
    RigidBodyDesc pivot_desc{};
    pivot_desc.shape       = box_shape;
    pivot_desc.motion_type = BodyMotionType::Static;
    pivot_desc.position_ws = {5.0, 1.0, 0.0};
    const BodyHandle pivot = world.create_body(pivot_desc);
    RigidBodyDesc door_desc{};
    door_desc.shape       = box_shape;
    door_desc.motion_type = BodyMotionType::Dynamic;
    door_desc.position_ws = {5.8, 1.0, 0.0};
    const BodyHandle door = world.create_body(door_desc);
    HingeConstraintDesc hinge_desc{};
    hinge_desc.a = pivot;
    hinge_desc.b = door;
    hinge_desc.anchor_ws = {5.4f, 1.0f, 0.0f};
    hinge_desc.axis_ws   = {0.0f, 1.0f, 0.0f};
    hinge_desc.break_impulse = 0.0f;
    const ConstraintHandle hinge_c = world.create_constraint(ConstraintDesc{hinge_desc});
    (void)hinge_c;

    // Character spawned above the floor.
    CharacterDesc cd{};
    cd.position_ws = {-3.0, 3.0, -3.0};
    cd.entity = 0xC0FFEE;
    const CharacterHandle ch = world.create_character(cd);

    // Tick the world.
    for (std::uint32_t i = 0; i < frames; ++i) world.step_fixed();

    const auto probe = probe_floor(world);
    const std::uint64_t hash_final = world.determinism_hash();
    const CharacterState cs = world.character_state(ch);

    std::fprintf(stdout,
        "[sandbox_physics] PHYSICS OK — frames=%u bodies=%zu active=%zu hash=%016llx "
        "probe_hits=%u/%u character_y=%.3f grounded=%d backend=%.*s\n",
        frames,
        world.body_count(),
        world.active_body_count(),
        static_cast<unsigned long long>(hash_final),
        probe.hits,
        probe.total,
        cs.position_ws.y,
        cs.on_ground ? 1 : 0,
        static_cast<int>(world.backend_name().size()),
        world.backend_name().data());

    world.shutdown();
    return 0;
}
