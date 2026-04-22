# 01 — Constitution, brief & session handoff



---

## Merged from `00_SPECIFICATION_Claude_Opus_4.7.md` *(constitution — now part of this file)*

# Constitution — Greywater_Engine (Claude Opus 4.7 / Sonnet 4.6 / Haiku 4.5)

**Documentation map:** `docs/README.md` — information architecture, layers, precedence (2026 reboot).

**Project:** Greywater Engine — proprietary runtime; **Sacrilege** is the flagship title it presents
**Engine:** Greywater_Engine (proprietary, Cold Coffee Labs)
**Game:** *Sacrilege* (Cold Coffee Labs → Greywater Engine → Sacrilege — one unified program)
**Target Development Hardware:** AMD Radeon RX 580 8GB · x86-64 · Windows + Linux
**AI Assistant:** Claude (Opus 4.7 primary · Sonnet 4.6 fast-path · Haiku 4.5 helper)
**Status:** Canonical · Non-negotiable · Last revised 2026-04-21

---

This is the **constitution** for AI work on Greywater. It defines authorization boundaries, binding constraints, and the code-generation style Claude must produce. Every session that touches the code must obey this file. Violations are rejected at review.

---

## 0. Project identity

**One unified system:**
- **Cold Coffee Labs** — the studio.
- **Greywater_Engine** — the proprietary C++23 engine; the platform.
- ***Sacrilege*** — the debut title that Greywater Engine presents; the forcing function for priorities and the player-facing product.

The engine is purpose-built to ship **Sacrilege** first. The **engine/game code separation** in §7 is absolute — the engine never imports from the game, and the engine remains reusable for future Cold Coffee Labs titles.

**Binding product documentation:** ***Sacrilege*** — `docs/07_SACRILEGE.md`. **Netcode/determinism:** `docs/09_NETCODE_DETERMINISM_PERF.md`. **Inventory:** `docs/04_SYSTEMS_INVENTORY.md`.

### 0.1 Supra-AAA production bar

**“AAA”** here means publisher-scale polish on the *surface* — not our ceiling. Cold Coffee Labs targets **supra-AAA**: a bar **above** conventional licensed-engine blockbuster development, measured on axes major studios rarely combine:

1. **Stack sovereignty** — full engine, tools, procedural stack, and AI authoring (BLD) in-house; no royalty ceiling, no roadmap veto from a platform vendor.  
2. **Deterministic scale** — Blacklake-class procedural generation (HEC / SDR / GPTM / TEBS) with byte-stable seeds, not a hand-painted open world with proc garnish.  
3. **Agent-native pipeline** — text-IR truth, MCP, and compile-time tool surfaces so **human + machine** throughput matches a team an order of magnitude larger.  
4. **Inclusive performance** — Tier A experiences **proven** on RX 580-class hardware; high-end is a resolution/FPS ladder, not a separate “PC ultra only” build.  
5. **IP-grade systems** — architecture and math documented for **patent and trade-secret** posture where applicable; we ship novelty, not checklists cloned from GDC slides.

*Sacrilege* is the first proof of this bar — not a demo reel, a **shipping** title on a **custom** stack.

*"For I stand on the ridge of the universe and await nothing."*

---

## 1. Core directive

Claude is authorized to act as **Principal Engine Architect** and **Senior Systems Engineer** on the Greywater project. This includes:

- Designing and implementing engine subsystems in C++23 and Rust (BLD only).
- Authoring `.gwscene` scenes, `vscript` IR, shaders (HLSL primary), build files (CMake, Cargo), and tests.
- Implementing **Sacrilege**-driven game subsystems per `docs/07_SACRILEGE.md`.
- Diagnosing bugs, refactoring systems, proposing ADRs.

Claude is **not** authorized to:
- Introduce a new compiler, new build system, new UI framework, or new scripting language without an approved ADR.
- Propose features that **cannot run on the RX 580 baseline** as Tier A content.
- Weaken any non-negotiable in §2.
- Commit secrets, API keys, or credentials.
- Rewrite canonical documents without explicit instruction.

---

## 2. Technical constraints (non-negotiable)

### 2.1 Hardware baseline — RX 580 reality

The **AMD Radeon RX 580 8GB** (Polaris, 2016, GCN 4.0, Vulkan 1.2/1.3) is the **baseline development and target platform**. Every design decision is filtered through what this GPU can do.

**Supported on RX 580 (Tier A — ships):**
- Vulkan 1.2 core + a subset of Vulkan 1.3 features (dynamic rendering, timeline semaphores, `VK_KHR_synchronization2`, descriptor indexing, buffer device address).
- Compute shaders (up to compute-capability 5).
- Async compute queue (AMD has this natively).
- Bindless descriptors via `VK_EXT_descriptor_indexing`.
- VMA-managed 8 GB VRAM budget.
- Hardware tessellation.
- 1080p @ 60 FPS target for the full simulation.

**Not supported on RX 580 (Tier G — hardware-gated, post-v1 at minimum):**
- Ray tracing pipeline (`VK_KHR_ray_tracing_pipeline`, `VK_KHR_acceleration_structure`, `VK_KHR_ray_query`).
- Mesh shaders (`VK_EXT_mesh_shader`).
- GPU work graphs (Vulkan 1.4).
- AV1 hardware decode.

**Consequences for the renderer:**
- No ray tracing in the baseline renderer — we use **screen-space techniques** (SSAO, SSR, SSGI) and pre-baked / IBL global illumination.
- No mesh shaders in the baseline culling path — we use **compute-shader-driven GPU culling** with `vkCmdDrawIndexedIndirect`.
- No work graphs in the baseline scheduler — the **CPU orchestrates the frame graph**, dispatching compute and draw on separate queues.

Advanced features are gated by `gw::render::RHICapabilities` and enabled only where the hardware reports support.

### 2.2 Language and toolchain

1. **C++23** for the engine core, editor, sandbox, runtime, gameplay module. Path to C++26 as Clang lands features. **No STL in engine hot paths** — use `gw::core::Array`, `gw::core::HashMap`, `gw::core::String` (rendering, physics, AI tick, procedural generation). STL allowed in editor, tools, tests.
2. **Rust stable (2024 edition)** for BLD only (the AI copilot subsystem). Nowhere else.
3. **Clang / LLVM 18+** only. On Windows, `clang-cl` targets the MSVC ABI. MSVC and GCC unsupported.
4. **CMake 3.28+** with Presets + Ninja. **Corrosion** bridges Cargo into CMake for BLD.
5. **Vulkan 1.2 baseline** (with 1.3 features opportunistic via `RHICapabilities`). **WebGPU** backend planned for post-v1 web/sandbox targets. **DirectX 12** and **Metal** are Post-Scope.
6. **Windows (x64) + Linux (x64)** from day one. macOS, consoles, mobile are post-scope.

### 2.3 UI stack — locked

- **Editor UI:** Dear ImGui + docking + viewports + ImNodes + ImGuizmo. **No custom UI framework.**
- **In-game UI** (HUD, menus): **RmlUi** with HarfBuzz + FreeType shaping. **Not ImGui.**

### 2.4 ECS & core

- **In-house archetype ECS** in `engine/ecs/`. No EnTT, no flecs dependency.
- **Custom fiber-based work-stealing scheduler** in `engine/jobs/`. No `std::async`, no raw `std::thread`.
- **No global `new`/`delete`** in engine code. Allocators passed explicitly (frame, arena, pool, system).
- **OS headers** only inside `engine/platform/`. Enforced by custom `clang-tidy` check.

### 2.5 BLD, scripting, MCP

- **BLD** lives outside `greywater_core` as a Rust crate. The engine core cannot depend on Rust code.
- **MCP (Model Context Protocol) spec `2025-11-25`** via `rmcp`. BLD is the server.
- **Visual scripting:** typed text IR is ground-truth. Node graph is a reversible projection with a sidecar layout file.
- **Hot-reload:** only the gameplay module (C++ dylib) hot-reloads. The Rust BLD crate and the engine core do not.

### 2.6 *Sacrilege* / engine-world mandates

The following are binding for the title layer and for `engine/world/` where it serves **Blacklake** + ***Sacrilege***:

- **Procedural generation is deterministic.** Same seed + same coordinates = identical output, every run, every machine. No wall-clock dependence, no unseeded RNG, no FPS-dependent sampling in generation pipelines.
- **"If it is not near the player, it does not exist."** On-demand, chunk-based streaming; nothing is simulated beyond its relevance window.
- **World-space math uses `float64` where scale demands it** (large arenas, planetary-scale Blacklake modes). Jitter from precision is a bug.
- **Floating origin** is mandatory whenever the camera traverses large extents.
- **Flagship art direction:** *Sacrilege* — authoritative in `docs/07_SACRILEGE.md` (GW-SAC-001 and follow-on numbered docs).

---

## 3. Code generation style guide

### C++

