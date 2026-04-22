# Greywater Engine — documentation

**Studio → engine → title:** **Cold Coffee Labs** builds **Greywater Engine** (C++23 / Vulkan proprietary stack). Greywater Engine presents ***Sacrilege*** as the debut title and forcing function for priorities.

**Production bar:** **Supra-AAA** — defined in `01_CONSTITUTION_AND_PROGRAM.md` §0.1 (constitution + executive brief merged there).

---

## Eleven-file suite

This directory contains **exactly eleven Markdown files** — no subfolders. Everything that previously lived in `architecture/`, `games/`, `adr/`, `daily/`, `perf/`, and scattered numbered roots is **merged** here; use **git history** if you need the old paths.

| # | File | What it is |
|---|------|----------------|
| — | `README.md` | **You are here** — IA, reading order, precedence |
| 01 | `01_CONSTITUTION_AND_PROGRAM.md` | Constitution (`00_`), doc-system map, executive brief, 90-day pre-dev, session handoff |
| 02 | `02_ROADMAP_AND_OPERATIONS.md` | Roadmap (`05_`) + Kanban (`06_`) — schedule and rituals |
| 03 | `03_PHILOSOPHY_AND_ENGINEERING.md` | Brew doctrine (`01_`) + engineering principles (`12_`) |
| 04 | `04_SYSTEMS_INVENTORY.md` | Subsystem inventory (`02_`) |
| 05 | `05_RESEARCH_BUILD_AND_STANDARDS.md` | Vulkan / C++ / Rust guides, build, coding standards, bootstrap + snippets |
| 06 | `06_ARCHITECTURE.md` | Blueprint + core architecture (merged `architecture/`) |
| 07 | `07_SACRILEGE.md` | Flagship program spec GW-SAC + `games/` README |
| 08 | `08_BLACKLAKE_AND_RIPPLE_STUDIO_SPECS.md` | Blacklake (BLF) + GWE × Ripple studio specifications |
| 09 | `09_NETCODE_DETERMINISM_PERF.md` | Netcode contract + perf budgets + determinism replay annexes |
| 10 | `10_APPENDIX_ADRS_AND_REFERENCES.md` | Glossary, research appendix, **full ADR archive** |

---

## Layers of truth

| Layer | Source |
| ----- | ------ |
| L0 | `README.md` |
| L1 | `01_CONSTITUTION_AND_PROGRAM.md` |
| L2 | `03_PHILOSOPHY_AND_ENGINEERING.md` |
| L3 | `06_ARCHITECTURE.md` |
| L3b | `08_BLACKLAKE_AND_RIPPLE_STUDIO_SPECS.md` |
| L4 | `07_SACRILEGE.md` |
| L5 | `04_SYSTEMS_INVENTORY.md`, `09_NETCODE_DETERMINISM_PERF.md` |
| L6 | `02_ROADMAP_AND_OPERATIONS.md`, `05_RESEARCH_BUILD_AND_STANDARDS.md` |
| L7 | ADRs (search `## ADR file:` inside `10_APPENDIX_ADRS_AND_REFERENCES.md`) |

**Precedence when docs appear to conflict:** `01` → `07` → `08` → `06` → `04` → `09`. Use `02` (schedule, Kanban) and `05` (build/research detail) for operational execution.

---

## Suggested reading order

1. `01_CONSTITUTION_AND_PROGRAM.md` — what is allowed and what is not  
2. `02_ROADMAP_AND_OPERATIONS.md` — where we are in the phase plan  
3. `03_PHILOSOPHY_AND_ENGINEERING.md` — values + review checklist  
4. `06_ARCHITECTURE.md` — how the platform is shaped  
5. `07_SACRILEGE.md` — what we are shipping first  
6. `08_…` / `04_` / `09_` / `10_` — depth as needed for your subsystem  

---

*Cold Coffee Labs — Greywater Engine — Sacrilege.*
