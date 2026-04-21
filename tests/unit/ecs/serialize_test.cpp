// tests/unit/ecs/serialize_test.cpp
// ADR-0006 ECS serialization — round-trip correctness, CRC, and error paths.

#include <ostream>
#include <doctest/doctest.h>

#include "engine/ecs/hierarchy.hpp"
#include "engine/ecs/serialize.hpp"
#include "engine/ecs/world.hpp"

#include <cstdint>
#include <vector>

using gw::ecs::Entity;
using gw::ecs::HierarchyComponent;
using gw::ecs::LoadOptions;
using gw::ecs::load_entity;
using gw::ecs::load_world;
using gw::ecs::save_entity;
using gw::ecs::save_world;
using gw::ecs::SerializationError;
using gw::ecs::SnapshotMode;
using gw::ecs::World;

namespace {

struct Pos    { float x = 0, y = 0, z = 0; };
struct Vel    { float dx = 0, dy = 0, dz = 0; };
struct Flags  { std::uint32_t bits = 0; };

static_assert(std::is_trivially_copyable_v<Pos>);
static_assert(std::is_trivially_copyable_v<Vel>);
static_assert(std::is_trivially_copyable_v<Flags>);

} // namespace

TEST_CASE("serialize — PieSnapshot round-trip preserves entity count and components") {
    World src;
    const auto a = src.create_entity();
    const auto b = src.create_entity();
    const auto c = src.create_entity();
    src.add_component(a, Pos{1.f, 2.f, 3.f});
    src.add_component(a, Vel{0.1f, 0.2f, 0.3f});
    src.add_component(b, Pos{10.f, 20.f, 30.f});
    src.add_component(c, Flags{0xDEADBEEFu});

    auto blob = save_world(src, SnapshotMode::PieSnapshot);
    REQUIRE_FALSE(blob.empty());

    World dst;
    // Ensure dst's ComponentRegistry knows about the three component types
    // — load_world requires the live registry to resolve stable_hash values.
    (void)dst.component_registry().id_of<Pos>();
    (void)dst.component_registry().id_of<Vel>();
    (void)dst.component_registry().id_of<Flags>();

    auto r = load_world(dst, blob);
    REQUIRE(r.has_value());
    REQUIRE(dst.entity_count() == 3);

    std::size_t pos_seen = 0, vel_seen = 0, flags_seen = 0;
    dst.for_each<Pos>([&](Entity, Pos&) { ++pos_seen; });
    dst.for_each<Vel>([&](Entity, Vel&) { ++vel_seen; });
    dst.for_each<Flags>([&](Entity, Flags&) { ++flags_seen; });
    CHECK(pos_seen   == 2);
    CHECK(vel_seen   == 1);
    CHECK(flags_seen == 1);

    // Value check — entity handles are preserved (reuse_bits path) so the
    // original handle still resolves.
    if (dst.is_alive(a)) {
        const auto* p = dst.get_component<Pos>(a);
        REQUIRE(p != nullptr);
        CHECK(p->x == doctest::Approx(1.f));
        CHECK(p->y == doctest::Approx(2.f));
        CHECK(p->z == doctest::Approx(3.f));
    }
    if (dst.is_alive(c)) {
        const auto* f = dst.get_component<Flags>(c);
        REQUIRE(f != nullptr);
        CHECK(f->bits == 0xDEADBEEFu);
    }
}

TEST_CASE("serialize — hierarchy references survive round-trip") {
    World src;
    const auto parent = src.create_entity();
    const auto child  = src.create_entity();
    src.add_component(parent, Pos{});
    src.add_component(child,  Pos{});
    REQUIRE(src.reparent(child, parent));

    auto blob = save_world(src, SnapshotMode::PieSnapshot);
    REQUIRE_FALSE(blob.empty());

    World dst;
    (void)dst.component_registry().id_of<Pos>();
    (void)dst.component_registry().id_of<HierarchyComponent>();

    REQUIRE(load_world(dst, blob).has_value());
    REQUIRE(dst.is_alive(child));
    const auto* ch = dst.get_component<HierarchyComponent>(child);
    REQUIRE(ch != nullptr);
    CHECK(ch->parent == parent);  // handle bits reused → external ref intact
}

TEST_CASE("serialize — EntitySnapshot / load_entity reconstructs one entity") {
    World src;
    const auto e = src.create_entity();
    src.add_component(e, Pos{7.f, 8.f, 9.f});
    src.add_component(e, Flags{0xAAAAAAAAu});

    auto blob = save_entity(src, e);
    REQUIRE_FALSE(blob.empty());

    World dst;
    (void)dst.component_registry().id_of<Pos>();
    (void)dst.component_registry().id_of<Flags>();

    auto r = load_entity(dst, blob);
    REQUIRE(r.has_value());
    REQUIRE(dst.is_alive(*r));

    const auto* p = dst.get_component<Pos>(*r);
    const auto* f = dst.get_component<Flags>(*r);
    REQUIRE(p != nullptr);
    REQUIRE(f != nullptr);
    CHECK(p->x == doctest::Approx(7.f));
    CHECK(f->bits == 0xAAAAAAAAu);
}

TEST_CASE("serialize — bad magic is rejected") {
    std::vector<std::uint8_t> junk(64, 0xFF);
    World w;
    auto r = load_world(w, junk);
    REQUIRE_FALSE(r.has_value());
    CHECK(r.error() == SerializationError::BadMagic);
}

TEST_CASE("serialize — CRC mismatch is detected") {
    World src;
    const auto e = src.create_entity();
    src.add_component(e, Pos{1.f, 2.f, 3.f});

    auto blob = save_world(src, SnapshotMode::PieSnapshot);
    REQUIRE(blob.size() > 32);

    // Flip a byte in the payload (past the 24-byte header).
    blob[40] ^= 0xFFu;

    World dst;
    (void)dst.component_registry().id_of<Pos>();
    auto r = load_world(dst, blob);
    REQUIRE_FALSE(r.has_value());
    CHECK(r.error() == SerializationError::CrcMismatch);
}

TEST_CASE("serialize — CRC verification can be disabled via LoadOptions") {
    World src;
    const auto e = src.create_entity();
    src.add_component(e, Pos{});
    auto blob = save_world(src, SnapshotMode::PieSnapshot);
    REQUIRE(blob.size() > 32);
    blob[40] ^= 0xFFu;  // corrupt a component byte

    World dst;
    (void)dst.component_registry().id_of<Pos>();

    LoadOptions opts;
    opts.verify_crc = false;
    // The load either succeeds (with corrupted values) or fails with a
    // structural error — crucially NOT CrcMismatch when verification is off.
    auto r = load_world(dst, blob, opts);
    if (!r) {
        CHECK(r.error() != SerializationError::CrcMismatch);
    }
}

TEST_CASE("serialize — truncated blob returns Truncated error") {
    World src;
    const auto e = src.create_entity();
    src.add_component(e, Pos{});
    auto blob = save_world(src, SnapshotMode::PieSnapshot);
    REQUIRE(blob.size() > 8);
    blob.resize(10);  // cut before the payload

    World dst;
    (void)dst.component_registry().id_of<Pos>();
    auto r = load_world(dst, blob);
    REQUIRE_FALSE(r.has_value());
    CHECK(r.error() == SerializationError::Truncated);
}

TEST_CASE("serialize — empty world round-trips") {
    World src;
    auto blob = save_world(src, SnapshotMode::PieSnapshot);
    REQUIRE_FALSE(blob.empty());

    World dst;
    REQUIRE(load_world(dst, blob).has_value());
    CHECK(dst.entity_count() == 0);
}
