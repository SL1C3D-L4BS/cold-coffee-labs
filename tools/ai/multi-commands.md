# ECC `multi-*` Commands (ADR-0119)

The ECC `ccg-workflow` package exposes cross-harness "fan-out" commands
so a single prompt can be executed against Cursor + Claude Code CLI +
Codex CLI + Gemini CLI simultaneously and the results merged.

Invocation (once the install flow in `tools/ai/README.md` has run):

```powershell
node third_party/everything-claude-code/packages/ccg-workflow/bin/cli.js \
  multi-review --target docs/prompts/preflight/pre-ed-theme-menu.md \
               --harnesses cursor,claude,codex
```

## Commands we rely on

| Command | Use case |
|---------|----------|
| `multi-review` | Cross-harness review of a diff; surfaces contradictions. |
| `multi-implement` | Fan out an implementation prompt; best-of-n over harnesses. |
| `multi-test` | Run generated tests across harnesses; collect pass/fail matrix. |
| `multi-explain` | Diff explanations from each harness for debugging divergences. |

## Safety rails

- Each harness runs in a read-only workspace by default; `multi-implement`
  explicitly opts a harness into write mode.
- Results are stored under `.cursor/multi/<run-id>/` and cleaned by the
  `onSessionEnd` hook.
- AgentShield enforces `max_files_per_session` and `max_lines_per_session`
  on the merged output, not on individual harness outputs.
