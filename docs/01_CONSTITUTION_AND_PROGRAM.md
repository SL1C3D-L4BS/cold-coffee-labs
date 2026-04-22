# 01 — Constitution, Program Mandate & Session Handoff

**Status:** Canonical · Non-negotiable · Last revised 2026-04-21  
**Precedence:** L1 — overrides all other documents except `README.md`

> *"For I stand on the ridge of the universe and await nothing."*

---

## §0. Project Identity

**One unified program:**

- **Cold Coffee Labs** — the studio.
- **Greywater Engine** — the proprietary C++23 / Vulkan platform.
- ***Sacrilege*** — the debut title; what the engine ships first; the forcing function for all priorities.

The engine is purpose-built to ship *Sacrilege*. The engine/game code separation in §7 is absolute — the engine never imports from the game, and the engine remains reusable for future Cold Coffee Labs titles.

**Binding product documentation:** `07_SACRILEGE.md` · **Netcode/determinism:** `09_NETCODE_DETERMINISM_PERF.md` · **Inventory:** `04_SYSTEMS_INVENTORY.md`

### §0.1 Supra-AAA Production Bar

**"AAA"** means publisher-scale polish on the surface — not our ceiling. Cold Coffee Labs targets **supra-AAA**: a bar *above* conventional licensed-engine blockbuster development, measured on axes major studios rarely combine:

1. **Stack sovereignty** — full engine, tools, procedural stack, and AI authoring (BLD) in-house; no royalty ceiling, no vendor roadmap veto.
2. **Deterministic scale** — Blacklake-class procedural generation (HEC / SDR / GPTM / TEBS) with byte-stable seeds, not a hand-painted open world with proc garnish.
3. **Agent-native pipeline** — text-IR truth, MCP, and compile-time tool surfaces so human + machine throughput matches a team an order of magnitude larger.
4. **Inclusive performance** — Tier A experiences proven on RX 580-class hardware; high-end is a resolution/FPS ladder, not a separate "PC ultra only" build.
5. **IP-grade systems** — architecture and math documented for patent and trade-secret posture where applicable; we ship novelty, not checklists cloned from GDC slides.

*Sacrilege* is the first proof of this bar — not a demo reel, a **shipping** title on a **custom** stack.

---

## §1. Core Directive

Claude is authorized to act as **Principal Engine Architect** and **Senior Systems Engineer** on the Greywater project. This includes:

- Designing and implementing engine subsystems in C++23 and Rust (BLD only).
- Authoring `.gwscene` scenes, `vscript` IR, shaders (HLSL primary), build files (CMake, Cargo), and tests.
- Implementing *Sacrilege*-driven game subsystems per `07_SACRILEGE.md`.
- Diagnosing bugs, refactoring systems, proposing ADRs.

Claude is **not** authorized to:

- Introduce a new compiler, build system, UI framework, or scripting language without an approved ADR.
- Propose features that cannot run on the RX 580 baseline as Tier A content.
- Weaken any non-negotiable in §2.
- Commit secrets, API keys, or credentials.
- Rewrite canonical documents without explicit instruction.

---

## §2. Technical Constraints (Non-Negotiable)

### §2.1 Hardware Baseline — RX 580

The **AMD Radeon RX 580 8 GB** (Polaris, 2016, GCN 4.0, Vulkan 1.2/1.3) is the baseline development and target platform. Every design decision is filtered through what this GPU can do.

**Tier A — ships on RX 580:**
- Vulkan 1.2 core + Vulkan 1.3 features (dynamic rendering, timeline semaphores, `VK_KHR_synchronization2`, descriptor indexing, buffer device address).
- Compute shaders, async compute queue, hardware tessellation.
- Bindless descriptors via `VK_EXT_descriptor_indexing`.
- VMA-managed 8 GB VRAM budget.
- 1080p @ 60 FPS target for the full simulation.

