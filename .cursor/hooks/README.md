# .cursor/hooks — ECC 15-Event Hook Chain (ADR-0119)

This directory is populated by the ECC install flow documented in
`tools/ai/README.md`. Each file is a small executable (shell / Node /
PowerShell) that fires on one of the 15 Cursor agent events:

| Event | File | Budget | Purpose |
|-------|------|--------|---------|
| `beforeSessionStart` | `01-before-session.sh` | 250 ms | Check `graphify-out/graph.json` freshness. |
| `afterSessionStart` | `02-after-session.sh` | 250 ms | Emit ECC session manifest row. |
| `beforeUserMessage` | `03-before-user-message.sh` | 100 ms | Redact secrets (API keys, Ed25519 pk). |
| `beforeModelRequest` | `04-before-model-request.sh` | 100 ms | Attach AgentShield session id. |
| `afterModelResponse` | `05-after-model-response.sh` | 100 ms | Capture response for replay. |
| `beforeToolCall` | `06-before-tool-call.sh` | 250 ms | Enforce `blocked_paths` in `agentshield.json`. |
| `afterToolCall` | `07-after-tool-call.sh` | 100 ms | Record tool result to session manifest. |
| `beforeFileEdit` | `08-before-file-edit.sh` | 250 ms | Shader reg / crypto / ADR gate (see `.cursor/rules/ecc.mdc`). |
| `afterFileEdit` | `09-after-file-edit.sh` | 100 ms | Re-run clang-format for touched C++ TUs. |
| `beforeShellExec` | `10-before-shell-exec.sh` | 250 ms | Block destructive commands outside the session manifest. |
| `afterShellExec` | `11-after-shell-exec.sh` | 100 ms | Stream stdout tail into the session log. |
| `beforeCommit` | `12-before-commit.sh` | 250 ms | AgentShield pre-commit policy check. |
| `afterCommit` | `13-after-commit.sh` | 250 ms | Run `graphify build` (via `tools/ai/hooks/post-commit`). |
| `onSessionEnd` | `14-on-session-end.sh` | 250 ms | Flush session manifest, close AgentShield span. |
| `onError` | `15-on-error.sh` | 100 ms | Forward to Sentry telemetry (opt-in). |

If a hook exceeds its budget, Cursor logs a warning and skips it for
the rest of the session. Measured P99 on a reference RX 580 dev box is
the source of truth — not local laptop benchmarks.

The hooks are **optional**: without them the repo still works, but
AgentShield / ECC checks degrade to "advisory only" in the PR workflow.
