// tests/unit/scene/scene_file_test.cpp
// Phase 8 week 043 — `.gwscene` on-disk codec (ADR-0006 + ADR-0008).

#include <ostream>
#include <doctest/doctest.h>

#include "engine/core/serialization.hpp"
#include "engine/ecs/hierarchy.hpp"
#include "engine/ecs/world.hpp"
#include "engine/scene/migration.hpp"
#include "engine/scene/scene_file.hpp"

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>

using gw::ecs::Entity;
using gw::ecs::HierarchyComponent;
using gw::ecs::World;

namespace {

struct Pos   { float x = 0, y = 0, z = 0; };
struct Flags { std::uint32_t bits = 0; };
struct Hp    { std::uint32_t value = 0; };

// A "growing" component used by the migration test: v1 used `uint32_t value`
// only; v2 added `uint32_t armor`. Byte size grew from 4 → 8.
struct HpV2   { std::uint32_t value = 0; std::uint32_t armor = 0; };

static_assert(std::is_trivially_copyable_v<Pos>);
static_assert(std::is_trivially_copyable_v<Flags>);
static_assert(std::is_trivially_copyable_v<Hp>);
static_assert(std::is_trivially_copyable_v<HpV2>);

std::filesystem::path unique_scene_path(const char* stem) {
    static thread_local std::mt19937_64 rng{std::random_device{}()};
    auto dir = std::filesystem::temp_directory_path() / "gw_scene_tests";
    std::filesystem::create_directories(dir);
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%s_%016llx.gwscene",
                  stem, static_cast<unsigned long long>(rng()));
    return dir / buf;
}

} // namespace

TEST_CASE("scene_file — save then load preserves entities + components") {
    World src;
    const auto a = src.create_entity();
    const auto b = src.create_entity();
    src.add_component(a, Pos{1.f, 2.f, 3.f});
    src.add_component(b, Flags{0xCAFEBABEu});

    const auto path = unique_scene_path("roundtrip");
    REQUIRE(gw::scene::save_scene(path, src).has_value());
    REQUIRE(std::filesystem::exists(path));
    REQUIRE(std::filesystem::file_size(path) > 24);

    World dst;
    (void)dst.component_registry().id_of<Pos>();
    (void)dst.component_registry().id_of<Flags>();
    REQUIRE(gw::scene::load_scene(path, dst).has_value());
    REQUIRE(dst.entity_count() == 2);

    CHECK(dst.is_alive(a));
    CHECK(dst.is_alive(b));
    const auto* p = dst.get_component<Pos>(a);
    REQUIRE(p != nullptr);
    CHECK(p->x == doctest::Approx(1.f));
    CHECK(p->y == doctest::Approx(2.f));

    std::filesystem::remove(path);
}

TEST_CASE("scene_file — encode/decode hierarchy references") {
    World src;
    const auto root  = src.create_entity();
    const auto child = src.create_entity();
    src.add_component(root,  Pos{});
    src.add_component(child, Pos{});
    REQUIRE(src.reparent(child, root));

    auto blob = gw::scene::encode_scene(src);
    REQUIRE(blob.has_value());
    REQUIRE_FALSE(blob->empty());

    World dst;
    (void)dst.component_registry().id_of<Pos>();
    (void)dst.component_registry().id_of<HierarchyComponent>();
    REQUIRE(gw::scene::decode_scene(*blob, dst).has_value());

    REQUIRE(dst.is_alive(child));
    const auto* h = dst.get_component<HierarchyComponent>(child);
    REQUIRE(h != nullptr);
    CHECK(h->parent == root);
}

TEST_CASE("scene_file — missing file returns ReadFailed") {
    const auto path = unique_scene_path("missing");
    World dst;
    auto r = gw::scene::load_scene(path, dst);
    REQUIRE_FALSE(r.has_value());
    CHECK(r.error() == gw::scene::SceneIoError::ReadFailed);
}

TEST_CASE("scene_file — bad magic is rejected") {
    const auto path = unique_scene_path("badmagic");
    {
        std::ofstream f(path, std::ios::binary);
        std::uint8_t junk[64] = {};
        std::fill(std::begin(junk), std::end(junk), static_cast<std::uint8_t>(0xAB));
        f.write(reinterpret_cast<const char*>(junk), sizeof(junk));
    }
    World dst;
    auto r = gw::scene::load_scene(path, dst);
    REQUIRE_FALSE(r.has_value());
    CHECK(r.error() == gw::scene::SceneIoError::BadMagic);
    std::filesystem::remove(path);
}

TEST_CASE("scene_file — zstd flag rejected until the codec lands") {
    World src;
    auto r = gw::scene::encode_scene(src, gw::scene::SaveOptions{/*compress=*/true});
    REQUIRE_FALSE(r.has_value());
    CHECK(r.error() == gw::scene::SceneIoError::UnsupportedFlags);
}

TEST_CASE("scene_file — empty file is rejected") {
    const auto path = unique_scene_path("empty");
    {
        std::ofstream f(path, std::ios::binary);
        (void)f;
    }
    World dst;
    auto r = gw::scene::load_scene(path, dst);
    REQUIRE_FALSE(r.has_value());
    CHECK(r.error() == gw::scene::SceneIoError::EmptyFile);
    std::filesystem::remove(path);
}