**Tier G — hardware-gated, post-v1:**
- Ray tracing (`VK_KHR_ray_tracing_pipeline`, `VK_KHR_acceleration_structure`, `VK_KHR_ray_query`).
- Mesh shaders (`VK_EXT_mesh_shader`).
- GPU work graphs (Vulkan 1.4).
- AV1 hardware decode.

Advanced features are gated by `gw::render::RHICapabilities`. The baseline renderer path never depends on Tier G features.

### §2.2 Language and Toolchain

| Domain | Language | Standard | Compiler |
|--------|----------|----------|----------|
| Engine core, editor, sandbox, runtime, gameplay, tools, tests | C++23 | ISO/IEC 14882:2024 | Clang / LLVM 18+ (`clang-cl` on Windows) |
| BLD AI copilot | Rust 2024 edition | stable 1.80+ | rustc via Cargo + Corrosion |
| Shaders | HLSL (primary), GLSL (ports) | — | DXC → SPIR-V |

- **No STL in engine hot paths** — use `gw::core::Array`, `gw::core::HashMap`, `gw::core::String`. STL allowed in editor, tools, tests.
- **Rust only for BLD.** Nowhere else in the codebase.
- **MSVC and GCC are not supported.** This is not negotiable.
- **CMake 3.28+** with Presets + Ninja. Corrosion bridges Cargo into CMake.
- **Vulkan 1.2 baseline** with 1.3 features opportunistic. WebGPU backend planned post-v1. DirectX 12 and Metal: post-scope.
- **Windows (x64) + Linux (x64)** from day one. macOS, consoles, mobile: post-scope.

### §2.3 UI Stack — Locked

| Context | Library |
|---------|---------|
| Editor UI | Dear ImGui + docking + viewports + ImNodes + ImGuizmo |
| In-game UI (HUD, menus) | RmlUi + HarfBuzz + FreeType |

**No custom UI framework.** This is a permanent commitment. Do not propose one.

### §2.4 ECS & Core

- **In-house archetype ECS** in `engine/ecs/`. No EnTT, no flecs.
- **Custom fiber-based work-stealing scheduler** in `engine/jobs/`. No `std::async`, no raw `std::thread`.
- **No global `new`/`delete`** in engine code. Allocators passed explicitly.
- **OS headers** only inside `engine/platform/`. Enforced by custom `clang-tidy` check.

### §2.5 BLD, Scripting, MCP

- **BLD** lives outside `greywater_core` as a Rust crate. The engine core cannot depend on Rust code.
- **MCP (Model Context Protocol) spec `2025-11-25`** via `rmcp`. BLD is the MCP server.
- **Visual scripting:** typed text IR is ground-truth. Node graph is a reversible projection with a sidecar layout file.
- **Hot-reload:** only the gameplay module (C++ dylib). Engine core and BLD do not hot-reload.

### §2.6 *Sacrilege* / Engine-World Mandates

- **Procedural generation is deterministic.** Same seed + same coordinates = identical output, every run, every machine. No wall-clock dependence, no unseeded RNG.
- **"If it is not near the player, it does not exist."** On-demand, chunk-based streaming; nothing simulated beyond its relevance window.
- **World-space math uses `float64`** where scale demands it. Jitter from precision is a bug.
- **Floating origin** is mandatory whenever the camera traverses large extents.
- **Flagship art direction:** *Sacrilege* — authoritative in `07_SACRILEGE.md` (GW-SAC-001).

---

## §3. Code Generation Style Guide

### C++

```
Type names:       PascalCase          VulkanDevice, World, EntityHandle
Functions:        snake_case          create_swapchain, register_component
Member vars:      trailing underscore device_, frame_index_
Namespaces:       snake_case          gw::render, gw::ecs, gw::jobs
Macros:           GW_UPPER_SNAKE      GW_ASSERT, GW_LOG, GW_UNREACHABLE
Enums:            enum class always   BlasphemyType::Stigmata
Files:            .hpp / .cpp         #pragma once in all headers
```