- **Standard:** C++23 (C++26 features as Clang stabilizes and CI clears).
- **Files:** `.hpp` for headers (use `#pragma once`), `.cpp` for sources.
- **Types:** PascalCase (`VulkanDevice`, `World`, `EntityHandle`).
- **Functions and methods:** `snake_case` (`create_swapchain`, `register_component`).
- **Member variables:** trailing underscore (`device_`, `frame_index_`).
- **Namespaces:** `snake_case` (`gw::render`, `gw::ecs`, `gw::jobs`, `gw::universe`).
- **Macros:** `UPPER_SNAKE` with `GW_` prefix (`GW_ASSERT`, `GW_LOG`, `GW_UNREACHABLE`).
- **Enums:** `enum class` always. Values PascalCase.
- **No exceptions** in engine code (validated at system boundaries via `Result<T, E>` or C++23 `std::expected`).
- **No RTTI** in hot paths.
- **No raw `new` / `delete`.** Use `std::unique_ptr`, `gw::Ref<T>`, or stack scope. (§12 A1.)
- **No two-phase `init()` / `shutdown()` on public APIs.** (§12 A2.)
- **No C-style casts.** (§12 B1.)
- **`std::string_view` never passed to a C API.** (§12 E1.)
- **Lambda captures always explicit** — no `[=]`, no `[&]`. (§12 D1.)
- **All world-space positions `float64`.** Single-precision `float` only for local-to-chunk math.

### Rust (BLD only)

- **Edition:** 2024 (stable; 2021 until 2024 lands on stable).
- **Style:** `rustfmt` defaults; `clippy` with `-D warnings`.
- **Everything `snake_case` except types (`PascalCase`) and constants (`SCREAMING_SNAKE`).**
- **`unsafe` requires a `// SAFETY:` comment.** Enforced by a custom clippy lint.
- **No `unwrap`/`expect` in `bld-ffi` or `bld-governance`.**
- **Tool definitions:** `#[bld_tool]` proc macro always.

### Shaders

- **HLSL primary** via DXC → SPIR-V. GLSL supported for ports.
- **Permutations** generated at cook time.
- **Flat shading or subtle-gradient shading** by default (realistic low-poly aesthetic).

### Build files

- `CMakeLists.txt` follows modern target-based design. No directory-level side effects.
- `Cargo.toml` pinned; `Cargo.lock` committed.

---

## 4. Memory and performance policy

- **Frame allocator** (`gw::memory::FrameAllocator`) — default for transient per-frame work. Reset once per frame.
- **Arena allocators** — scoped to subsystems, levels, or load phases.
- **Pool allocators** — fixed-size, for hot component types and particle systems.
- **System heap** — last resort, permitted only at startup or for OS-owned buffers.
- **No STL containers in hot paths.**
- **Chunk streaming:** LRU caching, time-sliced generation across multiple frames (no single-frame stalls).
- **VRAM budget:** 8 GB total (RX 580). Per-subsystem budget enforced in engine config (see `docs/04_SYSTEMS_INVENTORY.md`).
- **Benchmarks** live with tests. **Perf regression ≥ 5 % on canonical scenes fails CI.**
- **Target: 1080p @ 60 FPS** on RX 580 reference hardware for the full simulation (Sponza-class terrain + 2 km visible range + 10 000 active entities + atmospheric + clouds).

---

## 5. AI context awareness

Claude must be aware when operating on this project:

- **Model family in use:** Opus 4.7 (1M context) for deep work, Sonnet 4.6 for iterative coding, Haiku 4.5 for short tasks. Behave identically under the same directives.
- **This is a real studio product.** Shipping commercial titles depends on this engine. Treat every decision accordingly.
- **BLD is the AI-agent subsystem inside the engine.** Do not confuse "Claude Code working on this repo" with "BLD running inside the editor." They are different agents at different times.
- **The engine's public C++ API is the BLD tool surface.** Every public API decision should be made with BLD's usability in mind.
- **Undo/redo integration is mandatory.** Every mutating operation constructs an `ICommand`. No back doors.
- **RX 580 first.** Whenever a design choice would only work on higher-end hardware, flag it, gate it behind `RHICapabilities`, and provide a fallback for RX 580.

---

## 6. Title-facing engine capabilities (*Sacrilege* + Blacklake)

**Full inventory:** `docs/04_SYSTEMS_INVENTORY.md`. **Flagship behavior:** `docs/07_SACRILEGE.md`. **Procedural math:** `docs/08_BLACKLAKE_AND_RIPPLE_STUDIO_SPECS.md`.

The platform must provide (non-exhaustive; tiers in `docs/04_SYSTEMS_INVENTORY.md`, phases in `docs/02_ROADMAP_AND_OPERATIONS.md`):

1. **Master seed & HEC-style derivation** — deterministic sub-seeds for arenas, circles, encounters.  
2. **Blacklake pipeline** — SDR/GPTM/TEBS and streaming chunk pipeline for hellscapes and inverted circles.  
3. **GPU terrain / organic mesh** — compute-driven surfaces, LOD, tessellation as budget allows (RX 580).  
4. **Forward+ rendering + GPU decals** — blood, persistent reads, horror-forward post.  
5. **Local volumetrics & fog** — smoke, censer clouds, circle-specific atmosphere.  
6. **Martyrdom gameplay hooks** — ECS state for Sin, Mantra, Rapture/Ruin; ability system.  
7. **High-speed player movement** — swept controller, slide, wall-kick, grapple, surf hooks per flagship spec.  
8. **AI & BT / vscript** — data-driven enemies; BT shares VM path with visual-script IR (ADRs 0009, 0043).  
9. **Networking** — replication, rollback-safe regions, lockstep where required (`09_NETCODE_DETERMINISM_PERF.md`, ADRs 0047–0055).  
10. **Spatial audio + dynamic score** — Steam Audio path; mixer supports intensity layers for *Sacrilege*.  
11. **BLD + MCP** — authoring for circles, encounters, tuning via tools.  

Future Cold Coffee Labs titles may extend `engine/world/`; the **code boundary** in §7 stays clean.

---

## 7. Engine-vs-game separation

**Greywater_Engine** (`greywater_core`, `editor`, `sandbox`, `runtime`, `bld`, and engine-side `engine/world/`) is the platform.

***Sacrilege*** lives in the **game layer**: gameplay code in `gameplay_module`, design docs in `docs/07_SACRILEGE.md`, content in `content/` and `assets/`.

**Engine code must never:**
- `#include` from `gameplay/`.
- Reference a title-specific scene as a hardcoded default.
- Encode *Sacrilege* names, roster strings, or scenario scripts in `engine/` (data-driven only).

**Game code may:**
- `#include` from public `engine/` APIs.
- Load `.gwscene` and assets from `assets/`.

The engine is purpose-built for ***Sacrilege*** first; needs are met through **public API**, never gameplay includes.

---

## 8. Authorization to generate

Claude is explicitly authorized to:

- Write new engine code under `engine/`, `editor/`, `sandbox/`, `runtime/`, `gameplay/`, `bld/`, `tools/`, `tests/` conforming to §3.
- Write new documentation under `docs/` and `docs/07_SACRILEGE.md` following established tone.
- Write CMake, Cargo, and GitHub Actions workflow files.
- Update `docs/02_ROADMAP_AND_OPERATIONS.md` and `docs/01_CONSTITUTION_AND_PROGRAM.md` as work progresses.
- Record new ADRs in `docs/10_APPENDIX_ADRS_AND_REFERENCES.md` (append using the ADR template there) for any decision that crosses a non-negotiable or changes an architectural invariant.

Claude is **not** authorized to:

- Modify canonical docs (`docs/01_CONSTITUTION_AND_PROGRAM.md`, `docs/03_PHILOSOPHY_AND_ENGINEERING.md`, `CLAUDE.md`, `docs/06_ARCHITECTURE.md`) without explicit user instruction.
- Rewrite `.gitignore`, CI workflow secrets, signing configuration, or release-branch automation.
- Merge to `main` (CI + human review required).
- Push to remote without explicit instruction.
- Propose features that cannot run on the RX 580 baseline as Tier A content.

---

## 9. Execution cadence (phase-complete)

This section is binding on every session. It is the cadence under which §1–§8 are enforced.

### 9.1 The unit of delivery is the phase, not the week

The 24 build phases in `02_ROADMAP_AND_OPERATIONS.md` are the contractual units of delivery. **When a phase opens, the entire deliverable set for that phase — every Tier A row listed under it in `02`, plus the week-by-week expansion in `01_CONSTITUTION_AND_PROGRAM.md` for Phases 1–3, plus every Tier A row in `docs/04_SYSTEMS_INVENTORY.md` owned by that phase — enters the Triaged pool simultaneously.** Nothing is held back "for a later week." If a deliverable is described under Phase N, it lands before Phase N's milestone is declared.

The per-week rows in `02` and the per-day rows in `01` §3 remain useful as **internal pace-markers** — they tell a reader the order the author expected work to happen in. They **do not** constitute a gating contract. The gate is the phase milestone demo.

### 9.2 Concurrency (WIP≤2) is not scope

`docs/02_ROADMAP_AND_OPERATIONS.md` holds the WIP limit at 2 cards In Progress. That governs *how many things are actively being worked*, not *what is in scope*. On phase entry, the Triaged pool is the full phase. Cards move Triaged → Next Sprint → In Progress in priority order; WIP≤2 pauses pulls, it does not shrink the phase.

