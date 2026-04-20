# MASTER_PLAN_BRIEF — Greywater_Engine

**Document owner:** Principal Engine Architect, Cold Coffee Labs
**Audience:** Executive Stakeholders, Engineering Leads, Investors
**Date:** 2026-04-19
**Status:** Active — drives the 37-month buildout

---

## The mandate

Cold Coffee Labs is chartering **Greywater** — a two-part program comprising:

- **Greywater_Engine** — a proprietary C++23 game engine built from first principles on 2026 engineering standards, targeting the **AMD Radeon RX 580 8GB at 1080p / 60 FPS** as its baseline hardware.
- **Greywater (the game)** — the first title shipping on the engine. A persistent procedural universe simulation with seamless ground-to-orbit traversal, PvP survival, base building with structural integrity, living AI ecosystems, and hybrid rollback + deterministic-lockstep multiplayer.

The mandate is to produce, within **29 months** (plus ongoing LTS sustenance), a release-candidate engine and first-title RC demo, with a differentiated competitive moat the major engines cannot match — *precisely because we refuse to cater only to RTX-class hardware and because we refuse to bolt AI on as a plugin*.

## Why we are doing this

Four reasons, in order of weight:

1. **Engineering sovereignty.** Every byte of the runtime is ours. No per-seat licenses, no royalty ceilings, no vendor-imposed roadmaps. We own the stack end-to-end.
2. **A genuinely differentiated product.** The industry in 2026 is converging on the same set of features in slightly different wrappers. Greywater commits to five choices that no major engine matches simultaneously: (a) a native-Rust AI copilot (BLD) as a first-class subsystem, (b) a text-IR-native visual scripting system that AI can author, (c) a Clang-only reproducible build on Windows and Linux, (d) a data-oriented ECS + Vulkan 1.2-baseline renderer targeting mid-range hardware from day one, and (e) a deterministic procedural universe simulation as the flagship gameplay.
3. **Accessibility by hardware target.** By choosing the RX 580 as the baseline, we include a vastly wider player base than the large commercial engines address out of the box. Greywater runs on hardware that has been in the field for a decade — and it runs *well* because every feature is designed around that hardware, not retrofitted as a "Low preset."
4. **A forcing function on quality.** BLD's tool surface is the engine's public C++ API. If the agent cannot do a task, the API is incomplete. This forcing function produces a materially cleaner codebase than engines where tool-use is an afterthought.

## What we are building

### The engine
A **static-library C++23 engine core** plus a **Rust BLD crate** linked into the editor, sandbox, and runtime executables. Editor UI is Dear ImGui. In-game UI is RmlUi. Renderer is **Vulkan 1.2-baseline** with a forward+ clustered reference implementation (Vulkan 1.3 features opportunistic). ECS is archetype-based, in-house, on a custom fiber-based job scheduler. Gameplay code hot-reloads as a dynamic library.

Subsystems with concrete library choices or custom implementations: audio (miniaudio + Steam Audio), physics (Jolt), animation (Ozz + ACL), networking (GameNetworkingSockets), localization (ICU + HarfBuzz), persistence (SQLite + custom save format), telemetry (Sentry), and more. Full inventory in `02_SYSTEMS_AND_SIMULATIONS.md`.

### The game (Greywater)
A persistent procedural universe: hierarchical seed-driven generation from galaxy cluster → system → planet → chunk; spherical planet rendering with quad-tree cube-sphere LOD; physically-based atmospheric scattering and volumetric clouds; orbital mechanics with `float64` precision; seamless ground-to-orbit transition through five atmospheric layers with re-entry plasma and structural stress; base building with graph-based structural-integrity collapse; PvP survival loop (Gather → Craft → Build → Raid → Dominate → Survive); hybrid rollback + deterministic-lockstep netcode; living ecosystem with predator/prey dynamics and utility-AI factions; optional async LLM dialogue post-MVP. Full game-vision specification in `docs/games/greywater/12_*` through `20_*`.

**Art direction: realistic low-poly.** Reduced polygon counts with realistic lighting and atmospheric scattering. The sky carries the mood. Seven specific properties characterise the style — see `docs/games/greywater/20_ART_DIRECTION.md` for the canonical specification.

## What we are not building

- Not another commercial-engine clone. Not pursuing feature-parity with anything.
- Not a tutorial engine, not an educational toy, not an open-source community project.
- Not a Mac, console, or mobile engine in v1. Those are post-scope.
- Not an RTX-only engine. Ray tracing / mesh shaders / work graphs are hardware-gated and post-v1.
- Not a custom UI framework. ImGui + RmlUi is the locked stack.
- Not a JavaScript / Python / C# scripting language. Visual scripting is a typed text IR; gameplay extension is hot-reloaded C++.
- Not a general-purpose engine unrelated to a specific game. Greywater_Engine is *purpose-built* for Greywater the game, with a clean engine/game code boundary so future Cold Coffee Labs titles can also ship on it.