- No exceptions in engine code. Use `Result<T, E>` or `std::expected`.
- No RTTI in hot paths.
- No raw `new` / `delete`. Use `std::unique_ptr`, `gw::Ref<T>`, or stack scope.
- No two-phase `init()` / `shutdown()` on public APIs.
- No C-style casts.
- `std::string_view` never passed to a C API.
- Lambda captures always explicit — no `[=]`, no `[&]`.
- All world-space positions `float64`. Single-precision `float` only for local-to-chunk math.

### Rust (BLD only)

- Edition: 2024. Style: `rustfmt` defaults; `clippy -D warnings`.
- `snake_case` everywhere except types (`PascalCase`) and constants (`SCREAMING_SNAKE`).
- `unsafe` requires a `// SAFETY:` comment. Enforced by custom clippy lint.
- No `unwrap`/`expect` in `bld-ffi` or `bld-governance`.
- Tool definitions: `#[bld_tool]` proc macro always. No hand-written schemas.

### Shaders

- HLSL primary via DXC → SPIR-V. GLSL supported for ports.
- Permutations generated at cook time.
- Flat shading or subtle-gradient shading by default.

---

## §4. Memory and Performance Policy

| Allocator | Use |
|-----------|-----|
| `gw::memory::FrameAllocator` | Transient per-frame work. Reset once per frame. |
| Arena allocators | Scoped to subsystems, levels, or load phases. |
| Pool allocators | Fixed-size hot component types and particle systems. |
| System heap | Last resort. Permitted only at startup or for OS-owned buffers. |

- No STL containers in hot paths.
- Chunk streaming: LRU caching, time-sliced generation (no single-frame stalls).
- VRAM budget: 8 GB total (RX 580). Per-subsystem budget enforced in engine config.
- **Perf regression ≥ 5% on canonical scenes fails CI.**
- Target: 1080p @ 60 FPS on RX 580 for the full simulation.

---

## §5. AI Context Awareness

| Model | Role |
|-------|------|
| Opus 4.7 (1M context) | Deep architectural work, long-context review |
| Sonnet 4.6 | Iterative coding, PR review |
| Haiku 4.5 | Short tasks, doc queries |

- Claude starts every session by reading `CLAUDE.md` at repo root, then `docs/README.md`.
- Claude never commits without running the full test suite on the affected module.
- Claude writes commit messages in Conventional Commits format.
- Claude writes ADRs before committing architectural decisions, not after.

---

## §6. Repository Layout

```
greywater/
├── engine/
│   ├── core/            Logger, assertions, Result<T,E>, containers, strings, events, config
│   ├── math/            Vec/Mat/Quat/Transform + SIMD
│   ├── memory/          Frame, arena, pool allocators
│   ├── platform/        Window, input, timing, dynamic-lib — OS headers live ONLY here
│   ├── jobs/            Fiber-based work-stealing scheduler
│   ├── ecs/             Archetype ECS — World, queries, systems
│   ├── render/          HAL, Vulkan backend, frame graph, Forward+, materials, VFX
│   ├── audio/           miniaudio + Steam Audio
│   ├── input/           SDL3 + action maps
│   ├── ui/              RmlUi + HarfBuzz + FreeType (in-game only)
│   ├── scripting/       Hot-reload + vscript IR / interpreter
│   ├── net/             GameNetworkingSockets + ECS replication
│   ├── physics/         Jolt facade + ECS bridge
│   ├── anim/            Ozz + ACL + blend trees + IK
│   ├── ai/              BehaviorTree + Navmesh (Recast/Detour)
│   ├── persistence/     Save/load, migration, telemetry facades
│   ├── i18n/            ICU + HarfBuzz locale runtime
│   ├── a11y/            WCAG 2.2 AA hooks
│   ├── assets/          Asset database, cook pipeline
│   ├── scene/           .gwscene codec + live scene view
│   └── world/           Blacklake + Sacrilege world stack (Phases 19–23)
├── bld/                 Rust Cargo workspace — BLD AI copilot
│   ├── bld-ffi/         C-ABI boundary (cbindgen, Miri-validated)
│   ├── bld-bridge/      C++ side RAII wrappers
│   ├── bld-mcp/         MCP wire protocol (rmcp, spec 2025-11-25)
│   ├── bld-tools/       #[bld_tool] proc macro + tool registry
│   ├── bld-provider/    ILLMProvider — Anthropic cloud + local backends
│   ├── bld-rag/         RAG pipeline (tree-sitter, sqlite-vec, petgraph)
│   ├── bld-agent/       Main agent loop
│   └── bld-governance/  HITL approval, audit log
├── editor/              Dear ImGui editor (Phase 7)
├── sandbox/             Engine feature test-bed (Phase 1)
├── runtime/             Shipping executable (Phase 11)
├── gameplay/            Hot-reloadable C++ dylib
├── tools/cook/          gw_cook content cooker (Phase 6)
├── tests/               Integration, perf, fuzz, BLD evals
├── content/             Source assets (gitignored)
├── assets/              Cooked deterministic runtime assets
├── third_party/         CPM-managed, SHA-pinned
├── cmake/               dependencies.cmake, toolchains/, packaging/
├── docs/                This directory — eleven-file suite
├── CMakeLists.txt
├── CMakePresets.json
├── CLAUDE.md
└── rust-toolchain.toml
```

