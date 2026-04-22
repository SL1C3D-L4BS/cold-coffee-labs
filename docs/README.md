# Greywater Engine — Documentation

> **Cold Coffee Labs → Greywater Engine → *Sacrilege*.**

**Cold Coffee Labs** builds **Greywater Engine** — a proprietary C++23 / Vulkan game engine. The engine presents ***Sacrilege*** as its one purpose: a fast, gory, 3D first-person action game with infinite procedural levels, built entirely in C++23 on custom 3D assets, with a dedicated level editor and toolchain that exists solely to build and ship *Sacrilege*.

**The vision in one sentence:** *Quake and Doom had a brutal, fast child in 1993 — then in 2026 Cold Coffee Labs rebuilt it from scratch with a custom engine, infinite procedural hellscapes, and a movement system that makes speedrunners cry.*

**Production bar:** **Supra-AAA** — defined in `01_CONSTITUTION_AND_PROGRAM.md §0.1`.

---

## What This Is — And What It Is Not

### What It Is

- A **C++23 game engine** purpose-built to ship *Sacrilege*
- A **3D game** with fully procedural infinite levels (Nine Inverted Circles)
- A **level editor** built into the engine — drag-and-drop encounter placement, real-time preview, asset hotswap — everything a 1993 level designer would recognize, modernized
- **All gameplay logic in C++23** — no scripting VM, no Lua, no Python in shipped binaries, no custom language (offline `tools/` Python: `01` §2.2.1)
- **Custom 3D assets** authored by the team and cooked by the engine's asset pipeline
- A **speedrunner's paradise** — frame-perfect movement, deterministic physics, seed-sharable levels
- Infinite replayability via **Blacklake** — same Hell Seed = same level, always; different seed = different hell

### What It Is Not

- Not a scripting language IDE — there is no `Ripple Script` as a player-visible language. Logic is C++.
- Not a universe simulator — no planets, no orbital mechanics, no galaxy generation. *Sacrilege* is arena-scale.
- Not a survival/open-world game — it is a fast FPS with procedural arenas
- Not a general-purpose engine for other developers — it serves *Sacrilege* first

---

## Document Suite — Eleven Files

| # | File | Contents |
|---|------|----------|
| — | `README.md` | **You are here** — vision, IA, reading order |
| 01 | `01_CONSTITUTION_AND_PROGRAM.md` | Non-negotiables, authorization boundaries, session handoff |
| 02 | `02_ROADMAP_AND_OPERATIONS.md` | 25-phase schedule, Kanban rituals, milestone definitions |
| 03 | `03_PHILOSOPHY_AND_ENGINEERING.md` | Brew Doctrine, code-review playbook (Sections A–K) |
| 04 | `04_SYSTEMS_INVENTORY.md` | Complete subsystem inventory, Tier A/B/C |
| 05 | `05_RESEARCH_BUILD_AND_STANDARDS.md` | Vulkan guide, C++23 discipline, build instructions |
| 06 | `06_ARCHITECTURE.md` | Module topology, subsystem deep-dives, editor architecture |
| 07 | `07_SACRILEGE.md` | GW-SAC-001 — the complete game specification |
| 08 | `08_BLACKLAKE.md` | Blacklake Framework — HEC/SDR/GPTM for arena generation |
| 09 | `09_NETCODE_DETERMINISM_PERF.md` | Netcode, determinism rules, performance budgets |
| 10 | `10_APPENDIX_ADRS_AND_REFERENCES.md` | Glossary, research references, ADR archive |

**Auxiliary (not in the L0–L7 stack):** `COMPOSER_CONTEXT.md` — short Composer session primer; if it disagrees with `01`, `CLAUDE.md`, or this README, **those win**.

---

## Layers of Truth

| Layer | Source | Role |
|-------|--------|------|
| L0 | `README.md` | Vision and IA |
| L1 | `01_CONSTITUTION_AND_PROGRAM.md` | Non-negotiables — cannot be overridden |
| L2 | `03_PHILOSOPHY_AND_ENGINEERING.md` | Values and review rubric |
| L3 | `06_ARCHITECTURE.md` | Platform shape and module topology |
| L3b | `08_BLACKLAKE.md` | Arena generation math |
| L4 | `07_SACRILEGE.md` | Game mandate — everything serves this |
| L5 | `04_SYSTEMS_INVENTORY.md`, `09_NETCODE_DETERMINISM_PERF.md` | Subsystem inventory and contracts |
| L6 | `02_ROADMAP_AND_OPERATIONS.md`, `05_RESEARCH_BUILD_AND_STANDARDS.md` | Schedule and build detail |
| L7 | ADRs in `10_APPENDIX_ADRS_AND_REFERENCES.md` | Point-in-time decisions |

**Conflict resolution:** `01` → `07` → `08` → `06` → `04` → `09`. ADRs always win over prose.

