# ADR 0059 — `ICloudSave` and conflict policy

- **Status:** Accepted
- **Date:** 2026-04-21

## Decision

1. **`ICloudSave`** exposes `put` / `get` / `list` / `remove` / `quota` / `backend_name`.
2. **`SlotStamp`**: `unix_ms`, `vector_clock`, `machine_salt` (FNV-1a of host + user id hash).
3. **`CloudConflictPolicy`**: `Prompt` (default), `PreferLocal`, `PreferCloud`, `PreferNewer`, `PreserveBoth`. Resolver table matches Phase 15 spec; Steam Cloud opacity implies `Prompt` is the safe default.
4. **Factories:** `local` dev FS backend always ships; `steam`, `eos`, `s3` compile only behind `GW_ENABLE_STEAMWORKS`, `GW_ENABLE_EOS`, `GW_ENABLE_CPR` respectively (stubs return null backend when OFF).
5. **`QuotaWarning`** event when `used_bytes / quota_bytes` crosses `persist.cloud.quota_warn_pct`.

## Consequences

- Phase 16 replaces stubs with production SDKs without interface churn.
