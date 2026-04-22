# Greywater Engine — Documentation

> **Cold Coffee Labs → Greywater Engine → *Sacrilege*.**

**Cold Coffee Labs** builds **Greywater Engine** — a proprietary C++23 / Vulkan platform, constructed from first principles, targeting the **AMD Radeon RX 580 8 GB at 1080p / 60 FPS** as the baseline. The engine presents ***Sacrilege*** as its debut title and forcing function for priorities.

**Production bar:** **Supra-AAA** — defined in `01_CONSTITUTION_AND_PROGRAM.md §0.1`.

---

## Document Suite

This directory contains **eleven Markdown files** — the complete, canonical documentation for Greywater Engine and *Sacrilege*. Everything lives here; use git history if you need prior paths.

| # | File | Contents |
|---|------|----------|
| — | `README.md` | **You are here** — information architecture, layers, precedence, reading order |
| 01 | `01_CONSTITUTION_AND_PROGRAM.md` | Constitution, non-negotiables, supra-AAA mandate, code-gen style, session handoff |
| 02 | `02_ROADMAP_AND_OPERATIONS.md` | 25-phase roadmap, week-by-week schedule, Kanban rituals, WIP limits |
| 03 | `03_PHILOSOPHY_AND_ENGINEERING.md` | The Brew Doctrine, engineering principles, code-review playbook |
| 04 | `04_SYSTEMS_INVENTORY.md` | Complete subsystem inventory with Tier A / B / C classification |
| 05 | `05_RESEARCH_BUILD_AND_STANDARDS.md` | Vulkan guide, C++23 / Rust discipline, build instructions, coding standards |
| 06 | `06_ARCHITECTURE.md` | Formal blueprint, module topology, subsystem deep-dives, Chapter-13 editor |
| 07 | `07_SACRILEGE.md` | GW-SAC-001 flagship program spec — Martyrdom Engine, Nine Circles, God Machine |
| 08 | `08_BLACKLAKE_AND_RIPPLE.md` | Blacklake Framework (HEC / SDR / GPTM / TEBS) + Ripple Script / studio specs |
| 09 | `09_NETCODE_DETERMINISM_PERF.md` | Netcode contract, determinism rules, performance budgets (Phases 12–17) |
| 10 | `10_APPENDIX_ADRS_AND_REFERENCES.md` | Glossary, research appendix, full ADR archive (ADR-0003 → ADR-0085+) |

---

## Layers of Truth

When documents appear to conflict, the lower-numbered layer wins.

| Layer | Source | Role |
|-------|--------|------|
| L0 | `README.md` | Information architecture and precedence map |
| L1 | `01_CONSTITUTION_AND_PROGRAM.md` | Non-negotiables — cannot be overridden |
| L2 | `03_PHILOSOPHY_AND_ENGINEERING.md` | Values and review rubric |
| L3 | `06_ARCHITECTURE.md` | Platform shape and module topology |
| L3b | `08_BLACKLAKE_AND_RIPPLE.md` | Blacklake math and Ripple studio spec |
| L4 | `07_SACRILEGE.md` | Flagship product mandate |
| L5 | `04_SYSTEMS_INVENTORY.md`, `09_NETCODE_DETERMINISM_PERF.md` | Subsystem inventory and contracts |
| L6 | `02_ROADMAP_AND_OPERATIONS.md`, `05_RESEARCH_BUILD_AND_STANDARDS.md` | Schedule and build detail |
| L7 | ADRs in `10_APPENDIX_ADRS_AND_REFERENCES.md` | Point-in-time decisions |

**Conflict resolution precedence:** `01` → `07` → `08` → `06` → `04` → `09`.

Use `02` (schedule, Kanban) and `05` (build detail) for operational execution. ADRs in `10` narrow or supersede narrative architecture where they conflict — find the relevant ADR number, read the decision, trust it over older prose.

---

## Reading Order

### First time — allow 60–90 minutes

1. **`01_CONSTITUTION_AND_PROGRAM.md`** — what is authorized, what is forbidden, what is non-negotiable
2. **`02_ROADMAP_AND_OPERATIONS.md`** — where we are in the phase plan right now
3. **`03_PHILOSOPHY_AND_ENGINEERING.md`** — why we make the choices we make; the code-review rubric
4. **`06_ARCHITECTURE.md`** — how the platform is shaped; module topology; BLD boundary
5. **`07_SACRILEGE.md`** — what we are shipping; Martyrdom Engine; Nine Circles; God Machine
6. **`08_BLACKLAKE_AND_RIPPLE.md`** — the math behind the procedural stack
7. **`04_SYSTEMS_INVENTORY.md`** / **`09_`** / **`10_`** — depth for your subsystem

### Before every coding session — 10 minutes

1. `02_ROADMAP_AND_OPERATIONS.md` — active phase, active cards, WIP
2. `01_CONSTITUTION_AND_PROGRAM.md §2` — non-negotiables refresher
3. `CLAUDE.md` at project root — per-session handoff

---

## Non-Negotiables (Summary)

These cannot be changed without executive sign-off. Full list: `01 §2`.

| Constraint | Commitment |
|------------|------------|
| Compiler | Clang / LLVM 18+ only. `clang-cl` on Windows. MSVC and GCC: never. |
| Languages | C++23 for engine, editor, gameplay. Rust 2024 for BLD only. Nowhere else. |
| Editor UI | Dear ImGui + docking + viewports + ImNodes + ImGuizmo. No custom framework. |
| Game UI | RmlUi + HarfBuzz + FreeType. Not ImGui. |
| Scripting | Typed text IR is ground-truth. Node graph is a reversible projection. No VM. |
| Hardware floor | RX 580 8 GB @ 1080p / 60 FPS for all Tier A features. No exceptions. |
| Platforms | Windows (x64) + Linux (x64) from day one. macOS and consoles: post-scope. |
| OS headers | Only inside `engine/platform/`. Enforced by clang-tidy in CI. |
| Allocators | No global `new` / `delete` in engine code. Pass allocators explicitly. |
| Procedural | Deterministic. Same seed + same coords = identical output, every run. |

---

## Studio

**Cold Coffee Labs** — slicedlabs.founder@proton.me  
Repo: `git@github.com:SL1C3D-L4BS/cold-coffee-labs.git`

*"For I stand on the ridge of the universe and await nothing."*

*Brewed slowly. Built deliberately. Shipped by Cold Coffee Labs.*
