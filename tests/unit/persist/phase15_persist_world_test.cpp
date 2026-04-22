// Phase 15 — PersistWorld integration (ADR-0056..0057).

#include <doctest/doctest.h>

#include "engine/core/config/cvar_registry.hpp"
#include "engine/core/version.generated.hpp"
#include "engine/ecs/serialize.hpp"
#include "engine/ecs/world.hpp"
#include "engine/persist/ecs_snapshot.hpp"
#include "engine/persist/fs_atomic.hpp"
#include "engine/persist/gwsave_format.hpp"
#include "engine/persist/gwsave_io.hpp"
#include "engine/persist/integrity.hpp"
#include "engine/persist/migration.hpp"
#include "engine/persist/persist_cvars.hpp"
#include "engine/persist/persist_types.hpp"
#include "engine/persist/persist_world.hpp"
#include "engine/platform/fs.hpp"

#include <cstdint>
#include <cstring>
#include <filesystem>

namespace {

struct Pos {
    float x = 0, y = 0, z = 0;
};

std::filesystem::path persist_temp(std::string_view tag) {
    auto p = std::filesystem::temp_directory_path() / "gw_phase15_pw" / tag;
    std::filesystem::remove_all(p);
    std::filesystem::create_directories(p);
    return p;
}

} // namespace

TEST_CASE("phase15 — PersistWorld forward engine refused when strict") {
    gw::config::CVarRegistry r;
    (void)gw::persist::register_persist_and_telemetry_cvars(r);

    const auto root = persist_temp("fwd");
    gw::persist::PersistConfig cfg{};
    cfg.save_dir     = root.string();
    cfg.sqlite_path  = (root / "p.db").string();

    gw::persist::PersistWorld pw;
    REQUIRE(pw.initialize(cfg, &r, nullptr, nullptr));

    gw::ecs::World w;
    (void)w.component_registry().id_of<Pos>();
    gw::persist::SaveMeta meta{};
    meta.display_name    = "t";
    meta.schema_version  = gw::persist::kCurrentSaveSchemaVersion;
    meta.session_seed    = 0x11223344ULL;
    meta.world_seed      = 1;
    meta.created_unix_ms = 1;
    REQUIRE(pw.save_slot(1, w, meta) == gw::persist::SaveError::Ok);

    auto path = root / "slot_1.gwsave";
    auto bytes = gw::platform::FileSystem::read_bytes(path);
    REQUIRE(bytes.has_value());
    std::vector<std::byte> file(bytes->size());
    std::memcpy(file.data(), bytes->data(), bytes->size());
    std::uint32_t evil = 0x00FFFFFFu;
    // magic(4) + ver(2) + flags(2) + schema(4) = 12 bytes before engine_version
    std::memcpy(file.data() + 12, &evil, sizeof(evil));
    const std::size_t body_end = file.size() - 36;
    std::memcpy(file.data() + body_end, gw::persist::gwsave::kFooterMagic, 4);
    const auto dig = gw::persist::blake3_digest_256(
        std::span<const std::byte>(file.data(), body_end + 4));
    std::memcpy(file.data() + body_end + 4, dig.data(), 32);
    REQUIRE(gw::persist::atomic_write_bytes(path, std::span<const std::byte>(file.data(), file.size())));

    gw::ecs::World w2;
    (void)w2.component_registry().id_of<Pos>();
    CHECK(pw.load_slot(1, w2) == gw::persist::LoadError::ForwardIncompatible);
}

TEST_CASE("phase15 — PersistWorld forward engine loads when strict off") {
    gw::config::CVarRegistry r;
    auto                     pc = gw::persist::register_persist_and_telemetry_cvars(r);
    REQUIRE(r.set_bool(pc.migration_strict.id, false));

    const auto root = persist_temp("fwd2");
    gw::persist::PersistConfig cfg{};
    cfg.save_dir    = root.string();
    cfg.sqlite_path = (root / "p.db").string();

    gw::persist::PersistWorld pw;
    REQUIRE(pw.initialize(cfg, &r, nullptr, nullptr));

    gw::ecs::World w;
    (void)w.component_registry().id_of<Pos>();
    gw::persist::SaveMeta meta{};
    meta.display_name    = "t";
    meta.schema_version  = gw::persist::kCurrentSaveSchemaVersion;
    meta.session_seed    = 9;
    REQUIRE(pw.save_slot(1, w, meta) == gw::persist::SaveError::Ok);

    auto path = root / "slot_1.gwsave";
    auto bytes = gw::platform::FileSystem::read_bytes(path);
    REQUIRE(bytes.has_value());
    std::vector<std::byte> file(bytes->size());
    std::memcpy(file.data(), bytes->data(), bytes->size());
    std::uint32_t evil = 0x00FFFFFFu;
    std::memcpy(file.data() + 12, &evil, sizeof(evil));
    const std::size_t body_end = file.size() - 36;
    std::memcpy(file.data() + body_end, gw::persist::gwsave::kFooterMagic, 4);
    const auto dig = gw::persist::blake3_digest_256(
        std::span<const std::byte>(file.data(), body_end + 4));
    std::memcpy(file.data() + body_end + 4, dig.data(), 32);
    REQUIRE(gw::persist::atomic_write_bytes(path, std::span<const std::byte>(file.data(), file.size())));

    gw::ecs::World w2;
    (void)w2.component_registry().id_of<Pos>();
    CHECK(pw.load_slot(1, w2) == gw::persist::LoadError::Ok);
}

