#!/usr/bin/env python3
"""Polish the 11-file docs/ suite: canonical paths, merge banners, rituals."""

from __future__ import annotations

import re
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
DOCS = REPO / "docs"

# (old, new) — order: longer strings first where relevant.
GLOBAL_SUBS: list[tuple[str, str]] = [
    ("05_ROADMAP_AND_MILESTONES.md", "02_ROADMAP_AND_OPERATIONS.md"),
    ("`05_ROADMAP_AND_MILESTONES`", "`02_ROADMAP_AND_OPERATIONS`"),
    # Do NOT globally replace legacy filenames that appear in "## Merged from `…`" banners
    # inside `01_CONSTITUTION_AND_PROGRAM.md` (10_PRE_, 11_HANDOFF_, MASTER_PLAN_BRIEF, 00_SPEC_).
    ("NETCODE_AND_DETERMINISM_CONTRACT.md", "09_NETCODE_DETERMINISM_PERF.md"),
    ("02_SYSTEMS_AND_SIMULATIONS.md", "04_SYSTEMS_INVENTORY.md"),
    ("`02_SYSTEMS_AND_SIMULATIONS`", "`04_SYSTEMS_INVENTORY`"),
    ("BLACKLAKE_FRAMEWORK_BLF-CCL-001-REV-A.md", "08_BLACKLAKE_AND_RIPPLE.md"),
    ("GREYWATER_ENGINE_GWE-ARCH-001-REV-A_Ripple-IDE-Spec.md", "08_BLACKLAKE_AND_RIPPLE.md"),
    ("Modify operational docs (`05`, `06`, `07`, `11`, `daily/*`)", "Update `docs/02_ROADMAP_AND_OPERATIONS.md` and `docs/01_CONSTITUTION_AND_PROGRAM.md`"),
    ("listed under it in `05`,", "listed under it in `02`,"),
    ("every Tier A row listed under it in `05`", "every Tier A row listed under it in `02`"),
    ("`docs/02_ROADMAP_AND_OPERATIONS.md*.md`", "`docs/02_ROADMAP_AND_OPERATIONS.md`"),
    ("(see `daily/README.md`)", "(daily logs removed in consolidation — use git history)"),
    ("The per-week rows in `05` and the per-day rows in `10 §3`", "The per-week rows in `02` and the per-day rows in `01` §3"),
    ("(non-exhaustive; tiers and phases in `02` / `05`):", "(non-exhaustive; tiers in `docs/04_SYSTEMS_INVENTORY.md`, phases in `docs/02_ROADMAP_AND_OPERATIONS.md`):"),
    (
        "- Modify canonical docs (`00`, `01`, `10`, `12`, `CLAUDE.md`, `docs/06_ARCHITECTURE.md*`) without explicit user instruction.",
        "- Modify canonical docs (`docs/01_CONSTITUTION_AND_PROGRAM.md`, `docs/03_PHILOSOPHY_AND_ENGINEERING.md`, `CLAUDE.md`, `docs/06_ARCHITECTURE.md`) without explicit user instruction.",
    ),
    (
        "- Write ADRs under `docs/10_APPENDIX_ADRS_AND_REFERENCES.md` for any decision that crosses a non-negotiable or changes an architectural invariant.",
        "- Record new ADRs in `docs/10_APPENDIX_ADRS_AND_REFERENCES.md` (append using the ADR template there) for any decision that crosses a non-negotiable or changes an architectural invariant.",
    ),
]

DAILY_BLOCK = """## Daily ritual (10 minutes)

At the start of every coding session, before writing code:

1. **Orient** — `git log --oneline -10` and yesterday’s commits: what shipped, what stalled?
2. **WIP hygiene** — Check Kanban **In Progress** (§Columns). Is either card blocked or stale (>2 days idle)?
3. **Pull** — If In Progress `< 2`, take the highest-priority card from **Next Sprint** (dependencies satisfied).
4. **Commit to focus** — Note the active card ID in your session scratch space (editor sticky / team board). *The legacy `docs/daily/` tree was removed in the 11-file consolidation; use commits as the audit trail.*
5. **Triage** — New dependency discovered? Add it to **Backlog** immediately.
6. **Deep work** — One **90-minute** focused block: no Kanban reshuffling mid-block.

---

## End-of-day ritual (5 minutes)

1. **Self-review** — Before **Review**, run `docs/03_PHILOSOPHY_AND_ENGINEERING.md` §J on your own diff.
2. **Move the card** — Done + criteria met? → **Review**. Still working? → stays **In Progress** with a note. Blocked? → **Blocked** with `Reason:`.
3. **Record** — Summarize in the PR or session notes: completed tasks, blockers, decisions, next intention. **Commit** with Conventional Commits (`feat(area):`, `fix(area):`, …).
"""


