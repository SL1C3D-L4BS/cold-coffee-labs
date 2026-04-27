// tests/unit/anim/skin_palette_test.cpp
// Community 0/8: mesh stream2 inverse binds × AnimationWorld world pose.
#include <doctest/doctest.h>
#include "engine/anim/animation_world.hpp"
#include "engine/anim/anim_config.hpp"
#include "engine/anim/skeleton.hpp"

#include <glm/mat4x4.hpp>

using namespace gw::anim;

TEST_CASE("AnimationWorld::build_skin_matrix_palette single joint identity") {
    AnimationWorld w;
    AnimationConfig cfg{};
    REQUIRE(w.initialize(cfg));

    SkeletonDesc sk{};
    sk.name         = "unit_skin";
    sk.joint_names  = {"root"};
    sk.parents      = {kInvalidJoint};
    sk.rest_pose    = {JointRest{}};
    const SkeletonHandle sh = w.create_skeleton(sk);
    REQUIRE(sh.id != 0);

    InstanceDesc id{};
    id.skeleton = sh;
    const InstanceHandle ih = w.create_instance(id);
    REQUIRE(ih.id != 0);

    const glm::mat4 ib{1.0f};
    std::vector<glm::mat4> inverse_bind{ib};
    std::vector<glm::mat4> palette(1);

    REQUIRE(w.build_skin_matrix_palette(ih, inverse_bind, palette));
    CHECK(palette[0][0][0] == doctest::Approx(1.0f));
    CHECK(palette[0][1][1] == doctest::Approx(1.0f));
    CHECK(palette[0][2][2] == doctest::Approx(1.0f));
    CHECK(palette[0][3][3] == doctest::Approx(1.0f));

    w.shutdown();
}
