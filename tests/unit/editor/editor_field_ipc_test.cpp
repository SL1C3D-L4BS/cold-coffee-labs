// tests/unit/editor/editor_field_ipc_test.cpp — Wave 1 BLD field IPC (Surface P).

#include <doctest/doctest.h>

#include "editor/bld_api/editor_bld_api.hpp"
#include "editor/scene/authoring_scene_load.hpp"
#include "editor/scene/components.hpp"
#include "editor/scene/transform_system.hpp"
#include "editor/selection/selection_context.hpp"
#include "editor/undo/command_stack.hpp"
#include "engine/ecs/world.hpp"

#include <cstring>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

using gw::ecs::Entity;
using gw::ecs::World;

namespace {

struct ScopedEditorGlobals {
    World                         world{};
    gw::editor::SelectionContext  sel{};
    gw::editor::undo::CommandStack stack{};

    ScopedEditorGlobals() {
        gw::editor::scene::ensure_authoring_scene_types_for_load(world);
        gw::editor::bld_api::g_globals.world     = &world;
        gw::editor::bld_api::g_globals.selection = &sel;
        gw::editor::bld_api::g_globals.cmd_stack = &stack;
    }
    ~ScopedEditorGlobals() {
        gw::editor::bld_api::g_globals.world     = nullptr;
        gw::editor::bld_api::g_globals.selection = nullptr;
        gw::editor::bld_api::g_globals.cmd_stack = nullptr;
    }
};

} // namespace

TEST_CASE("gw_editor_set_field / get_field — TransformComponent.position round-trip") {
    ScopedEditorGlobals g;
    const Entity        e = g.world.create_entity();
    g.world.add_component(e, gw::editor::scene::TransformComponent{});

    CHECK(gw_editor_set_field(e.raw_bits(), "TransformComponent.position", "[1.5,2.25,-3]"));
    const char* json = gw_editor_get_field(e.raw_bits(), "TransformComponent.position");
    REQUIRE(json != nullptr);
    CHECK(std::string_view{json}.find("1.5") != std::string_view::npos);
    gw_free_string(json);

    const auto* tr = g.world.get_component<gw::editor::scene::TransformComponent>(e);
    REQUIRE(tr != nullptr);
    CHECK(tr->position.x == doctest::Approx(1.5));
    CHECK(tr->position.y == doctest::Approx(2.25));
    CHECK(tr->position.z == doctest::Approx(-3.0));
}

TEST_CASE("gw_editor_set_field — VisibilityComponent.visible") {
    ScopedEditorGlobals g;
    const Entity        e = g.world.create_entity();
    g.world.add_component(e, gw::editor::scene::VisibilityComponent{});

    CHECK(gw_editor_set_field(e.raw_bits(), "VisibilityComponent.visible", "false"));
    const auto* vis = g.world.get_component<gw::editor::scene::VisibilityComponent>(e);
    REQUIRE(vis != nullptr);
    CHECK_FALSE(vis->visible);
}

TEST_CASE("gw_editor_set_field / get_field — BlockoutPrimitiveComponent.gwmat_rel") {
    ScopedEditorGlobals g;
    const Entity        e = g.world.create_entity();
    gw::editor::scene::BlockoutPrimitiveComponent bp{};
    bp.shape = 1;
    g.world.add_component(e, bp);

    CHECK(gw_editor_set_field(e.raw_bits(), "BlockoutPrimitiveComponent.gwmat_rel",
                              "\"content/materials/test.gwmat\""));
    const char* json =
        gw_editor_get_field(e.raw_bits(), "BlockoutPrimitiveComponent.gwmat_rel");
    REQUIRE(json != nullptr);
    CHECK(std::string_view{json}.find("test.gwmat") != std::string_view::npos);
    gw_free_string(json);

    const auto* out = g.world.get_component<gw::editor::scene::BlockoutPrimitiveComponent>(e);
    REQUIRE(out != nullptr);
    CHECK(std::string_view{out->gwmat_rel.data()} == "content/materials/test.gwmat");
}

