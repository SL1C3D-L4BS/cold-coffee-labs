// tests/unit/scene/transform_system_test.cpp
// Phase 8 week 044 — dvec3 TransformComponent + hierarchical transform system.

#include <doctest/doctest.h>

#include "editor/scene/components.hpp"
#include "editor/scene/transform_system.hpp"

#include "engine/core/serialization.hpp"
#include "engine/ecs/hierarchy.hpp"
#include "engine/ecs/serialize.hpp"
#include "engine/scene/migration.hpp"

#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

using gw::ecs::Entity;
using gw::ecs::HierarchyComponent;
using gw::ecs::World;
using gw::editor::scene::TransformComponent;
using gw::editor::scene::WorldMatrixComponent;

TEST_CASE("TransformComponent — position is float64 (dvec3)") {
    // CLAUDE.md non-negotiable #17: world-space positions use float64.
    // The layout test is load-bearing for the v1→v2 migration.
    static_assert(std::is_same_v<decltype(TransformComponent::position), glm::dvec3>);
    CHECK(sizeof(TransformComponent::position) == 24);
    // dvec3 (24, align 8) + quat (16, align 4) + vec3 (12, align 4) = 52 logical
    // bytes, rounded up to 56 by the 8-byte struct alignment.
    CHECK(sizeof(TransformComponent)           == 56);
}

TEST_CASE("transform_system — flat world writes WorldMatrixComponent per entity") {
    World w;
    const auto a = w.create_entity();
    const auto b = w.create_entity();
    w.add_component(a, TransformComponent{glm::dvec3{10.0, 0.0, 0.0},
                                           glm::quat{1.f, 0.f, 0.f, 0.f},
                                           glm::vec3{1.f}});
    w.add_component(b, TransformComponent{glm::dvec3{0.0, 5.0, 0.0},
                                           glm::quat{1.f, 0.f, 0.f, 0.f},
                                           glm::vec3{1.f}});

    const auto touched = gw::editor::scene::update_transforms(w);
    CHECK(touched == 2);

    const auto* wa = w.get_component<WorldMatrixComponent>(a);
    const auto* wb = w.get_component<WorldMatrixComponent>(b);
    REQUIRE(wa != nullptr);
    REQUIRE(wb != nullptr);
    CHECK(wa->world[3].x == doctest::Approx(10.0));
    CHECK(wb->world[3].y == doctest::Approx(5.0));
    CHECK(wa->dirty == 0);
}

TEST_CASE("transform_system — child world matrix is parent * local") {
    World w;
    const auto root  = w.create_entity();
    const auto child = w.create_entity();
    w.add_component(root,  TransformComponent{glm::dvec3{100.0, 0.0, 0.0}});
    w.add_component(child, TransformComponent{glm::dvec3{3.0, 0.0, 0.0}});
    REQUIRE(w.reparent(child, root));

    CHECK(gw::editor::scene::update_transforms(w) == 2);

    const auto* wc = w.get_component<WorldMatrixComponent>(child);
    REQUIRE(wc != nullptr);
    // Translate-only parent → child world origin is parent.pos + child.pos.
    CHECK(wc->world[3].x == doctest::Approx(103.0));
}

TEST_CASE("transform_system — child with Transform-only parent in hierarchy uses local as world") {
    // Parent has `HierarchyComponent` and reparents the child, but has no
    // `TransformComponent` — the transform pass cannot walk through it, so
    // the child is ordered as a root and only local is applied to world.
    World            w;
    const auto parent = w.create_entity();
    const auto child  = w.create_entity();
    w.add_component(parent, TransformComponent{});
    w.add_component(child, TransformComponent{glm::dvec3{2.0, 0.0, 0.0}});
    REQUIRE(w.reparent(child, parent));
    w.remove_component<TransformComponent>(parent);
    // Child still has parent; parent is not in the transform `records` set.
    CHECK(gw::editor::scene::update_transforms(w) == 1);
    const auto* wc = w.get_component<WorldMatrixComponent>(child);
    REQUIRE(wc != nullptr);
    CHECK(wc->world[3].x == doctest::Approx(2.0));
}