### 9.3 Escape hatches

A deliverable may be deferred out of the current phase **only** via one of three mechanisms:

1. **Tier downgrade** — a Tier A item cannot be cut without stakeholder-level scope decision (§2.1, §2 table). A Tier B may be cut to Post-v1 at engineering-director discretion, recorded in an ADR.
2. **Hardware gate** — a Tier G item is conditional on `RHICapabilities`. If the target hardware does not support it, it is recorded as such in the phase exit doc and ships only where the gate opens.
3. **Written escalation** — any other deferral requires an ADR under `docs/10_APPENDIX_ADRS_AND_REFERENCES.md` naming the deferred item, the reason, and the phase that will absorb it. No silent slip.

### 9.4 Directive coherence

A change in directive — in this section, in `CLAUDE.md` non-negotiables, or anywhere in the canonical set — updates this file and the affected docs **before** any code in response to that directive is written. The repo's rules and the repo's code must not diverge, not even for one commit.

### 9.5 Milestone gating

The phase exit demo named in `02_ROADMAP_AND_OPERATIONS.md` is a recorded artifact, not a verbal claim. A phase is not declared complete until every deliverable in §9.1 has landed AND the milestone demo has been recorded. `docs/02_ROADMAP_AND_OPERATIONS.md §Milestone tracker` tracks the demo, not the code commits.

---

*Brewed slowly. Built deliberately. For I stand on the ridge of the universe and await nothing.*


---

## Merged from `DOCUMENTATION_SYSTEM.md` *(information architecture)*

# Documentation system — Greywater Engine

**Status:** Canonical  
**Revised:** 2026-04-21  
**Directive:** Single unified program — **Cold Coffee Labs → Greywater Engine → *Sacrilege***. Legacy game docs removed (ADR-0084). Studio specs live in **`08_BLACKLAKE_AND_RIPPLE_STUDIO_SPECS.md`** (ADR-0085 consolidation). **Supra-AAA** bar: §0.1 in the constitution section above.

---

## 1. Purpose

This file defines how `docs/` is organized. Engine capability, flagship product, **studio mathematical / architectural specifications**, and operations are the only layers.

**Engineering history** (what shipped):

- **`docs/02_ROADMAP_AND_OPERATIONS.md`** — phase closeouts, test counts, milestones (merged roadmap + Kanban).  
- **`docs/10_APPENDIX_ADRS_AND_REFERENCES.md`** — all ADRs + glossary + appendix.

---

## 2. Layers of truth

| Layer | Role | Paths |
| ----- | ---- | ----- |
| **L0** | Navigation | `docs/README.md` |
| **L1** | Constitution & handoff | `docs/01_CONSTITUTION_AND_PROGRAM.md`, repo `CLAUDE.md` |
| **L2** | Values & review | `docs/03_PHILOSOPHY_AND_ENGINEERING.md` |
| **L3** | Architecture | `docs/06_ARCHITECTURE.md` |
| **L3b** | Studio deep specs | `docs/08_BLACKLAKE_AND_RIPPLE_STUDIO_SPECS.md` |
| **L4** | Flagship title | `docs/07_SACRILEGE.md` |
| **L5** | Contracts & inventory | `docs/04_SYSTEMS_INVENTORY.md`, `docs/09_NETCODE_DETERMINISM_PERF.md` |
| **L6** | Operations | `docs/02_ROADMAP_AND_OPERATIONS.md` |
| **L7** | Decision records | `docs/10_APPENDIX_ADRS_AND_REFERENCES.md` (ADR archive) |

**Precedence:** `docs/01_CONSTITUTION_AND_PROGRAM.md` + `CLAUDE.md` → `docs/07_SACRILEGE.md` → `docs/08_BLACKLAKE_AND_RIPPLE_STUDIO_SPECS.md` → `docs/06_ARCHITECTURE.md` → `docs/04_SYSTEMS_INVENTORY.md`.

### Studio specifications (L3b)

| File | Designation | Contents |
| ---- | ----------- | -------- |
| `docs/08_BLACKLAKE_AND_RIPPLE_STUDIO_SPECS.md` (Blacklake section) | BLF-CCL-001-REV-A | HEC, SDR, GPTM, TEBS, procedural math |
| `docs/08_BLACKLAKE_AND_RIPPLE_STUDIO_SPECS.md` (Ripple section) | GWE-ARCH-001-REV-A | Greywater Engine, Ripple IDE, Ripple Script — reconcile with shipped `vscript`/BLD via ADRs |

---

## 3. Repository map

```
docs/
├── README.md
├── 01_CONSTITUTION_AND_PROGRAM.md
├── 02_ROADMAP_AND_OPERATIONS.md
├── 03_PHILOSOPHY_AND_ENGINEERING.md
├── 04_SYSTEMS_INVENTORY.md
├── 05_RESEARCH_BUILD_AND_STANDARDS.md
├── 06_ARCHITECTURE.md
├── 07_SACRILEGE.md
├── 08_BLACKLAKE_AND_RIPPLE_STUDIO_SPECS.md
├── 09_NETCODE_DETERMINISM_PERF.md
└── 10_APPENDIX_ADRS_AND_REFERENCES.md
```

---

## 4. Authoring rules

| Content | Location |
| ------- | -------- |
| *Sacrilege* mechanics, levels, art | `docs/07_SACRILEGE.md` |
| Blacklake + GWE / Ripple / RSL | `docs/08_BLACKLAKE_AND_RIPPLE_STUDIO_SPECS.md` (+ ADRs in `docs/10_…` when code diverges) |
| Engine subsystem narrative | `docs/06_ARCHITECTURE.md` + ADR |
| Netcode / rollback / perf annexes | `docs/09_NETCODE_DETERMINISM_PERF.md` + `docs/01_…` + `docs/03_…` |
| Schedule & rituals | `docs/02_ROADMAP_AND_OPERATIONS.md` + ADR |

---

## 5. Completed work (immutable summary)

Phases **1–17** closed as recorded in **`docs/02_ROADMAP_AND_OPERATIONS.md`** (merged roadmap). That record is not reinterpreted.

---

*One map. One flagship. Studio specs beside the constitution.*


---

## Merged from `MASTER_PLAN_BRIEF.md`

# MASTER_PLAN_BRIEF — Greywater Engine

**Document owner:** Principal Engine Architect, Cold Coffee Labs
**Audience:** Executive Stakeholders, Engineering Leads, Investors
**Date:** 2026-04-21 (ADR-0083–0085 — unified IA, legacy purge, studio specs consolidated into `08_`)  
**Status:** Active — drives the 37-month buildout

---

## The mandate

**One unified system.** Cold Coffee Labs charters **Greywater Engine** — the proprietary C++23 platform — and Greywater Engine **presents *Sacrilege*** as the debut title. The program comprises:

- **Greywater_Engine** — built from first principles on 2026 engineering standards, targeting the **AMD Radeon RX 580 8GB at 1080p / 60 FPS** as its baseline hardware.
- ***Sacrilege*** — the flagship game: what the engine ships first; vision under `docs/07_SACRILEGE.md`. Procedural depth is expressed through the **Blacklake** framework (`docs/08_BLACKLAKE_AND_RIPPLE_STUDIO_SPECS.md`).

The mandate is to produce, within **29 months** (plus ongoing LTS sustenance), a release-candidate engine and first-title RC demo, with a differentiated competitive moat the major engines cannot match — *precisely because we refuse to cater only to RTX-class hardware and because we refuse to bolt AI on as a plugin*.

## Supra-AAA production bar

Cold Coffee Labs does **not** treat “AAA” as the finish line — that label usually describes **high polish on a rented engine** with **procurement-driven scope**. Our target is **supra-AAA**:

- **Sovereignty** — Greywater Engine, Blacklake, BLD, and *Sacrilege* are **one owned stack**; no engine vendor owns our feature clock or royalty math.  
- **Depth over decoration** — deterministic procedural scale (see `docs/08_BLACKLAKE_AND_RIPPLE_STUDIO_SPECS.md`) and simulation contracts that licensed stacks do not expose to small teams.  
- **Agentic throughput** — BLD + MCP + text-IR tooling so a lean studio runs with **effective headcount** far above payroll.  
- **Inclusive baseline** — Tier A proven on **RX 580**; ultra settings are a ladder, not a separate game.  
- **Defensible novelty** — systems documented for **IP posture** (patent/trade secret), not feature parity clones.

Binding language: `docs/01_CONSTITUTION_AND_PROGRAM.md` §0.1. *Sacrilege* is the first commercial proof, not a tech demo.

## Why we are doing this

Four reasons, in order of weight:

