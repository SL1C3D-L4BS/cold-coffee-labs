// tests/seq/seq_spline_test.cpp — CameraSpline + CutSystem (Phase 18-B).

#include <doctest/doctest.h>

#include "editor/scene/components.hpp"
#include "engine/ecs/world.hpp"
#include "engine/scene/seq/seq_camera.hpp"
#include "engine/scene/seq/seq_cut.hpp"
#include "engine/scene/seq/seq_spline.hpp"

#include <cmath>

using gw::math::Quatd;
using gw::math::Vec3d;
using gw::seq::CameraBlendStateComponent;
using gw::seq::CameraSpline;
using gw::seq::CameraSplineSample;
using gw::seq::CinematicCameraComponent;
using gw::seq::CinematicCameraSystem;
using gw::seq::CutBlendCurve;
using gw::seq::CutEntry;
using gw::seq::CutListComponent;
using gw::seq::CutSystem;
using gw::seq::SplineKnot;
using namespace gw::seq;
using namespace gw::editor::scene;

namespace {

[[nodiscard]] bool near3(const Vec3d& a, const Vec3d& b, const double e) {
    return std::abs(a.x() - b.x()) <= e && std::abs(a.y() - b.y()) <= e && std::abs(a.z() - b.z()) <= e;
}

[[nodiscard]] double quat_len(const Quatd& q) {
    return std::sqrt(q.w() * q.w() + q.x() * q.x() + q.y() * q.y() + q.z() * q.z());
}

}  // namespace

TEST_CASE("CameraSpline — evaluate at endpoints and midpoint (Catmull-Rom)") {
    CameraSpline s;
    SplineKnot   k0{};
    k0.frame     = 0;
    k0.position  = Vec3d(0.0, 0.0, 0.0);
    k0.rotation  = Quatd(1, 0, 0, 0);
    k0.fov_deg   = 60.f;
    SplineKnot k1 = k0;
    k1.frame     = 10;
    k1.position  = Vec3d(1.0, 0.0, 0.0);
    s.set_knots({k0, k1});

    const CameraSplineSample a = s.evaluate(0.0);
    const CameraSplineSample b = s.evaluate(10.0);
    const CameraSplineSample m = s.evaluate(5.0);
    CHECK(near3(a.position, Vec3d(0, 0, 0), 1e-6));
    CHECK(near3(b.position, Vec3d(1, 0, 0), 1e-6));
    // Mid-frame between two knots: chord midpoint when endpoints duplicated in CR window.
    CHECK(near3(m.position, Vec3d(0.5, 0.0, 0.0), 1e-2));
    CHECK(m.fov_deg == doctest::Approx(60.f).epsilon(1e-3f));
}

TEST_CASE("CameraSpline — slerp continuity, unit quaternions") {
    CameraSpline s;
    SplineKnot   a{};
    a.frame    = 0;
    a.rotation = Quatd(1, 0, 0, 0);
    SplineKnot b{};
    b.frame    = 10;
    // 90° about +Y: half-angle 45° in w,x,y,z (w,cy,sy here as x=z=0, y=sin(45))
    b.rotation = Quatd(0.7071067811865476, 0, 0.7071067811865476, 0);
    b.position = a.position;
    a.fov_deg  = 50.f;
    b.fov_deg  = 50.f;
    s.set_knots({a, b});

    for (double t = 0.0; t <= 10.0; t += 0.5) {
        const CameraSplineSample o = s.evaluate(t);
        CHECK_FALSE(std::isnan(o.rotation.w()));
        CHECK_FALSE(std::isnan(o.rotation.x()));
        CHECK_FALSE(std::isnan(o.rotation.y()));
        CHECK_FALSE(std::isnan(o.rotation.z()));
        const double l = quat_len(o.rotation);
        CHECK(l == doctest::Approx(1.0).epsilon(1e-5));
    }
}

TEST_CASE("CutSystem — hard cut activates to_camera immediately after cut frame") {
    gw::ecs::World w;

    const auto from = w.create_entity();
    const auto to   = w.create_entity();
    w.add_component<TransformComponent>(from, TransformComponent{});
    w.add_component<TransformComponent>(to, TransformComponent{});
    w.add_component<CinematicCameraComponent>(from, CinematicCameraComponent{.active = false, .fov_deg = 60.f});
    w.add_component<CinematicCameraComponent>(to, CinematicCameraComponent{.active = false, .fov_deg = 60.f});

    const auto dir = w.create_entity();
    CutListComponent list{};
    list.cut_count  = 1u;
    list.cuts[0]    = CutEntry{from, to, 10u, 0u, CutBlendCurve::Cut};
    w.add_component<CutListComponent>(dir, list);

    CutSystem::tick(w, 5.0);
    CHECK(w.get_component<CinematicCameraComponent>(from)->active == true);
    CHECK(w.get_component<CinematicCameraComponent>(to)->active == false);

    CutSystem::tick(w, 10.0);
    CHECK(w.get_component<CinematicCameraComponent>(from)->active == false);
    CHECK(w.get_component<CinematicCameraComponent>(to)->active == true);
}

TEST_CASE("CutSystem — blend writes weight in [0,1] and enables CameraBlendStateComponent") {
    gw::ecs::World w;

    const auto from = w.create_entity();
    const auto to   = w.create_entity();
    w.add_component<TransformComponent>(from, TransformComponent{});
    w.add_component<TransformComponent>(to, TransformComponent{});
    w.add_component<CinematicCameraComponent>(from, CinematicCameraComponent{.active = false, .fov_deg = 60.f});
    w.add_component<CinematicCameraComponent>(to, CinematicCameraComponent{.active = false, .fov_deg = 60.f});

    const auto dir = w.create_entity();
    CutListComponent list{};
    list.cut_count  = 1u;
    list.cuts[0] =
        CutEntry{from, to, 0u, 20u, CutBlendCurve::Linear};
    w.add_component<CutListComponent>(dir, list);

    CutSystem::tick(w, 0.0);
    w.for_each<CameraBlendStateComponent>([&](const gw::ecs::Entity, const CameraBlendStateComponent& s) {
        CHECK(s.blend_active == true);
        CHECK(s.weight == doctest::Approx(0.f).epsilon(0.01f));
    });

    CutSystem::tick(w, 10.0);
    w.for_each<CameraBlendStateComponent>([&](const gw::ecs::Entity, const CameraBlendStateComponent& s) {
        CHECK(s.weight >= 0.f);
        CHECK(s.weight <= 1.f);
        CHECK(s.weight == doctest::Approx(0.5f).epsilon(0.02f));
        CHECK(s.blend_active == true);
    });

    CinematicCameraSystem cine{};
    cine.tick(w);
    CHECK(cine.output().valid == true);
}