TEST_CASE("transform_system — grandchild composes through two parents") {
    World w;
    const auto a = w.create_entity();
    const auto b = w.create_entity();
    const auto c = w.create_entity();
    w.add_component(a, TransformComponent{glm::dvec3{1.0,  0.0, 0.0}});
    w.add_component(b, TransformComponent{glm::dvec3{0.0, 10.0, 0.0}});
    w.add_component(c, TransformComponent{glm::dvec3{0.0,  0.0, 100.0}});
    REQUIRE(w.reparent(b, a));
    REQUIRE(w.reparent(c, b));

    CHECK(gw::editor::scene::update_transforms(w) == 3);

    const auto* wc = w.get_component<WorldMatrixComponent>(c);
    REQUIRE(wc != nullptr);
    CHECK(wc->world[3].x == doctest::Approx(1.0));
    CHECK(wc->world[3].y == doctest::Approx(10.0));
    CHECK(wc->world[3].z == doctest::Approx(100.0));
}

TEST_CASE("transform_system — float64 positions stay precise at planet scale") {
    // A real Earth-radius offset (6.378 million metres). With float32 we would
    // lose sub-millimetre precision by a large margin; with float64 we
    // recover the child offset exactly.
    constexpr double kEarthRadius = 6'378'137.0;
    World w;
    const auto root  = w.create_entity();
    const auto child = w.create_entity();
    w.add_component(root,  TransformComponent{glm::dvec3{kEarthRadius, 0.0, 0.0}});
    w.add_component(child, TransformComponent{glm::dvec3{0.001, 0.0, 0.0}});
    REQUIRE(w.reparent(child, root));
    REQUIRE(gw::editor::scene::update_transforms(w) == 2);

    const auto* wc = w.get_component<WorldMatrixComponent>(child);
    REQUIRE(wc != nullptr);
    // Sub-millimetre precision preserved under float64 composition.
    CHECK(wc->world[3].x == doctest::Approx(kEarthRadius + 0.001).epsilon(1e-9));
}