1. **Engineering sovereignty.** Every byte of the runtime is ours. No per-seat licenses, no royalty ceilings, no vendor-imposed roadmaps. We own the stack end-to-end.
2. **A genuinely differentiated product.** The industry in 2026 is converging on the same set of features in slightly different wrappers. Greywater Engine commits to five choices that no major engine matches simultaneously: (a) a native-Rust AI copilot (BLD) as a first-class subsystem, (b) a text-IR-native visual scripting system that AI can author, (c) a Clang-only reproducible build on Windows and Linux, (d) a data-oriented ECS + Vulkan 1.2-baseline renderer targeting mid-range hardware from day one, and (e) a deterministic **Blacklake** procedural stack in service of ***Sacrilege*** and future titles — not a bolt-on biome painter.
3. **Accessibility by hardware target.** By choosing the RX 580 as the baseline, we include a vastly wider player base than the large commercial engines address out of the box. Greywater runs on hardware that has been in the field for a decade — and it runs *well* because every feature is designed around that hardware, not retrofitted as a "Low preset."
4. **A forcing function on quality.** BLD's tool surface is the engine's public C++ API. If the agent cannot do a task, the API is incomplete. This forcing function produces a materially cleaner codebase than engines where tool-use is an afterthought.

## What we are building

### The engine
A **static-library C++23 engine core** plus a **Rust BLD crate** linked into the editor, sandbox, and runtime executables. Editor UI is Dear ImGui. In-game UI is RmlUi. Renderer is **Vulkan 1.2-baseline** with a forward+ clustered reference implementation (Vulkan 1.3 features opportunistic). ECS is archetype-based, in-house, on a custom fiber-based job scheduler. Gameplay code hot-reloads as a dynamic library.

Subsystems with concrete library choices or custom implementations: audio (miniaudio + Steam Audio), physics (Jolt), animation (Ozz + ACL), networking (GameNetworkingSockets), localization (ICU + HarfBuzz), persistence (SQLite + custom save format), telemetry (Sentry), and more. Full inventory in `docs/04_SYSTEMS_INVENTORY.md`.

**Snapshot (2026-04-21):** Phase 17 (*Studio Renderer*) is engineering-complete — shader permutation matrix + optional Slang path, `MaterialWorld` / `.gwmat` v1, GPU VFX (particles, ribbons, deferred decals), and the `engine/render/post/` chain (dual-Kawase bloom, k-DOP TAA, McGuire motion blur, CoC DoF, PBR Neutral + ACES). `dev-win` ships **711/711** CTests; exit marker: `sandbox_studio_renderer` → `STUDIO RENDERER`. Next phase gate: Phase 18 (*Studio Ready*). See `02_ROADMAP_AND_OPERATIONS.md` §Phase 17.

### The game (*Sacrilege*)
The shipping product face of Greywater Engine. Full vision docs accumulate under `docs/07_SACRILEGE.md` (start with `README.md`). Engine capabilities live in the codebase and in `docs/06_ARCHITECTURE.md`; **Blacklake** thesis: `docs/08_BLACKLAKE_AND_RIPPLE_STUDIO_SPECS.md`. IDE/Ripple depth: `docs/08_BLACKLAKE_AND_RIPPLE_STUDIO_SPECS.md`.

**Art direction** for shipping is entirely in `docs/07_SACRILEGE.md` (GW-SAC-001 and follow-on docs).

## What we are not building

- Not another commercial-engine clone. Not pursuing feature-parity with anything.
- Not a tutorial engine, not an educational toy, not an open-source community project.
- Not a Mac, console, or mobile engine in v1. Those are post-scope.
- Not an RTX-only engine. Ray tracing / mesh shaders / work graphs are hardware-gated and post-v1.
- Not a custom UI framework. ImGui + RmlUi is the locked stack.
- Not a JavaScript / Python / C# scripting language. Visual scripting is a typed text IR; gameplay extension is hot-reloaded C++.
- Not a general-purpose engine unrelated to a flagship. Greywater_Engine is *purpose-built* to ship ***Sacrilege*** first, with a clean engine/game code boundary so future Cold Coffee Labs titles can also ship on it.

## Shape of the work

**Twenty-four build phases plus ongoing LTS sustenance. 162 weeks, ~37 months.** Named milestones gate each phase that has one. Phases 1–18 build the engine and its general-purpose subsystems. Phases 19–23 build **Blacklake + *Sacrilege***-facing subsystems for the RC demo (see `docs/02_ROADMAP_AND_OPERATIONS.md`). Phase 24 hardens and ships. Phase 25 sustains.

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
| 17 | Shader Pipeline Maturity, Materials & VFX | 108–113 | 6w | *Studio Renderer* |
| 18 | Cinematics & Mods | 114–118 | 5w | *Studio Ready* |
| 19 | **Blacklake Core & Deterministic Genesis** | 119–126 | 8w | *Infinite Seed* |
| 20 | **Arena Topology, GPTM & Sacrilege LOD** | 127–134 | 8w | *Nine Circles Ground* |
| 21 | **Sacrilege Frame — Rendering, Atmosphere, Audio** | 135–142 | 8w | *Hell Frame* |
| 22 | **Martyrdom Combat & Player Stack** | 143–148 | 6w | *Martyrdom Online* |
| 23 | **Damned Host, Circles Pipeline & Boss** | 149–154 | 6w | *God Machine RC* |
| 24 | Hardening & Release | 155–162 | 8w | *Release Candidate* |
| 25 | LTS Sustenance | 163–∞ | ongoing | (quarterly LTS cuts) |

Full schedule with week-by-week deliverables: `02_ROADMAP_AND_OPERATIONS.md`.

**Schedule index:** Phases are numbered **1–25** on this table. Legal, repo hygiene, and CI bootstrap before roadmap week **001** is **pre-Phase-1 bootstrap** — not a numbered “Phase 0.”

## Risk posture

Risk #1 is **solo-dev / small-team burnout.** The plan defends against it structurally: WIP limit of 2 on the Kanban, one task per day, mandatory rest days, 30-minute weekly retros with no coding. See `docs/02_ROADMAP_AND_OPERATIONS.md` §10 (Anti-Patterns) and `01_CONSTITUTION_AND_PROGRAM.md` §1 (risks).

Risk #2 is **FFI tax on the Rust BLD boundary.** Mitigated by a narrow, stable C-ABI generated by cbindgen; `bld-ffi` is covered by Miri nightly; every boundary change requires an ADR.

Risk #3 is **hardware / driver drift** on Vulkan across vendors. Mitigated by pinned SDK version, validation layers on in debug, RenderDoc captures archived for every release candidate.

Full risk matrix: `docs/06_ARCHITECTURE.md` §6.

## Deliverable at month 37

- Release-candidate-quality engine, LTS branch cut, installer signed and packaged for Windows and Linux
- BLD shipping with ~60 tools covering scene, component, build, runtime, code, docs, vscript, debug, project
- Sponza scene at 144 FPS @ 1440p with forward+ lighting (**stretch / non-baseline** — marketing or high-tier demo; Tier-A constitution baseline remains **1080p @ 60 FPS** on RX 580 per §2.1)
- Multiplayer sandbox demo at 4 clients with voice chat
- WCAG 2.2 AA accessibility audit passed
- Engineering handbook, onboarding guide, LTS runbook — all indexable by BLD
- Ten-minute board-room demo running end-to-end without intervention

## The ask of the reader

- **Stakeholders:** review `docs/06_ARCHITECTURE.md` and sign.
- **Engineers:** read the canonical set (README → MASTER → 00 → 01 → architecture/) and start at Phase 1.
- **AI agents:** read `CLAUDE.md` first, every session.

---

*Brewed with BLD. Built deliberately. Shipped by Cold Coffee Labs.*


---

## Merged from `10_PRE_DEVELOPMENT_MASTER_PLAN.md`

## First 90 days — master plan

**Status:** Canonical · Binding for the first 13 weeks
**Horizon:** First 90 days (Phase 1 + Phase 2 + first four weeks of Phase 3, within the 24-phase roadmap plus LTS)
**Date:** 2026-04-19

This is the plan that drives the first three months of Greywater_Engine. Everything after week 13 is governed by `02_ROADMAP_AND_OPERATIONS.md`. What you find here is the **design crystallization** plus **ready-to-compile code generation** for week-one scaffolding.

**Unified program map:** `docs/README.md` — flagship ***Sacrilege***; Phases 19–23 retargeted (ADR-0083–0084); studio specs at `docs/BLACKLAKE_*` / `docs/GREYWATER_ENGINE_*` (ADR-0085). **Supra-AAA** bar: `docs/01_CONSTITUTION_AND_PROGRAM.md` §0.1.

---

## 1. Executive summary & technical viability

Greywater_Engine is technically viable on 2026 infrastructure. The three risks most likely to sink the project, in order:

### Risk #1 — Solo/small-team burnout

**Likelihood:** High. **Impact:** Catastrophic. **Mitigation:**

- Kanban WIP limit of 2 is hard-enforced (`docs/02_ROADMAP_AND_OPERATIONS.md`).
- Daily log discipline → explicit single-focus blocks.
- Two mandatory rest days per month.
- 30-minute Sunday retro with **no coding**.
- Milestones are gated by recorded demos, not by "it compiles" — forcing the team to experience the work as users.

### Risk #2 — FFI boundary tax on BLD

