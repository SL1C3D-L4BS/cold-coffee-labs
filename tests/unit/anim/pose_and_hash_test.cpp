// tests/unit/anim/pose_and_hash_test.cpp — Phase 13.

#include <doctest/doctest.h>

#include "engine/anim/pose.hpp"
#include "engine/anim/skeleton.hpp"

using namespace gw::anim;

TEST_CASE("pose_lerp between identity and translated pose is midpoint at w=0.5") {
    Pose a(2);
    Pose b(2);
    b.joints()[0].translation = glm::vec3{2.0f, 0.0f, 0.0f};
    b.joints()[1].translation = glm::vec3{-1.0f, 4.0f, 0.0f};

    Pose out(2);
    pose_lerp(out, a, b, 0.5f);

    CHECK(out.joints()[0].translation.x == doctest::Approx(1.0f));
    CHECK(out.joints()[1].translation.y == doctest::Approx(2.0f));
}

TEST_CASE("pose_hash is stable under identical inputs and quaternion sign") {
    Pose p(1);
    p.joints()[0].rotation = glm::quat{0.7071f, 0.0f, 0.7071f, 0.0f};
    const auto h1 = pose_hash(p);

    // Flip the sign — rotation is equivalent.
    p.joints()[0].rotation = glm::quat{-0.7071f, 0.0f, -0.7071f, 0.0f};
    const auto h2 = pose_hash(p);
    CHECK(h1 == h2);
}

TEST_CASE("skeleton_content_hash changes when parents change") {
    SkeletonDesc s{};
    s.name = "a";
    s.joint_names = {"root", "spine"};
    s.parents     = {std::uint16_t(0xFFFF), std::uint16_t(0)};
    s.rest_pose   = {JointRest{}, JointRest{}};
    const auto h1 = skeleton_content_hash(s);

    s.parents[1] = 0xFFFF;
    const auto h2 = skeleton_content_hash(s);
    CHECK(h1 != h2);
}
