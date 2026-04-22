# ADR 0060 — Telemetry crash backend (Sentry Native)

- **Status:** Accepted
- **Date:** 2026-04-21

## Decision

1. **`TelemetryWorld`** PIMPL facade; public headers never include `<sentry.h>`.
2. **`GW_ENABLE_SENTRY`** gates `impl/sentry_backend.cpp`. Default OFF on `dev-*`; ON on `persist-*` presets.
3. **DSN** resolves via OS keyring helper (Phase 9 pattern); never from env, CLI, or logs. Miss → null backend + one-shot warn + `CrashBufferedOffline` event.
4. **Crashpad** handler path and database path follow platform conventions (`%LOCALAPPDATA%\CCL\Greywater\sentry-db`, `$XDG_CACHE_HOME/ccl-greywater/sentry-db`).
5. **Attachments:** allow-list only (log head/tail); saves, queues, and secrets are never attached. `.gwreplay` only if `tele.attach_replay` and consent ≥ `CrashOnly+` as defined in ADR-0062.
6. **`before_send`** runs PII scrub chain (ADR-0061).

## Consequences

- Pin **Sentry Native 0.7.19**; bumps require ADR-0064 revision + full crash round-trip CI.