TEST_CASE("phase15 — 1024 entities round-trip determinism hash") {
    gw::ecs::World w;
    (void)w.component_registry().id_of<Pos>();
    for (int i = 0; i < 1024; ++i) {
        const auto e = w.create_entity();
        w.add_component(e, Pos{static_cast<float>(i), 0.0f, 0.0f});
    }
    auto chunks = gw::persist::build_chunk_grid_demo(w);
    gw::persist::gwsave::HeaderPrefix hdr{};
    hdr.schema_version  = gw::persist::kCurrentSaveSchemaVersion;
    hdr.engine_version  = gw::persist::pack_engine_semver(GW_VERSION_MAJOR, GW_VERSION_MINOR, GW_VERSION_PATCH);
    hdr.chunk_count     = static_cast<std::uint32_t>(chunks.size());
    const auto c        = gw::persist::centre_chunk_payload(chunks);
    hdr.determinism_hash =
        gw::persist::ecs_blob_determinism_hash(std::span<const std::byte>(
            reinterpret_cast<const std::byte*>(c.data()), c.size()));
    auto file = gw::persist::write_gwsave_container(0, hdr, chunks);
    const auto peek = gw::persist::peek_gwsave(std::span<const std::byte>(file.data(), file.size()), true);
    REQUIRE(peek.err == gw::persist::LoadError::Ok);
    std::vector<gw::persist::gwsave::ChunkPayload> loaded;
    REQUIRE(gw::persist::load_gwsave_all_chunks(std::span<const std::byte>(file.data(), file.size()), peek,
                                                loaded)
            == gw::persist::LoadError::Ok);
    gw::ecs::World w2;
    (void)w2.component_registry().id_of<Pos>();
    const auto blob = gw::persist::centre_chunk_payload(loaded);
    std::vector<std::uint8_t> raw(blob.size());
    std::memcpy(raw.data(), blob.data(), blob.size());
    const auto lr = gw::ecs::load_world(w2, raw);
    REQUIRE(lr.has_value());
    CHECK(w2.entity_count() == 1024u);
    const auto det = gw::persist::ecs_blob_determinism_hash(std::span<const std::byte>(
        reinterpret_cast<const std::byte*>(blob.data()), blob.size()));
    CHECK(det == peek.hdr.determinism_hash);
}

TEST_CASE("phase15 — session_seed round-trips in gwsave header") {
    gw::ecs::World w;
    auto           chunks = gw::persist::build_chunk_grid_demo(w);
    gw::persist::gwsave::HeaderPrefix hdr{};
    hdr.schema_version  = gw::persist::kCurrentSaveSchemaVersion;
    hdr.engine_version  = 1;
    hdr.session_seed    = 0xDEADBEEFCAFEBABEULL;
    hdr.chunk_count     = static_cast<std::uint32_t>(chunks.size());
    const auto c        = gw::persist::centre_chunk_payload(chunks);
    hdr.determinism_hash =
        gw::persist::ecs_blob_determinism_hash(std::span<const std::byte>(
            reinterpret_cast<const std::byte*>(c.data()), c.size()));
    auto file = gw::persist::write_gwsave_container(0, hdr, chunks);
    const auto peek = gw::persist::peek_gwsave(std::span<const std::byte>(file.data(), file.size()), true);
    REQUIRE(peek.err == gw::persist::LoadError::Ok);
    CHECK(peek.hdr.session_seed == 0xDEADBEEFCAFEBABEULL);
}

