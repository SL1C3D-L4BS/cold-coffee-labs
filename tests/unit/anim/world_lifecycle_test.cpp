// tests/unit/anim/world_lifecycle_test.cpp — Phase 13 Wave 13A.

#include <doctest/doctest.h>

#include "engine/anim/animation_world.hpp"

#include <string>

using namespace gw::anim;

TEST_CASE("AnimationWorld default-constructs and reports null backend") {
    AnimationWorld w;
    CHECK_FALSE(w.initialized());
    CHECK(w.backend() == BackendKind::Null);
    CHECK(std::string{w.backend_name()} == "null");
}

TEST_CASE("AnimationWorld::initialize is idempotent and applies config") {
    AnimationWorld w;
    AnimationConfig cfg{};
    cfg.fixed_dt = 1.0f / 60.0f;
    REQUIRE(w.initialize(cfg));
    CHECK(w.initialized());
    CHECK(w.config().fixed_dt == doctest::Approx(1.0f / 60.0f));
    CHECK(w.initialize(cfg));
}

TEST_CASE("AnimationWorld::shutdown then reinitialize yields empty world") {
    AnimationWorld w;
    REQUIRE(w.initialize(AnimationConfig{}));

    SkeletonDesc sdesc{};
    sdesc.name = "stickman";
    sdesc.joint_names = {"root", "spine"};
    sdesc.parents = {std::uint16_t(0xFFFF), std::uint16_t(0)};
    sdesc.rest_pose = {JointRest{}, JointRest{}};
    auto sh = w.create_skeleton(sdesc);
    REQUIRE(sh.valid());
    CHECK(w.skeleton_info(sh).joint_count == 2);

    w.shutdown();
    CHECK_FALSE(w.initialized());

    REQUIRE(w.initialize(AnimationConfig{}));
    CHECK(w.instance_count() == 0);
}

TEST_CASE("AnimationWorld pause gates step progression") {
    AnimationWorld w;
    REQUIRE(w.initialize(AnimationConfig{}));
    const auto f0 = w.frame_index();
    w.pause(true);
    (void)w.step(1.0f / 60.0f);
    CHECK(w.frame_index() == f0);
    w.pause(false);
    (void)w.step(1.0f / 60.0f);
    CHECK(w.frame_index() == f0 + 1);
}
