// tests/unit/editor_pie/gameplay_host_test.cpp
// Phase 7 fullstack Path-A — PIE snapshot/restore via ecs::save_world /
// load_world (ADR-0006). Covers the enter_play → stop transition without a
// gameplay dll so the test has no dependency on a prebuilt plugin binary.

#include <doctest/doctest.h>

#include "editor/pie/gameplay_host.hpp"
#include "engine/core/time.hpp"
#include "engine/ecs/hierarchy.hpp"
#include "engine/ecs/world.hpp"

using gw::TimeState;
using gw::ecs::Entity;
using gw::ecs::HierarchyComponent;
using gw::ecs::World;
using gw::editor::GameplayContext;
using gw::editor::GameplayHost;

namespace {

struct Health { int hp = 100; };
struct Pos    { float x = 0.f, y = 0.f, z = 0.f; };

static_assert(std::is_trivially_copyable_v<Health>);
static_assert(std::is_trivially_copyable_v<Pos>);

// Small helper that stamps a scene we can diff before/after a PIE round trip.
Entity seed_world(World& w) {
    const Entity a = w.create_entity();
    const Entity b = w.create_entity();
    w.add_component(a, Pos{1.f, 2.f, 3.f});
    w.add_component(a, Health{75});
    w.add_component(b, Pos{10.f, 20.f, 30.f});
    REQUIRE(w.reparent(b, a));
    return a;
}

}  // namespace

TEST_CASE("gameplay_host — take_snapshot / restore_snapshot round-trips a world") {
    World src;
    const Entity root = seed_world(src);
    (void)root;

    GameplayHost host;
    REQUIRE(host.take_snapshot(src));
    CHECK(host.snapshot_size() > 0);

    // Mutate the world after snapshot — Play-mode style.
    src.for_each<Health>([](Entity, Health& h) { h.hp = 0; });
    src.for_each<Pos>([](Entity, Pos& p) { p.x = -999.f; });
    const Entity ghost = src.create_entity();
    src.add_component(ghost, Pos{42.f, 42.f, 42.f});
    REQUIRE(src.entity_count() == 3);

    REQUIRE(host.restore_snapshot(src));

    CHECK(src.entity_count() == 2);

    std::size_t h_seen = 0;
    src.for_each<Health>([&](Entity, Health& h) {
        CHECK(h.hp == 75);
        ++h_seen;
    });
    CHECK(h_seen == 1);

    std::size_t p_seen = 0;
    src.for_each<Pos>([&](Entity, Pos& p) {
        CHECK(p.x != doctest::Approx(-999.f));
        ++p_seen;
    });
    CHECK(p_seen == 2);
}

TEST_CASE("gameplay_host — hierarchy survives snapshot/restore") {
    World src;
    const Entity parent = src.create_entity();
    const Entity child  = src.create_entity();
    src.add_component(parent, Pos{});
    src.add_component(child,  Pos{});
    REQUIRE(src.reparent(child, parent));

    GameplayHost host;
    REQUIRE(host.take_snapshot(src));

    // Blow the hierarchy away between snapshot and restore.
    REQUIRE(src.reparent(child, gw::ecs::Entity::null()));
    CHECK(src.get_component<HierarchyComponent>(child)->parent ==
          gw::ecs::Entity::null());

    REQUIRE(host.restore_snapshot(src));
    // Handle bits are preserved by load_world's reuse_bits path — the
    // original `child` handle still resolves and points at the same parent.
    REQUIRE(src.is_alive(child));
    const auto* h = src.get_component<HierarchyComponent>(child);
    REQUIRE(h != nullptr);
    CHECK(h->parent == parent);
}

TEST_CASE("gameplay_host — enter_play snapshots, stop restores (no dll)") {
    World world;
    seed_world(world);
    const std::size_t seeded_count = world.entity_count();

    TimeState       time{};
    GameplayContext ctx{};
    ctx.world = &world;
    ctx.time  = &time;

    GameplayHost host;
    // No lib_path — enter_play must still succeed (snapshot + no-dll path).
    REQUIRE(host.enter_play(ctx));
    CHECK(host.in_play());
    CHECK(host.state() == GameplayHost::PIEState::Playing);
    CHECK(host.snapshot_size() > 0);

    // Simulate in-Play mutation.
    world.for_each<Health>([](Entity, Health& h) { h.hp = -1; });
    (void)world.create_entity();
    CHECK(world.entity_count() == seeded_count + 1);

    host.stop(ctx);
    CHECK_FALSE(host.in_play());
    CHECK(host.state() == GameplayHost::PIEState::Editor);

    CHECK(world.entity_count() == seeded_count);
    std::size_t restored_hp_ok = 0;
    world.for_each<Health>([&](Entity, Health& h) {
        CHECK(h.hp == 75);
        ++restored_hp_ok;
    });
    CHECK(restored_hp_ok == 1);
    // Snapshot buffer is released after stop().
    CHECK(host.snapshot_size() == 0);
}

TEST_CASE("gameplay_host — enter_play rejects a null world") {
    GameplayContext ctx{};  // world == nullptr
    GameplayHost    host;
    CHECK_FALSE(host.enter_play(ctx));
    CHECK_FALSE(host.in_play());
}

TEST_CASE("gameplay_host — pause/resume toggles without unloading snapshot") {
    World world;
    seed_world(world);

    TimeState       time{};
    GameplayContext ctx{};
    ctx.world = &world;
    ctx.time  = &time;

    GameplayHost host;
    REQUIRE(host.enter_play(ctx));
    CHECK(host.state() == GameplayHost::PIEState::Playing);
    host.pause();
    CHECK(host.state() == GameplayHost::PIEState::Paused);
    CHECK(host.in_play());
    host.resume();
    CHECK(host.state() == GameplayHost::PIEState::Playing);
    host.stop(ctx);
    CHECK(host.state() == GameplayHost::PIEState::Editor);
}
