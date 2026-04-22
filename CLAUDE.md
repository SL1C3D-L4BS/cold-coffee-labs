# CLAUDE.md — Greywater Engine × Cold Coffee Labs

Cold-start primer for **Claude Code** (and any AI assistant) on this repository. Read before every session.

---

## What this project is

**One unified system:** **Cold Coffee Labs** ships **Greywater Engine** — the proprietary C++23 platform — and Greywater Engine presents ***Sacrilege*** as the debut title and forcing function for priorities.

- **Cold Coffee Labs** — studio.  
- **Greywater_Engine** — `greywater_core`, editor, jobs, ECS, Vulkan renderer, BLD, `engine/world/`, tools.  
- ***Sacrilege*** — flagship game; vision in `docs/07_SACRILEGE.md`. Procedural stack: **Blacklake** (`docs/08_BLACKLAKE_AND_RIPPLE.md`).  

**Engine/game separation is absolute:** the engine never imports from game code.

- **C++23** core; **Rust** only for **BLD**.  
- **Clang/LLVM only** (`clang-cl` on Windows, clang on Linux).  
- **Vulkan 1.2 baseline**, **1.3 features** via `RHICapabilities`.  
- **Baseline hardware:** AMD Radeon RX 580 8GB @ 1080p / 60 FPS.  
- **CMake + Corrosion**; Windows + Linux.  
- **Dear ImGui** (editor); **RmlUi** (in-game UI).  
- **MCP** (`2025-11-25`) — BLD wire protocol.  
- **Art & tone for shipping:** *Sacrilege* — `docs/07_SACRILEGE.md`; studio math + Ripple depth — `docs/08_BLACKLAKE_AND_RIPPLE.md`.  

**Documentation (11 files under `docs/`):** `docs/README.md` · **Constitution & handoff:** `docs/01_CONSTITUTION_AND_PROGRAM.md` · **Architecture:** `docs/06_ARCHITECTURE.md` · **ADRs (merged):** `docs/10_APPENDIX_ADRS_AND_REFERENCES.md`

*"For I stand on the ridge of the universe and await nothing."*

---

## Non-negotiables (violate at your peril)

### Hardware-driven
1. **RX 580 is the baseline** for Tier A. Ray tracing, mesh shaders, work graphs: Tier G.  
2. **Vulkan 1.2 baseline;** 1.3 via `RHICapabilities`.

### Language & build
3. **Clang only** for C++.  
4. **Rust only for BLD.**

### Code discipline
5–11. No raw `new`/`delete` in engine hot paths; no two-phase public `init`/`shutdown`; no C-style casts; no `string_view` into C APIs; no STL in hot paths; no `std::async` / raw threads (use `engine/jobs/`); OS headers only in `engine/platform/`.

### UI & scripting
12–14. ImGui editor-only; RmlUi in-game; **text IR** is ground truth for visual scripting.

### BLD & world
15–18. BLD outside `greywater_core`; deterministic generation; **`float64`** world space + floating origin where required; no secrets in repo.

### Execution cadence
19–20. Phase-complete delivery; **documentation never lags directive** — update `docs/` before code when the directive changes.

Full rationale: `docs/03_PHILOSOPHY_AND_ENGINEERING.md`, `docs/01_CONSTITUTION_AND_PROGRAM.md` §2 and §9.

---

## Per-session protocol

1. `git log --oneline -10`  
2. `CLAUDE.md` *(this file)*  
3. `docs/01_CONSTITUTION_AND_PROGRAM.md` *(handoff lives in merged §)*  
4. `docs/02_ROADMAP_AND_OPERATIONS.md` *(Kanban + roadmap)*  
5. Phase-relevant sections in `docs/02_ROADMAP_AND_OPERATIONS.md`

---

## Where to start (first time)

1. `docs/README.md`  
2. `docs/01_CONSTITUTION_AND_PROGRAM.md`  
3. `docs/03_PHILOSOPHY_AND_ENGINEERING.md`  
4. `docs/06_ARCHITECTURE.md`  
5. `docs/07_SACRILEGE.md`  
6. `docs/08_BLACKLAKE_AND_RIPPLE.md` *(skim ToC + your subsystem)*  

---

## Canonical vs operational

- **Canonical:** `docs/README.md`, `docs/01`–`docs/10` (eleven Markdown files total under `docs/`), `CLAUDE.md`  
- **Operational:** schedule + rituals in `docs/02_ROADMAP_AND_OPERATIONS.md`  
- **ADRs:** full text merged into `docs/10_APPENDIX_ADRS_AND_REFERENCES.md` (search for `## ADR file:`)

---

## House style

**C++:** PascalCase types, `snake_case` functions, trailing `_` members, `gw::` namespaces, `GW_` macros.  
**Rust:** idiomatic Rust.  
**Headers:** `#pragma once`, `.hpp` / `.cpp`.  
**World space:** `float64`; `float` only local-to-chunk / GPU after origin subtraction.

---

## Studio

**Cold Coffee Labs** — slicedlabs.founder@proton.me  

*Brewed with BLD.*
