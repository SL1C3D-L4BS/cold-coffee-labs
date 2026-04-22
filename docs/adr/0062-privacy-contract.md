# ADR 0062 — Privacy contract (GDPR, CCPA, COPPA 2026)

- **Status:** Accepted
- **Date:** 2026-04-21

## Decision

1. **Opt-in defaults:** `tele.enabled=0`, `tele.crash.enabled=0`, `tele.events.enabled=0` until first-launch consent UI completes.
2. **Consent tiers (disaggregated):** `None`, `CrashOnly`, `CoreTelemetry`, `AnalyticsAllowed`, `MarketingAllowed`. COPPA posture: under `tele.consent.age_gate_years` (default 13, locale-adjustable), choices clamp to `CrashOnly` maximum for third-party SDK exposure.
3. **State machine:** Initial → LegalNotice → AgeGate → ConsentDisaggregated → Persisted → Revocable. UI lives in **RmlUi** (`ui/privacy/`), not ImGui (CLAUDE.md #12).
4. **Persistence:** consent version string forces re-prompt on mismatch; SQLite `settings` namespace `tele.consent.*`.
5. **DSAR:** export manifest (JSON schema in implementation) + user-selected output dir; **deletion** nulls PII fields, purges telemetry queue rows for `user_id_hash`, removes cloud slots; gameplay blobs may remain if non-PII.
6. **BLD / agents:** consent-changing CVars are not user-writable from untrusted automation (treat as user Execute-tier; registry flags `kCVarReadOnly` for console except programmatic trusted paths).

## Schema block (DSAR manifest v1)

```json
{
  "schema_version": 1,
  "exported_unix_ms": 0,
  "user_id_hash": "hex64",
  "slots": [],
  "settings_namespaces": [],
  "telemetry_queue_rows": 0,
  "consent": { "tier": "None", "version": "", "granted_ms": 0 }
}
```

## Consequences

- Legal review updates this ADR when statutes or platform rules change.