**Likelihood:** Medium. **Impact:** Medium (bounded by design). **Mitigation:**

- The C-ABI between Rust and C++ is narrow and stable. Every change requires an ADR.
- Miri runs nightly on `bld-ffi` to catch UB.
- Tool-call frequency is low (dozens/minute) and payloads are already serialized (MCP JSON) — the FFI tax does not land in the per-frame hot path.
- If Rust ecosystem shifts in 2027, the boundary is narrow enough to swap BLD's language with a defined effort (~4 weeks).

### Risk #3 — Hardware / driver divergence across Vulkan vendors

**Likelihood:** Medium. **Impact:** Medium. **Mitigation:**

- Validation layers on in debug; zero tolerance for warnings in release-candidate build.
- Reference hardware list is published; exit criteria are measured on it.
- RenderDoc captures archived for every milestone.
- `RHICapabilities` gates optional features; code paths conditionally adapt without runtime branching in shaders.

All three risks have *structural* mitigations — baked into workflow and architecture, not into "remember to do this."

---

## 2. Critical design decisions (crystallized)

The following are not up for re-argument during the build. They are captured here so a reader understands the *why* behind the scaffolding.

### 2.1 Renderer — custom HAL over Vulkan 1.3

**Decision.** Hand-written HAL in `engine/render/hal/` with a Vulkan 1.3 backend. WebGPU post-v1.

**Why.** `wgpu` is shaped for the WebGPU intersection of APIs and costs us modern-Vulkan features. `ash` is the same as hand-rolling in C++ (same unsafe API, different syntax). Hand-rolling in C++ keeps the engine-core dependency graph clean.

**Cost we accept.** ~4 months of HAL + renderer implementation work (Phases 4–5).

### 2.2 ECS — in-house, archetype-based

**Decision.** Custom `engine/ecs/`. Reference EnTT/flecs for design but do not depend on them.

**Why.** The ECS is the most load-bearing subsystem. Owning it end-to-end is strategically correct. Also: the ECS is where BLD's tool surface is densest (`component.*`, `scene.*`) — we want to control reflection and schema evolution.

**Cost.** Higher risk and longer Phase 3. Mitigated by keeping design simple and referencing proven patterns.

### 2.3 Language split — C++23 engine, Rust BLD only

**Decision.** Rust nowhere except the BLD crate. The rest of the codebase is C++23.

**Why.** Three concrete Rust wins in BLD: proc-macro tool surfaces > libclang codegen, `rmcp` > any C++ MCP SDK, `candle`/`llama-cpp-rs` > piecing together libcurl + simdjson. Everywhere else, Rust loses (renderer, editor ecosystem, AAA precedent, hiring pool, template metaprogramming, hot-reload).

**Cost.** FFI boundary maintenance. Mitigated by keeping the boundary narrow.

### 2.4 UI stack — locked ImGui + RmlUi

**Decision.** Editor UI is Dear ImGui + docking + viewports + ImNodes + ImGuizmo, **locked**. In-game UI is RmlUi. No custom UI framework.

**Why.** Writing a UI framework is an 18-36 month distraction with no product differentiation. Our product is the engine, not the UI. ImGui is the correct answer for engine editors in 2026 and will still be correct in 2030.

**Cost.** RmlUi has a smaller community than commercial UI. Mitigated by scoping UI tightly and contributing upstream when we find bugs.

### 2.5 Visual scripting — text IR ground-truth

**Decision.** A typed text IR is canonical; the node graph is a reversible projection with a sidecar layout file.

**Why.** BLD can read and write text. Humans can diff and merge text. Blueprint-style graph-as-ground-truth doesn't merge and forces nativization. We avoid both problems.

**Cost.** IR design in Phase 8 is slightly slower than "just use the graph." Recouped every time a merge doesn't conflict.

### 2.6 Job system — custom fiber-based

**Decision.** Custom work-stealing fiber scheduler (`engine/jobs/`). Fibers preserve stack state across yields; worker threads pull from work-stealing deques; fibers let long-running tasks suspend without thread explosion.

**Why.** Fibers preserve stack across yields, critical for long-running tasks (asset cook, RAG indexing, physics step). `std::async` and `std::thread` cannot express this. Third-party (`marl`, TBB) either constrains design or adds weight.

**Cost.** Custom fiber primitives on Linux need validation — `makecontext`/`swapcontext` have ABI pitfalls. Mitigated by a small platform-specific shim and focused tests.

### 2.7 Hot-reload scope

**Decision.** Only the C++ gameplay module hot-reloads. The Rust BLD crate and the engine core do not.

**Why.** Rust hot-reload is hard (monomorphization, trait-object layout). Engine-core hot-reload would require dynamic linking through the entire core — too invasive. Gameplay module hot-reload covers the 90 % case (gameplay iteration).

**Cost.** Engine-core changes require a restart to see. Acceptable.

### 2.8 Memory policy

**Decision.** No global `new`/`delete` in engine code. No STL containers in hot paths. Allocators passed explicitly.

**Why.** Allocation discipline is a performance and correctness property. STL container allocator customization is painful; bespoke containers are cleaner.

**Cost.** Engineers coming from STL-heavy codebases need an onboarding ramp. Mitigated by `engine/core/` exposing familiar-shaped APIs (`span`, `vector`, `hash_map`) with allocator-first signatures.

---

## 3. 90-day sprint plan (weeks 01–13)

Covers Phase 1 (scaffolding, 4w) + Phase 2 (Platform & Core Utilities, 5w) + first 4 weeks of Phase 3 (Math, ECS, Jobs & Reflection). HAL bootstrap begins in Phase 4 at week 17 — outside this 13-week horizon.

