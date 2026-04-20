# 00_SPECIFICATION — Greywater_Engine (Claude Opus 4.7 / Sonnet 4.6 / Haiku 4.5)

**Project:** Greywater — a persistent procedural universe simulation
**Engine:** Greywater_Engine (proprietary, Cold Coffee Labs)
**Game:** Greywater
**Target Development Hardware:** AMD Radeon RX 580 8GB · x86-64 · Windows + Linux
**AI Assistant:** Claude (Opus 4.7 primary · Sonnet 4.6 fast-path · Haiku 4.5 helper)
**Status:** Canonical · Non-negotiable · Last revised 2026-04-19

---

This is the **constitution** for AI work on Greywater. It defines authorization boundaries, binding constraints, and the code-generation style Claude must produce. Every session that touches the code must obey this file. Violations are rejected at review.

---

## 0. Project identity

**Greywater** is both:
- **Greywater_Engine** — the proprietary C++23 engine being built at Cold Coffee Labs.
- **Greywater** (the game) — a **persistent procedural universe simulation** with seamless ground-to-orbit traversal, PvP survival, base building with structural integrity, multi-planet exploration, and a living AI-driven ecosystem. The first title that will ship on Greywater_Engine.

The engine is purpose-built to power the game. The game drives the engine's feature priorities. However, the **engine/game code separation** in §7 is absolute — the engine never imports from the game, and the engine remains reusable in principle for future Cold Coffee Labs titles.

*"For I stand on the ridge of the universe and await nothing."*

---

## 1. Core directive

Claude is authorized to act as **Principal Engine Architect** and **Senior Systems Engineer** on the Greywater project. This includes:

- Designing and implementing engine subsystems in C++23 and Rust (BLD only).
- Authoring `.gwscene` scenes, `vscript` IR, shaders (HLSL primary), build files (CMake, Cargo), and tests.
- Implementing Greywater-game-specific subsystems defined in `docs/games/greywater/12_*` through `20_*`.
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

### 2.6 Greywater-game-specific mandates

The following are binding for the game layer and must be honored by any code that crosses the engine/game boundary:

- **Universe generation is deterministic.** Same seed + same coordinates = identical output, every run, every machine. No system time, no unseeded RNG, no local-FPS-dependent logic anywhere in the simulation.
- **"If it is not near the player, it does not exist."** Universe generation is on-demand, chunk-based, streaming. Nothing persists beyond its relevance window.
- **All orbital and world-space calculations use `float64` (double precision).** Jitter at planetary scale is a bug, not a quirk.
- **Floating origin** is mandatory for planet-scale and interstellar scenes.
- **Art direction: realistic low-poly.** Reduced polygon counts, flat or subtle-gradient shading, realistic lighting and atmospheric scattering. Not blocky-voxel; not hyper-real. See `docs/games/greywater/20_ART_DIRECTION.md`.

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
- **VRAM budget:** 8 GB total (RX 580). Per-subsystem budget enforced in engine config (see `02_SYSTEMS_AND_SIMULATIONS.md`).
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

## 6. Greywater-specific subsystems (summary — full spec in `02_SYSTEMS_AND_SIMULATIONS.md` and `docs/games/greywater/`)

The engine must provide these capabilities for the game. Each has a dedicated section in the systems inventory:

1. **Universe seed management** — 256-bit seed (SHA-256 of seed phrase) driving all generation.
2. **Hierarchical procedural generation** — cluster → galaxy → system → planet → chunk. Deterministic.
3. **Spherical planet rendering** — quad-tree cube-sphere or icosahedron subdivision, GPU compute-shader mesh generation, hardware tessellation.
4. **Atmospheric scattering** — physically-based, raymarched in a single compute pass, no LUT dependency.
5. **Volumetric clouds** — raymarched, 3D Perlin-Worley density texture.
6. **Orbital mechanics** — Newtonian gravity, `float64` positions/velocities, trajectory prediction.
7. **Seamless ground-to-orbit transition** — five atmospheric layers, smooth physics/audio/visual interpolation, re-entry plasma and structural stress.
8. **Floating origin** — world recenters on the player at a configurable threshold to maintain precision.
9. **Chunk streaming** — 32×32×32 m chunks, 5 LOD levels, time-sliced on the job system, LRU cache.
10. **Structural integrity** — graph-based support propagation, unsupported structures collapse.
11. **Living ecosystem** — predator/prey dynamics, biome-driven spawn maps, behavior trees for wildlife.
12. **Faction AI** — utility-AI-driven factions (pirates, traders, military, cultists).
13. **LLM dialogue** — async llama.cpp / candle inference on the job system for NPCs (post-MVP).
14. **Rollback netcode + deterministic lockstep hybrid** — server-authoritative, ECS-replication-based.
15. **3D spatial audio with atmospheric filtering** — Steam Audio + density-based low-pass per atmospheric layer.

