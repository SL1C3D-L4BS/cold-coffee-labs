# CLAUDE.md — Greywater × Cold Coffee Labs

This file is the cold-start primer for **Claude Code** (and any AI assistant) operating on this repository. Read it before every session. Do not skip.

---

## What this project is

**Greywater** — a persistent procedural universe simulation, shipping on its own proprietary engine.

- **Greywater_Engine** — the C++23 engine built by Cold Coffee Labs.
- **Greywater (the game)** — the first title built on the engine. Seamless ground-to-orbit traversal. Procedural multi-planet universe. PvP survival + base building + space combat. Living AI ecosystem.

The engine is purpose-built to power the game. The game drives the engine's feature priorities. **Engine/game code separation remains absolute** (engine never imports from game).

- **C++23** engine core; **Rust** only for BLD (AI copilot) subsystem.
- **Clang/LLVM only** (`clang-cl` on Windows, clang on Linux).
- **Vulkan 1.2 baseline** with **1.3 features opportunistic** (via `RHICapabilities`).
- **Target hardware: AMD Radeon RX 580 8GB @ 1080p / 60 FPS.**
- **CMake + Corrosion**; Windows + Linux from day one.
- **Dear ImGui + docking + viewports + ImNodes + ImGuizmo** for the editor (locked).
- **RmlUi** for in-game UI (not ImGui).
- **MCP** (Model Context Protocol 2025-11-25) as BLD's wire protocol.
- **Art direction: realistic low-poly** — reduced polygon counts with physically-based realistic lighting. See `docs/games/greywater/20_ART_DIRECTION.md`.
- **Studio:** Cold Coffee Labs · Repo: `git@github.com:SL1C3D-L4BS/cold-coffee-labs.git`

See `docs/architecture/` for the formal blueprint and the narrative core architecture. Game-specific vision docs live under `docs/games/greywater/`.

*"For I stand on the ridge of the universe and await nothing."*

---

## Non-negotiables (violate at your peril)

### Hardware-driven
1. **RX 580 is the baseline.** Every Tier A feature must run at 1080p / 60 FPS on an RX 580. No ray tracing, no mesh shaders, no work graphs as baseline — all are Tier G (hardware-gated, post-v1).
2. **Vulkan 1.2 baseline;** Vulkan 1.3 features opportunistic via `RHICapabilities`.

### Language & build
3. **Clang is the only supported C++ compiler.** No MSVC, no GCC.
4. **Rust is used only for BLD** (`bld/` crate). Nowhere else.

### Code discipline
5. **No raw `new`/`delete`** in engine code. Use `std::unique_ptr`, `gw::Ref<T>`, or stack scope. (§12 A1)
6. **No two-phase `init()`/`shutdown()`** on public APIs. RAII is the default; delayed activation is protected-only. (§12 A2)
7. **No C-style casts.** Use `static_cast` / `reinterpret_cast` / `const_cast` / `dynamic_cast`. (§12 B1)
8. **`std::string_view` never goes to a C API.** Take `const std::string&` when you'll pass to C. (§12 E1)
9. **No STL containers in engine hot paths.** Use `engine/core/` containers.
10. **No `std::async`, no raw `std::thread`** in engine code. All work goes through `engine/jobs/`.
11. **OS headers** only inside `engine/platform/`.

### UI & scripting
12. **ImGui is editor-only.** In-game UI is RmlUi.
13. **No custom UI framework.** Do not propose one.
14. **Visual scripting: text IR is ground-truth.** Node graph is a projection.

### BLD & world
15. **BLD cannot live in `greywater_core`.** Separate Rust crate, linked via Corrosion + narrow C-ABI.
16. **Universe generation is deterministic.** Same seed + same coordinates = identical output, every run, every machine.
17. **All world-space positions use `float64`.** Floating origin is mandatory for planet-scale scenes.
18. **Do not commit secrets.** `.env`, `*.key`, OS-keychain credentials never enter the repo or model context.

