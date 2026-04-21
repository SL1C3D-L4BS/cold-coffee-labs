#include "cook_cache.hpp"
#include <SQLiteCpp/SQLiteCpp.h>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <stdexcept>

namespace gw::tools::cook {

// Schema matching spec §5.2.
static const char* kSchema = R"sql(
CREATE TABLE IF NOT EXISTS cook_cache (
    cook_key     BLOB(16) PRIMARY KEY,
    output_path  TEXT NOT NULL,
    cooked_at    INTEGER NOT NULL,
    cook_version INTEGER NOT NULL,
    platform     INTEGER NOT NULL,
    config       INTEGER NOT NULL
);
)sql";

static std::string key_to_blob(const CookKey& k) {
    char buf[16];
    std::memcpy(buf,     &k.hi, 8);
    std::memcpy(buf + 8, &k.lo, 8);
    return std::string(buf, 16);
}

static CookKey blob_to_key(const std::string& blob) {
    CookKey k{};
    if (blob.size() == 16) {
        std::memcpy(&k.hi, blob.data(),     8);
        std::memcpy(&k.lo, blob.data() + 8, 8);
    }
    return k;
}

CookCache::CookCache(const std::filesystem::path& db_path) {
    std::filesystem::create_directories(db_path.parent_path());

    // Open in WAL mode: safe on interrupted writes (spec §15).
    db_ = std::make_unique<SQLite::Database>(
        db_path.string(),
        SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    db_->exec("PRAGMA journal_mode=WAL;");
    db_->exec("PRAGMA synchronous=NORMAL;");
    create_schema();
}

CookCache::~CookCache() = default;

void CookCache::create_schema() {
    db_->exec(kSchema);
}

std::optional<CacheEntry> CookCache::lookup(const CookKey& key) const {
    try {
        SQLite::Statement q(*db_,
            "SELECT cook_key, output_path, cooked_at, cook_version, platform, config "
            "FROM cook_cache WHERE cook_key = ?");
        q.bind(1, key_to_blob(key));
        if (!q.executeStep()) return std::nullopt;

        CacheEntry e{};
        e.cook_key     = blob_to_key(q.getColumn(0).getString());
        e.output_path  = q.getColumn(1).getString();
        e.cooked_at    = q.getColumn(2).getInt64();
        e.cook_version = static_cast<uint32_t>(q.getColumn(3).getInt());
        e.platform     = static_cast<uint8_t>(q.getColumn(4).getInt());
        e.config       = static_cast<uint8_t>(q.getColumn(5).getInt());
        return e;
    } catch (...) {
        return std::nullopt;
    }
}

AssetResult<std::monostate> CookCache::insert(const CacheEntry& entry) {
    try {
        const int64_t now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        SQLite::Transaction txn(*db_);
        SQLite::Statement s(*db_,
            "INSERT OR REPLACE INTO cook_cache "
            "(cook_key, output_path, cooked_at, cook_version, platform, config) "
            "VALUES (?, ?, ?, ?, ?, ?)");
        s.bind(1, key_to_blob(entry.cook_key));
        s.bind(2, entry.output_path);
        s.bind(3, entry.cooked_at > 0 ? entry.cooked_at : now);
        s.bind(4, static_cast<int>(entry.cook_version));
        s.bind(5, static_cast<int>(entry.platform));
        s.bind(6, static_cast<int>(entry.config));
        s.exec();
        txn.commit();
        return std::monostate{};
    } catch (const std::exception& ex) {
        return std::unexpected(gw::assets::AssetError{AssetErrorCode::CookFailed,
                                                        ex.what()});
    }
}

void CookCache::prune(int64_t max_age_seconds) {
    if (max_age_seconds <= 0) return;
    try {
        const int64_t now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        SQLite::Statement s(*db_,
            "DELETE FROM cook_cache WHERE cooked_at < ?");
        s.bind(1, now - max_age_seconds);
        s.exec();
    } catch (...) {}
}

} // namespace gw::tools::cook
