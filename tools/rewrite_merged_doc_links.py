#!/usr/bin/env python3
"""Rewrite stale in-repo paths inside the 11-file docs/ suite after consolidation."""

import re
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
DOCS = REPO / "docs"

ADR_MD = re.compile(r"docs/adr/\d{4}-[a-z0-9-]+\.md")

# Longest keys first (substring replacement order).
REPLACEMENTS: list[tuple[str, str]] = [
    ("docs/games/sacrilege/12_SACRILEGE_PROGRAM_SPEC_GW-SAC-001_REV-A.md", "docs/07_SACRILEGE.md"),
    ("docs/games/sacrilege/", "docs/07_SACRILEGE.md"),
    ("docs/architecture/greywater-engine-core-architecture.md", "docs/06_ARCHITECTURE.md"),
    ("docs/architecture/grey-water-engine-blueprint.md", "docs/06_ARCHITECTURE.md"),
    ("docs/architecture/README.md", "docs/06_ARCHITECTURE.md"),
    ("docs/BLACKLAKE_FRAMEWORK_BLF-CCL-001-REV-A.md", "docs/08_BLACKLAKE_AND_RIPPLE_STUDIO_SPECS.md"),
    ("docs/GREYWATER_ENGINE_GWE-ARCH-001-REV-A_Ripple-IDE-Spec.md", "docs/08_BLACKLAKE_AND_RIPPLE_STUDIO_SPECS.md"),
    ("docs/NETCODE_AND_DETERMINISM_CONTRACT.md", "docs/09_NETCODE_DETERMINISM_PERF.md"),
    ("docs/00_SPECIFICATION_Claude_Opus_4.7.md", "docs/01_CONSTITUTION_AND_PROGRAM.md"),
    ("docs/DOCUMENTATION_SYSTEM.md", "docs/README.md"),
    ("docs/MASTER_PLAN_BRIEF.md", "docs/01_CONSTITUTION_AND_PROGRAM.md"),
    ("docs/10_PRE_DEVELOPMENT_MASTER_PLAN.md", "docs/01_CONSTITUTION_AND_PROGRAM.md"),
    ("docs/11_HANDOFF_TO_CLAUDE_CODE.md", "docs/01_CONSTITUTION_AND_PROGRAM.md"),
    ("docs/05_ROADMAP_AND_MILESTONES.md", "docs/02_ROADMAP_AND_OPERATIONS.md"),
    ("docs/06_KANBAN_BOARD.md", "docs/02_ROADMAP_AND_OPERATIONS.md"),
    ("docs/12_ENGINEERING_PRINCIPLES.md", "docs/03_PHILOSOPHY_AND_ENGINEERING.md"),
    ("docs/01_ENGINE_PHILOSOPHY.md", "docs/03_PHILOSOPHY_AND_ENGINEERING.md"),
    ("docs/02_SYSTEMS_AND_SIMULATIONS.md", "docs/04_SYSTEMS_INVENTORY.md"),
    ("docs/03_VULKAN_RESEARCH_GUIDE.md", "docs/05_RESEARCH_BUILD_AND_STANDARDS.md"),
    ("docs/04_LANGUAGE_RESEARCH_GUIDE.md", "docs/05_RESEARCH_BUILD_AND_STANDARDS.md"),
    ("docs/CODING_STANDARDS.md", "docs/05_RESEARCH_BUILD_AND_STANDARDS.md"),
    ("docs/BUILDING.md", "docs/05_RESEARCH_BUILD_AND_STANDARDS.md"),
    ("docs/08_GLOSSARY.md", "docs/10_APPENDIX_ADRS_AND_REFERENCES.md"),
    ("docs/09_APPENDIX_RESEARCH_AND_REFERENCES.md", "docs/10_APPENDIX_ADRS_AND_REFERENCES.md"),
    ("docs/perf/", "docs/09_NETCODE_DETERMINISM_PERF.md"),
    ("docs/determinism/", "docs/09_NETCODE_DETERMINISM_PERF.md"),
    ("docs/adr/README.md", "docs/10_APPENDIX_ADRS_AND_REFERENCES.md"),
]


def main() -> None:
    for path in sorted(DOCS.glob("*.md")):
        text = path.read_text(encoding="utf-8", errors="replace")
        orig = text
        text = ADR_MD.sub("docs/10_APPENDIX_ADRS_AND_REFERENCES.md", text)
        for old, new in REPLACEMENTS:
            text = text.replace(old, new)
        text = text.replace("docs/adr/*", "docs/10_APPENDIX_ADRS_AND_REFERENCES.md")
        text = text.replace("docs/adr/", "docs/10_APPENDIX_ADRS_AND_REFERENCES.md")
        if text != orig:
            path.write_text(text, encoding="utf-8")
            print("updated", path.name)


if __name__ == "__main__":
    main()
