# ADR 0058 — SQLite local store (`ILocalStore`)

- **Status:** Accepted
- **Date:** 2026-04-21

## Decision

1. **`ILocalStore`** abstracts slots, settings KV, achievements mirror, telemetry queue, mods catalog, and `schema_version` rows.
2. **SQLiteCpp 3.3.1** (pinned in `cmake/dependencies.cmake`) is the only approved wrapper; raw `sqlite3` API is confined to `impl/sqlite_store.cpp`.
3. **Pragma stack** on open: `journal_mode=WAL`, `synchronous=NORMAL`, `foreign_keys=ON`. Tables are `STRICT` where SQLite ≥ 3.37.
4. **Core tables** match Phase 15 spec: `schema_version`, `slots`, `settings`, `achievements_mirror`, `telemetry_queue`, `mods`.
5. **Forward migrations** for DB schema are registered beside code; `PRAGMA wal_checkpoint(TRUNCATE)` runs on clean shutdown before cloud sync.

## Consequences

- Cook cache and runtime persistence share the same SQLite linkage discipline.