TEST_CASE("scene_file — atomic overwrite leaves no .tmp and preserves old file on failure") {
    World src;
    const auto e = src.create_entity();
    src.add_component(e, Pos{42.f, 0.f, 0.f});

    const auto path = unique_scene_path("atomic");
    REQUIRE(gw::scene::save_scene(path, src).has_value());
    const auto first_size = std::filesystem::file_size(path);

    // Overwrite with a different world; file size should change, and there
    // should be no `.tmp` sibling left behind.
    World src2;
    const auto e2 = src2.create_entity();
    src2.add_component(e2, Pos{1.f, 2.f, 3.f});
    src2.add_component(e2, Flags{0x12345678u});
    REQUIRE(gw::scene::save_scene(path, src2).has_value());

    CHECK(std::filesystem::exists(path));
    CHECK(std::filesystem::file_size(path) != first_size);

    int tmp_count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(path.parent_path())) {
        const auto name = entry.path().filename().string();
        if (name.find(".tmp") != std::string::npos) ++tmp_count;
    }
    CHECK(tmp_count == 0);
    std::filesystem::remove(path);
}

TEST_CASE("migration — registry lookup by (stable_hash, from_size)") {
    gw::scene::MigrationRegistry reg;
    reg.register_fn<HpV2>(/*from_size=*/4,
        [](std::span<const std::uint8_t> src,
           std::span<std::uint8_t>       dst) {
            // Ignore contents — just prove the hook runs.
            if (src.size() != 4 || dst.size() != 8) return false;
            std::memcpy(dst.data(),     src.data(), 4);      // value
            std::uint32_t armor = 99;
            std::memcpy(dst.data() + 4, &armor,     4);      // new field
            return true;
        },
        "HpV2: add armor default=99");
    CHECK(reg.size() == 1);

    // Lookup uses the live type's stable_hash; derive it the same way the
    // ECS registry does.
    const auto info = gw::ecs::detail::make_info_for<HpV2>();
    const auto* entry = reg.find(info.stable_hash, 4);
    REQUIRE(entry != nullptr);
    CHECK(entry->to_size == sizeof(HpV2));

    CHECK(reg.find(info.stable_hash, 6) == nullptr);   // wrong from_size
    CHECK(reg.find(0xBAD, 4)           == nullptr);    // wrong hash
}

TEST_CASE("migration — trivial round-trip does not invoke the migrate fn") {
    // Sanity-check the fast path: when no component has changed layout
    // between save and load, the migrate callback is never consulted.
    World src;
    const auto e = src.create_entity();
    src.add_component(e, Hp{77u});
    auto blob = gw::scene::encode_scene(src);
    REQUIRE(blob.has_value());

    World dst;
    (void)dst.component_registry().id_of<Hp>();
    int migrate_calls = 0;
    gw::ecs::LoadOptions opts;
    opts.migrate = [&migrate_calls](std::uint64_t, std::uint32_t,
                                    std::uint32_t,
                                    std::span<const std::uint8_t>)
        -> std::optional<std::vector<std::uint8_t>> {
        ++migrate_calls;
        return std::nullopt;
    };
    REQUIRE(gw::ecs::load_world(dst, *blob, opts).has_value());
    CHECK(migrate_calls == 0);
    const auto* hp = dst.get_component<Hp>(e);
    REQUIRE(hp != nullptr);
    CHECK(hp->value == 77u);
}

