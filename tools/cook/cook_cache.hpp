#pragma once
// tools/cook/cook_cache.hpp
// CookCache — xxHash3-keyed SQLite3 disk cache.
// Phase 6 spec §5.2.

#include "engine/assets/asset_types.hpp"
#include "engine/assets/asset_error.hpp"
#include <filesystem>
#include <memory>
#include <string>
#include <optional>
#include <cstdint>

// Forward-declare SQLiteCpp to avoid pulling the header into every TU.
namespace SQLite { class Database; }

namespace gw::tools::cook {

using gw::assets::CookKey;
using gw::assets::CookPlatform;
using gw::assets::CookConfig;
using gw::assets::AssetPath;
using gw::assets::AssetResult;
using gw::assets::asset_ok;
using gw::assets::asset_err;
using gw::assets::AssetErrorCode;

struct CacheEntry {
    CookKey   cook_key;
    AssetPath output_path;
    int64_t   cooked_at   = 0;  // unix timestamp
    uint32_t  cook_version= 0;
    uint8_t   platform    = 0;
    uint8_t   config      = 0;
};

class CookCache {
public:
    explicit CookCache(const std::filesystem::path& db_path);
    ~CookCache();

    CookCache(const CookCache&)            = delete;
    CookCache& operator=(const CookCache&) = delete;

    // Look up a cook_key.  Returns nullopt on cache miss.
    [[nodiscard]] std::optional<CacheEntry> lookup(const CookKey& key) const;

    // Insert or update a cache entry.
    AssetResult<std::monostate> insert(const CacheEntry& entry);

    // Remove all entries older than max_age_seconds (0 = keep all).
    void prune(int64_t max_age_seconds = 0);

private:
    void create_schema();

    std::unique_ptr<SQLite::Database> db_;
};

} // namespace gw::tools::cook