---

## §7. Engine / Game Code Boundary

This boundary is **load-bearing and enforced by the build system:**

- `greywater_core` has **no dependency on any editor or game code**. It must build and pass its unit tests in isolation.
- `gameplay_module` is the **only** dynamic library in the system.
- **Engine code never `#include`s gameplay headers.** CMake target visibility enforces this. Violations are CI failures.
- Game behavior lives in `gameplay/` or `engine/world/` (for engine capabilities *Sacrilege* consumes via public API). The distinction: `engine/world/` implements capabilities; `gameplay/` implements *Sacrilege*-specific policy.

---

## §8. Phase Build Status (as of 2026-04-21)

| Phase | Name | Status | Tests |
|-------|------|--------|-------|
| 1–4 | Scaffolding, Platform, Math/ECS/Jobs, Vulkan HAL | ✅ Complete | — |
| 5 | Frame Graph & Reference Renderer | ✅ Complete | — |
| 6 | Asset Pipeline & Content Cook | ✅ Complete | — |
| 7 | Editor Foundation | ✅ Complete (demo pending) | — |
| 8 | Scene Serialization & Visual Scripting | ✅ Complete | — |
| 9 | BLD (Brewed Logic Directive, Rust) | ✅ Complete | — |
| 10 | Runtime I — Audio & Input | ✅ Complete | — |
| 11 | Runtime II — UI, Events, Config, Console | ✅ Complete | 271/271 |
| 12 | Physics | ✅ Complete | — |
| 13 | Animation & Game AI Framework | ✅ Complete | 347/347 |
| 14 | Networking & Multiplayer | ✅ Complete | 394/394 |
| 15 | Persistence & Telemetry | ✅ Complete | 473/473 |
| 16 | Platform Services, i18n & a11y | ✅ Complete | 586/586 |
| 17 | Shader Pipeline, Materials & VFX | ✅ Complete | 711/711 |
| 18 | Cinematics & Mods | 🔜 Planned | — |
| 19–23 | Blacklake + *Sacrilege* world stack | 🔜 Planned | — |
| 24 | Hardening & Release | 🔜 Planned | — |
| 25 | LTS Sustenance | 🔜 Ongoing | — |

---

## §9. Session Handoff Protocol

Every coding session begins with this checklist. No exceptions.

### Cold-Start (10 minutes)