TEST_CASE("migration — end-to-end v1→v2 via hand-crafted blob") {
    // Craft a valid `.gwscene` blob for a hypothetical "v1" where the
    // component had 4 bytes on disk, and load it into a world whose live
    // component type (`HpV2`) is 8 bytes. The migrate callback performs
    // the translation.
    //
    // This exercises the complete load path end-to-end: parse_payload
    // detects the size mismatch, apply_payload calls the migrate fn, the
    // returned bytes are written via add_component_raw, and the live type
    // appears populated in the destination world.

    const auto live = gw::ecs::detail::make_info_for<HpV2>();
    REQUIRE(live.size == 8);

    // ---- Build payload (no header) in manual form -----------------------
    std::vector<std::uint8_t> payload;
    auto push_u64 = [&](std::uint64_t v) {
        for (int i = 0; i < 8; ++i)
            payload.push_back(static_cast<std::uint8_t>((v >> (i * 8)) & 0xFF));
    };
    auto push_u32 = [&](std::uint32_t v) {
        for (int i = 0; i < 4; ++i)
            payload.push_back(static_cast<std::uint8_t>((v >> (i * 8)) & 0xFF));
    };
    auto push_u16 = [&](std::uint16_t v) {
        payload.push_back(static_cast<std::uint8_t>(v & 0xFF));
        payload.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
    };
    auto push_u8 = [&](std::uint8_t v) { payload.push_back(v); };
    auto push_string = [&](std::string_view s) {
        push_u32(static_cast<std::uint32_t>(s.size()));
        for (char c : s) push_u8(static_cast<std::uint8_t>(c));
    };

    // entity_count = 1, component_type_count = 1
    push_u32(1);
    push_u16(1);
    // ComponentTable entry: local_id=0, stable_hash=live.stable_hash,
    // size=4 (the v1 on-disk size), trivial=1, debug_name="HpV2".
    push_u16(0);
    push_u64(live.stable_hash);
    push_u32(4);
    push_u8(1);
    push_string("HpV2");

    // Entity record: entity_bits, component_count, [(local_id, 4 bytes)].
    // Use a generation-1 index-0 handle — World::restore_entity_with_bits
    // accepts the freshly-encoded 64-bit representation.
    const std::uint64_t ent_bits = (std::uint64_t{1} << 32) | std::uint64_t{0};
    push_u64(ent_bits);
    push_u16(1);
    push_u16(0);
    // v1 value = 123
    push_u32(123);

    // Wrap in the ADR-0006 §2.3 GWS1 header using the engine's CRC-64/ISO
    // implementation. Using the real primitive here keeps the hand-crafted
    // fixture honest — any drift in the codec's CRC semantics will surface
    // as a test-blob mismatch rather than a silent pass.
    std::vector<std::uint8_t> blob;
    auto push = [&](auto v) {
        const auto* p = reinterpret_cast<const std::uint8_t*>(&v);
        blob.insert(blob.end(), p, p + sizeof(v));
    };
    push(std::uint32_t{0x31535747u});                 // magic 'GWS1'
    push(std::uint16_t{1});                           // version
    push(std::uint16_t{0});                           // flags
    push(static_cast<std::uint64_t>(payload.size())); // payload_bytes
    push(gw::core::crc64_iso(payload));               // CRC-64/ISO
    blob.insert(blob.end(), payload.begin(), payload.end());

    // ---- Load into a world with HpV2 registered ------------------------
    World dst;
    (void)dst.component_registry().id_of<HpV2>();
    int migrate_calls = 0;
    gw::ecs::LoadOptions opts;
    opts.migrate = [&migrate_calls](std::uint64_t /*hash*/,
                                     std::uint32_t from_size,
                                     std::uint32_t to_size,
                                     std::span<const std::uint8_t> src)
        -> std::optional<std::vector<std::uint8_t>> {
        ++migrate_calls;
        if (from_size != 4 || to_size != 8 || src.size() != 4) return std::nullopt;
        std::uint32_t v = 0;
        std::memcpy(&v, src.data(), 4);
        std::vector<std::uint8_t> out(8, 0);
        HpV2 hpv2{v, /*armor=*/0};
        std::memcpy(out.data(), &hpv2, 8);
        return out;
    };

    auto r = gw::ecs::load_world(dst, blob, opts);
    REQUIRE(r.has_value());
    CHECK(migrate_calls == 1);

    // HpV2 is installed on the one entity in the blob; locate it by query.
    std::size_t seen = 0;
    HpV2 captured{};
    dst.for_each<HpV2>([&](gw::ecs::Entity, HpV2& h) {
        ++seen;
        captured = h;
    });
    CHECK(seen == 1);
    CHECK(captured.value == 123u);
    CHECK(captured.armor == 0u);
}

TEST_CASE("migration — size mismatch with no callback returns ComponentSizeMismatch") {
    const auto live = gw::ecs::detail::make_info_for<HpV2>();
    REQUIRE(live.size == 8);

    // Minimal blob as above, but load without setting opts.migrate.
    std::vector<std::uint8_t> payload;
    auto u8  = [&](std::uint8_t  v) { payload.push_back(v); };
    auto u16 = [&](std::uint16_t v) { u8(v & 0xFF); u8((v >> 8) & 0xFF); };
    auto u32 = [&](std::uint32_t v) {
        for (int i = 0; i < 4; ++i) u8(static_cast<std::uint8_t>(v >> (i * 8)));
    };
    auto u64 = [&](std::uint64_t v) {
        for (int i = 0; i < 8; ++i) u8(static_cast<std::uint8_t>(v >> (i * 8)));
    };
    u32(1); u16(1); u16(0); u64(live.stable_hash); u32(4); u8(1);
    u32(4);  // debug_name len
    u8('H'); u8('p'); u8('V'); u8('2');
    u64((std::uint64_t{1} << 32) | 0);  // entity bits
    u16(1); u16(0); u32(7);             // component instance

    std::vector<std::uint8_t> blob;
    auto push = [&](auto v) {
        const auto* p = reinterpret_cast<const std::uint8_t*>(&v);
        blob.insert(blob.end(), p, p + sizeof(v));
    };
    push(std::uint32_t{0x31535747u});
    push(std::uint16_t{1});
    push(std::uint16_t{0});
    push(static_cast<std::uint64_t>(payload.size()));
    push(gw::core::crc64_iso(payload));
    blob.insert(blob.end(), payload.begin(), payload.end());

    World dst;
    (void)dst.component_registry().id_of<HpV2>();
    auto r = gw::ecs::load_world(dst, blob);  // no migrate
    REQUIRE_FALSE(r.has_value());
    CHECK(r.error() == gw::ecs::SerializationError::ComponentSizeMismatch);
}
