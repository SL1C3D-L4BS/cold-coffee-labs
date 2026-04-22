// tests/unit/ecs/world_test.cpp
// Phase 7 gap-fill (ECS fullstack) — see docs/10_APPENDIX_ADRS_AND_REFERENCES.md.
//
// Covers:
//   - entity lifecycle (create / destroy / is_alive / generation reuse)
//   - component add/remove → archetype migration
//   - typed queries (for_each<Components...>)
//   - hierarchy reparent / traversal / cycle rejection
//   - non-trivially-copyable component lifecycle
//   - bulk 10 000-entity stress path

#include <ostream>
#include <doctest/doctest.h>

#include "engine/ecs/world.hpp"
#include "engine/ecs/hierarchy.hpp"

#include <string>
#include <unordered_set>

using gw::ecs::Entity;
using gw::ecs::HierarchyComponent;
using gw::ecs::World;

namespace {

struct Position { float x = 0, y = 0, z = 0; };
struct Velocity { float dx = 0, dy = 0, dz = 0; };
struct Tag      {};

// Non-trivially-copyable — forces the function-pointer lifecycle path.
struct Name {
    std::string value;
    Name() = default;
    Name(std::string v) : value(std::move(v)) {}
};

} // namespace

TEST_CASE("World — create_entity produces unique live handles") {
    World w;
    const auto a = w.create_entity();
    const auto b = w.create_entity();
    CHECK(w.is_alive(a));
    CHECK(w.is_alive(b));
    CHECK_FALSE(a == b);
    CHECK(w.entity_count() == 2);
}

TEST_CASE("World — destroy_entity invalidates the handle; reuse bumps generation") {
    World w;
    const auto a  = w.create_entity();
    const auto a0 = a;
    w.destroy_entity(a);
    CHECK_FALSE(w.is_alive(a0));
    CHECK(w.entity_count() == 0);

    const auto b = w.create_entity();
    // New entity reuses the slot but carries a higher generation.
    CHECK(b.index() == a0.index());
    CHECK(b.generation() > a0.generation());
    CHECK_FALSE(w.is_alive(a0));     // stale handle still dead
    CHECK(w.is_alive(b));
}

TEST_CASE("World — add/get/has/remove component (trivial)") {
    World w;
    const auto e = w.create_entity();
    CHECK_FALSE(w.has_component<Position>(e));

    w.add_component<Position>(e, {1.f, 2.f, 3.f});
    CHECK(w.has_component<Position>(e));
    auto* p = w.get_component<Position>(e);
    REQUIRE(p != nullptr);
    CHECK(p->x == 1.f);
    CHECK(p->y == 2.f);
    CHECK(p->z == 3.f);

    w.remove_component<Position>(e);
    CHECK_FALSE(w.has_component<Position>(e));
    CHECK(w.get_component<Position>(e) == nullptr);
}

TEST_CASE("World — add_component twice overwrites in place") {
    World w;
    const auto e = w.create_entity();
    w.add_component<Position>(e, {1, 2, 3});
    w.add_component<Position>(e, {4, 5, 6});
    const auto* p = w.get_component<Position>(e);
    REQUIRE(p != nullptr);
    CHECK(p->x == 4);
    CHECK(p->y == 5);
    CHECK(p->z == 6);
}

TEST_CASE("World — archetype migration preserves other components") {
    World w;
    const auto e = w.create_entity();
    w.add_component<Position>(e, {10, 20, 30});
    w.add_component<Velocity>(e, {1, 2, 3});

    CHECK(w.has_component<Position>(e));
    CHECK(w.has_component<Velocity>(e));

    // Remove Position — Velocity must survive the archetype migration.
    w.remove_component<Position>(e);
    CHECK_FALSE(w.has_component<Position>(e));
    REQUIRE(w.has_component<Velocity>(e));
    const auto* v = w.get_component<Velocity>(e);
    CHECK(v->dx == 1);
    CHECK(v->dy == 2);
    CHECK(v->dz == 3);
}

