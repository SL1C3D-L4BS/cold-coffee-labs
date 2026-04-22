# ADR 0057 — ECS snapshot wire format and save migration

- **Status:** Accepted
- **Date:** 2026-04-21

## Context

ECS state must round-trip across OSes and engine versions with an append-only migration chain.

## Decision

1. **Wire format** reuses ADR-0006 v3 primitives: `save_world(..., SnapshotMode::PieSnapshot)` bytes are the canonical chunk payload building block.
2. **`SchemaVersion`** (`schema_version` in `.gwsave` header) gates migrations. Current schema is published as a `constexpr` in `persist_types.hpp`.
3. **`SaveMigrationStep`**: `{ from_version, to_version, apply(SaveReader&, SaveWriter&) }`. Registry is `std::array`-backed append-only; steps are pure (no wall clock, no globals).
4. On load, if `on_disk_schema < current`, run the chain forward. If `engine_version` on disk exceeds the running engine and `persist.migration.strict=1`, refuse load (`ForwardIncompatible`).
5. **`determinism_hash`**: recomputed after deserialize; mismatch is `LoadError::DeterminismMismatch` unless best-effort mode (not used in strict default).

## Consequences

- Golden fixtures in `tests/fixtures/saves/v1/` are validated by `gw_tests` and future `gw_save_tool`.
