# Greywater Engine — Documentation

> **Cold Coffee Labs → horror studio → Greywater Engine → *Sacrilege* (flagship) → franchise platform.**

**Cold Coffee Labs** is a **horror studio**. We build **Greywater Engine** — a proprietary C++23 / Vulkan game engine designed as a **franchise content platform** — and ship ***Sacrilege*** as its flagship: a fast, gory, 3D first-person action game with infinite procedural hellscapes. The engine is factored into **seven IP-agnostic franchise services** (Material Forge, Level Architect, Combat Simulator, Gore System, Audio Weave, AI Director, Editor Co-Pilot) so the Sacrilege family — *Apostasy*, *Silentium*, *The Witness* — and future horror IPs can all ride the same spine.

**The vision in one sentence:** *Quake and Doom had a brutal, fast child in 1993 — then in 2026 Cold Coffee Labs rebuilt it from scratch with a custom engine, infinite procedural hellscapes, a deterministic runtime-AI Director, and the Malakor / Niccolò Story Bible laid across nine inverted Circles and a Grace finale.*

**Production bar:** **Supra-AAA** — defined in `01_CONSTITUTION_AND_PROGRAM.md §0.1`.

---

## What This Is — And What It Is Not

### What It Is

- A **C++23 game engine** purpose-built to ship *Sacrilege* and factored for horror-franchise reuse (seven IP-agnostic services)
- A **3D game** with fully procedural infinite levels (Nine Inverted Circles with narrative location-skins mapping to the Vatican / Rome / Hell / Heaven / Silentium cosmology)
- A **level editor** built into the engine with three selectable theme profiles (Brewed Slate default, Corrupted Relic horror aesthetic, Field Test High Contrast for a11y), declarative module manifests, and Sacrilege-specific authoring panels (Circle Editor, Encounter Editor, Dialogue Graph, Sin-Signature, Act/Phase, AI Director Sandbox, Material Forge, PCG Node Graph, Shader Forge)
- **All gameplay logic in C++23** — no scripting VM, no Lua, no Python in shipped binaries, no custom language (offline `tools/` Python: `01` §2.2.1)
- **Bounded runtime ML** — on-device C++ inference only under the `01` §2.2.2 carve-out (deterministic, seed-fed, pinned weights, no internet): AI Director, symbolic adaptive music, optional neural material evaluator
- **Custom 3D assets** authored by the team and cooked by the engine's asset pipeline (Ed25519-signed)
- A **speedrunner's paradise** — frame-perfect movement, deterministic physics, seed-sharable levels, gameplay replay capture < 1 MB
- Infinite replayability via **Blacklake** — same Hell Seed = same level, always; different seed = different hell

### What It Is Not

- Not a scripting language IDE — there is no `Ripple Script` as a player-visible language. Logic is C++.
- Not a universe simulator — no planets, no orbital mechanics, no galaxy generation. *Sacrilege* is arena-scale (Silentium is a special mirror-dimension procedural profile, not open world).
- Not a survival/open-world game — it is a fast FPS with procedural arenas
- Not an "AI game" — runtime ML is bounded (≤ 0.74 ms/frame total on RX 580), presentation-or-rollback-safe, always deterministic, never internet-connected
- Not a general-purpose engine for third parties today — the seven-service contract is the **future** platform surface; *Sacrilege* is the v1 consumer

---

## Document Suite — Twelve Canonical Files

