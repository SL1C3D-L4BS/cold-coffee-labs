# ECC Dashboard (ADR-0119)

The ECC toolkit ships a small dashboard that summarises agent activity
across harnesses. Once the install flow in `tools/ai/README.md` has
run, the dashboard is available via:

```powershell
# Cursor
node third_party/everything-claude-code/packages/dashboard/bin/cli.js \
  --root . --open

# Headless (CI)
node third_party/everything-claude-code/packages/dashboard/bin/cli.js \
  --root . --format=json > .cursor/dashboard.json
```

## Metrics surfaced

- Session count + average session duration per harness.
- AgentShield policy hits (warnings vs blocks).
- Graphify freshness (seconds since last `graphify build`).
- Hook budget compliance — any hook that exceeded its 250 ms budget
  in the last 7 days.
- Commit trailer compliance — percentage of AI-assisted commits that
  carried an `Agent-Session:` trailer.
- Multi-harness fan-out calls via `ccg-workflow`.

## Alerts

Dashboard writes a JSON file at `.cursor/dashboard.json` when run in
headless mode; CI can ingest it to post a comment on every PR. The
alerts policy is defined in `.cursor/agentshield.json`.
