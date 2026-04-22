// engine/persist/impl/sqlite_store.cpp — ADR-0058 (header quarantine).

#include "engine/persist/local_store.hpp"
#include "engine/persist/detail/sqlite_connection_pragmas.hpp"

#include <SQLiteCpp/SQLiteCpp.h>

namespace gw::persist {

namespace {

void create_schema_v1(SQLite::Database& db) {
    db.exec(R"SQL(
CREATE TABLE IF NOT EXISTS schema_version (
    component TEXT NOT NULL PRIMARY KEY,
    version   INTEGER NOT NULL
) STRICT;

CREATE TABLE IF NOT EXISTS slots (
    slot_id       INTEGER NOT NULL PRIMARY KEY,
    display_name  TEXT    NOT NULL,
    unix_ms       INTEGER NOT NULL,
    engine_ver    INTEGER NOT NULL,
    schema_ver    INTEGER NOT NULL,
    determinism_h INTEGER NOT NULL,
    path          TEXT    NOT NULL,
    size_bytes    INTEGER NOT NULL,
    blake3_hex    TEXT    NOT NULL
) STRICT;

CREATE TABLE IF NOT EXISTS settings (
    ns     TEXT NOT NULL,
    key    TEXT NOT NULL,
    value  TEXT NOT NULL,
    PRIMARY KEY (ns, key)
) STRICT;

CREATE TABLE IF NOT EXISTS achievements_mirror (
    platform   TEXT NOT NULL,
    app_id     TEXT NOT NULL,
    ach_id     TEXT NOT NULL,
    unlocked   INTEGER NOT NULL,
    unlocked_ms INTEGER,
    PRIMARY KEY (platform, app_id, ach_id)
) STRICT;

CREATE TABLE IF NOT EXISTS telemetry_queue (
    row_id      INTEGER PRIMARY KEY AUTOINCREMENT,
    enqueued_ms INTEGER NOT NULL,
    batch_id    BLOB    NOT NULL,
    payload     BLOB    NOT NULL,
    attempts    INTEGER NOT NULL DEFAULT 0,
    next_try_ms INTEGER NOT NULL,
    blake3_128  BLOB    NOT NULL
) STRICT;

CREATE TABLE IF NOT EXISTS mods (
    mod_id    TEXT NOT NULL PRIMARY KEY,
    enabled   INTEGER NOT NULL,
    version   TEXT    NOT NULL,
    hash      TEXT    NOT NULL
) STRICT;
)SQL");
}

class SqliteLocalStore final : public ILocalStore {
public:
    explicit SqliteLocalStore(std::string path) : path_(std::move(path)) {}

    bool open_or_create() override {
        try {
            db_ = std::make_unique<SQLite::Database>(path_, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
            detail::apply_sqlite_connection_pragmas(*db_);
            create_schema_v1(*db_);
            SQLite::Statement st(*db_, R"SQL(
INSERT INTO schema_version(component, version) VALUES('persist_db', 1)
ON CONFLICT(component) DO NOTHING;
)SQL");
            st.exec();
            return true;
        } catch (...) {
            return false;
        }
    }

    bool upsert_slot(const SlotRow& row) override {
        try {
            SQLite::Statement st(
                *db_,
                R"SQL(
REPLACE INTO slots(slot_id, display_name, unix_ms, engine_ver, schema_ver, determinism_h, path, size_bytes, blake3_hex)
VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?);
)SQL");
            st.bind(1, static_cast<int>(row.slot_id));
            st.bind(2, row.display_name);
            st.bind(3, static_cast<std::int64_t>(row.unix_ms));
            st.bind(4, static_cast<int>(row.engine_ver));
            st.bind(5, static_cast<int>(row.schema_ver));
            st.bind(6, static_cast<std::int64_t>(row.determinism_h));
            st.bind(7, row.path);
            st.bind(8, static_cast<std::int64_t>(row.size_bytes));
            st.bind(9, row.blake3_hex);
            st.exec();
            return true;
        } catch (...) {
            return false;
        }
    }

    bool delete_slot(SlotId id) override {
        try {
            SQLite::Statement st(*db_, "DELETE FROM slots WHERE slot_id = ?;");
            st.bind(1, static_cast<int>(id));
            st.exec();
            return true;
        } catch (...) {
            return false;
        }
    }

    bool list_slots(std::vector<SlotRow>& out) override {
        out.clear();
        try {
            SQLite::Statement st(*db_, "SELECT slot_id, display_name, unix_ms, engine_ver, schema_ver, determinism_h, path, size_bytes, blake3_hex FROM slots ORDER BY slot_id;");
            while (st.executeStep()) {
                SlotRow r;
                r.slot_id       = st.getColumn(0).getInt();
                r.display_name  = st.getColumn(1).getString();
                r.unix_ms       = st.getColumn(2).getInt64();
                r.engine_ver    = st.getColumn(3).getInt();
                r.schema_ver    = st.getColumn(4).getInt();
                r.determinism_h = st.getColumn(5).getInt64();
                r.path          = st.getColumn(6).getString();
                r.size_bytes    = st.getColumn(7).getInt64();
                r.blake3_hex    = st.getColumn(8).getString();
                out.push_back(std::move(r));
            }
            return true;
        } catch (...) {
            return false;
        }
    }

