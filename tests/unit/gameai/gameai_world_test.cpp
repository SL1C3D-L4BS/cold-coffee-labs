// tests/unit/gameai/gameai_world_test.cpp — Phase 13 Wave 13E.

#include <doctest/doctest.h>

#include "engine/gameai/gameai_world.hpp"

#include <string>

using namespace gw::gameai;

namespace {

BTStatus always_ok(void*, BTContext&) { return BTStatus::Success; }
bool     always_true(void*, BTContext&) { return true; }

BTDesc make_sequence_tree() {
    // Build: Sequence(Action("ok1"), Condition("is_true")).
    // Topology rule (ADR-0043): children indices must be > parent index.
    BTDesc d{};
    d.name = "t1";
    BTNodeDesc seq{}; seq.kind = BTNodeKind::Sequence;
    seq.children = {std::uint16_t(1), std::uint16_t(2)};
    BTNodeDesc a{};   a.kind = BTNodeKind::ActionLeaf;    a.name = "ok1";
    BTNodeDesc c{};   c.kind = BTNodeKind::ConditionLeaf; c.name = "is_true";
    d.nodes.push_back(seq);
    d.nodes.push_back(a);
    d.nodes.push_back(c);
    d.root_index = 0;
    return d;
}

NavmeshDesc make_open_grid(int tx, int tz, int cells_per_tile = 4) {
    NavmeshDesc d{};
    d.name = "chunk";
    d.tile_count_x = tx;
    d.tile_count_z = tz;
    d.tile_size_m = 1.0f;                 // tile = 1m
    d.cell_size_m = 1.0f / cells_per_tile;
    d.agent_radius_m = 0.1f;
    d.walkable.assign(static_cast<std::size_t>(tx * tz * cells_per_tile * cells_per_tile), 1);
    return d;
}

} // namespace

TEST_CASE("GameAIWorld lifecycle + backend") {
    GameAIWorld w;
    CHECK_FALSE(w.initialized());
    CHECK(w.backend() == BackendKind::Null);
    CHECK(std::string{w.backend_name()} == "null");
    REQUIRE(w.initialize(GameAIConfig{}));
    CHECK(w.initialized());
    CHECK(w.initialize(GameAIConfig{}));  // idempotent
    w.shutdown();
    CHECK_FALSE(w.initialized());
}

TEST_CASE("validate_bt rejects malformed trees") {
    BTDesc d = make_sequence_tree();
    CHECK(validate_bt(d) == 0);

    // Remove children from the composite — must flag CompositeNoChildren.
    BTDesc bad = d;
    bad.nodes[0].children.clear();
    CHECK((validate_bt(bad) & kBT_V_CompositeNoChildren) != 0);

    // Child index pointing backward (child index <= parent index).
    bad = d;
    bad.nodes[2].children.push_back(std::uint16_t(1));  // leaf 2 points to earlier leaf 1
    CHECK((validate_bt(bad) & kBT_V_ChildIndexBeforeSelf) != 0);
}

TEST_CASE("BT executor runs a simple Sequence to Success") {
    GameAIWorld w;
    REQUIRE(w.initialize(GameAIConfig{}));
    w.action_registry().register_action("ok1",       &always_ok);
    w.action_registry().register_condition("is_true",&always_true);

    auto th = w.create_tree(make_sequence_tree());
    REQUIRE(th.valid());
    auto ih = w.create_bt_instance(th, /*entity*/ 42);
    REQUIRE(ih.valid());
    CHECK(w.bt_instance_count() == 1);

    w.tick_bt_fixed();
    CHECK(w.last_status(ih) == BTStatus::Success);
}

TEST_CASE("bt_content_hash is stable and path-hash diverges on node changes") {
    BTDesc d = make_sequence_tree();
    const auto h1 = bt_content_hash(d);
    const auto h2 = bt_content_hash(d);
    CHECK(h1 == h2);

    d.nodes[0].name = "ok1_renamed";
    const auto h3 = bt_content_hash(d);
    CHECK(h1 != h3);
}