def polish_file(path: Path) -> bool:
    text = path.read_text(encoding="utf-8", errors="strict")
    orig = text

    for old, new in GLOBAL_SUBS:
        text = text.replace(old, new)

    if path.name == "02_ROADMAP_AND_OPERATIONS.md":
        text = text.replace(
            "## Merged from `docs/02_ROADMAP_AND_OPERATIONS.md`\n\n# 06_KANBAN_BOARD",
            "## Merged from `06_KANBAN_BOARD.md`\n\n# 06_KANBAN_BOARD",
        )
        text = text.replace(
            "reconcile detailed week rows with `docs/07_SACRILEGE.md` as that folder grows",
            "reconcile detailed week rows with `docs/07_SACRILEGE.md` as that document evolves",
        )
        # Replace daily ritual section
        m = re.search(
            r"## Daily ritual \(10 minutes\).*?## Weekly ritual",
            text,
            flags=re.DOTALL,
        )
        if m:
            text = text[: m.start()] + DAILY_BLOCK + "\n## Weekly ritual" + text[m.end() :]
        text = text.replace(
            "1. Read the last seven daily logs.",
            "1. Review the last seven **sessions** (commits + merged PRs).",
        )

    if path.name == "04_SYSTEMS_INVENTORY.md":
        text = text.replace(
            "## Merged from `docs/04_SYSTEMS_INVENTORY.md`\n\n# 02_SYSTEMS_AND_SIMULATIONS",
            "## Merged from `02_SYSTEMS_AND_SIMULATIONS.md`\n\n## Former title: 02_SYSTEMS_AND_SIMULATIONS",
        )

    if path.name == "03_PHILOSOPHY_AND_ENGINEERING.md":
        text = text.replace(
            "## Merged from `docs/03_PHILOSOPHY_AND_ENGINEERING.md`\n\n# 01_ENGINE_PHILOSOPHY",
            "## Merged from `01_ENGINE_PHILOSOPHY.md`\n\n# 01_ENGINE_PHILOSOPHY",
            1,
        )
        text = text.replace(
            "## Merged from `docs/03_PHILOSOPHY_AND_ENGINEERING.md`\n\n# 12_ENGINEERING_PRINCIPLES",
            "## Merged from `12_ENGINEERING_PRINCIPLES.md`\n\n# 12_ENGINEERING_PRINCIPLES",
            1,
        )

    if path.name == "10_APPENDIX_ADRS_AND_REFERENCES.md":
        text = text.replace(
            "**Phase (1–11).** The eleven-stage buildout in `05_ROADMAP_AND_MILESTONES`.",
            "**Phase (1–25).** The full buildout through LTS is in `docs/02_ROADMAP_AND_OPERATIONS.md`.",
        )
        # ADR 0083/0084 stale bullets
        text = text.replace(
            "- **Pre-generated** `docs/02_ROADMAP_AND_OPERATIONS.md` may still mention old phase *names* in headers; operational truth is **`02_`** (daily logs removed in consolidation — use git history).  \n",
            "- **Schedule truth** lives in `docs/02_ROADMAP_AND_OPERATIONS.md` (this file subsumes the former roadmap + Kanban).  \n",
        )

    if text != orig:
        path.write_text(text, encoding="utf-8", newline="\n")
        return True
    return False


def main() -> None:
    changed = []
    for p in sorted(DOCS.glob("*.md")):
        if polish_file(p):
            changed.append(p.name)
    print("Polished:", ", ".join(changed) if changed else "(no changes)")


if __name__ == "__main__":
    main()