## Shape of the work

**Twenty-four build phases plus ongoing LTS sustenance. 162 weeks, ~37 months.** Named milestones gate each phase that has one. Phases 1–18 build the engine and its general-purpose subsystems. Phases 19–23 build the Greywater-game-specific subsystems. Phase 24 hardens and ships. Phase 25 sustains.

| Phase | Name | Weeks | Duration | Milestone |
| ----- | ---- | ----- | -------- | --------- |
| 1 | Scaffolding & CI/CD | 001–004 | 4w | *First Light* |
| 2 | Platform & Core Utilities | 005–009 | 5w | — |
| 3 | Math, ECS, Jobs & Reflection | 010–016 | 7w | *Foundations Set* |
| 4 | Renderer HAL & Vulkan Bootstrap | 017–024 | 8w | — |
| 5 | Frame Graph & Reference Renderer | 025–032 | 8w | *Foundation Renderer* |
| 6 | Asset Pipeline & Content Cook | 033–036 | 4w | — |
| 7 | Editor Foundation | 037–042 | 6w | *Editor v0.1* |
| 8 | Scene Serialization & Visual Scripting | 043–047 | 5w | — |
| 9 | BLD (Brewed Logic Directive, Rust) | 048–059 | 12w | *Brewed Logic* |
| 10 | Runtime I — Audio & Input | 060–065 | 6w | — |
| 11 | Runtime II — UI, Events, Config, Console | 066–071 | 6w | *Playable Runtime* |
| 12 | Physics | 072–077 | 6w | — |
| 13 | Animation & Game AI Framework | 078–083 | 6w | *Living Scene* |
| 14 | Networking & Multiplayer | 084–095 | 12w | *Two Client Night* |
| 15 | Persistence & Telemetry | 096–101 | 6w | — |
| 16 | Platform Services, i18n & a11y | 102–107 | 6w | *Ship Ready* |
| 17 | Shader Pipeline Maturity, Materials & VFX | 108–113 | 6w | — |
| 18 | Cinematics & Mods | 114–118 | 5w | *Studio Ready* |
| 19 | **Greywater Universe Generation** | 119–126 | 8w | *Infinite Seed* |
| 20 | **Greywater Planetary Rendering** | 127–134 | 8w | *First Planet* |
| 21 | **Greywater Atmosphere & Space** | 135–142 | 8w | *First Launch* |
| 22 | **Greywater Survival Systems** | 143–148 | 6w | *Gather & Build* |
| 23 | **Greywater Ecosystem & Faction AI** | 149–154 | 6w | *Living World* |
| 24 | Hardening & Release | 155–162 | 8w | *Release Candidate* |
| 25 | LTS Sustenance | 163–∞ | ongoing | (quarterly LTS cuts) |

Full schedule with week-by-week deliverables: `05_ROADMAP_AND_MILESTONES.md`.

## Risk posture

Risk #1 is **solo-dev / small-team burnout.** The plan defends against it structurally: WIP limit of 2 on the Kanban, one task per day, mandatory rest days, 30-minute weekly retros with no coding. See `06_KANBAN_BOARD.md` §10 (Anti-Patterns) and `10_PRE_DEVELOPMENT_MASTER_PLAN.md` §1 (risks).

Risk #2 is **FFI tax on the Rust BLD boundary.** Mitigated by a narrow, stable C-ABI generated by cbindgen; `bld-ffi` is covered by Miri nightly; every boundary change requires an ADR.

Risk #3 is **hardware / driver drift** on Vulkan across vendors. Mitigated by pinned SDK version, validation layers on in debug, RenderDoc captures archived for every release candidate.

Full risk matrix: `architecture/grey-water-engine-blueprint.md` §6.

## Deliverable at month 37

- Release-candidate-quality engine, LTS branch cut, installer signed and packaged for Windows and Linux
- BLD shipping with ~60 tools covering scene, component, build, runtime, code, docs, vscript, debug, project
- Sponza scene at 144 FPS @ 1440p with forward+ lighting
- Multiplayer sandbox demo at 4 clients with voice chat
- WCAG 2.2 AA accessibility audit passed
- Engineering handbook, onboarding guide, LTS runbook — all indexable by BLD
- Ten-minute board-room demo running end-to-end without intervention

## The ask of the reader

- **Stakeholders:** review `architecture/grey-water-engine-blueprint.md` and sign.
- **Engineers:** read the canonical set (README → MASTER → 00 → 01 → architecture/) and start at Phase 1.
- **AI agents:** read `CLAUDE.md` first, every session.

---

*Brewed with BLD. Built deliberately. Shipped by Cold Coffee Labs.*
