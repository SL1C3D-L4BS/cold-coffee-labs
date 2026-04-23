---
id: security-scan
description: Run AgentShield + the ECC security-scan skill against the working tree.
---

# /security-scan

Runs a full security sweep combining:

1. **AgentShield (policy gate)** — local `tools/ai/agentshield_cli.js`
   against `.cursor/agentshield.json`. Identical to the gate enforced
   by `.github/workflows/agentshield.yml` on PR. Checks blocked-path
   edits, ADR-reference requirements, per-session volume caps, and
   commit-trailer presence.

2. **ECC security-scan skill** — rules in `.cursor/skills/security-scan/`.
   Flags hard-coded secrets, insecure crypto patterns, SSRF-adjacent
   fetches, and OWASP Top-10 shape issues in the current diff.

## Usage

```powershell
# Local advisory run (no exit on violation)
node tools/ai/agentshield_cli.js `
  --config .cursor/agentshield.json `
  --base origin/main --head HEAD `
  --fail-on none

# CI-equivalent run (hard-fail on critical)
node tools/ai/agentshield_cli.js `
  --config .cursor/agentshield.json `
  --base origin/main --head HEAD `
  --fail-on critical
```

For the ECC security-scan skill, invoke the skill directly from chat
(`read .cursor/skills/security-scan/SKILL.md` then follow its runbook)
or run it via the `security-reviewer` subagent listed in `AGENTS.md`.

## What counts as a violation (from `.cursor/agentshield.json` v1.2)

- `blocked_paths` — files under `assets/ai/**/*.bin`,
  `bld/bld-governance/policy/**`, `engine/core/crypto/**`,
  `.cursor/agentshield.json`, or `.github/workflows/**` touched
  without an ADR-dddd reference in the commit range OR a
  `Waiver: <planning-doc>` trailer.
- `require_adr_reference_for` — edits to
  `docs/01_CONSTITUTION_AND_PROGRAM.md`,
  `docs/02_ROADMAP_AND_OPERATIONS.md`,
  `docs/10_APPENDIX_ADRS_AND_REFERENCES.md`, or `CMakePresets.json`
  without an ADR-dddd reference.
- `max_files_per_session = 200`, `max_lines_per_session = 15000` —
  vendor / generated paths (`third_party/**`, `.cursor/**`,
  harness adapter dirs, `graphify-out/**`) are exempt.
- `enforce_commit_trailer = true` — every commit in the range must
  carry `Agent-Session: <id>`.

## Related

- `ADR-0120` — policy rationale (`docs/10_APPENDIX_ADRS_AND_REFERENCES.md`).
- `.cursor/skills/security-scan/SKILL.md` — ECC skill body.
- `.github/workflows/agentshield.yml` — CI enforcement.