TEST_CASE("World — for_each iterates matching archetypes") {
    World w;
    const auto a = w.create_entity();
    const auto b = w.create_entity();
    const auto c = w.create_entity();
    w.add_component<Position>(a, {1, 0, 0});
    w.add_component<Velocity>(a, {});
    w.add_component<Position>(b, {2, 0, 0});
    w.add_component<Position>(c, {3, 0, 0});  // no Velocity

    std::unordered_set<std::uint64_t> seen;
    w.for_each<Position, Velocity>([&](Entity e, Position& /*p*/, Velocity& /*v*/) {
        seen.insert(e.raw_bits());
    });
    CHECK(seen.size() == 1);
    CHECK(seen.count(a.raw_bits()) == 1);

    seen.clear();
    w.for_each<Position>([&](Entity e, Position& /*p*/) { seen.insert(e.raw_bits()); });
    CHECK(seen.size() == 3);
}

TEST_CASE("World — non-trivial component is destructed on entity destroy") {
    World w;
    const auto e = w.create_entity();
    w.add_component<Name>(e, Name{"hello"});
    {
        auto* n = w.get_component<Name>(e);
        REQUIRE(n != nullptr);
        CHECK(n->value == "hello");
    }
    w.destroy_entity(e);
    // No assertion possible post-destroy, but ASAN/MSAN would catch leaks.
    CHECK(w.entity_count() == 0);
}

TEST_CASE("World — hierarchy reparent + visit_descendants") {
    World w;
    const auto root = w.create_entity();
    const auto a    = w.create_entity();
    const auto b    = w.create_entity();
    const auto c    = w.create_entity();
    REQUIRE(w.reparent(a, root));
    REQUIRE(w.reparent(b, root));
    REQUIRE(w.reparent(c, a));

    std::vector<Entity> visited;
    w.visit_descendants(root, [&](Entity e) { visited.push_back(e); });
    CHECK(visited.size() == 4);   // root + a + c + b (DFS)
    CHECK(visited.front() == root);

    // Reparent c to root.
    REQUIRE(w.reparent(c, root));
    visited.clear();
    w.visit_descendants(a, [&](Entity e) { visited.push_back(e); });
    CHECK(visited.size() == 1);   // a has no children now
}

TEST_CASE("World — reparent rejects cycles") {
    World w;
    const auto p  = w.create_entity();
    const auto c1 = w.create_entity();
    const auto c2 = w.create_entity();
    REQUIRE(w.reparent(c1, p));
    REQUIRE(w.reparent(c2, c1));
    CHECK_FALSE(w.reparent(p, c2));       // would create a cycle
    CHECK_FALSE(w.reparent(p, p));        // self-reparent
}

TEST_CASE("World — 10 000-entity stress with mixed archetypes") {
    World w;
    std::vector<Entity> entities;
    entities.reserve(10'000);
    for (int i = 0; i < 10'000; ++i) {
        const auto e = w.create_entity();
        entities.push_back(e);
        w.add_component<Position>(e, {float(i), 0, 0});
        if ((i & 1) == 0) w.add_component<Velocity>(e, {0, 0, 0});
    }
    CHECK(w.entity_count() == 10'000);

    int pos_count = 0, vel_count = 0;
    w.for_each<Position>([&](Entity, Position&) { ++pos_count; });
    w.for_each<Velocity>([&](Entity, Velocity&) { ++vel_count; });
    CHECK(pos_count == 10'000);
    CHECK(vel_count == 5'000);

    // Destroy every 3rd entity; counts should shrink accordingly.
    for (std::size_t i = 0; i < entities.size(); i += 3) {
        w.destroy_entity(entities[i]);
    }
    int pos_after = 0;
    w.for_each<Position>([&](Entity, Position&) { ++pos_after; });
    const int expected = 10'000 - static_cast<int>((10'000 + 2) / 3);
    CHECK(pos_after == expected);
}