> **Cadence note (binding).** The Monday-through-Friday breakdown below is a **pace budget**, not a per-day gate. When Phase 1 opens, all Phase 1 deliverables are in-scope simultaneously (see `docs/01_CONSTITUTION_AND_PROGRAM.md` §9, `CLAUDE.md` non-negotiable #19, `docs/02_ROADMAP_AND_OPERATIONS.md §Phase entry`). The per-day breakdown tells you the order the author expected things to happen in — not "what is allowed to land today." A session that ships Monday's, Tuesday's, and Wednesday's items together is not violating anything; it is executing the phase. The gate is the phase milestone (*First Light* at week 004 for Phase 1), not the week row.

### Week 01 — Repository & build skeleton

**Pull card:** `P1.01` (`chore: scaffold repo and CMake root`).

Monday:
- Run `docs/bootstrap_command.txt` to create the directory tree.
- Commit the empty skeleton: `feat(scaffold): initialize engine/editor/sandbox/runtime/gameplay/bld trees`.
- Write root `CMakeLists.txt` (see §6.1 below).
- Write `CMakePresets.json` (see §6.2 below).

Tuesday:
- Integrate CPM.cmake: add `cmake/dependencies.cmake`.
- Pin first four C++ dependencies: `glfw`, `volk`, `VulkanMemoryAllocator`, `doctest`. Commit SHAs in `cmake/dependencies.cmake`.
- Write `cmake/toolchains/clang-cl.cmake` for Windows and `cmake/toolchains/clang.cmake` for Linux.

Wednesday:
- Set up `bld/` Cargo workspace with stub crates: `bld-mcp`, `bld-tools`, `bld-provider`, `bld-rag`, `bld-agent`, `bld-governance`, `bld-bridge`, `bld-ffi`. Each has a minimal `Cargo.toml` and `lib.rs` with one test.
- Write `rust-toolchain.toml` pinning stable.
- Integrate Corrosion into the root CMake: `find_package(Corrosion REQUIRED)` and `corrosion_import_crate(MANIFEST_PATH bld/Cargo.toml)`.

Thursday:
- Write `.github/workflows/ci.yml` with the matrix: `{windows-latest, ubuntu-latest} × {debug, release}`.
- Steps: checkout → install Vulkan SDK → `cmake --preset {os}-{config}` → `cmake --build` → `ctest`.
- Add `cargo build --release` and `cargo test` for the Rust side.

Friday:
- Write `.clang-format`, `.clang-tidy`, `rustfmt.toml`, `.editorconfig`, `.gitattributes`.
- Open first PR; land with green CI on both OSes and both configs.

Exit criteria:
- `cmake --preset dev-linux && cmake --build --preset dev-linux` produces an empty but linking set of artifacts on Linux.
- Same on Windows with `dev-win`.
- CI green on `main`.

### Week 02 — Hello triangle + hello BLD

**Pull card:** `P1.02` (`feat: hello-triangle sandbox end-to-end`).

Implementation:
- `engine/platform/window.hpp` + GLFW wrapper.
- `engine/render/hal/device.hpp` — just enough to create a `VkInstance` and pick a physical device.
- `sandbox/main.cpp` — creates a window, a Vulkan device, a swapchain, records a command buffer that clears the screen.
- **Milestone 1A:** The triangle.
- `bld/bld-ffi/src/lib.rs` — exposes one function `bld_hello() -> *const c_char` returning a compiled-in greeting.
- `editor/bld_bridge/hello.cpp` — loads the greeting via `cxx`, prints to stdout.
- **Milestone 1B:** Hello BLD C-ABI smoke test.

Exit criteria (*First Light* milestone):
- `sandbox/gw_sandbox` renders a triangle on both Windows and Linux.
- `editor/gw_editor` prints the BLD greeting on both.
- Record two-minute video; commit to `docs/milestones/first_light.mp4` (or external link if binary storage is off-repo).

### Weeks 03–04 — Scaffolding polish

Card set:
- `P1.03` — `docs(standards): write CODING_STANDARDS.md derived from 00 + 04`.
- `P1.04` — `chore(ci): enable sanitizer matrix nightlies (ASan/UBSan/TSan/Miri)`.
- `P1.05` — `chore(ci): wire sccache on Windows and Linux runners`.
- `P1.06` — `docs(build): write BUILDING.md with onboarding steps`.

Exit criteria for Phase 1:
- A fresh clone on a fresh machine on either OS builds and runs both milestones via a single preset invocation.
- CI is green; blocking required-check set is configured on `main`.

### Week 05 — Platform layer (Phase 2)

**Pull cards:** `P2.01`–`P2.03`.

- `engine/platform/window.cpp` (complete GLFW wrap)
- `engine/platform/input.cpp` (keyboard + mouse baseline; SDL3 gamepad in Phase 10)
- `engine/platform/time.cpp` (high-resolution clock)
- `engine/platform/fs.cpp` (filesystem abstraction, virtual-mount prep)
- `engine/platform/dll.cpp` (dynamic-library load for hot-reload)

### Week 06 — Memory allocators (Phase 2)

**Pull cards:** `P2.04`–`P2.06`.

- `engine/memory/frame_allocator.hpp/cpp` (linear, bump, reset per frame)
- `engine/memory/arena_allocator.hpp/cpp` (stack-like, scoped)
- `engine/memory/pool_allocator.hpp/cpp` (fixed-size free list)
- `engine/memory/freelist_allocator.hpp/cpp`
- Unit tests with ASan on; zero leaks, zero UB.

### Week 07 — Core utilities (Phase 2)

**Pull cards:** `P2.07`–`P2.10`.

- `engine/core/log.hpp/cpp`
- `engine/core/assert.hpp/cpp`
- `engine/core/result.hpp` (`Result<T, E>`)
- `engine/core/error.hpp` (error taxonomy)

### Week 08 — Containers (Phase 2)

**Pull cards:** `P2.11`–`P2.15`.

- `engine/core/span.hpp`
- `engine/core/array.hpp`
- `engine/core/vector.hpp` (allocator-first signature)
- `engine/core/hash_map.hpp` (heterogeneous-lookup friendly)
- `engine/core/ring_buffer.hpp`

### Week 09 — Strings, OS-header lint (Phase 2 exit)

**Pull cards:** `P2.16`–`P2.18`.

- `engine/core/string.hpp/cpp` (UTF-8 with small-string optimisation)
- Custom `clang-tidy` check `gw-no-os-headers-outside-platform` authored and enabled.
- Phase 2 exit gate review.

### Week 10 — Math library (Phase 3 start)

**Pull cards:** `P3.01`–`P3.04`.

- `engine/math/vec.hpp` (Vec2, Vec3, Vec4 with SIMD-capable storage)
- `engine/math/mat.hpp` (Mat3, Mat4)
- `engine/math/quat.hpp`
- `engine/math/transform.hpp`
- SIMD path behind `GW_ENABLE_SIMD`; mainline correct without.

### Week 11 — Command bus (Phase 3)

**Pull cards:** `P3.05`–`P3.06`.

- `engine/core/command/command.hpp` (`ICommand`)
- `engine/core/command/stack.hpp/cpp` (`CommandStack` with transaction brackets)
- External registration API exposed via opaque function pointers (for BLD C-ABI in Phase 9).

### Week 12 — Jobs skeleton (Phase 3)

**Pull cards:** `P3.07`–`P3.09`.

- `engine/jobs/fiber.hpp/cpp` (platform-specific fiber primitives — `makecontext`/`swapcontext` on Linux, Fiber API on Windows)
- `engine/jobs/scheduler.hpp/cpp` (work-stealing deque per worker)
- `engine/jobs/job.hpp` (Job handle, dependencies)

### Week 13 — Jobs submission + parallel_for (Phase 3 continues)

**Pull cards:** `P3.10`–`P3.12`.

- `engine/jobs/submit.hpp/cpp` (user-facing submit / wait API)
- `engine/jobs/parallel_for.hpp` (convenience)
- Benchmarks versus enkiTS on reference workload (Phase 3 milestone gate is week 16; this is on-track).

Phase 3's remaining three weeks (ECS archetype storage + queries + reflection + serialization primitives + *Foundations Set* milestone demo at week 16) are covered in `02_ROADMAP_AND_OPERATIONS.md`. The 90-day Master Plan ends here.

---

## 4. Recommended C++ project structure (enforced)

```
cold-coffee-labs/
├── engine/
│   ├── platform/        OS abstraction — only place OS headers are permitted
│   ├── memory/          Allocators (frame, arena, pool, free-list)
│   ├── core/            Logger, asserts, Result, containers
│   │   ├── command/     ICommand + CommandStack
│   │   ├── events/      Pub/sub bus (Phase 11)
│   │   └── config/      CVars + TOML (Phase 11)
│   ├── math/            Vec/Mat/Quat/Transform
│   ├── jobs/            Fiber-based scheduler (Phase 3)
│   ├── ecs/             Archetype ECS
│   ├── render/
│   │   ├── hal/         Hardware Abstraction Layer
│   │   ├── vk/          Vulkan 1.3 backend
│   │   ├── shader/      Permutations, DXC/SPIR-V, optional Slang (Phase 17)
│   │   ├── material/    MaterialWorld, .gwmat v1 (Phase 17)
│   │   └── post/        Bloom, TAA, MB, DoF, tonemap (Phase 17)
│   ├── assets/          Import/cook/load
│   ├── scene/           Scene view + .gwscene serialization (Phase 8)
│   ├── scripting/       Dynamic-lib loader (gameplay hot-reload)
│   ├── vscript/         Visual-script IR runtime (Phase 8)
│   ├── audio/           miniaudio + Steam Audio (Phase 10)
│   ├── net/             GameNetworkingSockets + replication (Phase 14)
│   ├── physics/         Jolt (Phase 12)
│   ├── anim/            Ozz + ACL (Phase 13)
│   ├── input/           SDL3 + action maps (Phase 10)
│   ├── ui/              RmlUi (Phase 11)
│   ├── i18n/            ICU + HarfBuzz (Phase 16)
│   ├── a11y/            Accessibility (Phase 16)
│   ├── persist/         Save + SQLite (Phase 15)
│   ├── telemetry/       Sentry + custom (Phase 15)
│   ├── console/         In-game dev console (Phase 11)
│   ├── gameai/          BT + Recast/Detour (Phase 13)
│   ├── vfx/             Particles, ribbons, decals (Phase 17)
│   ├── sequencer/       Cinematics (Phase 18)
│   ├── mod/             Mod sandbox (Phase 18)
│   └── platform_services/   Steam/EOS/Galaxy (Phase 16)
├── bld/                 Rust crate — Cargo workspace (Phase 9)
├── editor/              C++ executable, statically links core + BLD (Phase 7)
├── sandbox/             C++ executable, engine feature test bed (Phase 1)
├── runtime/             C++ shipping executable (Phase 11)
├── gameplay/            Hot-reloadable C++ dylib (Phase 2)
├── tools/cook/          gw_cook content cooker (Phase 6)
├── tests/               Integration, perf, fuzz, BLD evals (Phase 1+)
├── content/             Source assets (gitignored)
├── assets/              Cooked deterministic runtime assets (tracked)
├── third_party/         CPM-managed, SHA-pinned
├── cmake/
│   ├── dependencies.cmake
│   ├── toolchains/
│   └── packaging/       CPack configs (Phase 18)
├── docs/                This directory
├── .github/workflows/
├── .clang-format
├── .clang-tidy
├── rustfmt.toml
├── .editorconfig
├── CMakeLists.txt
├── CMakePresets.json
├── CLAUDE.md
├── README.md
└── LICENSE
```

---

## 5. Dependency graph & initialization order

```
           +------------+
           |  Platform  |
           +-----+------+
                 |
                 v
           +------------+     +------------+
           |   Memory   |<----|   Core     |
           +-----+------+     +-----+------+
                 |                  |
                 +------+-----------+
                        |
                        v
                   +---------+
                   |  Math   |
                   +----+----+
                        |
                        v
                   +---------+
                   |  Jobs   |
                   +----+----+
                        |
          +-------------+--------------+
          |             |              |
          v             v              v
      +------+     +---------+    +--------+
      | ECS  |     |  HAL    |    |  FS/IO |
      +--+---+     +----+----+    +---+----+
         |              |             |
         +-----+--------+-------------+
               |
               v
        +---------------+
        | Scene / Asset |
        +-------+-------+
                |
                v
           +---------+
           | Editor  |
           +----+----+
                |
                v
           +--------+
           |  BLD   |  (linked via C-ABI)
           +--------+
```

Initialization at sandbox startup (canonical order):

1. `platform::init()` — window, input, timing, fs.
2. `memory::init()` — allocator pools and frame allocator.
3. `core::init()` — logger, assertions, config.
4. `jobs::init()` — scheduler and worker threads.
5. `render::hal::init()` — Vulkan device, queues, swapchain.
6. `ecs::init()` — World construction, system registration.
7. `assets::init()` — asset database.
8. `scene::init()` — scene view and serialization.
9. `scripting::init()` — hot-reload file watcher and gameplay_module load.
10. *(editor only)* `bld_bridge::init()` — Rust crate handle, MCP server.

Teardown reverses the order.

---

## 6. First-week code generation (ready to compile)

The following files are authoritative for week-one scaffolding. Copy verbatim into the repo; CI should be green after commit.

### 6.1 `CMakeLists.txt` (root)

```cmake
cmake_minimum_required(VERSION 3.28)

project(greywater
    VERSION 0.0.1
    DESCRIPTION "Greywater_Engine by Cold Coffee Labs"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(cmake/dependencies.cmake)

find_package(Corrosion REQUIRED)

# Greywater_Engine core (C++23 static library)
add_subdirectory(engine)

# BLD — Rust workspace compiled via Corrosion
corrosion_import_crate(
    MANIFEST_PATH bld/Cargo.toml
    PROFILE release
)

# Executables
add_subdirectory(sandbox)
add_subdirectory(editor)
add_subdirectory(runtime)
add_subdirectory(gameplay)

# Tools
add_subdirectory(tools/cook)

# Tests
enable_testing()
add_subdirectory(tests)
```

### 6.2 `CMakePresets.json`

```json
{
    "version": 6,
    "configurePresets": [
        {
            "name": "dev-linux",
            "displayName": "Developer Linux (Debug)",
            "generator": "Ninja",
            "binaryDir": "${sourceDir}/build/${presetName}",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Debug",
                "CMAKE_C_COMPILER": "clang",
                "CMAKE_CXX_COMPILER": "clang++",
                "CMAKE_TOOLCHAIN_FILE": "${sourceDir}/cmake/toolchains/clang.cmake"
            }
        },
        {
            "name": "dev-win",
            "displayName": "Developer Windows (Debug)",
            "generator": "Ninja",
            "binaryDir": "${sourceDir}/build/${presetName}",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Debug",
                "CMAKE_TOOLCHAIN_FILE": "${sourceDir}/cmake/toolchains/clang-cl.cmake"
            }
        },
        {
            "name": "release-linux",
            "inherits": "dev-linux",
            "cacheVariables": { "CMAKE_BUILD_TYPE": "Release" }
        },
        {
            "name": "release-win",
            "inherits": "dev-win",
            "cacheVariables": { "CMAKE_BUILD_TYPE": "Release" }
        },
        {
            "name": "linux-asan",
            "inherits": "dev-linux",
            "cacheVariables": {
                "CMAKE_CXX_FLAGS": "-fsanitize=address -fno-omit-frame-pointer"
            }
        },
        {
            "name": "linux-ubsan",
            "inherits": "dev-linux",
            "cacheVariables": { "CMAKE_CXX_FLAGS": "-fsanitize=undefined" }
        },
        {
            "name": "linux-tsan",
            "inherits": "dev-linux",
            "cacheVariables": { "CMAKE_CXX_FLAGS": "-fsanitize=thread" }
        }
    ],
    "buildPresets": [
        { "name": "dev-linux",     "configurePreset": "dev-linux" },
        { "name": "dev-win",       "configurePreset": "dev-win" },
        { "name": "release-linux", "configurePreset": "release-linux" },
        { "name": "release-win",   "configurePreset": "release-win" },
        { "name": "linux-asan",    "configurePreset": "linux-asan" },
        { "name": "linux-ubsan",   "configurePreset": "linux-ubsan" },
        { "name": "linux-tsan",    "configurePreset": "linux-tsan" }
    ],
    "testPresets": [
        { "name": "dev-linux", "configurePreset": "dev-linux", "output": { "outputOnFailure": true } },
        { "name": "dev-win",   "configurePreset": "dev-win",   "output": { "outputOnFailure": true } }
    ]
}
```

### 6.3 `cmake/dependencies.cmake`

```cmake
include(${CMAKE_SOURCE_DIR}/cmake/CPM.cmake)

CPMAddPackage(
    NAME glfw
    GITHUB_REPOSITORY glfw/glfw
    GIT_TAG 3.4
)

CPMAddPackage(
    NAME volk
    GITHUB_REPOSITORY zeux/volk
    GIT_TAG 1.3.270
)

CPMAddPackage(
    NAME VulkanMemoryAllocator
    GITHUB_REPOSITORY GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
    GIT_TAG v3.1.0
)

CPMAddPackage(
    NAME doctest
    GITHUB_REPOSITORY doctest/doctest
    GIT_TAG v2.4.11
)

# Phase 7+: imgui (docking branch), imnodes, imguizmo, imgui_md
# Phase 10+: SDL3, miniaudio, Steam Audio
# Phase 11+: RmlUi, HarfBuzz, FreeType
# Phase 12+: Jolt Physics
# Phase 13+: Ozz-animation, ACL, Recast/Detour
# Phase 14+: GameNetworkingSockets, Opus
# Phase 15+: SQLite, sentry-native
# Phase 16+: ICU
```

### 6.4 `rust-toolchain.toml`

```toml
[toolchain]
channel = "stable"
components = ["rustfmt", "clippy", "rust-src"]
profile = "minimal"
```

### 6.5 `bld/Cargo.toml` (workspace)

```toml
[workspace]
resolver = "2"
members = [
    "bld-mcp",
    "bld-tools",
    "bld-provider",
    "bld-rag",
    "bld-agent",
    "bld-governance",
    "bld-bridge",
    "bld-ffi",
]

[workspace.package]
edition = "2021"
rust-version = "1.80"
version = "0.0.1"
authors = ["Cold Coffee Labs"]
license-file = "../LICENSE"

[workspace.dependencies]
serde = { version = "1", features = ["derive"] }
serde_json = "1"
tokio = { version = "1", features = ["rt-multi-thread", "macros", "sync"] }
thiserror = "2"
anyhow = "1"

[profile.release]
lto = "thin"
codegen-units = 1
```

### 6.6 `.clang-format`

```yaml
BasedOnStyle: LLVM
IndentWidth: 4
ColumnLimit: 100
PointerAlignment: Left
ReferenceAlignment: Left
NamespaceIndentation: None
AllowShortFunctionsOnASingleLine: Empty
BinPackArguments: false
BinPackParameters: false
AlwaysBreakTemplateDeclarations: Yes
IncludeBlocks: Regroup
```

### 6.7 `.clang-tidy`

```yaml
Checks: >
  bugprone-*,
  cert-*,
  clang-analyzer-*,
  concurrency-*,
  cppcoreguidelines-*,
  misc-*,
  modernize-*,
  performance-*,
  portability-*,
  readability-*,
  -readability-magic-numbers,
  -modernize-use-trailing-return-type,
  -cppcoreguidelines-avoid-magic-numbers
HeaderFilterRegex: '^(?!.*third_party).*$'
WarningsAsErrors: '*'
```

### 6.8 `rustfmt.toml`

```toml
edition = "2021"
max_width = 100
use_small_heuristics = "Max"
imports_granularity = "Module"
```

### 6.9 `.editorconfig`

```ini
root = true

[*]
indent_style = space
indent_size = 4
end_of_line = lf
charset = utf-8
trim_trailing_whitespace = true
insert_final_newline = true

[*.{yml,yaml,toml,json}]
indent_size = 2

[Makefile]
indent_style = tab
```

---

## 7. Research deep-dive: bindless descriptors + BDA

Phase 4's correctness depends on getting bindless and buffer device address right (Phase 4 begins at week 17, after this master plan's 13-week horizon — but the groundwork is worth locking in early):

