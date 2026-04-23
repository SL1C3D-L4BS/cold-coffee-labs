# tools/ai — AI tooling rails

This directory pins the optional-but-recommended AI tooling layered on top
of the Greywater / Sacrilege workspace. Everything here is *agent-facing*,
not runtime code: it improves Cursor / Claude / Codex output quality when
editing the repo but never ships with the game.

Install order (authoritative — also tracked as DAG edges in
`docs/prompts/pack.yml`):

1. `graphify` (ADR-0118) — code-to-knowledge-graph indexer. Builds
   `graphify-out/graph.json` + `GRAPH_REPORT.md` that agents read
   before grepping.
2. `everything-claude-code` / ECC (ADR-0119) — cross-harness skill /
   hook / rule bundle. Wires Cursor hooks, multi-agent adapters, and
   AgentShield into the repo.
3. `penpax` (waitlist) — commercial knowledge-graph service tracked
   here so we adopt it the moment the waitlist opens.

See `docs/10_APPENDIX_ADRS_AND_REFERENCES.md` §ADR-0118 / §ADR-0119 for
the rationale and acceptance criteria.

## Install

### Prereqs

- Python 3.12 + pipx on PATH (CPython, not Conda). On Windows, install
  via `winget install Python.Python.3.12` and then
  `py -m pip install --user pipx && py -m pipx ensurepath`.
- Git 2.40+ (already required by the repo).
- Node 20+ for ECC harness adapters (step 2 only).

### graphify

```powershell
# Pin to the version in requirements.txt (ADR-0118).
pipx install "graphifyy==0.4.31"
# Drop the Cursor skill rule into .cursor/rules/graphify.mdc.
graphify install --platform cursor
# Run the initial code-only graph extract (no LLM needed).
graphify update .
# Install the post-commit + post-checkout git hooks that keep the graph fresh.
graphify hook install
```

`graphify update <path>` parses the repo via tree-sitter and emits
`graphify-out/{graph.json, graph.html, GRAPH_REPORT.md}` plus a per-file
`graphify-out/cache/`. Commit only `graph.json` + `GRAPH_REPORT.md`; the
HTML, cache, manifest, cost, memory, and raw subdirs are `.gitignore`d.

For doc / PDF / image / video ingestion (uses Claude + Whisper APIs),
run `graphify add <url>` or trigger `/graphify --update` from your
Cursor / Claude assistant — both require `ANTHROPIC_API_KEY` in env.

**Tracked-copy backup.** `tools/ai/hooks/post-commit` + `post-checkout`
are byte-for-byte copies of what `graphify hook install` drops into
`.git/hooks/`. If a contributor wipes `.git/hooks/`, restore with:

```powershell
Copy-Item tools/ai/hooks/post-commit   .git/hooks/post-commit   -Force
Copy-Item tools/ai/hooks/post-checkout .git/hooks/post-checkout -Force
```

### everything-claude-code (ECC)

ECC v1.10.0 is vendored under `third_party/everything-claude-code/` at
pinned SHA `846ffb75da9a5f4e677d927af1ad4a1951652267` (ADR-0119). The
installer uses upstream's actual `--target` flag, not the older
`--scope` flag documented in some drafts:

```powershell
# One-time Node dep fetch (~200 packages, ~55 MB).
Push-Location third_party/everything-claude-code
npm install --no-audit --no-fund --loglevel=error
Pop-Location

# User-global install (~/.claude/{agents,skills,commands,rules,hooks,...}).
node third_party/everything-claude-code/scripts/install-apply.js `
  --target claude --profile full

# Project install (.cursor/{agents,skills,commands,rules,hooks,...}).
node third_party/everything-claude-code/scripts/install-apply.js `
  --target cursor --profile full
```

Both runs write an `install-state.json` (user-global at
`~/.claude/ecc/install-state.json`, project at
`.cursor/ecc-install-state.json`) recording the exact modules applied
so reinstalls are idempotent and uninstalls are reversible.

Harness adapter dirs (`.codex/`, `.opencode/`, `.gemini/`, `.kiro/`,
`.trae/`, `.agents/`) at the repo root are populated verbatim from the
vendor tree so Codex / OpenCode / Gemini CLI / Kiro / Trae sessions
pick up the same rule pack.

### multi-* slash commands

Installed to both `~/.claude/commands/` and `.cursor/commands/` by the
ECC `--profile full` run:

| Command | Purpose |
|---------|---------|
| `/multi-plan` | Fan a plan out to Claude + Codex + Gemini for cross-model review. |
| `/multi-execute` | Dispatch the same implementation prompt to all three and diff outputs. |
| `/multi-backend` | Backend-tuned multi-model pipeline. |
| `/multi-frontend` | Frontend-tuned multi-model pipeline. |
| `/multi-workflow` | Full `plan -> execute -> verify` loop across harnesses. |

The runtime wrapper ships as `ccg` (npm package `ccg-workflow`).
Initialize interactively with `npx ccg-workflow init --lang en`; for
CI / headless use, it reads `~/.claude/.ccg/config.json`.