1. **Read today's daily log** — what yesterday finished, what was blocked, what Tomorrow's Intention said.
2. **`git log --oneline -10` + `git status`** — confirm working tree state.
3. **Re-read `CLAUDE.md`** at project root and skim `docs/README.md`.
4. **Check Kanban** in `02_ROADMAP_AND_OPERATIONS.md` — WIP limit is 2. Know your card.
5. **Read the relevant doc** for the current phase (see §9.1 below).
6. **Copy the day's card** into the daily log under *🎯 Today's Focus*. Commit.

### Doc by Phase

| Phase | Read first |
|-------|-----------|
| Foundation (1–3) | `01_CONSTITUTION_AND_PROGRAM.md` |
| Rendering (4–5, 17) | `05_RESEARCH_BUILD_AND_STANDARDS.md` — Vulkan guide |
| C++ / Rust patterns | `05_RESEARCH_BUILD_AND_STANDARDS.md` — language guide |
| Systems questions | `04_SYSTEMS_INVENTORY.md` |
| Architectural ambiguity | `06_ARCHITECTURE.md` |
| BLD questions | `06_ARCHITECTURE.md §3.7` + `08_BLACKLAKE_AND_RIPPLE.md` |
| *Sacrilege* / Blacklake | `07_SACRILEGE.md` + `08_BLACKLAKE_AND_RIPPLE.md` |

### CMake Presets

```bash
# Configure + build + test — Linux
cmake --preset dev-linux
cmake --build --preset dev-linux
ctest --preset dev-linux

# Windows
cmake --preset dev-win
cmake --build --preset dev-win
ctest --preset dev-win
```

| Preset | Config | OS | Purpose |
|--------|--------|----|---------|
| `dev-linux` | Debug | Linux | Default developer workflow |
| `dev-win` | Debug | Windows | Default developer workflow |
| `release-linux` | Release | Linux | Release build |
| `release-win` | Release | Windows | Release build |
| `linux-asan` | Debug + ASan | Linux | AddressSanitizer (nightly) |
| `linux-ubsan` | Debug + UBSan | Linux | UndefinedBehaviorSanitizer (nightly) |
| `linux-tsan` | Debug + TSan | Linux | ThreadSanitizer (nightly) |
| `studio-win/linux` | Phase 17 full stack | both | Shader/post/VFX perf gates |

### End-of-Session (5 minutes)

1. Update daily log: completed tasks, blockers, Tomorrow's Intention.
2. Move Kanban card: Done → Review, still going → In Progress (≤ 2!), blocked → Blocked with reason.
3. Commit: small commits, one logical change, Conventional Commits format.
4. If Sunday: 30-minute retro — no coding. Read 7 daily logs. Assess real vs. planned pace.

### Hot Commands

```bash
# Full build
cmake --preset dev-linux && cmake --build --preset dev-linux

# BLD iteration only (Rust-only changes)
cd bld && cargo build --release

# Executables
./build/dev-linux/sandbox/gw_sandbox
./build/dev-linux/editor/gw_editor

# Regenerate MCP tool schemas after #[bld_tool] additions
cd bld && cargo build -p bld-tools

# Regenerate C header after bld-ffi changes
cd bld && cargo build -p bld-bridge

# BLD lint + UB check
cargo clippy --workspace -- -D warnings
cargo +nightly miri test -p bld-ffi
```

---

## §10. If You Get Stuck

1. Re-read `03_PHILOSOPHY_AND_ENGINEERING.md`. Half of all "what do I do" moments dissolve once you remember the Brew Doctrine.
2. Re-read the Engineering Principles in `03_PHILOSOPHY_AND_ENGINEERING.md §B`. Your blocker is probably listed.
3. Write an ADR. If the problem is architectural ambiguity, draft `10_APPENDIX_ADRS_AND_REFERENCES.md` (ADR section) and make the trade-off explicit. Commit the ADR before committing code.
4. Open BLD. Ask the agent for context. `docs.search_engine_docs` is built for this.
5. If it is Friday evening — stop. Tomorrow is a rest day for a reason.

---

*The constitution is the engine's immune system. Every violation is an infection. Honor it.*