- Create one large descriptor set (e.g., 65 536 textures, 65 536 storage buffers) at device init.
- Use `VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT` + `VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT`.
- Texture handles are plain `u32`s in push constants.
- For per-draw buffer data, pass a `VkDeviceAddress` (u64) in push constants; shaders dereference via `GL_EXT_buffer_reference`.
- Validate with `VK_EXT_descriptor_indexing_features.descriptorBindingPartiallyBound = VK_TRUE` + `shaderSampledImageArrayNonUniformIndexing = VK_TRUE`.

Full treatment in `03_VULKAN_RESEARCH_GUIDE.md` §3 (Stages 2–3).

---

## 8. Daily workflow protocol (mantra)

One card. Ninety minutes. Commit. Update log. Rest.

The protocol only works if you follow it literally. See `docs/02_ROADMAP_AND_OPERATIONS.md` for the full set. The abbreviated form:

1. **Morning:** read today's daily log; check the Kanban; pull one card.
2. **Block 1 (90 min):** focused work on that card. No Slack, no browser, no "quick search."
3. **Lunch.**
4. **Block 2 (90 min, optional):** continue the card or pick up a small secondary. Max In Progress stays at 2.
5. **End of day:** move the card; update the log; commit.

If the day felt scattered, **stop earlier**, note the scatter in the log, and come back tomorrow. Pushing tired code is a future cost.