TEST_CASE("phase15 — migrate_slot bumps schema version") {
    gw::config::CVarRegistry r;
    (void)gw::persist::register_persist_and_telemetry_cvars(r);
    const auto root = persist_temp("mig");
    gw::persist::PersistConfig cfg{};
    cfg.save_dir    = root.string();
    cfg.sqlite_path = (root / "p.db").string();
    gw::persist::PersistWorld pw;
    REQUIRE(pw.initialize(cfg, &r, nullptr, nullptr));
    gw::ecs::World w;
    (void)w.component_registry().id_of<Pos>();
    gw::persist::SaveMeta meta{};
    meta.display_name   = "m";
    meta.schema_version = 1;
    REQUIRE(pw.save_slot(2, w, meta) == gw::persist::SaveError::Ok);
    REQUIRE(pw.migrate_slot(2) == gw::persist::SaveError::Ok);
    auto bytes = gw::platform::FileSystem::read_bytes(root / "slot_2.gwsave");
    REQUIRE(bytes.has_value());
    std::vector<std::byte> file(bytes->size());
    std::memcpy(file.data(), bytes->data(), bytes->size());
    const auto peek = gw::persist::peek_gwsave(std::span<const std::byte>(file.data(), file.size()), true);
    REQUIRE(peek.err == gw::persist::LoadError::Ok);
    CHECK(peek.hdr.schema_version == gw::persist::kCurrentSaveSchemaVersion);
}

TEST_CASE("phase15 — save_migration_steps contains identity v1") {
    const auto steps = gw::persist::save_migration_steps();
    REQUIRE_FALSE(steps.empty());
    CHECK(steps[0].from_schema == 1u);
    CHECK(steps[0].to_schema == 1u);
}

#if GW_ENABLE_ZSTD

TEST_CASE("phase15 — gwsave zstd chunk round-trip") {
    gw::ecs::World w;
    (void)w.component_registry().id_of<Pos>();
    const auto e = w.create_entity();
    w.add_component(e, Pos{1.0f, 2.0f, 3.0f});
    auto chunks = gw::persist::build_chunk_grid_demo(w);
    gw::persist::gwsave::HeaderPrefix hdr{};
    hdr.schema_version  = gw::persist::kCurrentSaveSchemaVersion;
    hdr.engine_version  = gw::persist::pack_engine_semver(0, 0, 1);
    hdr.chunk_count     = static_cast<std::uint32_t>(chunks.size());
    const auto c        = gw::persist::centre_chunk_payload(chunks);
    hdr.determinism_hash =
        gw::persist::ecs_blob_determinism_hash(std::span<const std::byte>(
            reinterpret_cast<const std::byte*>(c.data()), c.size()));
    auto file =
        gw::persist::write_gwsave_container(gw::persist::gwsave::kFlagZstd, hdr, chunks);
    REQUIRE_FALSE(file.empty());
    const auto peek = gw::persist::peek_gwsave(std::span<const std::byte>(file.data(), file.size()), true);
    REQUIRE(peek.err == gw::persist::LoadError::Ok);
    CHECK((peek.hdr.flags & gw::persist::gwsave::kFlagZstd) != 0);
    std::vector<gw::persist::gwsave::ChunkPayload> loaded;
    REQUIRE(gw::persist::load_gwsave_all_chunks(std::span<const std::byte>(file.data(), file.size()), peek,
                                                loaded)
            == gw::persist::LoadError::Ok);
    gw::ecs::World w2;
    (void)w2.component_registry().id_of<Pos>();
    const auto blob = gw::persist::centre_chunk_payload(loaded);
    std::vector<std::uint8_t> raw(blob.size());
    std::memcpy(raw.data(), blob.data(), blob.size());
    const auto lr = gw::ecs::load_world(w2, raw);
    REQUIRE(lr.has_value());
    CHECK(w2.entity_count() == 1u);
}

#endif

#if !GW_ENABLE_ZSTD

TEST_CASE("phase15 — gwsave zstd flag without zstd build yields empty container") {
    gw::ecs::World w;
    auto           chunks = gw::persist::build_chunk_grid_demo(w);
    gw::persist::gwsave::HeaderPrefix hdr{};
    hdr.schema_version = gw::persist::kCurrentSaveSchemaVersion;
    hdr.engine_version = 1;
    hdr.chunk_count    = static_cast<std::uint32_t>(chunks.size());
    const auto c       = gw::persist::centre_chunk_payload(chunks);
    hdr.determinism_hash =
        gw::persist::ecs_blob_determinism_hash(std::span<const std::byte>(
            reinterpret_cast<const std::byte*>(c.data()), c.size()));
    auto file =
        gw::persist::write_gwsave_container(gw::persist::gwsave::kFlagZstd, hdr, chunks);
    CHECK(file.empty());
}

#endif