namespace {

// Encode a TransformComponent v1 (legacy vec3-position, 40-byte) payload and
// wrap it in a valid GWS1 header. Returns the full blob. This is how we
// simulate "a .gwscene saved before Wave 2".
std::vector<std::uint8_t> make_v1_scene_with_transform(
    std::uint64_t transform_stable_hash,
    const glm::vec3& pos, const glm::quat& rot, const glm::vec3& scl) {

    std::vector<std::uint8_t> payload;
    auto u8  = [&](std::uint8_t v) { payload.push_back(v); };
    auto u16 = [&](std::uint16_t v){ u8(v & 0xFF); u8((v >> 8) & 0xFF); };
    auto u32 = [&](std::uint32_t v){
        for (int i = 0; i < 4; ++i) u8(static_cast<std::uint8_t>(v >> (i*8)));
    };
    auto u64 = [&](std::uint64_t v){
        for (int i = 0; i < 8; ++i) u8(static_cast<std::uint8_t>(v >> (i*8)));
    };
    auto bytes = [&](const void* p, std::size_t n) {
        const auto* b = static_cast<const std::uint8_t*>(p);
        payload.insert(payload.end(), b, b + n);
    };

    u32(1);              // entity_count
    u16(1);              // component_type_count
    // ComponentTable[0]: local_id=0, stable_hash, size=40 (v1), trivial=1
    u16(0);
    u64(transform_stable_hash);
    u32(40);             // v1 size: vec3 + quat + vec3 = 12+16+12
    u8(1);
    u32(18); for (char c : std::string_view{"TransformComponent"}) u8(static_cast<std::uint8_t>(c));
    // Entity record
    u64((std::uint64_t{1} << 32) | 0);  // gen=1, idx=0
    u16(1);               // component_count
    u16(0);               // local_id
    bytes(&pos, sizeof(pos));
    bytes(&rot, sizeof(rot));
    bytes(&scl, sizeof(scl));

    std::vector<std::uint8_t> blob;
    auto push = [&](auto v) {
        const auto* p = reinterpret_cast<const std::uint8_t*>(&v);
        blob.insert(blob.end(), p, p + sizeof(v));
    };
    push(std::uint32_t{0x31535747u});                 // 'GWS1'
    push(std::uint16_t{1});                           // version
    push(std::uint16_t{0});                           // flags
    push(static_cast<std::uint64_t>(payload.size()));
    push(gw::core::crc64_iso(payload));
    blob.insert(blob.end(), payload.begin(), payload.end());
    return blob;
}

// Sibling of editor_bld_api.cpp::register_editor_migrations_once — duplicated
// here because the test binary does not link the editor BLD API TU (it would
// pull in the full editor globals object with ImGui/volk deps). The logic is
// identical — if it drifts, the migration won't round-trip, which this test
// will catch.
void register_transform_migration(gw::scene::MigrationRegistry& reg) {
    constexpr std::uint32_t kV1Size = 12u + 16u + 12u;
    reg.register_fn<TransformComponent>(
        kV1Size,
        [](std::span<const std::uint8_t> src,
           std::span<std::uint8_t>       dst) {
            if (src.size() != kV1Size ||
                dst.size() != sizeof(TransformComponent)) return false;
            float pos_f[3];
            std::memcpy(pos_f, src.data(), sizeof(pos_f));
            TransformComponent t{};
            t.position = glm::dvec3(pos_f[0], pos_f[1], pos_f[2]);
            std::memcpy(&t.rotation, src.data() + 12, sizeof(t.rotation));
            std::memcpy(&t.scale,    src.data() + 12 + 16, sizeof(t.scale));
            std::memcpy(dst.data(), &t, sizeof(t));
            return true;
        },
        "TransformComponent: widen position vec3->dvec3");
}

} // namespace

TEST_CASE("migration — v1 vec3 TransformComponent loads into v2 dvec3 world") {
    // Register the migration on a test-scoped registry so we don't contaminate
    // the process-wide global that other suites read.
    gw::scene::MigrationRegistry reg;
    register_transform_migration(reg);
    REQUIRE(reg.size() == 1);

    const auto info = gw::ecs::detail::make_info_for<TransformComponent>();

    const auto blob = make_v1_scene_with_transform(
        info.stable_hash,
        glm::vec3{100.f, -50.f, 12.5f},
        glm::quat{1.f, 0.f, 0.f, 0.f},
        glm::vec3{1.f, 2.f, 3.f});

    World dst;
    (void)dst.component_registry().id_of<TransformComponent>();
    gw::ecs::LoadOptions opts;
    opts.migrate = [&reg](std::uint64_t h, std::uint32_t from,
                          std::uint32_t to,
                          std::span<const std::uint8_t> src)
        -> std::optional<std::vector<std::uint8_t>> {
        const auto* entry = reg.find(h, from);
        if (!entry || entry->to_size != to) return std::nullopt;
        std::vector<std::uint8_t> out(to, std::uint8_t{0});
        if (!entry->fn(src, out)) return std::nullopt;
        return out;
    };

    auto r = gw::ecs::load_world(dst, blob, opts);
    REQUIRE(r.has_value());

    std::size_t seen = 0;
    TransformComponent captured{};
    dst.for_each<TransformComponent>([&](Entity, TransformComponent& t) {
        ++seen;
        captured = t;
    });
    CHECK(seen == 1);
    CHECK(captured.position.x == doctest::Approx(100.0));
    CHECK(captured.position.y == doctest::Approx(-50.0));
    CHECK(captured.position.z == doctest::Approx(12.5));
    CHECK(captured.scale.y    == doctest::Approx(2.f));
}
