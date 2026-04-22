#pragma once

// Shared SQLite per-connection pragmas (ADR-0058). Connection-local settings
// are not visible on other handles; keep one implementation for store + tests.

#include <SQLiteCpp/SQLiteCpp.h>

namespace gw::persist::detail {

inline void apply_sqlite_connection_pragmas(SQLite::Database& database) {
    database.exec("PRAGMA journal_mode=WAL;");
    database.exec("PRAGMA synchronous=NORMAL;");
    database.exec("PRAGMA foreign_keys=ON;");
    database.exec("PRAGMA analysis_limit=400;");
}

} // namespace gw::persist::detail