### MCP servers (`.cursor/mcp.json`)

Merged from `third_party/everything-claude-code/mcp-configs/mcp-servers.json`.
Seven servers are wired to env vars instead of inline secrets:

| Server | Env var(s) | Notes |
|--------|------------|-------|
| `github` | `GITHUB_PERSONAL_ACCESS_TOKEN` | PRs, issues, repos. |
| `context7` | — | Live docs lookup; no auth. |
| `exa` | `EXA_API_KEY` | Web research; disable via `ECC_DISABLED_MCPS=exa`. |
| `memory` | — | Local sqlite at `.cursor/memory.sqlite`. |
| `playwright` | — | Browser automation / e2e. |
| `sequential-thinking` | — | Chain-of-thought; no auth. |
| `supabase` | `SUPABASE_URL`, `SUPABASE_ANON_KEY` | Optional; disable via `ECC_DISABLED_MCPS=supabase`. |

Set env vars at the OS or shell level; `.cursor/mcp.json` resolves
`${env:VAR}` at session start. The `ECC_DISABLED_MCPS` comma list
strips servers at startup (see `.cursor/hooks/session-start.js`).

### Environment variable map (full)

| Var | Required by | Default |
|-----|-------------|---------|
| `ANTHROPIC_API_KEY` | `graphify add <url>`, `/graphify --update`, `ccg` fan-out | — |
| `OPENAI_API_KEY` | `ccg` Codex leg (optional) | — |
| `GEMINI_API_KEY` | `ccg` Gemini leg (optional) | — |
| `GITHUB_PERSONAL_ACCESS_TOKEN` | `github` MCP, `gh` PR commands | — |
| `EXA_API_KEY` | `exa` MCP (optional) | — |
| `SUPABASE_URL` + `SUPABASE_ANON_KEY` | `supabase` MCP (optional) | — |
| `ECC_HOOK_PROFILE` | `.cursor/hooks/adapter.js` — pick event budget profile | `minimal` |
| `ECC_DISABLED_HOOKS` | Comma list of hook files to skip (e.g. `before-shell-execution`) | — |
| `ECC_DISABLED_MCPS` | Comma list of MCP servers to strip at session start | — |
| `ANTHROPIC_BASE_URL` | Route Claude / `ccg` calls to a proxy / relay | Anthropic cloud |
| `GRAPHIFY_GRAPH_PATH` | Override graph location for MCP / CI | `graphify-out/graph.json` |
| `AGENTSHIELD_CONFIG` | Override AgentShield policy path | `.cursor/agentshield.json` |

## Penpax (waitlist)

Penpax is a hosted code-knowledge-graph service that complements
graphify's local index with team-wide semantic search. We are on the
waitlist as of 2026-04-22; when access opens, follow their `penpax init`
CLI and point it at `graphify-out/graph.json` so the two layers stay in
sync. Tracked by `penpax-waitlist` in `docs/prompts/pack.yml`.

## One-shot install / rollback (Tracks C1–C5)

Re-run the full **graphify + ECC + ccg-workflow** rail on a new machine
(idempotent: safe to run twice):

- **Windows:** `pwsh tools/ai/install_ecc.ps1`  
  Optional switches: `-SkipGraphify`, `-SkipECC`, `-SkipHooks`, `-DryRun`
- **macOS / Linux:** `bash tools/ai/install_ecc.sh`  
  Optional flags: `--skip-graphify`, `--skip-ecc`, `--skip-hooks`, `--dry-run`

Both scripts assert `third_party/everything-claude-code/VERSION` matches
`1.10.0` (ADR-0119) before mutating anything.

**Reverse** user-global and project-scoped installs (uses upstream
`scripts/uninstall.js`):

- `pwsh tools/ai/uninstall_ecc.ps1` or `pwsh tools/ai/uninstall_ecc.ps1 -DryRun`
- `bash tools/ai/uninstall_ecc.sh` or `DRY_RUN=1 bash tools/ai/uninstall_ecc.sh`

**Status dashboard** (read-only; stdlib Python):

- `python tools/ai/ecc_dashboard.py` — human table  
- `python tools/ai/ecc_dashboard.py --json` — machine-readable

## Files

- `requirements.txt` — pinned Python tooling (graphify).
- `README.md` — this file.
- `install_ecc.ps1` / `install_ecc.sh` — orchestrated install.
- `uninstall_ecc.ps1` / `uninstall_ecc.sh` — orchestrated uninstall.
- `ecc_dashboard.py` — health / inventory summary.
- `agentshield_cli.js` — local AgentShield policy CLI (CI + `/security-scan`).
- `hooks/` — tracked copies of the post-commit / post-checkout hooks
  that `graphify hook install` drops into `.git/hooks/`.
- `../../graphify-out/` — generated by `graphify update .`; commit
  `graph.json` + `GRAPH_REPORT.md`, ignore the rest (see `.gitignore`).
- `../../.cursor/rules/graphify.mdc` — rule instructing every agent to
  consult `graphify-out/GRAPH_REPORT.md` before grepping.
- `../../.graphifyignore` — graphify-side ignore list.
