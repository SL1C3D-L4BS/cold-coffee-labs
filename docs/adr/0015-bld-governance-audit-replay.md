# ADR 0015 — BLD governance, audit, replay & secret filter policy

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 9 wave 9E opening)
- **Deciders:** Cold Coffee Labs engine group (Claude Opus 4.7 drafting; founder final)
- **Supersedes:** —
- **Superseded by:** —
- **Related:** `CLAUDE.md` non-negotiables #18 (no secrets in repo or model context); ADR-0007 (C-ABI freeze — Surface P v3 session handles); ADR-0011 (elicitation policy); ADR-0012 (tool tier annotation); ADR-0014 (agent loop).

## 1. Context

Wave 9E is where BLD graduates from "cool chat panel" to "shippable product". Three threads converge here:

1. **Governance.** Not every tool should run unattended. The agent must not `asset.delete` or `scene.destroy_entity` without confirmation; it should freely `docs.search` or `project.find_refs`.
2. **Audit.** Every tool call is a persisted event. Incident review, replay, and cost attribution depend on this being complete and tamper-evident.
3. **Secret filter.** A file-read path that resolves to `.env` cannot reach the model — not once, not ever. Defence in depth, not trust.

Failure of any of these is a *trust* failure, not merely a bug: secret leakage through a model context is reputation-ending for a studio.

## 2. Decision

### 2.1 Three-tier authorization

Every tool carries one of three tiers, set at `#[bld_tool(tier = "…")]` time (ADR-0012):

| Tier       | Elicitation default | Approval cache default | Examples                                                        |
|------------|---------------------|--------------------------|-----------------------------------------------------------------|
| `Read`     | None                | N/A                      | `docs.search`, `project.find_refs`, `scene.query`, `code.read_file` |
| `Mutate`   | Form                | 1 h per tool_id          | `scene.create_entity`, `component.set`, `code.apply_patch`, `vscript.write_ir` |
| `Execute`  | Form, per-call (no cache) | None              | `build.build_target`, `runtime.play`, `runtime.hot_reload`, `asset.delete` |

**Cache semantics.** On first approval of a `Mutate`-tier tool the user can tick "remember for this session (1 h)". Subsequent calls of *the same tool_id with comparable args* are approved silently. "Comparable" means structurally equivalent JSON minus timestamps; defined in `governance::args_are_comparable`. Never comparable across sessions.

**`Execute` is never cached.** Every invocation asks. Rationale: Execute-tier tools have real-world side effects (builds, launches, deletes) whose safety depends on *current* state, not "was safe an hour ago".

Users can **promote** a `Mutate` tool to always-approve (within a session) via a slash command `/grant scene.create_entity`. Promotions persist in `.greywater/agent/grants.toml` per-project if the user pins them.

### 2.2 Audit log

**Format.** Append-only JSONL at `.greywater/agent/audit/{session_id}/{yyyy-mm-dd}.jsonl`. One record per tool invocation. Records never rewritten — errors append a new record referencing the prior one by sequence number.

**Record shape:**

```json
{
  "seq":            129,
  "ts_utc":         "2026-04-24T18:44:02.147Z",
  "session_id":     "01HW3J9ZC9XN3NGTSTXQG0VZ7K",
  "turn_id":        "01HW3JA7R5P8Y6TVBDEJRC6X2K",
  "tool_id":        "scene.create_entity",
  "tier":           "Mutate",
  "args_digest":    "blake3-b64:Vv4o8m…",
  "args_bytes":     217,
  "result_digest":  "blake3-b64:9jF2xu…",
  "result_bytes":   64,
  "tier_decision":  "ApprovedCached",
  "elapsed_ms":     13,
  "provider":       "anthropic:claude-opus-4-7",
  "outcome":        "Ok"
}
```

**Digests.** BLAKE3 over the canonical JSON form (keys sorted, no whitespace). Full payloads are stored only when the per-session archive flag is **on** (off by default) in a sibling `.archive.jsonl` — because shipping every tool args dump to disk by default is a privacy and cost footgun.

**Rotation.** One file per UTC day per session. Sessions > 30 days old are eligible for compaction via `tools/bld_audit_compact` (9E polish or later).

### 2.3 Session replay — `gw_agent_replay`

A new binary shipped from `tools/bld_replay/`. Usage:

```
gw_agent_replay <session_id> [--from <seq>] [--to <seq>]
                [--dry-run] [--allow-mutate]
                [--bit-identical-gate]
```

Semantics:

- `--dry-run` (default): re-dispatches every tool call but replaces **every mutating-tier** tool with a no-op that records what *would have happened*. Output: a diff against the live scene/FS.
- `--allow-mutate`: actually re-applies mutations. Combined with `git stash && gw_agent_replay <id> --allow-mutate` this reproduces a session's end state on a clean checkout.
- `--bit-identical-gate`: asserts the resulting `.gwscene` + project file tree hashes match the originally-captured post-session tree hash (recorded at session end by the audit channel as a `session_end` record). Exit code 0 = bit-identical; non-zero = drift (printed diff).

The *bit-identical gate* is the replay leg of non-negotiable #16 (determinism). The eval harness (Phase 9 plan §5, row "Replay round-trip") uses `--bit-identical-gate` directly.

### 2.4 Secret filter

**Compile-time deny-list** (checked into `bld-governance/src/secret_filter.rs` as a const `&[&str]`):

```
.env          .env.*       *.key        *.pem        *_rsa        id_*
.ssh/*        .config/gcloud/*           .aws/credentials
*.pfx         *.p12        *.keystore   *.jks
*.htpasswd    secrets.yml  secrets.yaml docker-compose.override.yml
```

**Runtime `.agent_ignore`** at project root — gitignore semantics — extends the deny-list per project. Parsed once at session open; not watched (no race window at open).

