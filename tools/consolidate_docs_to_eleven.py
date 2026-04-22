#!/usr/bin/env python3
"""
One-shot: rebuild docs/ as exactly 11 Markdown files (README + 01–10).

Run from repo root:
  python tools/consolidate_docs_to_eleven.py

Reads the existing docs tree, writes merged files, deletes everything else under docs/.
"""

from __future__ import annotations

import re
import shutil
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
DOCS = REPO / "docs"
STAGE = REPO / ".docs_eleven_stage"


def read_text(rel: str) -> str:
    p = DOCS / rel
    if not p.is_file():
        return f"\n\n> *(Missing source `{rel}`.)*\n\n"
    return p.read_text(encoding="utf-8", errors="replace")


def banner(rel: str) -> str:
    return f"\n\n---\n\n## Merged from `{rel}`\n\n"


def main() -> None:
    if STAGE.exists():
        shutil.rmtree(STAGE)
    STAGE.mkdir(parents=True)

    # --- README.md ---
    readme = f"""# Greywater Engine — documentation (11-file suite)

**Program:** **Cold Coffee Labs** builds **Greywater Engine**. Greywater Engine presents ***Sacrilege*** as the debut title and forcing function.

**Production bar:** **Supra-AAA** — see `01_CONSTITUTION_AND_PROGRAM.md` §0.1 (merged from the former `00_SPECIFICATION`).

This folder intentionally contains **exactly eleven Markdown files** (this `README` plus `01`–`10`). Subfolders, daily logs, and per-topic ADR files were **merged into** `10_APPENDIX_ADRS_AND_REFERENCES.md` and **removed**; recover prior layout from git history if needed.

---

## Reading order

| Step | File | Role |
| ---- | ---- | ---- |
| 1 | `01_CONSTITUTION_AND_PROGRAM.md` | Binding rules, executive brief, handoff |
| 2 | `02_ROADMAP_AND_OPERATIONS.md` | Phases, milestones, Kanban |
| 3 | `03_PHILOSOPHY_AND_ENGINEERING.md` | Brew doctrine + review principles |
| 4 | `04_SYSTEMS_INVENTORY.md` | Subsystem tiers |
| 5 | `05_RESEARCH_BUILD_AND_STANDARDS.md` | Vulkan/C++/Rust guides, build, coding standards |
| 6 | `06_ARCHITECTURE.md` | Blueprint + core architecture narrative |
| 7 | `07_SACRILEGE.md` | Flagship program spec (GW-SAC) |
| 8 | `08_BLACKLAKE_AND_RIPPLE_STUDIO_SPECS.md` | BLF + GWE×Ripple long-form studio specs |
| 9 | `09_NETCODE_DETERMINISM_PERF.md` | Netcode contract + perf/determinism annexes |
| 10 | `10_APPENDIX_ADRS_AND_REFERENCES.md` | Glossary, research appendix, **all ADRs** |

---

## Information architecture (formerly `DOCUMENTATION_SYSTEM.md`)

| Layer | Truth |
| ----- | ----- |
| L0 | This `README.md` |
| L1 | `01_CONSTITUTION_AND_PROGRAM.md` |
| L2 | `03_PHILOSOPHY_AND_ENGINEERING.md` |
| L3 | `06_ARCHITECTURE.md` |
| L3b | `08_BLACKLAKE_AND_RIPPLE_STUDIO_SPECS.md` |
| L4 | `07_SACRILEGE.md` |
| L5 | `04_SYSTEMS_INVENTORY.md`, `09_NETCODE_DETERMINISM_PERF.md` |
| L6 | `02_ROADMAP_AND_OPERATIONS.md`, `05_RESEARCH_BUILD_AND_STANDARDS.md` |
| L7 | ADR bodies live in §ADRs inside `10_APPENDIX_ADRS_AND_REFERENCES.md` |

**Precedence:** `01_` → `07_` → `08_` → `06_` → `04_`.

---

*Cold Coffee Labs — Greywater Engine — Sacrilege.*
"""
    (STAGE / "README.md").write_text(readme, encoding="utf-8")

    # --- 01 ---
    parts = [
        "# 01 — Constitution, brief & session handoff\n\n",
        banner("00_SPECIFICATION_Claude_Opus_4.7.md"),
        read_text("00_SPECIFICATION_Claude_Opus_4.7.md"),
        banner("DOCUMENTATION_SYSTEM.md"),
        read_text("DOCUMENTATION_SYSTEM.md"),
        banner("MASTER_PLAN_BRIEF.md"),
        read_text("MASTER_PLAN_BRIEF.md"),
        banner("10_PRE_DEVELOPMENT_MASTER_PLAN.md"),
        read_text("10_PRE_DEVELOPMENT_MASTER_PLAN.md"),
        banner("11_HANDOFF_TO_CLAUDE_CODE.md"),
        read_text("11_HANDOFF_TO_CLAUDE_CODE.md"),
    ]
    (STAGE / "01_CONSTITUTION_AND_PROGRAM.md").write_text("".join(parts), encoding="utf-8")

    # --- 02 ---
    (STAGE / "02_ROADMAP_AND_OPERATIONS.md").write_text(
        "# 02 — Roadmap & operations\n\n"
        + banner("05_ROADMAP_AND_MILESTONES.md")
        + read_text("05_ROADMAP_AND_MILESTONES.md")
        + banner("06_KANBAN_BOARD.md")
        + read_text("06_KANBAN_BOARD.md")
        + "\n\n---\n\n## Daily work logs\n\n"
        + "Pre-generated `docs/daily/*.md` files were removed in this consolidation. "
        + "Use `git log` and team rituals from the Kanban section above. "
        + "The former generator lived at `docs/07_DAILY_TODO_GENERATOR.py` (see git history).\n",
        encoding="utf-8",
    )

    # --- 03 ---
    (STAGE / "03_PHILOSOPHY_AND_ENGINEERING.md").write_text(
        "# 03 — Philosophy & engineering principles\n\n"
        + banner("01_ENGINE_PHILOSOPHY.md")
        + read_text("01_ENGINE_PHILOSOPHY.md")
        + banner("12_ENGINEERING_PRINCIPLES.md")
        + read_text("12_ENGINEERING_PRINCIPLES.md"),
        encoding="utf-8",
    )

    # --- 04 ---
    (STAGE / "04_SYSTEMS_INVENTORY.md").write_text(
        "# 04 — Systems inventory\n\n"
        + banner("02_SYSTEMS_AND_SIMULATIONS.md")
        + read_text("02_SYSTEMS_AND_SIMULATIONS.md"),
        encoding="utf-8",
    )

    # --- 05 ---
    (STAGE / "05_RESEARCH_BUILD_AND_STANDARDS.md").write_text(
        "# 05 — Research, build & coding standards\n\n"
        + banner("03_VULKAN_RESEARCH_GUIDE.md")
        + read_text("03_VULKAN_RESEARCH_GUIDE.md")
        + banner("04_LANGUAGE_RESEARCH_GUIDE.md")
        + read_text("04_LANGUAGE_RESEARCH_GUIDE.md")
        + banner("BUILDING.md")
        + read_text("BUILDING.md")
        + banner("CODING_STANDARDS.md")
        + read_text("CODING_STANDARDS.md")
        + banner("bootstrap_command.txt")
        + "```text\n"
        + read_text("bootstrap_command.txt").strip()
        + "\n```\n"
        + banner("snippets/ecs_system_example.cpp")
        + "```cpp\n"
        + read_text("snippets/ecs_system_example.cpp").strip()
        + "\n```\n"
        + banner("snippets/bld_tool_example.rs")
        + "```rust\n"
        + read_text("snippets/bld_tool_example.rs").strip()
        + "\n```\n",
        encoding="utf-8",
    )

    # --- 06 ---
    (STAGE / "06_ARCHITECTURE.md").write_text(
        "# 06 — Architecture (blueprint + core)\n\n"
        + banner("architecture/README.md")
        + read_text("architecture/README.md")
        + banner("architecture/grey-water-engine-blueprint.md")
        + read_text("architecture/grey-water-engine-blueprint.md")
        + banner("architecture/greywater-engine-core-architecture.md")
        + read_text("architecture/greywater-engine-core-architecture.md"),
        encoding="utf-8",
    )

    # --- 07 ---
    (STAGE / "07_SACRILEGE.md").write_text(
        "# 07 — Sacrilege (flagship)\n\n"
        + banner("games/README.md")
        + read_text("games/README.md")
        + banner("games/sacrilege/README.md")
        + read_text("games/sacrilege/README.md")
        + banner("games/sacrilege/12_SACRILEGE_PROGRAM_SPEC_GW-SAC-001_REV-A.md")
        + read_text("games/sacrilege/12_SACRILEGE_PROGRAM_SPEC_GW-SAC-001_REV-A.md"),
        encoding="utf-8",
    )

    # --- 08 ---
    (STAGE / "08_BLACKLAKE_AND_RIPPLE_STUDIO_SPECS.md").write_text(
        "# 08 — Studio specifications (Blacklake + GWE × Ripple)\n\n"
        + banner("BLACKLAKE_FRAMEWORK_BLF-CCL-001-REV-A.md")
        + read_text("BLACKLAKE_FRAMEWORK_BLF-CCL-001-REV-A.md")
        + banner("GREYWATER_ENGINE_GWE-ARCH-001-REV-A_Ripple-IDE-Spec.md")
        + read_text("GREYWATER_ENGINE_GWE-ARCH-001-REV-A_Ripple-IDE-Spec.md"),
        encoding="utf-8",
    )

    # --- 09 ---
    nine = [
        "# 09 — Netcode, determinism & performance annexes\n\n",
        banner("NETCODE_AND_DETERMINISM_CONTRACT.md"),
        read_text("NETCODE_AND_DETERMINISM_CONTRACT.md"),
    ]
    perf_dir = DOCS / "perf"
    if perf_dir.is_dir():
        for p in sorted(perf_dir.glob("*.md")):
            nine.append(banner(f"perf/{p.name}"))
            nine.append(p.read_text(encoding="utf-8", errors="replace"))
    det_dir = DOCS / "determinism"
    if det_dir.is_dir():
        for p in sorted(det_dir.glob("*.md")):
            nine.append(banner(f"determinism/{p.name}"))
            nine.append(p.read_text(encoding="utf-8", errors="replace"))
    for name in sorted(DOCS.glob("a11y_phase*_selfcheck.md")):
        nine.append(banner(name.name))
        nine.append(name.read_text(encoding="utf-8", errors="replace"))
    (STAGE / "09_NETCODE_DETERMINISM_PERF.md").write_text("".join(nine), encoding="utf-8")

    # --- 10 ---
    ten = [
        "# 10 — Appendix, glossary & architecture decision records\n\n",
        banner("08_GLOSSARY.md"),
        read_text("08_GLOSSARY.md"),
        banner("09_APPENDIX_RESEARCH_AND_REFERENCES.md"),
        read_text("09_APPENDIX_RESEARCH_AND_REFERENCES.md"),
        "\n\n---\n\n# Architecture Decision Records (full text)\n\n",
        "Individual `docs/adr/*.md` files were merged here for the 11-file layout. "
        "Original filenames are repeated in each subsection header.\n\n",
    ]
    adr_dir = DOCS / "adr"
    adr_files = []
    if adr_dir.is_dir():
        for p in adr_dir.glob("*.md"):
            if p.name.upper() == "README.MD":
                continue
            m = re.match(r"^(\d+)-", p.name)
            key = int(m.group(1)) if m else 9999
            adr_files.append((key, p))
    for _, p in sorted(adr_files, key=lambda t: t[0]):
        ten.append(f"\n\n---\n\n## ADR file: `adr/{p.name}`\n\n")
        ten.append(p.read_text(encoding="utf-8", errors="replace"))
    (STAGE / "10_APPENDIX_ADRS_AND_REFERENCES.md").write_text("".join(ten), encoding="utf-8")

    # --- swap into docs/ ---
    keep_names = {
        "README.md",
        "01_CONSTITUTION_AND_PROGRAM.md",
        "02_ROADMAP_AND_OPERATIONS.md",
        "03_PHILOSOPHY_AND_ENGINEERING.md",
        "04_SYSTEMS_INVENTORY.md",
        "05_RESEARCH_BUILD_AND_STANDARDS.md",
        "06_ARCHITECTURE.md",
        "07_SACRILEGE.md",
        "08_BLACKLAKE_AND_RIPPLE_STUDIO_SPECS.md",
        "09_NETCODE_DETERMINISM_PERF.md",
        "10_APPENDIX_ADRS_AND_REFERENCES.md",
    }
    for child in list(DOCS.iterdir()):
        if child.name in keep_names and child.is_file():
            child.unlink()
        elif child.is_dir():
            shutil.rmtree(child)
        elif child.is_file():
            child.unlink()
    for f in STAGE.iterdir():
        shutil.move(str(f), str(DOCS / f.name))
    STAGE.rmdir()

    # Verify
    remaining = sorted(DOCS.iterdir())
    names = [p.name for p in remaining]
    assert len(names) == 11, names
    assert set(names) == keep_names, set(names) ^ keep_names
    print("OK: docs/ now has exactly 11 Markdown files:")
    for n in names:
        print(" ", n)


if __name__ == "__main__":
    main()