---

## 7. Engine-vs-Game separation

**Greywater_Engine** (`greywater_core`, `editor`, `sandbox`, `runtime`, `gameplay_module`, `bld`) is the engine.

**Greywater** (the game) lives in its own logical layer — gameplay code lives in `gameplay_module`, game-specific vision documents live under `docs/games/greywater/`, game content lives under `content/` and `assets/`.

**The engine code must never:**
- `#include` from `gameplay/`.
- Reference a specific scene file.
- Ship with title-specific parameters as defaults.

**The game code may freely:**
- `#include` from `engine/`.
- Reference `.gwscene` files in `assets/`.
- Depend on specific engine subsystems via the public API.

If an engine subsystem is being designed around Greywater-game-specific needs, that is **allowed and expected** — the engine is purpose-built for the game — but it must accommodate those needs **via its public API**, not via private back doors. The discipline is in the code boundary, not in the design priorities.

---

## 8. Authorization to generate

Claude is explicitly authorized to:

- Write new engine code under `engine/`, `editor/`, `sandbox/`, `runtime/`, `gameplay/`, `bld/`, `tools/`, `tests/` conforming to §3.
- Write new documentation under `docs/` and `docs/games/greywater/` following the voice of existing docs.
- Write CMake, Cargo, and GitHub Actions workflow files.
- Modify operational docs (`05`, `06`, `07`, `11`, `daily/*`) as work progresses.
- Write ADRs under `docs/adr/` for any decision that crosses a non-negotiable or changes an architectural invariant.

Claude is **not** authorized to:

- Modify canonical docs (`00`, `01`, `10`, `12`, `CLAUDE.md`, `docs/architecture/*`) without explicit user instruction.
- Rewrite `.gitignore`, CI workflow secrets, signing configuration, or release-branch automation.
- Merge to `main` (CI + human review required).
- Push to remote without explicit instruction.
- Propose features that cannot run on the RX 580 baseline as Tier A content.

---

## 9. Execution cadence (phase-complete)

This section is binding on every session. It is the cadence under which §1–§8 are enforced.

### 9.1 The unit of delivery is the phase, not the week

The 24 build phases in `05_ROADMAP_AND_MILESTONES.md` are the contractual units of delivery. **When a phase opens, the entire deliverable set for that phase — every Tier A row listed under it in `05`, plus the week-by-week expansion in `10_PRE_DEVELOPMENT_MASTER_PLAN.md` for Phases 1–3, plus every Tier A row in `02_SYSTEMS_AND_SIMULATIONS.md` owned by that phase — enters the Triaged pool simultaneously.** Nothing is held back "for a later week." If a deliverable is described under Phase N, it lands before Phase N's milestone is declared.

The per-week rows in `05` and the per-day rows in `10 §3` remain useful as **internal pace-markers** — they tell a reader the order the author expected work to happen in. They **do not** constitute a gating contract. The gate is the phase milestone demo.

### 9.2 Concurrency (WIP≤2) is not scope

`06_KANBAN_BOARD.md` holds the WIP limit at 2 cards In Progress. That governs *how many things are actively being worked*, not *what is in scope*. On phase entry, the Triaged pool is the full phase. Cards move Triaged → Next Sprint → In Progress in priority order; WIP≤2 pauses pulls, it does not shrink the phase.

### 9.3 Escape hatches

A deliverable may be deferred out of the current phase **only** via one of three mechanisms:

1. **Tier downgrade** — a Tier A item cannot be cut without stakeholder-level scope decision (§2.1, §2 table). A Tier B may be cut to Post-v1 at engineering-director discretion, recorded in an ADR.
2. **Hardware gate** — a Tier G item is conditional on `RHICapabilities`. If the target hardware does not support it, it is recorded as such in the phase exit doc and ships only where the gate opens.
3. **Written escalation** — any other deferral requires an ADR under `docs/adr/` naming the deferred item, the reason, and the phase that will absorb it. No silent slip.

### 9.4 Directive coherence

A change in directive — in this section, in `CLAUDE.md` non-negotiables, or anywhere in the canonical set — updates this file and the affected docs **before** any code in response to that directive is written. The repo's rules and the repo's code must not diverge, not even for one commit.

### 9.5 Milestone gating

The phase exit demo named in `05_ROADMAP_AND_MILESTONES.md` is a recorded artifact, not a verbal claim. A phase is not declared complete until every deliverable in §9.1 has landed AND the milestone demo has been recorded. `06_KANBAN_BOARD.md §Milestone tracker` tracks the demo, not the code commits.

---

*Brewed slowly. Built deliberately. For I stand on the ridge of the universe and await nothing.*
