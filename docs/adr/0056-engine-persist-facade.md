# ADR 0056 — Engine persist facade and `.gwsave` v1

- **Status:** Accepted
- **Date:** 2026-04-21
- **Deciders:** Cold Coffee Labs engine group

## Context

Phase 15 introduces durable gameplay state with chunk-scoped streaming, integrity guarantees, and a PIMPL facade mirroring `NetworkWorld` / `PhysicsWorld`.

## Decision

1. **`PersistWorld`** in `engine/persist/persist_world.hpp` is the only header callers include for persistence orchestration. Implementation owns container IO, optional `ILocalStore`, and optional `ICloudSave`.
2. **`.gwsave` v1** is a little-endian binary container: magic `GWSV`, versioned header (≤16 KiB readable without decompression), chunk TOC, compressed chunk bodies (zstd when `GW_ENABLE_ZSTD`), footer `FTR!` + BLAKE3-256 over all bytes preceding the digest.
3. **Header quarantine:** No `<sqlite3.h>`, `<zstd.h>`, `<blake3.h>` in any `engine/persist/*.hpp` public header. Vendor includes appear only in `engine/persist/impl/*.cpp` (and `integrity.cpp` for BLAKE3).
4. **Null / local FS backend** is the default for `dev-*` presets: atomic `tmp + rename` writes under an expandable `persist.save.dir`.
5. **`determinism_hash`** in the header fingerprints the ECS snapshot payload (canonical serialization bytes); it is advisory for debugging. **Integrity** is enforced only by the BLAKE3 footer.

## Consequences

- Save tooling and CI gates key off the frozen v1 layout in ADR-0064.
- Phase 19 `ChunkStreamingBridge` consumes chunk TOC entries without loading distant chunks.

## References

- ADR-0006 (ECS serialization), ADR-0037 (determinism), ADR-0053 (session seed), ADR-0055 §11 (hand-off).
