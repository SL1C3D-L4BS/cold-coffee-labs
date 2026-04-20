# Greywater — Documentation

This directory is the documentation suite for **Greywater** — the proprietary game engine (`Greywater_Engine`) **and** the flagship game it powers (`Greywater`), both built by **Cold Coffee Labs**.

- **Engine-side docs:** numbered `00`–`12`, plus `architecture/`, plus operational files.
- **Game-side docs:** under `games/greywater/`, numbered `12`–`20` (extending the root-level `00`–`12` canonical sequence).
- **Hardware baseline:** AMD Radeon RX 580 8GB @ 1080p / 60 FPS. Every Tier A decision is filtered through what this card can do.
- **Art direction:** realistic low-poly (see `games/greywater/20_ART_DIRECTION.md`).

The documents below form a single coherent system. Read them in the order given. Do not skim the canonical set — those are binding.

---

## Reading order

### First time (~55 minutes)

| Order | Document                                            | Read time | Role          |
| ----- | --------------------------------------------------- | --------- | ------------- |
| 1     | `README.md` *(this file)*                           | 2 min     | Map           |
| 2     | `MASTER_PLAN_BRIEF.md`                              | 5 min     | Mandate       |
| 3     | `00_SPECIFICATION_Claude_Opus_4.7.md`               | 15 min    | Constitution  |
| 4     | `01_ENGINE_PHILOSOPHY.md`                           | 10 min    | Values        |
| 5     | `12_ENGINEERING_PRINCIPLES.md`                      | 15 min    | Review rubric |
| 6     | `architecture/grey-water-engine-blueprint.md`       | 20 min    | Formal spec   |
| 7     | `architecture/greywater-engine-core-architecture.md`| 15 min    | Narrative     |

### Before any coding session (~10 minutes)

1. `daily/YYYY-MM-DD.md` — today's log
2. `git log --oneline -10`
3. `CLAUDE.md` (project root)
4. `11_HANDOFF_TO_CLAUDE_CODE.md` — session protocol
5. `06_KANBAN_BOARD.md` — what's In Progress
6. Relevant numbered doc for current phase

---

## Document hierarchy

### Canonical (binding — do not modify in flight)
| File | Purpose |
| --- | --- |
| `00_SPECIFICATION_Claude_Opus_4.7.md` | Constraints, directives, authorization boundaries for AI agents |
| `01_ENGINE_PHILOSOPHY.md` | Values, aesthetic, commitments — *The Brew Doctrine* |
| `10_PRE_DEVELOPMENT_MASTER_PLAN.md` | 90-day sprint plan, code generation, first-week scaffolding |
| `12_ENGINEERING_PRINCIPLES.md` | Code-review playbook — binding at every Review |
| `architecture/grey-water-engine-blueprint.md` | Formal architectural blueprint (board-ready) |
| `architecture/greywater-engine-core-architecture.md` | Narrative core-architecture walkthrough |

### Operational (updated frequently)
| File | Purpose |
| --- | --- |
| `05_ROADMAP_AND_MILESTONES.md` | 18-phase plan + LTS Sustenance, 125-week schedule, named milestones |
| `06_KANBAN_BOARD.md` | Workflow, WIP limits, rituals, milestone tracker |
| `07_DAILY_TODO_GENERATOR.py` | Generates pre-dated daily logs (Python, run once at project start) |
| `11_HANDOFF_TO_CLAUDE_CODE.md` | Per-session cold-start protocol and CMake presets |
| `daily/YYYY-MM-DD.md` | Daily log; one per working day across the 29-month build |

### Reference (stable, updated for clarification only)
| File | Purpose |
| --- | --- |
| `02_SYSTEMS_AND_SIMULATIONS.md` | Complete system inventory, Tier A/B/C classification |
| `03_VULKAN_RESEARCH_GUIDE.md` | Rendering deep-dive, staged Vulkan learning path |
| `04_LANGUAGE_RESEARCH_GUIDE.md` | C++23 discipline + Rust-for-BLD patterns |
| `08_GLOSSARY.md` | Alphabetical technical terms with citations |
| `09_APPENDIX_RESEARCH_AND_REFERENCES.md` | Research summaries, external references |

### Archival (preserved with notice)
| File | Purpose |
| --- | --- |
| `90_GREYWATER_VISION_SUBMITTED_2026-04-19.md` | Original board-submission vision, preserved as-is |

### Game-side (Greywater the game)
| File | Purpose |
| --- | --- |
| `games/greywater/README.md` | Greywater game overview — pillars, pitch, reading order |
| `games/greywater/12_GAME_ARCHITECTURE.md` | Universe hierarchy, coordinates, deterministic rule systems |
| `games/greywater/13_PROCEDURAL_UNIVERSE.md` | Seed-to-world generation, noise, biomes, resources, WFC |
| `games/greywater/14_PLANETARY_SYSTEMS.md` | Spherical planets, quad-tree LOD, terrain, oceans, vegetation |
| `games/greywater/15_ATMOSPHERE_AND_SPACE.md` | Atmospheric scattering, volumetric clouds, orbital mechanics, re-entry |
| `games/greywater/16_SURVIVAL_AND_BUILDING.md` | Resources, crafting, base building, structural integrity, raiding |
| `games/greywater/17_MULTIPLAYER.md` | Rollback + deterministic-lockstep hybrid, ECS replication, anti-cheat |
| `games/greywater/18_ECOSYSTEM_AI.md` | Wildlife, factions, navigation, LLM dialogue, RL inference |
| `games/greywater/19_AUDIO_DESIGN.md` | Spatial audio, atmospheric filtering, re-entry audio |
| `games/greywater/20_ART_DIRECTION.md` | **Realistic low-poly style guide (binding for all art)** |

### Bootstrap
| File | Purpose |
| --- | --- |
| `bootstrap_command.txt` | One-line directory creation for first checkout |
| `snippets/` | Pattern-demonstrating code examples |

---

## Naming conventions

- Numbered prefixes (`00`–`11`) indicate canonical reading order.
- `90`–`99` indicate archival / historical.
- `architecture/` holds the formal architecture specs (separate because they are referenced by external stakeholders).
- `daily/` holds one file per working day across the full 29-month build.
- All documents are Markdown (`.md`) except `07_DAILY_TODO_GENERATOR.py` (script) and `snippets/*.cpp|*.rs` (example code).

---

## Non-negotiables

The canonical set enforces a small number of commitments that **cannot** be modified without executive sign-off. The most important ones:

- **Clang/LLVM only** for C++. No MSVC. No GCC.
- **Rust only for BLD.** Nowhere else.
- **ImGui is editor-only;** in-game UI is **RmlUi**.
- **Text IR is the ground-truth** for visual scripting; the node graph is a projection.
- **No custom UI framework.** Do not propose one.

Full non-negotiables list in `00_SPECIFICATION_Claude_Opus_4.7.md` §2.

---

## Studio

**Cold Coffee Labs** — slicedlabs.founder@proton.me
Repo: `git@github.com:SL1C3D-L4BS/cold-coffee-labs.git`

*Brewed with BLD.*