    std::optional<SlotRow> get_slot(SlotId id) override {
        try {
            SQLite::Statement st(*db_, "SELECT slot_id, display_name, unix_ms, engine_ver, schema_ver, determinism_h, path, size_bytes, blake3_hex FROM slots WHERE slot_id = ?;");
            st.bind(1, static_cast<int>(id));
            if (!st.executeStep()) return std::nullopt;
            SlotRow r;
            r.slot_id       = st.getColumn(0).getInt();
            r.display_name  = st.getColumn(1).getString();
            r.unix_ms       = st.getColumn(2).getInt64();
            r.engine_ver    = st.getColumn(3).getInt();
            r.schema_ver    = st.getColumn(4).getInt();
            r.determinism_h = st.getColumn(5).getInt64();
            r.path          = st.getColumn(6).getString();
            r.size_bytes    = st.getColumn(7).getInt64();
            r.blake3_hex    = st.getColumn(8).getString();
            return r;
        } catch (...) {
            return std::nullopt;
        }
    }

    bool set_setting(std::string_view ns, std::string_view key, std::string_view value) override {
        try {
            SQLite::Statement st(*db_,
                                 "INSERT INTO settings(ns, key, value) VALUES(?, ?, ?) "
                                 "ON CONFLICT(ns, key) DO UPDATE SET value = excluded.value;");
            st.bind(1, std::string{ns});
            st.bind(2, std::string{key});
            st.bind(3, std::string{value});
            st.exec();
            return true;
        } catch (...) {
            return false;
        }
    }

    std::optional<std::string> get_setting(std::string_view ns, std::string_view key) override {
        try {
            SQLite::Statement st(*db_, "SELECT value FROM settings WHERE ns = ? AND key = ?;");
            st.bind(1, std::string{ns});
            st.bind(2, std::string{key});
            if (!st.executeStep()) return std::nullopt;
            return st.getColumn(0).getString();
        } catch (...) {
            return std::nullopt;
        }
    }

    bool telemetry_enqueue(std::int64_t enqueued_ms, std::span<const std::byte> batch_id,
                           std::span<const std::byte> payload, std::int32_t attempts,
                           std::int64_t next_try_ms, std::span<const std::byte> blake3_128) override {
        try {
            SQLite::Statement st(*db_,
                                 "INSERT INTO telemetry_queue(enqueued_ms, batch_id, payload, attempts, next_try_ms, blake3_128) "
                                 "VALUES(?, ?, ?, ?, ?, ?);");
            st.bind(1, enqueued_ms);
            st.bind(2, batch_id.data(), static_cast<int>(batch_id.size()));
            st.bind(3, payload.data(), static_cast<int>(payload.size()));
            st.bind(4, static_cast<int>(attempts));
            st.bind(5, next_try_ms);
            st.bind(6, blake3_128.data(), static_cast<int>(blake3_128.size()));
            st.exec();
            return true;
        } catch (...) {
            return false;
        }
    }

    std::int64_t telemetry_pending_count() override {
        try {
            SQLite::Statement st(*db_, "SELECT COUNT(*) FROM telemetry_queue;");
            if (!st.executeStep()) return -1;
            return st.getColumn(0).getInt64();
        } catch (...) {
            return -1;
        }
    }

    bool telemetry_delete_all_rows() override {
        try {
            db_->exec("DELETE FROM telemetry_queue;");
            return true;
        } catch (...) {
            return false;
        }
    }

    std::int64_t telemetry_delete_older_than(std::int64_t cutoff_ms) override {
        try {
            SQLite::Statement st(*db_, "DELETE FROM telemetry_queue WHERE enqueued_ms < ?;");
            st.bind(1, cutoff_ms);
            st.exec();
            return static_cast<std::int64_t>(db_->getChanges());
        } catch (...) {
            return -1;
        }
    }

    bool wipe_pii_for_user(std::string_view user_id_hash_hex) override {
        (void)user_id_hash_hex;
        try {
            // Phase 15 stub: clear tele.consent namespace and telemetry queue; expand in ADR-0062.
            db_->exec("DELETE FROM telemetry_queue;");
            db_->exec("DELETE FROM settings WHERE ns LIKE 'tele.consent%' OR ns LIKE 'user.pii%';");
            db_->exec("UPDATE slots SET display_name = '';");
            return true;
        } catch (...) {
            return false;
        }
    }

    void shutdown() override {
        try {
            if (db_) db_->exec("PRAGMA wal_checkpoint(TRUNCATE);");
        } catch (...) {
        }
        db_.reset();
    }

private:
    std::string                        path_;
    std::unique_ptr<SQLite::Database> db_{};
};

} // namespace

std::unique_ptr<ILocalStore> make_sqlite_local_store(std::string db_path) {
    return std::make_unique<SqliteLocalStore>(std::move(db_path));
}

} // namespace gw::persist
