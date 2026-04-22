# ADR 0061 — Telemetry event pipeline

- **Status:** Accepted
- **Date:** 2026-04-21

## Decision

1. **`ITelemetrySink`** batches `EventEnvelope` records: `{ schema_version, ts, session_id_hash, event_name, props_json }`.
2. **Pipeline:** record → PII scrub → consent gate → in-memory ring → when `batch_size` or `flush_interval_s` or shutdown, enqueue SQLite row with BLAKE3-128 digest (`telemetry_queue`).
3. **Flush worker** uses `jobs::Scheduler` jobs (telemetry_io lane convention); HTTP POST to `tele.endpoint` when non-empty; exponential backoff on 5xx/network; 4xx drops row as permanent failure.
4. **`GW_TELEMETRY_COMPILED=OFF`** (release SKUs without TRC clearance) compiles stubs only; overhead target ≤60 µs cold path (ADR-0064).
5. **Session id** = SplitMix64 mix of `session_seed` xor `user_id_hash` (no raw PII).

## Consequences

- Offline queue survives restart; `tele.queue.max_age_days` enforces storage minimisation.
