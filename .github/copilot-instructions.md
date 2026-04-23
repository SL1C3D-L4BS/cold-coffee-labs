# GitHub Copilot instructions (Cold Coffee Labs)

> Copilot reads this file automatically. It is the shortest possible
> pointer to the full agent-facing playbook for this repo.

## Start every non-trivial task by reading these, in order

1. [`CLAUDE.md`](../CLAUDE.md) — top-of-file protocol (rules, graphify
   check, ECC workflow menu).
2. [`AGENTS.md`](../AGENTS.md) — registered agents and when to invoke
   which one.
3. [`graphify-out/GRAPH_REPORT.md`](../graphify-out/GRAPH_REPORT.md) —
   current knowledge-graph summary. Consult before grep / semantic
   search per ADR-0118.
4. `docs/10_APPENDIX_ADRS_AND_REFERENCES.md` — architectural decision
   records; ADR-0118 / 0119 / 0120 are load-bearing for any AI-tooling
   change.
5. The relevant `.cursor/rules/ecc-<lang>.md` rule pack for the file
   type you are editing.

## Non-negotiable gates

- **AgentShield** (`.cursor/agentshield.json` + CI) rejects edits to
  determinism / fuzz / workflows / presets without an ADR reference in
  the commit trailer (policy v1.1).
- **Graphify freshness** (`.git/hooks/post-commit`) rebuilds the
  knowledge graph after every commit. Do not bypass the hook.
- **ECC 15-event hook chain** (`.cursor/hooks/README.md`) fires on
  pre-commit, pre-edit-shader, pre-test, etc. Hooks must finish under
  250 ms; measure on the reference RX 580 dev box, not on laptop.

## Harness parity

The project is installed for Cursor, Claude Code CLI, Codex CLI,
OpenCode, Gemini CLI, Kiro, Trae, and GitHub Copilot. Each has its
own adapter directory (`.cursor/`, `.codex/`, `.gemini/`, `.kiro/`,
`.opencode/`, `.trae/`, `.agents/`). The canonical rule pack lives
in `third_party/everything-claude-code/` at pinned SHA per ADR-0119;
all adapters are generated from that source.

When in doubt, prefer the Claude / Cursor playbook — it is the
primary authoring surface.
