# ADR 0011 — BLD MCP transport & elicitation policy

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 9 opening, wave 9A)
- **Deciders:** Cold Coffee Labs engine group (Claude Opus 4.7 drafting; founder final)
- **Supersedes:** —
- **Superseded by:** —
- **Related:** `CLAUDE.md` non-negotiables #18 (no secrets committed/into model context); `docs/00_SPECIFICATION_Claude_Opus_4.7.md` §2.5; `docs/02_SYSTEMS_AND_SIMULATIONS.md §13`; MCP spec 2025-11-25 (<https://modelcontextprotocol.io/specification/2025-11-25>); ADR-0007 (BLD C-ABI freeze); ADR-0010 (provider abstraction); ADR-0015 (governance — forward reference).

## 1. Context

BLD speaks **Model Context Protocol (MCP)** 2025-11-25 to:

1. Our own editor chat panel (in-process client).
2. External MCP clients — Claude Code, Cursor, MCP Inspector, any studio's CI harness.

Shipping one protocol for both is the whole point: the engineering we do internally becomes what external tools drive. The implementation question is *which MCP feature set* and *how elicitation is used safely*.

Pinned from `docs/00 §2.5`: rmcp (official Rust SDK), schema version `2025-11-25`. Not litigated here — carried forward. The decisions below concern *how* we use that SDK.

### 1.1 Research deltas since docs were written

- MCP 2025-11-25 elicitation has **two modes**: `form` (structured data entry) and `url` (external browser-driven). **Servers MUST NOT use `form` mode for secrets** (passwords, API keys, payment credentials). Only `url` is acceptable for those — the spec is explicit. Our earlier wording in `docs/architecture/grey-water-engine-blueprint.md §3.7` did not draw this distinction; it does now.
- `rmcp` 1.x supports both stdio and **Streamable HTTP** transports, with elicitation feature-gated. Streamable HTTP was a planned 9E polish item — it remains there, out of scope for 9A.

## 2. Decision

### 2.1 Transport layers

- **Default transport: stdio.** BLD ships as a library loaded in-process by `gw_editor` and exposes its MCP server over a pair of OS pipes owned by `bld-mcp`. The in-process chat-panel client bridges frames directly (no pipes). External clients spawn `bld_mcp_host` (a thin exe binary built in 9E) and talk over stdio.
- **Secondary: Streamable HTTP** (9E polish). Disabled by default; user enables in `bld_config.toml`. Listens on `127.0.0.1:47111` (unprivileged, localhost-only). Authentication: a UUID token minted at first run, stored via `keyring`; clients present it via `Authorization: Bearer`.

The editor's chat panel uses neither transport. It calls `bld-mcp`'s in-process API directly through the `BldSession*` handle (Surface P v3 — see §2.4). This avoids marshalling JSON for the common case and keeps first-token latency low.

### 2.2 Capabilities

Advertised (declared in `bld-mcp/src/server.rs`):

```json
{
  "capabilities": {
    "tools":       { "listChanged": true },
    "resources":   { "subscribe": true, "listChanged": true },
    "prompts":     { "listChanged": true },
    "elicitation": { }
  },
  "serverInfo": { "name": "bld", "version": "0.9.0" },
  "protocolVersion": "2025-11-25"
}
```

`listChanged` on tools/resources/prompts fires when a tool/resource/prompt set changes at runtime — e.g. a hot-reloaded gameplay module re-registers its component-tools.

### 2.3 Elicitation modes — when to use which

BLD emits elicitations for two reasons: (a) **human approval** of a proposed action, (b) **secret input** the agent cannot see.

| Use case                         | Mode   | UI                              | Who enforces |
|----------------------------------|--------|----------------------------------|--------------|
| Confirm destructive tool         | form   | Diff panel + Approve / Deny      | `bld-governance` |
| Enter a provider API key         | url    | External browser -> token-write  | `bld-provider::secrets` |
| OAuth flow                       | url    | External browser                 | `bld-provider::secrets` |
| Choose among N options           | form   | Radio group                      | `bld-governance` |
| Accept a file-write              | form   | Unified diff in editor           | `bld-governance` |
| SSH/signing passphrase           | url    | External browser -> keyring-write | `bld-provider::secrets` |

**Enforcement.** `bld-mcp::elicit::validate_request` statically checks that any `ElicitationMode::Form` request does **not** carry fields matching a compile-time regex list (`password`, `api[_-]?key`, `secret`, `token`, `passphrase`, `credential`, `ssn`, `card[_-]?number`, CVV etc.). A match is a hard error — elicitation is rejected and an audit event is emitted. Defence in depth, not trust.

### 2.4 Surface P ABI bumps

Phase 9 requires two additive ABI bumps per ADR-0007 additive-only policy:

- **`BLD_ABI_VERSION = 2`** (lands in wave 9B). Adds:
  - `gw_editor_enumerate_components(EnumFn cb, void* user)` — iterates the reflection registry for `bld-tools` `component.*` autogen.
  - `gw_editor_run_command(uint32_t opcode, const void* payload, size_t payload_bytes)` — enqueue a serialised `ICommand` for the main thread to `CommandStack::execute`.
  - `gw_editor_run_command_batch(uint32_t opcode_count, const uint32_t* opcodes, const void* payloads, const size_t* payload_offsets)` — transaction bracket (multiple opcodes, single undo entry).
- **`BLD_ABI_VERSION = 3`** (lands in wave 9E). Adds:
  - `BldSession*` opaque handle (multi-session support).
  - `gw_editor_session_*` family for session create/destroy/route.

ADR-0007 is amended in a sibling commit to carry this versioning trail in the ABI version log.

### 2.5 Transport-loop discipline

Single `tokio` runtime (`Builder::new_multi_thread().worker_threads(2)`), owned by `bld-mcp`. All providers, RAG, watcher, and agent share it. No stray `std::thread::spawn`. This mirrors non-negotiable #10 on the C++ side — same policy, different language.

Cancellation is a `tokio_util::sync::CancellationToken` passed into every long-running call. Ctrl+C in an external MCP client flows through the transport's cancel signal into this token.

## 3. Alternatives considered

- **Custom JSON-RPC framing.** Rejected: MCP 2025-11-25 is the explicit wire protocol per `docs/00 §2.5`; we use `rmcp` verbatim.
- **gRPC transport.** Rejected: no demonstrated MCP interop today; cost not worth vs. Streamable HTTP.
- **Default Streamable HTTP.** Rejected: stdio is lower-latency for the in-process common case, and avoids the "localhost port is open" footprint for non-power-users.

## 4. Consequences

- External MCP clients (Claude Code, Cursor) can drive `gw_editor` with no new code on our side once the tool surface stabilises in 9B/9D.
- Secret-input flows cannot accidentally round-trip through `form`-mode elicitation — compile-time refusal.
- One audit record per elicitation (see ADR-0015) with `{mode, decision, duration_ms}`.

## 5. Follow-ups

- Streamable HTTP default-off in 9E; `bld_mcp_host` binary for external clients.
- Per-transport contract tests in `bld-mcp/tests/transport.rs`.

## 6. References

- MCP spec: <https://modelcontextprotocol.io/specification/2025-11-25>
- `rmcp`: <https://docs.rs/rmcp>
- Anthropic elicitation guidance: MCP draft-to-final notes, 2025-11.