**Enforcement.** Every file path entering a tool argument passes through `secret_filter::check(path)`. A hit returns `ToolError::Forbidden` and emits an audit event with `outcome: "SecretFilterHit"` and **no `args_digest`** (the path itself might leak content). Defence in depth: the filter runs *before* the tool dispatch, and every tool that takes a path MUST go through `ToolCtx::read_file` / `ToolCtx::write_file`, never `std::fs` directly. A clippy lint (custom, in `bld-tools/src/lint.rs`) blocks direct `std::fs::*` in any function marked `#[bld_tool]` — compile-time belt + runtime braces.

**Env-var filter.** Similar treatment for environment variables the agent tries to read: any env-name matching `(AWS|AZURE|GCP|ANTHROPIC|OPENAI|GEMINI|GITHUB|STRIPE|NPM)_[A-Z_]*` pattern is masked.

**Pre-commit hook.** `tools/hooks/bld_secret_scan.py` runs on every `git commit` of an agent-authored diff and re-scans the diff for secret patterns (AWS access keys, PEM blocks, JWT tokens, Discord webhooks, Slack tokens, Stripe keys). A hit blocks the commit. Docs reference in `BUILDING.md`.

### 2.5 Multi-session — Surface P v3

`BLD_ABI_VERSION = 3` (lands in 9E, additive-only per ADR-0007):

```c
typedef struct BldSession BldSession;  /* opaque */

/* Introduced in v3. */
BldSession* gw_editor_session_create(const char* display_name);

/* Introduced in v3. */
void gw_editor_session_destroy(BldSession* session);

/* Introduced in v3. */
const char* gw_editor_session_id(BldSession* session);  /* ULID string */

/* Introduced in v3. Route an arbitrary MCP request into a named session. */
const char* gw_editor_session_request(BldSession* session,
                                       const char* request_json);
```

Chat panel tabs each own a `BldSession*`. Sessions do **not** share: tool registries are shared globally (registered at `bld_register`), but each session has its own history, approval cache, audit stream, and agent `CancellationToken`. 

### 2.6 Slash commands as MCP Prompts

`/diagnose`, `/optimize`, `/explain`, `/test`, `/grant <tool>`, `/revoke <tool>`, `/cancel` — each a single template living in `bld-mcp/prompts/`. Editor uses `prompts/get` to fetch the filled prompt, `completion/complete` to resolve arguments.

A slash command is never auto-approved — it goes through the same tier gating as any other interaction.

### 2.7 Cross-panel integrations

Editor-side only (C++); the right-click menu + panel hook APIs live in `editor/panels/` and call `bld-mcp::ingress::send_with_resource` to pass the context resource (entity handle, diagnostic, profile sample, vscript node slice) alongside the user prompt.

Wired in 9E:
- Outliner → "Ask BLD about this entity" → resource `entity://<handle>`.
- Console compile error → "Fix this" → resource `diagnostic://<error_id>`, tool list primed to `code.*`.
- Stats panel frame-spike click → "Diagnose" → resource `profile://<sample_id>`.
- VScript panel node → "Explain this node" → resource `vscript_node://<graph>:<node_id>`.

## 3. Alternatives considered

- **Binary audit log.** Rejected: JSONL is human-greppable during an incident; binary needs a tool to read.
- **Always archive full payloads.** Rejected: privacy + disk. Opt-in preserves the choice without making it the default.
- **Sole runtime `.agent_ignore`, no compile-time deny-list.** Rejected: a user without an ignore file would be undefended. Defaults must be safe.
- **Session replay against the live scene.** Rejected: the clean-checkout replay is the only way to prove bit-identical.

## 4. Consequences

- Every agent action is a persistent, diffable record.
- Every destructive action is reversible (CommandStack) and replayable.
- No secret-shaped path reaches the model.
- Multi-session is first-class; tabs cannot cross-contaminate.

## 5. Follow-ups

- Phase 16 (compliance) will fold BLD audit into the GDPR data-collection UI.
- Phase 24 (hardening) runs `cargo-mutants` against `bld-governance`; any surviving mutant blocks release.

## 6. References

- ULID for session IDs (crate `ulid`).
- BLAKE3 for digests (crate `blake3`).
- MCP 2025-11-25 prompts & completion spec.

## 7. Acceptance notes (2026-04-21, wave 9E close)

- Slash commands now parse and execute in-crate via
  `bld_governance::slash::{SlashCommand, SlashDispatcher}`. Wave 9E
  ships `/grant`, `/revoke`, `/reset`, `/offline`, `/model`,
  `/sessions`, `/help`; the remaining MCP-prompt slash commands
  (`/diagnose`, `/optimize`, `/explain`, `/test`, `/cancel`) stay in
  `bld-mcp/prompts/` per §2.6 and are not in scope for this wave.
- `SessionManager` realises §2.5 multi-session isolation in Rust
  first — each `SessionHandle` owns its own `Governance`,
  `AuditLog`, mutable `SessionContext` (offline flag, selected
  model, panel context), and `KeepAlive` heartbeat. The C-ABI
  `BldSession*` wrappers layer over this manager in `bld-bridge`.
- `ElicitationFactory` is the seam for per-session elicitation
  handlers so cross-panel context (`entity://`, `diagnostic://`,
  `profile://`, `vscript_node://`) can be injected before
  `Governance::require` runs.
- `gw_agent_replay` CLI gained `--json`, `--bit-identical`,
  `--limit N`, `--filter <prefix>`, and `-q/--quiet` in the same
  wave; CI now consumes the JSON output and the `--bit-identical`
  flag is required in release pipelines.
- Secret filter, audit log, and pre-commit hook changes already
  landed in wave 9A — no revisions in 9E.
