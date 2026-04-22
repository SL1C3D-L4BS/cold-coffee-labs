# ADR 0063 — Secure local storage

- **Status:** Accepted
- **Date:** 2026-04-21

## Decision

1. **Keyring wrap** (Win DPAPI / Linux libsecret) holds DSN key names and OAuth refresh tokens; same helper family as Phase 9 BLD keyring.
2. **Save encryption:** optional XChaCha20-Poly1305 for SP saves with user-authored PII; MP saves default plaintext performance path. Wrong user binding fails round-trip at open.
3. **Redaction:** logs and attachments never contain secrets, session tickets, or full DSN.

## Consequences

- Phase 15 ships keyring integration points as stubs where OS helpers are not yet linked; production `persist-*` presets enable them.
