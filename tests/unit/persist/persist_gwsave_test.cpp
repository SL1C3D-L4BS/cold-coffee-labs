// tests/unit/persist/persist_gwsave_test.cpp — Phase 15 (ADR-0056..0057).

#include <doctest/doctest.h>

#include "engine/core/version.generated.hpp"
#include "engine/ecs/serialize.hpp"
#include "engine/ecs/world.hpp"
#include "engine/persist/ecs_snapshot.hpp"
#include "engine/persist/gwsave_io.hpp"
#include "engine/persist/integrity.hpp"
#include "engine/persist/migration.hpp"
#include "engine/persist/persist_types.hpp"

#include <cstring>
#include <vector>

using gw::ecs::Entity;
using gw::ecs::World;

namespace {

struct Pos {
    float x = 0, y = 0, z = 0;
};

} // namespace

TEST_CASE("persist — gwsave round-trip zero entities") {
    World w;
    auto chunks = gw::persist::build_chunk_grid_demo(w);
    REQUIRE(chunks.size() == 9u);

    gw::persist::gwsave::HeaderPrefix hdr{};
    hdr.schema_version   = gw::persist::kCurrentSaveSchemaVersion;
    hdr.engine_version   = gw::persist::pack_engine_semver(GW_VERSION_MAJOR, GW_VERSION_MINOR, GW_VERSION_PATCH);
    hdr.world_seed       = 1;
    hdr.session_seed     = 2;
    hdr.created_unix_ms  = 3;
    hdr.anchor_x = hdr.anchor_y = hdr.anchor_z = 0.0;
    const auto centre = gw::persist::centre_chunk_payload(chunks);
    hdr.determinism_hash = gw::persist::ecs_blob_determinism_hash(std::span<const std::byte>(
        reinterpret_cast<const std::byte*>(centre.data()), centre.size()));
    hdr.chunk_count      = static_cast<std::uint32_t>(chunks.size());

    auto file = gw::persist::write_gwsave_container(0, hdr, chunks);
    REQUIRE(file.size() > 64u);

    const auto peek = gw::persist::peek_gwsave(std::span<const std::byte>(file.data(), file.size()), true);
    REQUIRE(peek.err == gw::persist::LoadError::Ok);
    CHECK(peek.hdr.determinism_hash == hdr.determinism_hash);

    World w2;
    (void)w2.component_registry().id_of<Pos>();
    std::vector<gw::persist::gwsave::ChunkPayload> loaded;
    REQUIRE(gw::persist::load_gwsave_all_chunks(std::span<const std::byte>(file.data(), file.size()), peek,
                                                loaded)
            == gw::persist::LoadError::Ok);
    const auto blob = gw::persist::centre_chunk_payload(loaded);
    std::vector<std::uint8_t> raw(blob.size());
    std::memcpy(raw.data(), blob.data(), blob.size());
    const auto lr = gw::ecs::load_world(w2, raw);
    REQUIRE(lr.has_value());
    CHECK(w2.entity_count() == 0);
}

TEST_CASE("persist — header peek without loading chunks") {
    World w;
    auto chunks = gw::persist::build_chunk_grid_demo(w);
    gw::persist::gwsave::HeaderPrefix hdr{};
    hdr.schema_version  = gw::persist::kCurrentSaveSchemaVersion;
    hdr.engine_version  = gw::persist::pack_engine_semver(0, 0, 1);
    hdr.chunk_count     = static_cast<std::uint32_t>(chunks.size());
    const auto c        = gw::persist::centre_chunk_payload(chunks);
    hdr.determinism_hash =
        gw::persist::ecs_blob_determinism_hash(std::span<const std::byte>(
            reinterpret_cast<const std::byte*>(c.data()), c.size()));
    auto file = gw::persist::write_gwsave_container(0, hdr, chunks);
    const auto peek =
        gw::persist::peek_gwsave(std::span<const std::byte>(file.data(), file.size()), true);
    REQUIRE(peek.err == gw::persist::LoadError::Ok);
    CHECK(peek.hdr.chunk_count == 9u);
}

TEST_CASE("persist — bad magic") {
    std::vector<std::byte> buf(200, std::byte{0});
    const auto peek = gw::persist::peek_gwsave(std::span<const std::byte>(buf.data(), buf.size()), false);
    CHECK(peek.err == gw::persist::LoadError::BadMagic);
}

TEST_CASE("persist — truncated container") {
    std::vector<std::byte> buf(8, std::byte{0});
    const auto peek = gw::persist::peek_gwsave(std::span<const std::byte>(buf.data(), buf.size()), false);
    CHECK(peek.err == gw::persist::LoadError::TruncatedContainer);
}

TEST_CASE("persist — integrity mismatch on tampered footer") {
    World w;
    const auto a = w.create_entity();
    w.add_component(a, Pos{1.f, 2.f, 3.f});
    (void)w.component_registry().id_of<Pos>();

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
    REQUIRE(file.size() > 40);
    file[file.size() - 1] ^= std::byte{0xFF};
    const auto peek = gw::persist::peek_gwsave(std::span<const std::byte>(file.data(), file.size()), true);
    CHECK(peek.err == gw::persist::LoadError::IntegrityMismatch);
}

TEST_CASE("persist — load centre chunk only (streaming contract)") {
    World w;
    (void)w.component_registry().id_of<Pos>();
    auto chunks = gw::persist::build_chunk_grid_demo(w);
    gw::persist::gwsave::HeaderPrefix hdr{};
    hdr.schema_version  = gw::persist::kCurrentSaveSchemaVersion;
    hdr.engine_version  = 1;
    hdr.chunk_count     = static_cast<std::uint32_t>(chunks.size());
    const auto c        = gw::persist::centre_chunk_payload(chunks);
    hdr.determinism_hash =
        gw::persist::ecs_blob_determinism_hash(std::span<const std::byte>(
            reinterpret_cast<const std::byte*>(c.data()), c.size()));
    auto file = gw::persist::write_gwsave_container(0, hdr, chunks);
    auto peek = gw::persist::peek_gwsave(std::span<const std::byte>(file.data(), file.size()), true);
    REQUIRE(peek.err == gw::persist::LoadError::Ok);
    std::uint32_t centre_idx = 0;
    for (std::uint32_t i = 0; i < peek.toc.size(); ++i) {
        if (peek.toc[i].cx == 0 && peek.toc[i].cy == 0 && peek.toc[i].cz == 0) {
            centre_idx = i;
            break;
        }
    }
    std::vector<std::byte> payload;
    REQUIRE(gw::persist::load_gwsave_chunk_by_index(std::span<const std::byte>(file.data(), file.size()),
                                                    peek, centre_idx, payload)
            == gw::persist::LoadError::Ok);
    CHECK(!payload.empty());
}

TEST_CASE("persist — migration registry exposes no-op step") {
    const auto steps = gw::persist::save_migration_steps();
    REQUIRE(steps.size() >= 1u);
    CHECK(steps[0].from_schema == 1u);
}

TEST_CASE("persist — blake3 deterministic") {
    const std::byte data[4] = {std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}};
    const auto a            = gw::persist::blake3_digest_256(std::span<const std::byte>(data, 4));
    const auto b            = gw::persist::blake3_digest_256(std::span<const std::byte>(data, 4));
    CHECK(a == b);
}