TEST_CASE("gw_editor_get_field — BlockoutPrimitiveComponent.gwmat_rel JSON-escapes quotes") {
    ScopedEditorGlobals g;
    const Entity        e = g.world.create_entity();
    gw::editor::scene::BlockoutPrimitiveComponent bp{};
    g.world.add_component(e, bp);
    auto* mut = g.world.get_component<gw::editor::scene::BlockoutPrimitiveComponent>(e);
    REQUIRE(mut != nullptr);
    static constexpr char kPath[] = "content/materials/weird\"name.gwmat";
    std::memcpy(mut->gwmat_rel.data(), kPath, sizeof(kPath));

    const char* json = gw_editor_get_field(e.raw_bits(), "BlockoutPrimitiveComponent.gwmat_rel");
    REQUIRE(json != nullptr);
    CHECK(std::string_view{json} ==
          R"JSON("content/materials/weird\"name.gwmat")JSON");
    gw_free_string(json);
}

TEST_CASE("gw_editor_set_field — gwmat_rel parses JSON string escapes") {
    ScopedEditorGlobals g;
    const Entity        e = g.world.create_entity();
    gw::editor::scene::BlockoutPrimitiveComponent bp{};
    g.world.add_component(e, bp);

    CHECK(gw_editor_set_field(e.raw_bits(), "BlockoutPrimitiveComponent.gwmat_rel",
                              "\"a\\\\b\\\"c.gwmat\""));
    const auto* out = g.world.get_component<gw::editor::scene::BlockoutPrimitiveComponent>(e);
    REQUIRE(out != nullptr);
    CHECK(std::string_view{out->gwmat_rel.data()} == "a\\b\"c.gwmat");
}

TEST_CASE("gw_editor_set_field / get_field — Name path and NameComponent.value") {
    ScopedEditorGlobals g;
    const Entity        e = g.world.create_entity();
    g.world.add_component(e, gw::editor::scene::NameComponent{});
    g.world.add_component(e, gw::editor::scene::TransformComponent{});

    CHECK(gw_editor_set_field(e.raw_bits(), "Name", R"("entity_a")"));
    const char* j1 = gw_editor_get_field(e.raw_bits(), "Name");
    REQUIRE(j1 != nullptr);
    CHECK(std::string_view{j1}.find("entity_a") != std::string_view::npos);
    gw_free_string(j1);

    CHECK(gw_editor_set_field(e.raw_bits(), "NameComponent.value", R"("entity_b")"));
    const char* j2 = gw_editor_get_field(e.raw_bits(), "NameComponent.value");
    REQUIRE(j2 != nullptr);
    CHECK(std::string_view{j2}.find("entity_b") != std::string_view::npos);
    gw_free_string(j2);
}

TEST_CASE("gw_editor_set_field / get_field — TransformComponent scale and rotation") {
    ScopedEditorGlobals g;
    const Entity        e = g.world.create_entity();
    g.world.add_component(e, gw::editor::scene::TransformComponent{});

    CHECK(gw_editor_set_field(e.raw_bits(), "TransformComponent.scale", "[1,2,3.5]"));
    CHECK(gw_editor_set_field(e.raw_bits(), "TransformComponent.rotation", "[1,0,0,0]"));

    const char* s = gw_editor_get_field(e.raw_bits(), "TransformComponent.scale");
    REQUIRE(s != nullptr);
    CHECK(std::string_view{s}.find('3') != std::string_view::npos);
    gw_free_string(s);
    const char* r = gw_editor_get_field(e.raw_bits(), "TransformComponent.rotation");
    REQUIRE(r != nullptr);
    CHECK(r[0] == '[');
    gw_free_string(r);
}

TEST_CASE("gw_editor_get_field — WorldMatrixComponent.world after transform pass") {
    ScopedEditorGlobals g;
    const Entity        e = g.world.create_entity();
    g.world.add_component(e, gw::editor::scene::TransformComponent{});
    (void)gw::editor::scene::update_transforms(g.world);

    const char* json = gw_editor_get_field(e.raw_bits(), "WorldMatrixComponent.world");
    REQUIRE(json != nullptr);
    CHECK(json[0] == '[');
    gw_free_string(json);

    CHECK(gw_editor_set_field(e.raw_bits(), "WorldMatrixComponent.dirty", "0"));
    const auto* wm = g.world.get_component<gw::editor::scene::WorldMatrixComponent>(e);
    REQUIRE(wm != nullptr);
    CHECK(wm->dirty == 0);
}