TEST_CASE("Navmesh create + find_path on an open grid") {
    GameAIWorld w;
    REQUIRE(w.initialize(GameAIConfig{}));
    auto nav = w.create_navmesh(make_open_grid(2, 2));
    REQUIRE(nav.valid());
    CHECK(w.navmesh_info(nav).tile_count_x == 2);
    CHECK(w.navmesh_hash(nav) != 0);

    PathQueryInput in{};
    in.navmesh = nav;
    in.start_ws = glm::vec3{0.1f, 0.0f, 0.1f};
    in.end_ws   = glm::vec3{1.9f, 0.0f, 1.9f};
    PathQueryOutput out{};
    CHECK(w.find_path(in, out));
    CHECK(out.status == PathStatus::Ok);
    CHECK(out.waypoints.size() >= 2);
    CHECK(out.length_m > 0.0f);
    CHECK(out.result_hash != 0);
}

TEST_CASE("Navmesh find_path returns NoPath when the grid is blocked") {
    GameAIWorld w;
    REQUIRE(w.initialize(GameAIConfig{}));
    auto desc = make_open_grid(2, 1);
    // Block a vertical strip in cell column 4 (tile boundary).
    const int cells_x = 2 * 4;
    const int cells_z = 1 * 4;
    for (int z = 0; z < cells_z; ++z) {
        desc.walkable[static_cast<std::size_t>(z) * cells_x + 4] = 0;
    }
    // Also walls at 5 to be sure there is no diagonal sneak.
    for (int z = 0; z < cells_z; ++z) {
        desc.walkable[static_cast<std::size_t>(z) * cells_x + 5] = 0;
    }
    auto nav = w.create_navmesh(desc);

    PathQueryInput in{};
    in.navmesh = nav;
    in.start_ws = glm::vec3{0.1f, 0.0f, 0.1f};
    in.end_ws   = glm::vec3{1.9f, 0.0f, 0.1f};
    PathQueryOutput out{};
    CHECK_FALSE(w.find_path(in, out));
    CHECK(out.status == PathStatus::NoPath);
}

TEST_CASE("Navmesh unload_tile flips walkability + tiles emit events") {
    GameAIWorld w;
    REQUIRE(w.initialize(GameAIConfig{}));
    std::size_t bakes = 0, unloads = 0;
    (void)w.bus_tile_baked().subscribe(
        [&](const gw::events::NavmeshTileBaked&) { ++bakes; });
    (void)w.bus_tile_unloaded().subscribe(
        [&](const gw::events::NavmeshTileUnloaded&) { ++unloads; });

    auto nav = w.create_navmesh(make_open_grid(2, 2));
    CHECK(bakes == 4);

    w.unload_tile(nav, 0, 0);
    CHECK(unloads == 1);
}

TEST_CASE("Agent walks along a path at the BT sub-rate") {
    GameAIWorld w;
    GameAIConfig cfg{};
    cfg.bt_tick_hz = 60.0f;
    cfg.bt_fixed_dt = 1.0f / 60.0f;
    REQUIRE(w.initialize(cfg));
    auto nav = w.create_navmesh(make_open_grid(2, 1));

    auto a = w.create_agent(nav, /*entity*/ 99, glm::vec3{0.1f, 0.0f, 0.1f});
    w.set_agent_target(a, glm::vec3{1.9f, 0.0f, 0.1f});
    CHECK_FALSE(w.agent_arrived(a));

    // Drive 5 simulated seconds at 60 Hz — agent with 1.5 m/s must arrive.
    for (int i = 0; i < 300; ++i) (void)w.tick_agents(1.0f / 60.0f);
    CHECK(w.agent_arrived(a));
}

TEST_CASE("determinism_hash is stable across two identical runs") {
    auto run_once = []() {
        GameAIWorld w;
        w.initialize(GameAIConfig{});
        (void)w.create_navmesh(make_open_grid(2, 2));
        w.action_registry().register_action("ok1",        &always_ok);
        w.action_registry().register_condition("is_true", &always_true);
        auto th = w.create_tree(make_sequence_tree());
        auto ih = w.create_bt_instance(th, 1);
        for (int i = 0; i < 8; ++i) w.step_fixed();
        (void)ih;
        return w.determinism_hash();
    };
    CHECK(run_once() == run_once());
}