---

## The Game Vision

*Sacrilege* is a **3D first-person action game** where the player descends through Nine Inverted Circles of Hell. Every circle is procedurally generated from a **Hell Seed** — the same seed always produces the same level, making seeds shareable and speedrun-verifiable. Different seeds produce entirely different hells.

**Core loop:** Move fast. Kill everything. Manage your Sin meter. Don't let it hit 100 or Rapture takes you. Survive long enough to reach the God Machine.

**What makes it feel 1990s (the good parts):**
- Movement is physics-first — bunny hop, air strafe, wall kick, slide, grapple, blood surf
- Levels are mazes of geometry, not open worlds — tight corridors, arena rooms, vertical shafts
- Weapons have weight and impact — no reloads, alt-fires, feel like they hurt
- Enemies are aggressive, fast, and tuned to punish hesitation
- No waypoints, no minimaps, no handholding

**What makes it 2026 (the good parts):**
- All geometry is procedural — infinite replayability with seed sharing
- Persistent gore — blood decals, ragdoll gibs, severed limbs that affect navigation
- The Martyrdom Engine — Sin/Mantra dual resource system unique to *Sacrilege*
- 144 FPS on an RX 580, no exceptions

---

## The Editor Vision

The **Greywater Editor** is the dedicated tool for building *Sacrilege* content. It is a native C++ application that lives inside the engine. Think GoldSrc's Hammer editor but built into the engine runtime, not a separate program.

**What it does:**
- Real-time 3D viewport with immediate preview — what you place is what ships
- Encounter editor — drag enemies into rooms, set patrol paths, configure behavior trees visually
- Circle editor — compose procedural parameters for each of the Nine Circles; tweak SDR noise visuals in real time
- Asset browser — browse cooked 3D assets, drag into scene
- BLD panel — ask the AI copilot to place encounters, generate geometry variations, or debug behavior
- Play-from-editor — hit F5 and the game starts from current cursor position

**What it does NOT do:**
- It does not have a scripting language IDE — gameplay logic is C++23 compiled code
- It does not have a node graph for gameplay logic — that is code
- It does not ship to players — it is a dev tool only

---

## Non-Negotiables (Summary)

| Constraint | Commitment |
|------------|------------|
| Compiler | Clang / LLVM 18+ only. `clang-cl` on Windows. |
| Language — engine | C++23. Everything. No exceptions (the keyword or the concept). |
| Language — BLD | Rust 2024 for the AI copilot only. Nowhere else. |
| Gameplay logic | C++23 compiled code. No scripting VM. No Lua. No Python in shipped binaries. No custom language. (Offline Python in `tools/`: `01` §2.2.1.) |
| Editor UI | Dear ImGui. No custom framework. |
| In-game UI | RmlUi. Not ImGui. |
| Hardware floor | RX 580 8 GB @ 1080p / 144 FPS for *Sacrilege*. No exceptions. |
| Platforms | Windows + Linux from day one. |
| Game assets | Custom 3D assets authored by the team. No licensed packs. |
| Procedural scope | Arena-scale (Nine Circles). Not planetary. Not universe-scale. |
| Game genre | Fast 3D FPS. Not open world. Not survival. Not MMO. |

Full list: `01_CONSTITUTION_AND_PROGRAM.md §2`.

### Performance targets (reconciled)

Do **not** mix these up:

- ***Sacrilege* Tier A (L1):** **1080p @ 144 FPS** on RX 580 — binding for flagship combat, Martyrdom, Nine Circles (`01` §2.1, `07`).
- **Engine reference scenes & some milestone exit proofs:** **≥ 60 FPS** on RX 580 for heavy renderer / simulation harnesses and historical gates such as *Foundation Renderer* — see `05` §1.4.

### Tooling languages (reconciled)

- **Engine, editor, gameplay, BLD binaries:** C++23 + Rust in `bld/` only.  
- **Python:** permitted **only** under the offline carve-out in `01` §2.2.1 (`tools/`, CI glue). Never runtime gameplay.

---

## Reading Order

### First time — 45 minutes

1. `01_CONSTITUTION_AND_PROGRAM.md` — what is allowed; what is forbidden
2. `07_SACRILEGE.md` — the complete game we are building
3. `02_ROADMAP_AND_OPERATIONS.md` — where we are right now
4. `03_PHILOSOPHY_AND_ENGINEERING.md` — why we make the choices we make
5. `06_ARCHITECTURE.md` — how the engine and editor are shaped

### Before every session — 5 minutes

1. `02_ROADMAP_AND_OPERATIONS.md` — active phase, current cards
2. `01_CONSTITUTION_AND_PROGRAM.md §2` — non-negotiables
3. `CLAUDE.md` at repo root

---

*Cold Coffee Labs — Greywater Engine — Sacrilege.*  
*"For I stand on the ridge of the universe and await nothing."*
