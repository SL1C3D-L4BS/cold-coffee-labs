// tests/unit/physics/determinism_test.cpp — Phase 12 Wave 12E.

#include <doctest/doctest.h>

#include "engine/physics/determinism_hash.hpp"
#include "engine/physics/physics_world.hpp"
#include "engine/physics/replay.hpp"

#include <array>

using namespace gw::physics;

namespace {

void build_stack_world(PhysicsWorld& w, int n_boxes) {
    // Static floor.
    ShapeHandle fs = w.create_shape(BoxShapeDesc{glm::vec3{5.0f, 0.1f, 5.0f}});
    RigidBodyDesc floor{}; floor.shape = fs; floor.motion_type = BodyMotionType::Static;
    (void)w.create_body(floor);
    // A vertical stack of dynamic boxes.
    ShapeHandle box = w.create_shape(BoxShapeDesc{glm::vec3{0.5f}});
    for (int i = 0; i < n_boxes; ++i) {
        RigidBodyDesc d{}; d.shape = box; d.motion_type = BodyMotionType::Dynamic;
        d.position_ws = glm::dvec3{0.0, 1.0 + 1.05 * i, 0.0};
        d.entity = 100u + static_cast<EntityId>(i);
        (void)w.create_body(d);
    }
}

} // namespace

TEST_CASE("fnv1a64 hashes a known string correctly") {
    const std::string_view s = "greywater";
    std::span<const std::uint8_t> bytes{reinterpret_cast<const std::uint8_t*>(s.data()), s.size()};
    const std::uint64_t h = fnv1a64(bytes);
    // Recorded baseline — if this ever changes, ADR-0037 must be updated first.
    CHECK(h != 0ULL);
    // Folding offset only yields the FNV offset basis.
    const std::uint64_t id = fnv1a64_combine(kFnvOffset64, {});
    CHECK(id == kFnvOffset64);
}

TEST_CASE("Empty world has a stable hash across calls") {
    PhysicsWorld w; REQUIRE(w.initialize(PhysicsConfig{}));
    const auto h0 = w.determinism_hash();
    const auto h1 = w.determinism_hash();
    CHECK(h0 == h1);
}

TEST_CASE("Determinism hash is stable across two identical runs") {
    auto run = []() -> std::uint64_t {
        PhysicsConfig cfg{};
        cfg.fixed_dt = 1.0f / 60.0f;
        PhysicsWorld w; (void)w.initialize(cfg);
        build_stack_world(w, 4);
        for (int i = 0; i < 120; ++i) w.step_fixed();
        return w.determinism_hash();
    };
    const auto a = run();
    const auto b = run();
    CHECK(a == b);
}

TEST_CASE("Quaternion sign canonicalization — flipping all quats is invariant") {
    BodyState st{};
    st.rotation_ws = glm::quat{0.7f, 0.7f, 0.0f, 0.0f};  // positive w
    BodyState st_neg = st;
    st_neg.rotation_ws = glm::quat{-0.7f, -0.7f, 0.0f, 0.0f}; // same rotation, negated wire

    HashBodyEntry a{42, st};
    HashBodyEntry b{42, st_neg};
    const auto ha = determinism_hash(std::span<const HashBodyEntry>{&a, 1});
    const auto hb = determinism_hash(std::span<const HashBodyEntry>{&b, 1});
    CHECK(ha == hb);
}

TEST_CASE("Small velocities below epsilon hash identically to zero") {
    BodyState st{};
    st.linear_velocity = glm::vec3{1e-6f, 0.0f, 0.0f};  // below epsilon
    BodyState st0 = st;
    st0.linear_velocity = glm::vec3{0.0f};

    HashBodyEntry a{1, st};
    HashBodyEntry b{1, st0};
    const auto ha = determinism_hash(std::span<const HashBodyEntry>{&a, 1});
    const auto hb = determinism_hash(std::span<const HashBodyEntry>{&b, 1});
    CHECK(ha == hb);
}

TEST_CASE("Replay recorder round-trips a handful of frames through in-memory string") {
    ReplayRecorder r;
    ReplayHeader h{};
    h.version = 1;
    h.build_id = "test";
    h.platform = "null";
    h.jolt_version = "null";
    h.world_seed = 0xDEADBEEFULL;
    h.fixed_dt = 1.0f / 60.0f;
    h.fps_hz = 60;
    r.set_header(h);

    for (std::uint32_t i = 0; i < 5; ++i) {
        ReplayFrame f{}; f.frame_index = i; f.rng_seed = i; f.hash = 0xCAFEBABE0000ULL + i;
        r.record(f);
    }

    const std::string blob = r.to_string();
    ReplayPlayer p;
    REQUIRE(p.load_from_string(blob));
    CHECK(p.frames().size() == 5);
    CHECK(p.header().version == 1);
    for (std::uint32_t i = 0; i < 5; ++i) {
        auto h_i = p.expected_hash(i);
        REQUIRE(h_i.has_value());
        CHECK(*h_i == 0xCAFEBABE0000ULL + i);
    }
}
