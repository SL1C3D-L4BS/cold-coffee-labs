// Phase 15 — SQLite local store (ADR-0058).

#include <doctest/doctest.h>

#include "engine/persist/detail/sqlite_connection_pragmas.hpp"
#include "engine/persist/local_store.hpp"
#include "engine/persist/persist_types.hpp"

#include <SQLiteCpp/SQLiteCpp.h>

#include <array>
#include <filesystem>
#include <vector>

namespace {

std::filesystem::path unique_temp_db(std::string_view tag) {
    auto base = std::filesystem::temp_directory_path() / "gw_phase15_sqlite";
    base /= tag;
    std::filesystem::remove_all(base);
    std::filesystem::create_directories(base);
    return base / "store.db";
}

} // namespace

TEST_CASE("phase15 — sqlite journal_mode is WAL") {
    const auto path = unique_temp_db("wal").string();
    auto                store = gw::persist::make_sqlite_local_store(path);
    REQUIRE(store->open_or_create());
    SQLite::Database db(path, SQLite::OPEN_READONLY);
    SQLite::Statement st(db, "PRAGMA journal_mode;");
    REQUIRE(st.executeStep());
    CHECK(st.getColumn(0).getString() == "wal");
}

TEST_CASE("phase15 — sqlite ADR-0058 connection pragmas (single handle)") {
    // synchronous / foreign_keys are per-connection; a second handle does not
    // inherit the store's settings. This matches the shared helper used by
    // SqliteLocalStore::open_or_create. Use a temp file (not :memory:) so
    // journal_mode can settle to wal instead of memory.
    const auto path = unique_temp_db("adr58").string();
    SQLite::Database db(path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    gw::persist::detail::apply_sqlite_connection_pragmas(db);
    {
        SQLite::Statement st(db, "PRAGMA journal_mode;");
        REQUIRE(st.executeStep());
        CHECK(st.getColumn(0).getString() == "wal");
    }
    {
        SQLite::Statement st(db, "PRAGMA synchronous;");
        REQUIRE(st.executeStep());
        CHECK(st.getColumn(0).getInt() == 1); // NORMAL
    }
    {
        SQLite::Statement st(db, "PRAGMA foreign_keys;");
        REQUIRE(st.executeStep());
        CHECK(st.getColumn(0).getInt() == 1);
    }
}

TEST_CASE("phase15 — STRICT table rejects wrong column type") {
    SQLite::Database db(":memory:", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    db.exec("CREATE TABLE t (a INTEGER NOT NULL) STRICT;");
    CHECK_THROWS_AS(db.exec("INSERT INTO t(a) VALUES ('nope');"), SQLite::Exception);
}

TEST_CASE("phase15 — slot insert list delete round-trip") {
    const auto path = unique_temp_db("slots").string();
    auto                store = gw::persist::make_sqlite_local_store(path);
    REQUIRE(store->open_or_create());
    gw::persist::SlotRow r{};
    r.slot_id       = 3;
    r.display_name  = "alpha";
    r.unix_ms       = 1000;
    r.engine_ver    = 7;
    r.schema_ver    = 2;
    r.determinism_h = 99;
    r.path          = "/tmp/x.gwsave";
    r.size_bytes    = 1234;
    r.blake3_hex    = std::string(64, 'a');
    REQUIRE(store->upsert_slot(r));
    std::vector<gw::persist::SlotRow> rows;
    REQUIRE(store->list_slots(rows));
    REQUIRE(rows.size() == 1u);
    CHECK(rows[0].slot_id == 3);
    const auto g = store->get_slot(3);
    REQUIRE(g.has_value());
    CHECK(g->display_name == "alpha");
    REQUIRE(store->delete_slot(3));
    rows.clear();
    REQUIRE(store->list_slots(rows));
    CHECK(rows.empty());
}

TEST_CASE("phase15 — settings namespace isolation") {
    const auto path = unique_temp_db("settings").string();
    auto                store = gw::persist::make_sqlite_local_store(path);
    REQUIRE(store->open_or_create());
    REQUIRE(store->set_setting("a", "k", "1"));
    REQUIRE(store->set_setting("b", "k", "2"));
    CHECK(*store->get_setting("a", "k") == "1");
    CHECK(*store->get_setting("b", "k") == "2");
}

TEST_CASE("phase15 — telemetry queue insert and pending count") {
    const auto path = unique_temp_db("tele").string();
    auto                store = gw::persist::make_sqlite_local_store(path);
    REQUIRE(store->open_or_create());
    const std::array<std::byte, 4> payload{std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}};
    const std::array<std::byte, 8>  bid{};
    const std::array<std::byte, 16> d128{};
    REQUIRE(store->telemetry_enqueue(42, bid, payload, 0, 42, d128));
    CHECK(store->telemetry_pending_count() == 1);
    REQUIRE(store->telemetry_delete_all_rows());
    CHECK(store->telemetry_pending_count() == 0);
}

TEST_CASE("phase15 — telemetry_delete_older_than drops aged rows (ADR-0061)") {
    const auto path = unique_temp_db("age").string();
    auto                store = gw::persist::make_sqlite_local_store(path);
    REQUIRE(store->open_or_create());
    const std::array<std::byte, 1>  payload{std::byte{0xAB}};
    const std::array<std::byte, 8>  bid{};
    const std::array<std::byte, 16> d128{};
    REQUIRE(store->telemetry_enqueue(  100, bid, payload, 0,   100, d128));
    REQUIRE(store->telemetry_enqueue(  500, bid, payload, 0,   500, d128));
    REQUIRE(store->telemetry_enqueue(10000, bid, payload, 0, 10000, d128));
    CHECK(store->telemetry_pending_count() == 3);
    CHECK(store->telemetry_delete_older_than(1000) == 2);
    CHECK(store->telemetry_pending_count() == 1);
    CHECK(store->telemetry_delete_older_than(0) == 0);
    CHECK(store->telemetry_delete_older_than(20000) == 1);
    CHECK(store->telemetry_pending_count() == 0);
}

TEST_CASE("phase15 — telemetry_delete_all_rows drains queue") {
    const auto path = unique_temp_db("drain").string();
    auto                store = gw::persist::make_sqlite_local_store(path);
    REQUIRE(store->open_or_create());
    const std::array<std::byte, 2>  payload{std::byte{9}, std::byte{9}};
    const std::array<std::byte, 8>  bid{};
    const std::array<std::byte, 16> d128{};
    REQUIRE(store->telemetry_enqueue(1, bid, payload, 0, 1, d128));
    REQUIRE(store->telemetry_enqueue(2, bid, payload, 0, 2, d128));
    CHECK(store->telemetry_pending_count() == 2);
    REQUIRE(store->telemetry_delete_all_rows());
    CHECK(store->telemetry_pending_count() == 0);
}

TEST_CASE("phase15 — schema_version row for persist_db") {
    const auto path = unique_temp_db("schema").string();
    auto                store = gw::persist::make_sqlite_local_store(path);
    REQUIRE(store->open_or_create());
    SQLite::Database db(path, SQLite::OPEN_READONLY);
    SQLite::Statement st(db, "SELECT version FROM schema_version WHERE component = 'persist_db';");
    REQUIRE(st.executeStep());
    CHECK(st.getColumn(0).getInt() == 1);
}

TEST_CASE("phase15 — LoadError UnsupportedCompression to_string") {
    CHECK(gw::persist::to_string(gw::persist::LoadError::UnsupportedCompression)
          == "UnsupportedCompression");
}

TEST_CASE("phase15 — local store shutdown is safe") {
    const auto path = unique_temp_db("shutdown").string();
    auto                store = gw::persist::make_sqlite_local_store(path);
    REQUIRE(store->open_or_create());
    store->shutdown();
    store->shutdown();
}

TEST_CASE("phase15 — wipe_pii clears display_name on slots") {
    const auto path = unique_temp_db("wipe").string();
    auto                store = gw::persist::make_sqlite_local_store(path);
    REQUIRE(store->open_or_create());
    gw::persist::SlotRow r{};
    r.slot_id       = 1;
    r.display_name  = "secret";
    r.unix_ms       = 0;
    r.engine_ver    = 0;
    r.schema_ver    = 1;
    r.determinism_h = 0;
    r.path          = "p";
    r.size_bytes    = 0;
    r.blake3_hex    = std::string(64, '0');
    REQUIRE(store->upsert_slot(r));
    REQUIRE(store->wipe_pii_for_user("deadbeef"));
    const auto g = store->get_slot(1);
    REQUIRE(g.has_value());
    CHECK(g->display_name.empty());
}
