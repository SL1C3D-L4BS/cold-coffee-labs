// tests/unit/render/post/phase17_motion_blur_dof_test.cpp — ADR-0079 Wave 17F.

#include <doctest/doctest.h>

#include "engine/render/post/dof.hpp"
#include "engine/render/post/motion_blur.hpp"

#include <vector>

using namespace gw::render::post;

TEST_CASE("phase17.mb.tilemax: reduces velocity field to tile_w*tile_h*2") {
    std::vector<float> v(32 * 32 * 2, 0.0f);
    v[0] = 5.0f; v[1] = 0.0f;     // inject a big pixel at (0,0)
    std::vector<float> tm;
    motion_blur_tilemax(v, 32, 32, 16, tm);
    CHECK(tm.size() == 2u * 2u * 2u);   // 32/16 = 2 tiles per axis
    CHECK(tm[0] == doctest::Approx(5.0f));
}

TEST_CASE("phase17.mb.tilemax: chooses max |velocity| per tile") {
    std::vector<float> v(16 * 16 * 2, 0.0f);
    v[(5 * 16 + 7) * 2 + 0] = 0.0f;
    v[(5 * 16 + 7) * 2 + 1] = 3.0f;
    v[(3 * 16 + 2) * 2 + 0] = 1.0f;
    v[(3 * 16 + 2) * 2 + 1] = 1.0f;
    std::vector<float> tm;
    motion_blur_tilemax(v, 16, 16, 16, tm);
    CHECK(tm.size() == 2u);
    CHECK(tm[1] == doctest::Approx(3.0f));
}

TEST_CASE("phase17.mb.tilemax: zero field yields zero tile_max") {
    std::vector<float> v(16 * 16 * 2, 0.0f);
    std::vector<float> tm;
    motion_blur_tilemax(v, 16, 16, 16, tm);
    CHECK(tm[0] == doctest::Approx(0.0f));
    CHECK(tm[1] == doctest::Approx(0.0f));
}

TEST_CASE("phase17.mb.neighbormax: 3x3 max over tile_max") {
    // 3x3 tile grid, put a hot spike in center.
    std::vector<float> tm(3 * 3 * 2, 0.0f);
    tm[(1 * 3 + 1) * 2 + 0] = 9.0f;
    std::vector<float> nm;
    motion_blur_neighbormax(tm, 3, 3, nm);
    // All 9 tiles include the center tile in their 3x3 kernel.
    for (std::size_t i = 0; i < 9; ++i) {
        CHECK(nm[i * 2 + 0] == doctest::Approx(9.0f));
    }
}

TEST_CASE("phase17.dof.coc: at focus distance → ~0 px") {
    DofConfig cfg;
    cfg.focal_mm = 50.0f; cfg.aperture = 2.8f; cfg.focus_distance_m = 5.0f;
    const float coc = coc_radius_px(cfg, 5.0f, 1080);
    CHECK(coc == doctest::Approx(0.0f).epsilon(1e-2));
}

TEST_CASE("phase17.dof.coc: near object gives positive CoC") {
    DofConfig cfg;
    cfg.focal_mm = 50.0f; cfg.aperture = 2.0f; cfg.focus_distance_m = 5.0f;
    const float coc = coc_radius_px(cfg, 1.0f, 1080);
    CHECK(coc > 0.0f);
}

TEST_CASE("phase17.dof.coc: wider aperture = larger CoC") {
    DofConfig base;
    base.focal_mm = 50.0f; base.focus_distance_m = 5.0f;
    DofConfig narrow = base; narrow.aperture = 16.0f;
    DofConfig wide   = base; wide.aperture   = 1.4f;
    CHECK(coc_radius_px(wide, 1.0f, 1080) > coc_radius_px(narrow, 1.0f, 1080));
}

TEST_CASE("phase17.dof.blend: near plane lights near_w, far plane lights far_w") {
    DofConfig cfg;
    cfg.focal_mm = 50.0f; cfg.aperture = 1.4f; cfg.focus_distance_m = 5.0f;
    const auto near_blend = coc_blend(cfg, 1.0f,  1080);
    const auto far_blend  = coc_blend(cfg, 50.0f, 1080);
    CHECK(near_blend.near_w > 0.0f);
    CHECK(near_blend.far_w  == doctest::Approx(0.0f));
    CHECK(far_blend.far_w   > 0.0f);
    CHECK(far_blend.near_w  == doctest::Approx(0.0f));
}