### Execution cadence
19. **Phase-complete execution.** When a phase opens, *every* deliverable listed for that phase in `docs/05_ROADMAP_AND_MILESTONES.md` (and the week-by-week expansion in `docs/10_PRE_DEVELOPMENT_MASTER_PLAN.md` for Phases 1–3) is in-scope from day one. **The phase milestone is the gate — not the week row.** No deliverable is parked "for next Tuesday" or "for Friday's card." If it is described under Phase N, it lands before Phase N closes. Weekly rows remain as internal pace-markers; they are budget, not gating contracts. Kanban WIP≤2 governs *concurrency*, not *scope* — the Triaged pool on phase entry is the full phase.
20. **Documentation never lags directive.** A change in directive — including this one — updates `CLAUDE.md` and every affected canonical/operational doc **before any code is written**. The repo's rules and the repo's code must not disagree. A session that introduces a new directive commits the doc change first and the code change second.

Full rationale and the code-review rubric behind these: `docs/12_ENGINEERING_PRINCIPLES.md`. Policy/principle rationale for #1–18: `docs/00_SPECIFICATION_Claude_Opus_4.7.md` §2. Execution-cadence rationale for #19–20: `docs/00_SPECIFICATION_Claude_Opus_4.7.md` §9.

---

## Per-session protocol (every cold start)

1. Open today's `docs/daily/YYYY-MM-DD.md` and read what was done yesterday.
2. Run `git log --oneline -10` to see recent commits.
3. Re-read this file (`CLAUDE.md`).
4. Re-read `docs/11_HANDOFF_TO_CLAUDE_CODE.md` for the full per-session checklist.
5. Check the Kanban board in `docs/06_KANBAN_BOARD.md` — which card is In Progress?
6. Read the numbered doc relevant to the current phase (see `docs/05_ROADMAP_AND_MILESTONES.md`).

---

## Where to start (first time)

1. `docs/README.md` — document hierarchy and reading order
2. `docs/MASTER_PLAN_BRIEF.md` — high-level mandate
3. `docs/00_SPECIFICATION_Claude_Opus_4.7.md` — binding constraints (RX 580, language, non-negotiables)
4. `docs/01_ENGINE_PHILOSOPHY.md` — The Brew Doctrine + realistic low-poly art direction
5. `docs/12_ENGINEERING_PRINCIPLES.md` — code-review playbook
6. `docs/architecture/grey-water-engine-blueprint.md` — formal stakeholder blueprint
7. `docs/architecture/greywater-engine-core-architecture.md` — narrative architecture
8. `docs/games/greywater/README.md` — Greywater game vision overview
9. `docs/games/greywater/12_*` through `20_*` — game-specific subsystem vision (universe, planetary, atmosphere, survival, multiplayer, ecosystem, audio, art direction, tooling)

---

## Canonical vs. operational vs. archival

- **Canonical** (binding, do not modify in flight): `00`, `01`, `10`, `12`, `CLAUDE.md`, `docs/architecture/*`, `docs/games/greywater/*`
- **Operational** (updated frequently): `05`, `06`, `07`, `11`, `docs/daily/*`, `docs/perf/phase*_budgets.md`
- **Reference** (stable): `02`, `03`, `04`, `08`, `09`
- **Archival** (preserved with notice): `90_*`

---

## House style

- **C++:** PascalCase types, `snake_case` functions/methods, trailing-underscore members (`device_`), `snake_case` namespaces (`gw::render`), UPPER_SNAKE macros (`GW_ASSERT`).
- **Rust:** standard Rust idioms (snake_case everywhere, PascalCase types, `SCREAMING_SNAKE` consts).
- **Files:** `.hpp` / `.cpp` / `.rs`. Headers use `#pragma once`.
- **World-space positions:** `float64` (`double`). Single-precision `float` only for local-to-chunk math.
- **Formatting:** enforced by `clang-format`, `clang-tidy`, `rustfmt`, `clippy`. Non-conforming PRs are auto-blocked.

---

## Studio contact

**Cold Coffee Labs** — founder: slicedlabs.founder@proton.me

*Brewed with BLD. For I stand on the ridge of the universe and await nothing.*