---

## 9. Final verification checklist (for sign-off)

Before declaring the Pre-Development Master Plan complete and entering Phase 1:

- [ ] `docs/01_CONSTITUTION_AND_PROGRAM.md` reviewed and signed by Principal Architect.
- [ ] `docs/03_PHILOSOPHY_AND_ENGINEERING.md` reviewed; aesthetic palette finalized.
- [ ] `docs/04_SYSTEMS_INVENTORY.md` tier assignments reviewed; all Tier A items confirmed shippable in v1.
- [ ] `02_ROADMAP_AND_OPERATIONS.md` week-by-week schedule accepted.
- [ ] `docs/02_ROADMAP_AND_OPERATIONS.md` WIP limits agreed.
- [ ] `docs/06_ARCHITECTURE.md` signed by stakeholders.
- [ ] `CLAUDE.md` present at repo root; referenced by `.github/workflows/ci.yml` as a mandatory read-on-start file.
- [ ] `bootstrap_command.txt` executes cleanly and produces the expected tree.
- [ ] `07_DAILY_TODO_GENERATOR.py` runs and produces the expected daily-log set.
- [ ] Founder/Principal Architect has taken one full day off immediately before Day 1 of Phase 1.

---

*Pre-developed deliberately. Week 1 has been rehearsed on paper so it ships on the first attempt.*


---

## Merged from `11_HANDOFF_TO_CLAUDE_CODE.md`

## Session handoff protocol

**Status:** Operational
**Purpose:** Every coding session on Greywater_Engine, whether driven by a human engineer or by Claude Code, begins with this protocol. No exceptions.

---

## Cold-start checklist (10 minutes)

Run through these six steps in order. Do not start coding until all six are complete.

### 1. Read today's daily log
```
docs/02_ROADMAP_AND_OPERATIONS.mdYYYY-MM-DD.md
```
If today's log does not exist, your `07_DAILY_TODO_GENERATOR.py` run missed a day — generate it now. Read what yesterday finished, what was blocked, what the *Tomorrow's Intention* said.

### 2. Read recent commits
```
git log --oneline -10
git status
```
Confirm the working tree is clean or intentionally dirty. Know what the last three commits did.

### 3. Re-read `CLAUDE.md` and the doc-system map
```
./CLAUDE.md  (at project root)
docs/README.md
```
The non-negotiables change subtly over time. The commitments do not. Re-read anyway. Skim `docs/README.md` when doc structure changes so you know **where flagship, architecture, ref, and ADRs live**.

### 4. Check Kanban state
```
docs/02_ROADMAP_AND_OPERATIONS.md
```
Confirm:
- How many cards are In Progress? (WIP limit is 2)
- Which card is yours for the next session?
- Is anything stuck in Review?

### 5. Read the relevant numbered doc for the current phase
Check `docs/02_ROADMAP_AND_OPERATIONS.md` for the current phase. Then read the numbered doc that matches:
- Phase 1–2 foundation work → `docs/01_CONSTITUTION_AND_PROGRAM.md`
- Rendering work → `docs/05_RESEARCH_BUILD_AND_STANDARDS.md`
- C++/Rust patterns → `docs/05_RESEARCH_BUILD_AND_STANDARDS.md`
- Systems questions → `docs/04_SYSTEMS_INVENTORY.md`
- Architectural ambiguity → `docs/06_ARCHITECTURE.md`

**Scope note.** Any deliverable listed under the active phase in `docs/02_ROADMAP_AND_OPERATIONS.md`, in `docs/04_SYSTEMS_INVENTORY.md` (Phase column), or — for Phases 1–3 — in `docs/01_CONSTITUTION_AND_PROGRAM.md §3`, is **in scope for this session** if it hasn't landed yet. Do not filter by week row. The phase is the unit of delivery (`00` §9, `CLAUDE.md` non-negotiable #19).

### 6. Copy the day's card into the daily log
In `docs/02_ROADMAP_AND_OPERATIONS.mdYYYY-MM-DD.md`, under *🎯 Today's Focus*, paste the Kanban card identifier and the *Context* field. You are now committed.

---

## CMake presets (canonical configurations)

Every Greywater build uses one of these presets. No ad-hoc `-D` flags in the IDE.

| Preset           | Config | OS      | Sanitizers | Purpose                                    |
| ---------------- | ------ | ------- | ---------- | ------------------------------------------ |
| `dev-win`        | Debug  | Windows | none       | Default developer workflow on Windows      |
| `dev-linux`      | Debug  | Linux   | none       | Default developer workflow on Linux        |
| `release-win`    | Release| Windows | none       | Release build on Windows                   |
| `release-linux`  | Release| Linux   | none       | Release build on Linux                     |
| `linux-asan`     | Debug  | Linux   | ASan       | AddressSanitizer run (nightly)             |
| `linux-ubsan`    | Debug  | Linux   | UBSan      | UndefinedBehaviorSanitizer run (nightly)   |
| `linux-tsan`     | Debug  | Linux   | TSan       | ThreadSanitizer run (nightly)              |
| `studio-linux`   | see preset | Linux | varies  | Phase 17 full shader/post stack; `phase17_budgets_perf`, `phase17_studio_renderer` |
| `studio-win`     | see preset | Windows | varies | Same on Windows |
| `ci`             | varies | both    | varies     | GitHub Actions matrix-driven               |

Usage:
```
cmake --preset dev-linux
cmake --build --preset dev-linux
ctest --preset dev-linux
```

### Rust-side (Cargo) presets

Cargo doesn't use CMake presets — it reads `rust-toolchain.toml` and `Cargo.toml`. The conventions are:

```
cargo build --release          # optimized BLD build
cargo test                     # unit tests
cargo test --release           # release-mode tests
cargo clippy -- -D warnings    # lint enforcement
cargo miri test -p bld-ffi     # UB check on the FFI boundary (nightly)
```

CMake invokes Cargo through Corrosion automatically during the main build — you only call Cargo directly when iterating on BLD.

---

## Hot commands (operational shortcuts)

```
# Full build (both C++ and Rust via Corrosion)
cmake --preset dev-linux && cmake --build --preset dev-linux

# BLD crate iteration (much faster for Rust-only changes)
cd bld && cargo build --release

# Run the sandbox
./build/dev-linux/sandbox/gw_sandbox

# Run the editor
./build/dev-linux/editor/gw_editor

# Regenerate pre-dated daily logs (one-time setup)
python git history

# Regenerate MCP tool schemas after adding a #[bld_tool]
cd bld && cargo build -p bld-tools

# Regenerate the C header for BLD after changing bld-ffi
cd bld && cargo build -p bld-bridge
```

---

## End-of-session protocol (5 minutes)

When the focus block ends:

### 1. Update the daily log
- Mark tasks completed under *📋 Tasks Completed*
- Note blockers under *🚧 Blockers*
- Write a one-sentence note under *🔮 Tomorrow's Intention*

### 2. Move the Kanban card
- If the card is done → to *Review*
- If still in progress → stays In Progress (don't exceed 2!)
- If blocked → to *Blocked* column with a reason

### 3. Commit
- Small commits. One logical change per commit. Follow Conventional Commits:
  ```
  feat(render): add timeline semaphore helper
  fix(ecs): resolve archetype-split determinism bug
  docs(blueprint): update §3.10 voice-chat latency budget
  ```
- CI runs on every push; do not break `main`.

### 4. If it is Sunday — weekly retro (30 min)
Read the seven most recent daily logs. Look for:
- What moved. What didn't.
- Which cards sat in Review too long.
- What is the *real* pace vs. planned pace for this phase.
- Is any Tier A work at risk? Escalate if so.

No coding during the weekly retro. This rule is load-bearing.

---

## If you get stuck

1. **Re-read `docs/03_PHILOSOPHY_AND_ENGINEERING.md`.** Half of all "I don't know what to do" moments dissolve once you remember the Brew Doctrine.
2. **Re-read `docs/03_PHILOSOPHY_AND_ENGINEERING.md`.** The code-review playbook names most common mistakes; odds are your blocker is listed.
3. **Write an ADR.** If the problem is architectural ambiguity, draft `docs/10_APPENDIX_ADRS_AND_REFERENCES.mdNNNN-decision-title.md` and make the trade-off explicit. Commit the ADR before committing code.
4. **Open BLD.** Ask the agent for context. `docs.search_engine_docs` is literally built for this.
5. **If it is Friday evening — stop.** Tomorrow is a rest day for a reason.

---

## What this document is not

- It is not the specification (`00`). It is not the philosophy (`01`). It is the *daily operational runbook*.
- It is not an onboarding guide. New engineers read the canonical set first.
- It is not a Kanban tutorial. `06` is.

---

*Read this file at the start of every session. Update it when the protocol changes. Ship by the protocol.*