| # | File | Contents |
|---|------|----------|
| — | `README.md` | **You are here** — vision, IA, reading order |
| 01 | `01_CONSTITUTION_AND_PROGRAM.md` | Non-negotiables (incl. §2.2.2 runtime-AI carve-out), authorization boundaries, session handoff |
| 02 | `02_ROADMAP_AND_OPERATIONS.md` | 29-phase schedule (~190w horizon), Kanban rituals, milestone definitions |
| 03 | `03_PHILOSOPHY_AND_ENGINEERING.md` | Brew Doctrine, horror-studio posture, content-engine principles, code-review playbook (A–K) |
| 04 | `04_SYSTEMS_INVENTORY.md` | Complete subsystem inventory, Tier A/B/C (incl. Runtime AI, Narrative, Franchise Services) |
| 05 | `05_RESEARCH_BUILD_AND_STANDARDS.md` | Vulkan guide, C++23 discipline, deterministic-ML standard, build instructions |
| 06 | `06_ARCHITECTURE.md` | Module topology (incl. ai_runtime, narrative, services), subsystem deep-dives, editor architecture |
| 07 | `07_SACRILEGE.md` | GW-SAC-001 — the complete game specification |
| 08 | `08_BLACKLAKE.md` | Blacklake Framework — HEC/SDR/GPTM + cook-time AI augmentation |
| 09 | `09_NETCODE_DETERMINISM_PERF.md` | Netcode, determinism rules (incl. runtime-AI determinism), performance budgets |
| 10 | `10_APPENDIX_ADRS_AND_REFERENCES.md` | Glossary, research references, ADR archive (up to ADR-0117) |
| 11 | `11_NARRATIVE_BIBLE.md` | **Sacrilege Story Bible** — cosmology, cast (Malakor/Niccolò), three Acts, location skins, factions, Grace finale, franchise future |

**Auxiliary (not in the L0–L7 stack):** `COMPOSER_CONTEXT.md` — short Composer session primer; if it disagrees with `01`, `CLAUDE.md`, or this README, **those win**. **`13_THIRD_PARTY_ASSETS.md`** — AmbientCG sync, cook, disk policy, and fallback downloader.

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
| L4b | `11_NARRATIVE_BIBLE.md` | Story Bible (cosmology, cast, Acts, Grace finale, franchise future) |
| L5 | `04_SYSTEMS_INVENTORY.md`, `09_NETCODE_DETERMINISM_PERF.md` | Subsystem inventory and contracts |
| L6 | `02_ROADMAP_AND_OPERATIONS.md`, `05_RESEARCH_BUILD_AND_STANDARDS.md` | Schedule and build detail |
| L7 | ADRs in `10_APPENDIX_ADRS_AND_REFERENCES.md` | Point-in-time decisions |

**Conflict resolution:** `01` → `07` → `11` → `08` → `06` → `04` → `09`. ADRs always win over prose. When `07` and `11` disagree, `07` owns mechanics and budgets; `11` owns narrative, voice, and canon lore.

**Implementation completeness (Cursor):** The **Stub completion program** plan (saved in Cursor Plans) is the engineering closeout for **live code** (no dead stubs in shipped surfaces). It does **not** override `01`: words like **optional** in this README still mean **product / player** optionals where the Constitution allows — not “leave the C++ path empty.” See `02_ROADMAP_AND_OPERATIONS.md` § *Stub Completion Program alignment*.

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

### First time — 55 minutes

1. `01_CONSTITUTION_AND_PROGRAM.md` — what is allowed; what is forbidden (incl. §2.2.2 runtime-AI carve-out)
2. `07_SACRILEGE.md` — the complete game we are building
3. `11_NARRATIVE_BIBLE.md` — the Story Bible (Malakor/Niccolò, three Acts, Grace finale)
4. `02_ROADMAP_AND_OPERATIONS.md` — where we are right now (29 phases, ~190w horizon)
5. `03_PHILOSOPHY_AND_ENGINEERING.md` — why we make the choices we make (horror-studio posture)
6. `06_ARCHITECTURE.md` — how the engine and editor are shaped (incl. ai_runtime, narrative, seven services)

### Before every session — 5 minutes

1. `02_ROADMAP_AND_OPERATIONS.md` — active phase, current cards
2. `01_CONSTITUTION_AND_PROGRAM.md §2` — non-negotiables
3. `CLAUDE.md` at repo root

---

*Cold Coffee Labs — Greywater Engine — Sacrilege.*  
*"For I stand on the ridge of the universe and await nothing."*
