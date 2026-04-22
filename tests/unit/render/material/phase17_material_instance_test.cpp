// tests/unit/render/material/phase17_material_instance_test.cpp — ADR-0075.

#include <doctest/doctest.h>

#include "engine/render/material/material_world.hpp"
#include "engine/render/material/material_config.hpp"
#include "engine/render/material/material_instance.hpp"
#include "engine/render/material/material_template.hpp"

using namespace gw::render::material;

namespace {
struct WorldFixture {
    MaterialWorld world;
    WorldFixture() {
        MaterialConfig cfg{};
        cfg.instance_budget = 16;
        REQUIRE(world.initialize(cfg));
    }
    ~WorldFixture() { world.shutdown(); }
};
} // namespace

TEST_CASE("phase17.mat.instance: seeds default templates") {
    WorldFixture f;
    auto& w = f.world;
    CHECK(w.template_count() >= 3);
    CHECK(w.find_template_by_name("pbr_opaque/metal_rough").valid());
    CHECK(w.find_template_by_name("pbr_opaque/clearcoat").valid());
}

TEST_CASE("phase17.mat.instance: create_instance produces valid id") {
    WorldFixture f;
    auto& w = f.world;
    const auto tid = w.find_template_by_name("pbr_opaque/metal_rough");
    const auto iid = w.create_instance(tid);
    CHECK(iid.valid());
    CHECK(w.get_instance(iid).valid());
}

TEST_CASE("phase17.mat.instance: budget enforced") {
    WorldFixture f;
    auto& w = f.world;
    const auto tid = w.find_template_by_name("pbr_opaque/metal_rough");
    int ok = 0;
    for (int i = 0; i < 32; ++i) {
        if (w.create_instance(tid).valid()) ++ok;
    }
    CHECK(ok == 16);
}

TEST_CASE("phase17.mat.instance: destroy frees slot for reuse") {
    WorldFixture f;
    auto& w = f.world;
    const auto tid = w.find_template_by_name("pbr_opaque/metal_rough");
    const auto iid = w.create_instance(tid);
    w.destroy_instance(iid);
    const auto iid2 = w.create_instance(tid);
    CHECK(iid2.valid());
}

TEST_CASE("phase17.mat.instance: set_param_vec4 persists + read-back") {
    WorldFixture f;
    auto& w = f.world;
    const auto tid = w.find_template_by_name("pbr_opaque/metal_rough");
    const auto iid = w.create_instance(tid);
    CHECK(w.set_param_vec4(iid, "base_color", 0.1f, 0.2f, 0.3f, 1.0f));
    ParameterValue v{};
    CHECK(w.get_param(iid, "base_color", v));
    CHECK(v.v[0] == doctest::Approx(0.1f));
    CHECK(v.v[2] == doctest::Approx(0.3f));
}

TEST_CASE("phase17.mat.instance: set_texture updates slot fingerprint") {
    WorldFixture f;
    auto& w = f.world;
    const auto tid = w.find_template_by_name("pbr_opaque/metal_rough");
    const auto iid = w.create_instance(tid);
    TextureHandle h{0xABCD'1234'5678'9ABCull};
    CHECK(w.set_texture(iid, 0, h, 0xDEADBEEFu));
    CHECK_FALSE(w.set_texture(iid, 42, h, 0u));  // out-of-range slot
}

TEST_CASE("phase17.mat.instance: content_hash changes on mutation") {
    WorldFixture f;
    auto& w = f.world;
    const auto tid = w.find_template_by_name("pbr_opaque/metal_rough");
    const auto iid = w.create_instance(tid);
    const auto h0 = w.get_instance(iid).content_hash();
    CHECK(w.set_param_vec4(iid, "base_color", 0.9f, 0.1f, 0.1f, 1.0f));
    const auto h1 = w.get_instance(iid).content_hash();
    CHECK(h0 != h1);
}

TEST_CASE("phase17.mat.instance: reload restores default block") {
    WorldFixture f;
    auto& w = f.world;
    const auto tid = w.find_template_by_name("pbr_opaque/metal_rough");
    const auto iid = w.create_instance(tid);
    CHECK(w.set_param_vec4(iid, "base_color", 0.5f, 0.5f, 0.5f, 1.0f));
    CHECK(w.reload_instance(iid));
    ParameterValue v{};
    CHECK(w.get_param(iid, "base_color", v));
    CHECK(v.v[0] == doctest::Approx(1.0f));  // default
}

TEST_CASE("phase17.mat.instance: reload_all returns count of live instances") {
    WorldFixture f;
    auto& w = f.world;
    const auto tid = w.find_template_by_name("pbr_opaque/metal_rough");
    (void)w.create_instance(tid);
    (void)w.create_instance(tid);
    const auto n = w.reload_all();
    CHECK(n >= 2);
}

TEST_CASE("phase17.mat.instance: queued reload executes on step()") {
    WorldFixture f;
    auto& w = f.world;
    const auto tid = w.find_template_by_name("pbr_opaque/metal_rough");
    const auto iid = w.create_instance(tid);
    CHECK(w.set_param_vec4(iid, "base_color", 0.5f, 0.5f, 0.5f, 1.0f));
    w.request_reload(iid);
    w.step(1.0 / 60.0);
    ParameterValue v{};
    CHECK(w.get_param(iid, "base_color", v));
    CHECK(v.v[0] == doctest::Approx(1.0f));
}

TEST_CASE("phase17.mat.instance: describe_instance emits valid JSON string") {
    WorldFixture f;
    auto& w = f.world;
    const auto tid = w.find_template_by_name("pbr_opaque/metal_rough");
    const auto iid = w.create_instance(tid);
    const auto desc = w.describe_instance(iid);
    CHECK(!desc.empty());
    CHECK(desc.find("\"template\":") != std::string::npos);
    CHECK(desc.find("\"hash\":") != std::string::npos);
}

TEST_CASE("phase17.mat.instance: list_templates + list_instances are stable") {
    WorldFixture f;
    auto& w = f.world;
    const auto tid = w.find_template_by_name("pbr_opaque/metal_rough");
    for (int i = 0; i < 3; ++i) (void)w.create_instance(tid);
    CHECK(w.list_templates().size() >= 3);
    CHECK(w.list_instances().size() == 3);
}
