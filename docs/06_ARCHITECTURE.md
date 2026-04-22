# 06 — Architecture

**Status:** Draft — pending stakeholder sign-off  
**Version:** 0.3 · 2026-04-21  
**Precedence:** L3 — platform shape and module topology; narrowed by ADRs in `10_APPENDIX_ADRS_AND_REFERENCES.md`

> ADRs record decisions that narrow or supersede narrative architecture where they conflict. Trust the ADR over older prose.

---

---

# `docs/06_ARCHITECTURE.md` — Greywater Engine architecture

**Layer:** L3 in `docs/README.md`.

## Files

| Document | Role |
| -------- | ---- |
| `grey-water-engine-blueprint.md` | Formal, board-ready blueprint — subsystems, risks, stakeholder view |
| `greywater-engine-core-architecture.md` | Narrative walkthrough of how the engine hangs together |

## Unified program context

**Cold Coffee Labs → Greywater Engine → *Sacrilege*.** These architecture docs describe the **platform**. Product-facing mandates for the debut title live in `docs/07_SACRILEGE.md`. Mathematical / procedural depth for **Blacklake:** `docs/08_BLACKLAKE_AND_RIPPLE.md`. GWE × Ripple / RSL: `docs/08_BLACKLAKE_AND_RIPPLE.md` (reconcile with shipped editor + `vscript` via ADRs).

## Engineering status

Implementation progress and phase gates are **operational truth** in `docs/02_ROADMAP_AND_OPERATIONS.md` and `docs/01_CONSTITUTION_AND_PROGRAM.md`. ADRs under `docs/10_APPENDIX_ADRS_AND_REFERENCES.md` record decisions that may narrow or supersede narrative architecture where they conflict.

---

*Architecture explains the machine. The map of all docs is `docs/README.md`.*


# Greywater — Architectural Blueprint & Phased Execution Plan

| Field              | Value                                                   |
| ------------------ | ------------------------------------------------------- |
| **Project**        | Greywater Engine + *Sacrilege* (unified program)          |
| **Engine**         | Greywater_Engine                                        |
| **Game**           | *Sacrilege* (debut title)                               |
| **Hardware baseline** | AMD Radeon RX 580 8GB @ 1080p / 60 FPS              |
| **Art direction**  | *Sacrilege* — crunchy low-poly horror (GW-SAC-001)      |
| **Document Owner** | Principal Engine Architect, Cold Coffee Labs            |
| **Audience**       | Executive Stakeholders, Engineering Leads, Technical Ops |
| **Status**         | **DRAFT — Pending Stakeholder Sign-Off**                |
| **Version**        | 0.3                                                     |
| **Date**           | 2026-04-21                                              |
| **Supersedes**     | All prior informal engine proposals                     |

---

## 1. Executive Summary

Cold Coffee Labs is chartering **Greywater Engine** — the proprietary C++23/Vulkan platform — and its debut flagship title, ***Sacrilege*** (**supra-AAA** bar: `docs/01_CONSTITUTION_AND_PROGRAM.md` §0.1). Procedural depth is delivered through the **Blacklake** framework (`docs/08_BLACKLAKE_AND_RIPPLE.md`). The engine is **purpose-built** to ship *Sacrilege* first, targets the **AMD Radeon RX 580 8GB at 1080p / 60 FPS** as its baseline hardware, and maintains a clean engine/game code separation so future Cold Coffee Labs titles can ship on the same stack.

Greywater is not an attempt to compete feature-for-feature with the large commercial engines. It is a focused, **data-oriented, tool-agnostic, hardware-accessible** technology stack designed to give Cold Coffee Labs the following durable advantages:

1. **Engineering sovereignty.** Every byte of the runtime is ours. No per-seat licenses, no royalty ceilings, no vendor-imposed roadmaps.
2. **Deterministic, reproducible builds** on every developer machine and CI runner — identical toolchain on Windows and Linux, enforced by policy.
3. **Performance headroom** through Data-Oriented Design (DOD) and a native Entity Component System (ECS) at the core. Our cache-line layout is a deliberate decision, not an accident of object graphs.
4. **A single-language, single-process editor experience.** No managed-runtime bridges, no inter-language marshalling, no "editor team" and "engine team" drifting apart.
5. **Hardware-accessible GPU strategy** — a thin Hardware Abstraction Layer (HAL) over **Vulkan 1.2 baseline with 1.3 features opportunistic**, targeting the RX 580 Polaris architecture at 1080p / 60 FPS. Ray tracing, mesh shaders, and GPU work graphs are **hardware-gated** (Tier G, post-v1). A WebGPU backend slots in for web and sandboxed targets, post-v1.
6. **Brewed Logic Directive (BLD)** — a first-class, native AI copilot agent baked into the engine as a **Rust crate**, linked into the editor via a narrow C-ABI boundary. Chat-driven, MCP-protocol, with authoritative tool-use over the engine's public surface. BLD is **not an editor plugin** — it is an engine subsystem. Rust is chosen **only for BLD** (not the engine core) because the Rust ecosystem decisively wins on three things that matter to the agent: proc-macro-generated tool surfaces (`#[bld_tool]`) beat libclang codegen outright, `rmcp` (the official Rust MCP SDK) is ahead of any C++ MCP SDK, and the Rust ML ecosystem (`candle`, `llama-cpp-rs`, `async-openai`) leads on embeddings and inference. **This is our primary competitive differentiator.** No other engine in 2026 treats an agentic AI copilot as a core subsystem with undo-stack integration and compile-time-generated tool surfaces.

This document is the blueprint for Phase 1 through Phase 24 of Greywater Engine construction (plus Phase 25 LTS). Phases 1–18 build general-purpose engine subsystems. Phases 19–23 build **Blacklake + *Sacrilege***-facing subsystems (see `docs/02_ROADMAP_AND_OPERATIONS.md`). Phase 24 hardens and ships. Approval authorizes implementation against the milestones in §5.

---

## 2. Architectural Posture — Choices Made and Choices Rejected

This section states the architectural choices that shape every other section of this document. Each choice is a commitment — approval of this blueprint is approval of these choices.

### 2.1 Commitments we make

| Commitment                                                                          | Rationale                                                                                                               |
| ----------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------- |
| Engine ships as a **static library**; editor, sandbox, and runtime are thin executables that link it. | Clean binary boundary. The editor cannot accidentally grow dependencies the runtime can't ship.               |
| Editor is **strictly decoupled** from the engine at the public-API level.            | Each can evolve independently. Enforced by CMake target visibility and a curated public header set.                     |
| Unified build output — all artifacts land in `build/<config>/bin`.                   | Editor and runtime share binaries side-by-side; no per-target output drift.                                              |
| **Clang / LLVM only** for C++. `clang-cl` on Windows; clang on Linux.                | Native parity across OSes. Identical frontend. One toolchain to debug.                                                   |
| **Windows + Linux from day one.** Neither is a "later."                              | Cross-platform retrofits always fail. CI blocks any PR that breaks either platform.                                      |
| **Thin platform abstraction layer** — OS headers only in `engine/platform/`.         | OS concerns are contained. The rest of the engine is portable by construction.                                           |
| Dedicated **`sandbox` executable** for engine feature validation, separate from the game. | Engine changes don't wait on gameplay to surface regressions; sandbox exercises subsystems directly.                 |
| **Hot-reloadable gameplay module** as a dynamic library. The engine itself is static. | Fast gameplay iteration; the engine core stays stable and doesn't pay the dylib tax.                                    |
| Editor is a **separate executable** that is **not shipped inside the runtime.**      | Editor code and editor-only dependencies never reach players.                                                            |

### 2.2 Options we explicitly reject

| Rejected option                                                           | Reason                                                                                                                                               |
| ------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------- |
| A managed-runtime editor (e.g., C#-based) with an interop layer to the C++ engine. | Introduces a second language, a marshalling cost, a debugger seam, and a Windows-coupled UI stack. An ImGui-based native editor eliminates the bridge entirely. |
| Manual IDE project files as the build source of truth.                    | IDE-coupled, non-reproducible, hostile to CI and to developers on other platforms. Build is infrastructure and must live in code — CMake.           |
| **Pure C** as the implementation language.                                  | C++23 gives us `constexpr`, `consteval`, concepts, modules, and reflection-adjacent metaprogramming without runtime cost. Rejecting those gains is a self-inflicted handicap. |
| Hand-rolled batch / bash scripts as the primary build system.             | Fine for a single maintainer; unacceptable for a studio. Does not scale to multiple configurations, sanitizer builds, CI matrices, or onboarding.   |
| Ad-hoc, un-phased development.                                            | Replaced by a phased, milestone-gated plan (§5).                                                                                                     |
| Targeting top-of-line GPUs only.                                          | Shuts out the larger player base. The RX 580 is our floor; every Tier A feature must run at 1080p/60 on it.                                         |
| Ray tracing, mesh shaders, and GPU work graphs as baseline features.      | Not supported on our baseline hardware. Tier G — capability-gated and post-v1 at the earliest.                                                       |

---

## 3. Architectural Blueprint

### 3.1 Module Topology

Grey Water is composed of four linkable artifacts. The boundary between them is load-bearing and enforced by the build system.

```
                +--------------------------+        +-----------------+
                |   greywater_core (.a)    |        |  bld (Rust .a)  |
                |   C++23                  |        |   MCP, tools,   |
                |   ECS, HAL, platform,    |        |   RAG, provider,|
                |   memory, math, assets   |        |   agent, gov.   |
                +-----------+--------------+        +--------+--------+
                            |                                |
                            |       C-ABI (cbindgen)         |
                            +--------------+-----------------+
                                           |
                                   (linked into editor)
                            |
             +--------------+---------------+
             |              |               |
   +---------v------+  +----v-----+  +------v-------+
   | greywater_     |  | sandbox  |  | game_runtime |
   | editor (.exe)  |  | (.exe)   |  | (.exe)       |
   |                |  |          |  |              |
   | ImGui docking  |  | engine   |  | ships to     |
   | + viewports    |  | feature  |  | players      |
   | + serialization|  | tests    |  |              |
   +----------------+  +----------+  +------+-------+
                                            |
                                  +---------v---------+
                                  | gameplay_module   |
                                  | (.dll / .so)      |
                                  | hot-reloadable    |
                                  +-------------------+
```

**Rules enforced at the CMake target level:**

- `greywater_core` has **no dependency on any editor or game code**. It must build and pass its unit tests in isolation.
- `greywater_editor`, `sandbox`, and `game_runtime` **statically link** `greywater_core`. There is no engine DLL.
- `gameplay_module` is the **only** dynamic library in the system. It links against `greywater_core` as an `INTERFACE` target (headers only — symbols resolve at load time).
- Only `platform/` may `#include <windows.h>`, `<unistd.h>`, or any other OS header. Violations are a CI failure via clang-tidy check.
- **BLD is a separate Rust crate** built by Cargo and integrated into the CMake build via **Corrosion**. It is linked as a static library into the editor (and into any other executable that wants an embedded agent — sandbox, runtime debug builds, CI tools). It is not in `greywater_core` because the C++ core cannot depend on Rust code.
- **The BLD/editor boundary is a single, narrow C-ABI** generated by `cbindgen` (Rust → C header) and consumed via `cxx` on the C++ side. Only POD messages, opaque handles, and function pointers cross. No Rust types, no C++ templates, no exceptions, no Rust generics. The boundary is intentionally small to bound the refactoring surface.
- **Editor UI is locked to Dear ImGui + docking + viewports + ImNodes + ImGuizmo.** No custom UI framework. This is a permanent commitment — do not propose a custom UI in any future phase.

### 3.2 Engine Core (Static Library)

The core is organized as a set of sibling modules, each with a clean public header surface:

| Module        | Responsibility                                                                                 |
| ------------- | ---------------------------------------------------------------------------------------------- |
| `platform/`   | OS abstraction: windowing, input, timing, filesystem, dynamic library loading, threading.      |
| `memory/`     | Arena, frame, pool, and free-list allocators. All engine allocations route through these.      |
| `core/`       | Logging, assertions, result/error types, strings, containers (cache-friendly, not STL wrappers). |
| `math/`       | Vec/mat/quat, SIMD where it earns its keep. Header-only, `constexpr` where possible.            |
| `ecs/`        | Archetype-based ECS. Entities are opaque handles; components are plain data; systems are functions over component spans. |
| `render/`     | Hardware Abstraction Layer + Vulkan backend. Frame graph, resource lifetimes, shader pipeline. |
| `assets/`     | Import, cook, pack. Binary asset format with stable on-disk layout. No runtime JSON parsing.    |
| `scene/`      | Scene graph is a thin view over the ECS. Serialization to the binary asset format.             |
| `scripting/`  | Dynamic-library loader + ABI contract for `gameplay_module`. Watches the DLL for hot reload.    |
| `vscript/`    | Visual-scripting IR runtime. Loads typed text-IR, executes in-editor and in-runtime. **The IR is the ground-truth serialized form**; the node graph is a projection (see §3.8). |

> **BLD is not in `greywater_core`.** It is a separate Rust crate (see §3.7) built by Cargo via Corrosion and linked into the editor target. The editor contains a thin `bld_bridge/` module (cxx-generated bindings + command-bus adapter) that is the only C++ code that talks to the Rust crate.

### 3.3 Editor (Executable)

The editor is a **pure C++ executable** that statically links `greywater_core`. Its UI is built on **Dear ImGui with docking and multi-viewport branches**, rendered through the same HAL the game uses.

Consequences of this choice:
- **Zero language boundary.** A gameplay programmer can step into editor code with the same debugger, same toolchain, same build.
- **The editor is the best stress test of the engine.** Anything flaky in the runtime surfaces in the editor first.
- **No "editor-only" renderer.** The editor shares render passes with the sandbox; debug visualizers are engine features, not editor features.
- **Serialization is a first-class engine concern**, not an editor-side export. The editor writes the same binary format the runtime reads.

### 3.4 Memory Management

DOD begins with allocation discipline. The default heap is treated as a resource of last resort.

- **Frame allocator** — linear bump allocator reset once per frame. Default for transient per-frame work.
- **Arena allocators** — scoped to subsystems, levels, or load phases. Freed in O(1) at scope exit.
- **Pool allocators** — fixed-size blocks for hot component types and particle systems.
- **System heap** — permitted only for long-lived allocations at startup or for OS-owned buffers.

All allocators implement a common `IAllocator` vtable-free interface (CRTP or concept-based) and are passed explicitly to APIs that allocate. There is no global `new`/`delete` in engine code.

### 3.5 Rendering HAL

The HAL exposes the vocabulary of modern GPUs — devices, queues, command buffers, pipelines, descriptors, synchronization primitives — without committing to a single API.

- **Primary backend (Phase 3):** Vulkan 1.3 with dynamic rendering, timeline semaphores, and `VK_KHR_synchronization2`. No render passes; no subpasses.
- **Planned secondary backend:** WebGPU via Dawn, for browser and sandboxed targets. Same HAL contract.
- **Not in scope:** DirectX 12, Metal. Reconsidered after Phase 4 based on platform commitments.

The HAL itself is ~30 types. Everything above it — frame graph, material system, lighting — is written against the HAL and is backend-agnostic.

### 3.6 Entity Component System

- **Archetype-based layout.** Entities with identical component sets live in contiguous, SoA-arranged chunks.
- **Systems are functions**, registered with explicit read/write declarations over component types. This enables the scheduler to run non-conflicting systems in parallel without locks.
- **Components are plain data.** No virtual functions, no constructors that allocate, no hidden lifetime.
- **Entities are generational handles** (32-bit index + 32-bit generation) — safe to store across frames, cheap to compare.

EnTT is a reference point, not a dependency. We implement the ECS in-house because it is the single most load-bearing subsystem in a DOD engine, and owning it end-to-end is strategically correct.

### 3.7 Brewed Logic Directive (BLD) — Native AI Copilot Subsystem (Rust)

BLD is the engine's native, agentic AI copilot and the primary differentiator of Greywater_Engine. It is implemented as a **standalone Rust crate** (`bld/`) built by Cargo via Corrosion, and linked as a static library into the editor. Rust is used **only for BLD** — the engine core, editor, sandbox, runtime, and gameplay module remain C++23.

**Why Rust for this subsystem specifically:**

1. **Proc-macro tool surface (`#[bld_tool]`) beats libclang codegen outright.** A tagged Rust function automatically generates its MCP schema, JSON argument decoder, dispatch handler, and registry entry at compile time. No external codegen tool, no CMake integration, no header-parsing edge cases.
2. **`rmcp` (official Rust MCP SDK) is ahead of any C++ MCP SDK** (`cpp-mcp`, Gopher-MCP). If BLD is the differentiator, we want the better SDK.
3. **The Rust ML ecosystem leads for our use case:** `candle` (pure-Rust ML runtime), `llama-cpp-rs`, and the Rust HTTP/SSE clients that let us talk to cloud LLM APIs. All more mature than C++ equivalents.
4. **`tree-sitter` has parity** across both languages — no loss there.

Rust is **not** used for the engine core because: the Rust renderer story is `ash` (same as hand-rolled C++ Vulkan) or `wgpu` (wrong fit for Vulkan-1.3-targeted AAA aspirations); no AAA-shipping precedent; loss of C++ template metaprogramming across a Rust/C++ editor boundary; weaker hot-reload story for the Rust core itself. The FFI tax is bounded to BLD's low-frequency, serialized call path — not taxed on every per-frame editor↔core interaction.

**Rust crate internal structure** (`bld/`, a Cargo workspace):

```
bld/
├── Cargo.toml             (workspace)
├── bld-mcp/               rmcp-based MCP server (JSON-RPC 2.0 + stdio + Streamable HTTP)
├── bld-tools/             proc-macro #[bld_tool], registry, dispatcher, tier/permission
├── bld-provider/          ILLMProvider trait + Claude cloud + alternate-cloud + candle/llama-cpp-rs backends
├── bld-rag/               tree-sitter chunking, embeddings (candle/Nomic), sqlite-vss index, symbol graph + PageRank
├── bld-agent/             plan/act/verify loop, sessions, memory, audit log, replay
├── bld-governance/        tier policy, HITL elicitation handlers, secret filter, code-author review
├── bld-bridge/            cbindgen-generated C header + cxx-facing exported API
└── bld-ffi/               C-ABI: opaque handles, POD message types, callback function pointers
```

**The C-ABI boundary** (`bld-ffi`) is the single narrow contract between Rust and C++. Only:
- Opaque handles (`BldSession*`, `BldToolHandle*`) — never dereferenced by C++.
- POD message structs (fixed layout, `#[repr(C)]`).
- Function pointers for editor-side callbacks (command submission, UI updates, approval prompts).
- C strings with explicit ownership annotations.

No Rust generics cross, no trait objects, no `Result`/`Option`, no C++ templates, no C++ exceptions. Error handling is stable error codes + out-param messages. This intentional narrowness is what makes the FFI tax bounded.

**The seven structural commitments — revised for Rust implementation:**

1. **The engine's public surface is the agent's tool surface.** Tool schemas are generated at compile time by `#[bld_tool]` proc macros in the `bld-tools` crate. Each tool either implements its logic entirely in Rust, or calls back into the C++ engine via a stable C-ABI callback exposed by the editor.
2. **The undo stack is the agent's commit log.** The C++ command bus is the source of truth. BLD's mutating tools construct opcode + payload messages and call back into the editor via the C-ABI to enqueue them as `ICommand` instances. Ctrl+Z behavior is identical whether the command came from a user click or from an agent tool call. Transaction brackets collapse multi-step agent actions into a single undo.
3. **MCP is the wire from day one.** `rmcp` hosts the server (spec 2025-11-25). Editor chat panel is one client; Claude Code, Cursor, Zed, and CI automation connect over the same protocol.
4. **BLD can be embedded by any executable** that accepts the Rust static library — editor, sandbox, runtime debug builds, CI tools. It is not editor-only.
5. **HITL approval is the protocol's `elicitation` primitive**, handled by `bld-governance`.
6. **Tool surface auto-synchronizes with the engine.** Adding or editing a `#[bld_tool]` function rebuilds the tool registry and schema on the next `cargo build`. No hand-written JSON.
7. **The agent stress-tests the public engine API every day** via the C-ABI callbacks. If BLD cannot accomplish a task, the C-ABI is missing an entry and the engine's exposed surface is incomplete — a deliberate forcing function on API design.

**Tool taxonomy** (~40–80 public tools at GA, composed server-side into finer operations):

```
scene.*       create_entity, destroy_entity, reparent, query
component.*   add, remove, get, set (auto-generated per component type via macro)
asset.*       import, reimport, find, create_material, edit_shader
build.*       configure_cmake, build_target, run_tests, get_diagnostics
runtime.*     play, pause, step_frame, attach_profiler, sample_frame
code.*        read_file, write_file, apply_patch, find_symbol, refs
vscript.*     read_ir, write_ir, validate, compile, run
debug.*       breakpoint, eval, stack, last_crash_report
docs.*        search_engine_docs, search_api, get_class_doc
project.*     list_scenes, current_selection, viewport_screenshot
```

**Permission tiers**, enforced by `bld-governance`:
- **Read** — auto-approved (queries, doc lookup, file reads inside roots).
- **Mutate** — approval per session or per action; scene edits, file writes, asset import, IR edits.
- **Execute** — always per-action approval; build/run/test, process spawn, network.

**LLM strategy:**
- **Cloud (default):** Claude Opus 4.6+ (strongest tool-use for our agent workloads). Other cloud providers pluggable via `ILLMProvider`. Rust crates: `reqwest` + `eventsource-stream` for SSE; per-provider client crates pinned to stable versions.
- **Local:** `candle` (pure-Rust, supports Llama/Mistral/Gemma/Qwen with CUDA/Metal/CPU) **or** `llama-cpp-rs` (FFI to llama.cpp). GBNF grammar compiled from tool JSON-schemas so even small local models emit structurally valid tool calls.
- **Offline mode** shows an explicit "reduced capability" banner; never silently degrades.
- API keys live only in the OS keychain (Windows Credential Manager / libsecret) via the `keyring` Rust crate.

**RAG strategy:**
- **tree-sitter-rs** for AST-aware chunking (grammars: C++, GLSL, HLSL, JSON, the vscript IR, `.gwscene`).
- **Symbol graph + PageRank** via `petgraph`.
- **Embeddings:** `nomic-embed-code` model via `candle` by default (offline, no keys). Cloud embedding providers plug in as studio-configurable upgrades.
- **Vector store:** SQLite + sqlite-vss via `rusqlite` + sqlite-vss bindings. No server process.
- **Two indices:** engine docs/source (shipped pre-built) + user project (built incrementally on filesystem-watch events via `notify`).

**Editor UX surface** (C++ side of the boundary, ImGui):
- Docked ImGui chat panel with `imgui_md` markdown rendering and streaming token output driven by callbacks from Rust.
- Slash commands (`/diagnose`, `/optimize`, `/explain`) mapped to MCP `Prompts`.
- Drag-and-drop attachments: viewport screenshot, entity, profiler capture, file → each becomes an MCP `Resource` the Rust side owns.
- Approval UI (elicitation handler): unified diff for code, tree-diff for scenes, thumbnail before/after for assets.
- Cross-panel integration: right-click entity → "Ask BLD"; click compile error → "Fix this"; right-click profiler spike → "Diagnose."
- Multi-session tabs backed by independent `BldSession*` handles.

**Safety & governance:**
- Append-only audit log (`.greywater/agent/audit/*.jsonl`) written by `bld-governance`.
- Session replay (`gw_agent_replay`) re-runs against a clean checkout for QA and incident review.
- Hard secret filter in every file-reading tool: `.env`, `*.key`, keychain entries never reach model context. Filter list is compile-time-constant.
- `agent_ignore` file for project-level exclusions (gitignore semantics).
- Pre-commit hook surfaces agent-authored changes; studios can require human approval on any agent commit.

### 3.8 Visual Scripting — Text-IR Ground-Truth, Graph as Projection

Visual scripting in Greywater_Engine is **text-first, AI-native, and graph-visualized** — not a Blueprint-style node-graph-as-ground-truth system.

**The architectural rule:** a typed text IR is the serialized source of truth. The node graph is a reversible projection with a sidecar layout file. BLD reads and writes the IR directly.

**Why this matters:**
- **BLD can author gameplay logic.** Because the IR is text, the same agent that writes C++ can write visual scripts, and the UI updates as a projection.
- **Diffs are meaningful.** Source control shows IR changes, not binary graph blobs.
- **Merges are resolvable.** Conflict resolution happens at the IR level; graph layout is orthogonal.
- **Undo/redo happens at the IR level**, integrated with the editor command bus.
- **No "nativization" nightmare.** No dual representations of ground-truth. No graph-to-C++ conversion step that would require maintaining two authoring paths.

**IR design** (to be detailed in Phase 4):
- Typed expressions, execution edges, stable-ID references to engine types and component members.
- Serialized as a compact text form (JSON variant or custom DSL — TBD in Phase 4 kickoff).
- Executed by the `vscript/` runtime in both editor (interpreted for fast iteration) and runtime (JIT-compiled or AOT-cooked to bytecode on asset cook).
- Node graph authored with **ImNodes**; layout persisted in a `.layout.json` sidecar. Node positions are never checked into the IR file — they are presentation.

**The AI-native consequence:** BLD's `vscript.*` tools operate on IR text. A designer can author by drag-and-drop, by chat ("make this projectile bounce three times"), or by typing IR directly. All three produce identical artifacts.

### 3.9 Audio Subsystem

- **Device/mixer backend:** **miniaudio** (MIT, single-header C, cross-platform, mature). Chosen for zero-friction Windows/Linux parity and permissive license.
- **Spatial audio:** **Steam Audio SDK** (free, industry-standard HRTF, occlusion, reflection baking). Integrated as an optional module; falls back to panning-only for platforms where Steam Audio is unavailable.
- **Formats:** Ogg Vorbis (music), Opus (voice, streaming), WAV (source SFX). Runtime format is a cooked, compressed, seek-optimized binary produced by the asset cooker.
- **ECS integration:** `AudioSource`, `AudioListener`, `AudioZone` components. Mixer state is a singleton system.
- **Voice chat:** same Opus codec, transport via Networking (§3.10), spatialization via Steam Audio.
- **Module:** `engine/audio/` — mixer, decoder, DSP (reverb/EQ/compression), streaming.

### 3.10 Networking & Multiplayer

- **Transport:** **GameNetworkingSockets** — open-source, battle-tested at large-scale multiplayer. Reliable/unreliable UDP, encryption, NAT punch, congestion control — all in-box.
- **Model:** **authoritative client-server** by default. **Deterministic lockstep** mode available for fighting-game-style titles.
- **Replication:** custom ECS replication layer — delta compression, relevancy/interest management, priority-based bandwidth allocation, component-level reliability tiers.
- **Lag compensation:** server-side rewind-and-replay with a 200ms default window.
- **Matchmaking:** `ISessionProvider` abstract interface; Steam/EOS backends behind it.
- **Voice:** Opus codec, GameNetworkingSockets transport, Steam Audio spatialization.
- **Security:** all traffic encrypted (GNS built-in). Server-authoritative design eliminates most cheat vectors; anti-cheat integration at §3.27.
- **Module:** `engine/net/` — transport, replication, voice, matchmaking adapters.

### 3.11 Physics

- **Engine:** **Jolt Physics** (open-source C++, production-proven). Best-in-class threading, clean API, active maintenance.
- **Features:** rigid body dynamics, character controller, raycasting, constraints, vehicles, soft bodies (Jolt's), triggers.
- **ECS components:** `RigidBody`, `Collider`, `CharacterController`, `Trigger`, `PhysicsConstraint`.
- **Determinism:** deterministic mode for lockstep multiplayer; documented as a per-scene setting.
- **Threading:** Jolt's job system bridged to our §3.18 scheduler.
- **Module:** `engine/physics/`.

### 3.12 Animation

- **Runtime:** **Ozz-animation** (open-source, minimal, widely used in AAA). Excellent compression, data-oriented, no allocations in the hot path.
- **Compression:** **ACL** (Animation Compression Library) for aggressive size reduction on shipped content.
- **Blend trees:** custom, ECS-backed, data-driven.
- **IK:** FABRIK + CCD (Ozz + custom layer).
- **Morph targets:** supported via glTF pipeline.
- **Authoring:** DCC in Blender/Maya, exported as glTF, cooked to Ozz runtime format.
- **Module:** `engine/anim/`.

### 3.13 Input & Action System

- **Device layer:** **SDL3** for gamepad, HID, haptics. Cross-platform, solved problem. Keyboard/mouse via platform layer directly.
- **Action maps:** logical actions (e.g., `"Jump"`, `"Fire"`) bound to physical inputs. Rebindable per user. Chord support.
- **Config:** default bindings in TOML, user overrides persisted via §3.16.
- **Accessibility:** full rebind, hold-to-toggle alternative, single-switch mode, configurable dead zones.
- **ECS:** `PlayerInput` component; actions fire events consumed by gameplay systems.
- **Module:** `engine/input/`.

### 3.14 In-Game UI (HUD, Menus, End-User Screens)

- **Deliberately not ImGui.** ImGui is editor-only. End-user UI needs a separate retained-mode system.
- **Library:** **RmlUi** (MIT, HTML/CSS-inspired, native C++, production-proven in shipping titles).
- **Text shaping:** **HarfBuzz** + **FreeType**; color fonts via COLR table.
- **Vector rendering:** through the Vulkan HAL — RmlUi's backend is rewritten to emit through our render layer.
- **Input:** routed through the §3.13 action system, not raw events.
- **Localization:** RmlUi string hooks call into §3.15.
- **Module:** `engine/ui/`.

### 3.15 Localization & Accessibility

- **String table:** binary key-value, loaded per-locale at runtime. Keys referenced from UI, vscript, subtitles, and gameplay code.
- **Runtime:** **ICU** for pluralization, date/number/currency formatting, collation, and Unicode normalization.
- **Shaping:** HarfBuzz for complex scripts (Arabic, Indic, East Asian), RTL, ligatures.
- **Font stacks:** per-locale fallback chains; missing glyphs resolve through fallback without crash.
- **Workflow:** keys exported to XLIFF → external translation → reimport to binary table.
- **Accessibility (WCAG 2.2 goal):** color-blind modes (lighting-integrated), text scaling, subtitles with speaker attribution, screen-reader hooks via platform APIs, configurable motion reduction, photosensitivity safe mode.
- **Module:** `engine/i18n/` + `engine/a11y/`.

### 3.16 Persistence, Save System & Database

- **Save format:** versioned binary ECS snapshot via the §3.2 assets serialization layer. Each save has a schema version; in-place migration hooks per version bump.
- **Local database:** **SQLite** (via the engine's C++ bindings) for structured data — save metadata, settings, achievements mirror, telemetry queue, mod registry.
- **Cloud save:** `ICloudSave` interface with Steam Cloud, EOS, and S3-compatible backends.
- **Config:** TOML for user-editable, binary for runtime-only state.
- **Module:** `engine/persist/`.

### 3.17 Telemetry, Analytics & Crash Reporting

- **Crash reporting:** **Sentry Native SDK** (C++, minidump upload, symbolication on server).
- **Telemetry:** custom event pipeline, batched, opt-in only, queued locally in SQLite (§3.16), shipped to a studio-operated ingest endpoint.
- **Anonymization:** mandatory — no PII without explicit user consent. Opt-in screen on first launch.
- **Compliance:** GDPR, CCPA, COPPA. Data-collection policy screen in-game menu; age-gated for EU/UK.
- **Certified builds:** telemetry compile-time-disabled by default in release SKUs; enabled per-platform only where platform TRC permits.
- **Module:** `engine/telemetry/`.

### 3.18 Job System & Threading

- **Custom work-stealing scheduler.** One worker thread per logical core minus one (main thread is not a worker).
- **Fiber-based** for long-running tasks. Fibers preserve stack state across yields without thread explosion.
- **Consumers:** ECS scheduler, render frame graph, physics, animation, asset cook, RAG indexing (BLD).
- **Hard rule:** no `std::async`, no raw `std::thread` in engine code. All work goes through the scheduler.
- **Module:** `engine/jobs/`.

### 3.19 Event System

- **Type-safe pub/sub** using ECS events.
- **In-frame events:** zero-allocation, double-buffered, drained at end-of-frame.
- **Cross-subsystem bus:** typed message bus for inter-subsystem communication (gameplay ↔ audio ↔ UI ↔ network).
- **BLD integration:** `event.*` tools let the agent subscribe to and emit events.
- **Module:** `engine/core/events/`.

### 3.20 Config, CVars & In-Game Dev Console

- **CVar registry:** typed variables, runtime-settable, persist-optional, tagged as `dev-only` or `user-facing`.
- **In-game dev console:** overlay panel in debug builds only — cvar get/set, command execution. **Not a replacement for BLD**; the dev console is local, synchronous, power-user-focused.
- **Config storage:** TOML for user-editable, binary for runtime state.
- **Module:** `engine/core/config/` + `engine/console/`.

### 3.21 Shader Pipeline, Materials & Lighting

- **Authoring languages:** HLSL primary (via **DXC** → SPIR-V), GLSL supported for ports.
- **Permutations:** feature-flag matrix generated at cook time; no runtime shader compilation in shipped builds.
- **Hot-reload** in editor via file-watch.
- **Material system:** data-driven material instances over shader templates (v1). Node-graph material editor is a later-phase deliverable (§3.22 authoring tooling).
- **Lighting:** clustered forward+, cascaded shadow maps (directional), atlas-based shadows (spot/point), IBL with split-sum approximation.
- **GI:** placeholder SSGI in v1. ReSTIR or baked-probe GI in a future phase.
- **Module:** `engine/render/shader/` + `engine/render/material/`.

### 3.22 VFX & Particle Systems

- **Particles:** GPU-driven, compute-based simulation. Emitter hierarchy authored as data.
- **Effects:** ribbon trails, decals, screen-space (bloom, depth-of-field, motion blur, chromatic aberration, film grain).
- **Authoring v1:** data-driven particle definitions (TOML/JSON).
- **Node-graph VFX editor:** later phase; shares IR/graph machinery with §3.8 visual scripting.
- **Modules:** `engine/vfx/` (particles, ribbons, decals) plus Phase-17 shader/material/post ownership in `engine/render/shader/`, `engine/render/material/`, and `engine/render/post/` (see `docs/04_SYSTEMS_INVENTORY.md` §2a / Phase 17 rows).

### 3.23 Game AI (distinct from BLD)

- **Behavior Trees:** custom, ECS-backed. No external dep. Editor graph + tooling track with **Phase 13** (*Living Scene* / ADR-0043); BLD/vscript parity is part of the same phase, not Phase 10.
- **Pathfinding:** **Recast/Detour** (industry standard, permissively licensed).
- **Higher-level layers:** Utility AI and GOAP available as optional overlays.
- **Authoring:** BT files authored as vscript IR (§3.8 applies) — so BLD can read and write them via `vscript.*` tools.
- **Module:** `engine/gameai/`.

### 3.24 Cinematics & Sequencer

- **Timeline editor:** docked ImGui panel with tracks (transform, property, event, camera, audio, subtitle).
- **Runtime:** sequencer evaluates tracks each frame, publishes events to the event system.
- **File format:** `.gwseq` — same serialization machinery as scenes.
- **BLD integration:** `sequencer.*` tools so the agent can author cutscenes conversationally.
- **Module:** `engine/sequencer/`.

### 3.25 Mod Support

- **Mod package format:** vscript IR (§3.8) + assets + optional signed native dylib.
- **Sandboxing:** modded vscript runs under a **reduced tool tier** — no filesystem write outside the mod sandbox, no network, no arbitrary shell.
- **Native mods:** gameplay_module-style dylibs; user-opt-in with per-mod consent; disabled in competitive multiplayer.
- **BLD:** has a `mod.*` tool family that operates in a tier-restricted sandbox for safe mod authoring.
- **Workshop integration** via §3.26.
- **Module:** `engine/mod/`.

### 3.26 Platform Services

- **Abstract interface:** `IPlatformServices` — achievements, leaderboards, cloud save, matchmaking, Workshop, DLC, presence, rich text invites.
- **Backends (progressive rollout):** Steam (first), EOS (second), Galaxy/GOG (third); console platforms added when title agreements exist.
- **Dev default:** in-memory backend for offline development.
- **Module:** `engine/platform_services/`.

### 3.27 Anti-Cheat Boundary

- **The engine is anti-cheat-agnostic.** We do not ship an AC solution; we provide integration points.
- **First line of defense:** server-authoritative networking (§3.10).
- **Content integrity:** SHA-256 hashes on every cooked asset; mismatch triggers a soft kick with replication telemetry.
- **AC integration points:** `IAntiCheat` interface — EAC, BattlEye, Denuvo, etc. plug in behind it.
- **Policy:** AC SDKs are **not** checked into the open repo. Studio-internal builds bring their own.

### 3.28 Content Pipeline

- **Source assets** (`/content/`, gitignored): Blender, Substance Painter, Photoshop, audio DAW projects.
- **Cooked assets** (`/assets/`, tracked): deterministic output from the cooker — content-hashed, incremental, parallel.
- **Importers:** glTF 2.0 (meshes/scenes), KTX2 (textures, BC7/ASTC), Ozz (animations), Ogg/Opus/WAV (audio), XLIFF (localization).
- **Cooker CLI:** `gw_cook` — invokable from CLI, editor, CI, and BLD. BLD has `asset.cook` and `asset.cook_diagnostics` tools.
- **Module:** `tools/cook/` (CLI) + `engine/assets/` (runtime loader).

### 3.29 Testing Strategy

- **Unit tests:** doctest (C++), `cargo test` (Rust).
- **Integration tests:** scene-based — a `.gwscene` runs in sandbox with assertion scripts; pass/fail is a CI gate.
- **Performance regression:** Tracy traces on canonical scenes compared to per-PR baselines; CI fails on > 5% frame-time regression.
- **Smoke:** every sample scene runs 60 seconds in CI on every PR.
- **Fuzzing:** libFuzzer on asset parsers, scene deserializer, network packet handling. Continuous (via OSS-Fuzz pattern).
- **Mutation testing (Rust):** `cargo-mutants` on `bld-governance` and `bld-ffi` (security-critical).
- **BLD eval harness:** canned tasks run against BLD in CI, scored on completion rate and tool-call accuracy; regressions block release.
- **Module:** `tests/` at repo root + per-module test dirs.

### 3.30 Packaging & Release

- **Installer generation:** CMake + **CPack**. MSIX on Windows, AppImage + DEB + RPM on Linux.
- **Symbol servers:** PDB (Windows), split DWARF (Linux), uploaded per release for crash symbolication.
- **Release cadence:** LTS branches cut quarterly; hotfix backports for 12 months from LTS branch creation.
- **Versioning:** engine follows SemVer; titles version independently.
- **Build signing:** Authenticode (Windows), GPG-signed DEBs/RPMs (Linux).
- **Changelog:** generated from PR labels via CI; human-edited release notes for LTS cuts.
- **Module:** `cmake/packaging/` + `.github/workflows/release.yml`.

### 3.31 Editor UI Architecture

The editor UI is the face the engineer sees every day. It is engineered with the same rigor as every other subsystem. Full narrative treatment lives in `docs/06_ARCHITECTURE.md` Chapter 13; this section is the formal spec for the build.

**Module:** `editor/ui/`.

**Sub-modules:**

| Sub-module                  | Responsibility                                                                        |
| --------------------------- | ------------------------------------------------------------------------------------- |
| `editor/ui/core/`           | Dockspace manager, ImGui style application, font-atlas loader, theme tokens           |
| `editor/ui/panels/`         | One file per panel; implements `IEditorPanel`; registered in `panels/registry.hpp`    |
| `editor/ui/widgets/`        | Custom widgets (drag-float-matrix, curve editor, histogram, colour picker, vector editor, flame graph, G-buffer viewer, node-graph wrapper, selectable-tree) |
| `editor/ui/command_palette/`| Palette registry, fuzzy search (prefix + substring + subsequence), invocation pipeline |
| `editor/ui/workspaces/`     | Built-in preset layouts, per-user persistence, workspace switching logic              |
| `editor/ui/fonts/`          | Shipped TTF files — one sans, one mono                                                |
| `editor/ui/theme/`          | Palette tokens (Obsidian / Bean / Greywater / Crema / Brew / Signal) + semantic roles |

**Dependencies:** Dear ImGui (docking + viewports branch), ImNodes, ImGuizmo, `imgui_md`. No other UI libraries are permitted.

**Panel registry.** A panel is registered by listing its type in `editor/ui/panels/registry.hpp`. The registry is a `constexpr` array consumed by the dockspace manager at init time. Adding a panel is one file (the panel class) + one line (the registry entry).

**Panel interface:**

```cpp
namespace gw::editor::ui {

enum class PanelCategory : u8 { Core, Diagnostic, Authoring, GreywaterGame };

struct IEditorPanel {
    virtual ~IEditorPanel() = default;

    virtual PanelId id() const noexcept = 0;
    virtual std::string_view title() const noexcept = 0;
    virtual PanelCategory category() const noexcept = 0;

    virtual void on_attach() {}
    virtual void on_detach() {}

    virtual void tick(f32 dt) {}
    virtual void render_ui() = 0;  // ImGui only — no GPU state mutation
};

} // namespace gw::editor::ui
```

**Panels that must ship by the *Editor v0.1* milestone (Phase 7, week 42):** Scene Hierarchy, Inspector, Asset Browser, Viewport, Console, Log, Stats. All others (BLD Chat, Profiler, Render Debug, Material Editor, VScript Node Graph, Timeline, Shader Editor, Texture Inspector, Animation Editor, Config/CVars, ECS Inspector, and all eight title-facing / *Sacrilege* workflow panels) ship in their owning phase; most are integrated by the end of Phase 10 or Phase 18.

**ImGui configuration.** A single function `gw::editor::ui::apply_brew_style(ImGuiStyle&)` writes the full style: colours (all 50+ `ImGuiCol_*` entries mapped from the palette tokens), spacing (`ItemSpacing`, `FramePadding`, `WindowPadding`, `IndentSpacing`), rounding (`FrameRounding = 4`, `WindowRounding = 4`, `GrabRounding = 2`), borders (`WindowBorderSize = 1`, `FrameBorderSize = 0`). Called once at init. Re-applied on theme hot-reload in debug builds.

**Flags:** `ImGuiConfigFlags_DockingEnable`, `ImGuiConfigFlags_ViewportsEnable`, `ImGuiConfigFlags_NavEnableKeyboard`. Gamepad nav disabled for the editor.

**Dockspace layout persistence.** Serialised to `.greywater/editor/layout.ini` per user. Workspace presets shipped in `editor/ui/workspaces/` as `.layout.ini` files, loaded on workspace switch.

**Fonts.** Two TTFs shipped in `editor/ui/fonts/`. Atlas built once at init, cached in `.greywater/editor/cache/`. Hot-reload watches the TTF files in debug builds.

**Command palette.** Every subsystem registers commands via `gw::editor::ui::CommandRegistry::register_command(...)` with: a unique ID, a display name, a tier (`read` / `mutate` / `execute`), optional keyboard shortcut, and a callback. The palette is invoked with Ctrl+K; fuzzy-searches across all registered commands; shows top 20 matches; enter executes. Mutate- and execute-tier commands running through the palette respect the same governance gates BLD uses (command-bus + approval dialog where applicable).

**Performance budget.**

| Metric                                             | Target                   |
| -------------------------------------------------- | ------------------------ |
| UI render per frame @ 1080p on RX 580              | ≤ 2 ms                   |
| Default workspace total frame cost @ 1080p RX 580  | ≤ 16.6 ms (60 FPS)       |
| Command palette first-result latency (500 cmds)    | < 50 ms                  |
| Asset-browser thumbnail first paint                | < 16 ms for cached items |
| Histogram widget rebuild (GPU)                     | ≤ 0.5 ms                 |

A PR that regresses the default-workspace frame time by ≥ 5 % on the reference RX 580 runner **fails CI**.

**Accessibility.** WCAG 2.2 AA. Text scaling 80 – 200 %. Motion reduction respected. Keyboard reaches every action. Screen-reader hooks on supported OSes.

**What this section explicitly forbids.** Custom UI frameworks, per-panel themes, gradients, drop shadows, skeuomorphism, animated chrome, tutorial overlays, mouse-only discovery. Full list in `greywater-engine-core-architecture.md` §13.12.

**Exit criteria (feeds into Phase 7 gate):**
- All required-by-v0.1 panels render without visual glitches.
- All four workspace presets load and switch without layout corruption.
- Command palette covers all currently-registered commands.
- 60 FPS maintained on RX 580 @ 1080p with the default workspace populated.
- All palette tokens meet WCAG 2.2 AA contrast thresholds against their use cases.
- Panel registry test suite passes on Windows and Linux.

---

## 4. Toolchain & Build Path

Build infrastructure is **code**. It is versioned, reviewed, and CI-enforced with the same rigor as runtime source.

### 4.1 Compilers

- **Clang / LLVM 18+** for all C++ code (engine core, editor, sandbox, runtime, gameplay module). On Windows, `clang-cl` targets the MSVC ABI — we consume the Windows SDK but not the MSVC compiler. MSVC and GCC are unsupported for C++.
- **Rust stable** (pinned in `rust-toolchain.toml`) for the BLD crate only. Cargo is the Rust build driver, invoked by CMake via Corrosion. Rust nightly is **not** used unless a specific BLD feature requires it, in which case the nightly pin is reviewed and explicitly recorded.
- C++ standard: **C++23**, with C++26 features adopted as Clang lands them and once they are stable in our CI matrix.
- Rust edition: **2024** (once stable; 2021 otherwise).

### 4.2 Build Generator

- **CMake 3.28+** using **file sets, target-based design, and presets.** CMake is the **single source of truth** for the whole build graph, including the Rust BLD crate.
- **Corrosion** bridges CMake and Cargo. CMake invokes Cargo as a sub-build for the `bld/` workspace, imports the resulting static library as a normal CMake target, and links it into the editor.
- **`cbindgen`** generates the C header for the Rust BLD crate at the end of each Cargo build; the header is consumed by the C++ `bld_bridge` module via `cxx`.
- A root `CMakePresets.json` defines the canonical configurations: `windows-debug`, `windows-release`, `linux-debug`, `linux-release`, `linux-asan`, `linux-tsan`, `linux-ubsan`.
- The Ninja generator is mandatory. No Make, no MSBuild, no Visual Studio solution files checked in.
- IDE support is via CMake's native integration (VS Code CMake Tools, CLion, Rider, Neovim) plus `rust-analyzer` for the BLD crate. **The repo does not ship an IDE-specific project file.**

### 4.3 Dependency Management

**C++ dependencies** via **CPM.cmake**, pinned by commit SHA. **Rust dependencies** via Cargo, pinned via `Cargo.lock` (committed). **No system-installed libraries** in the build graph except the OS SDK (Windows SDK, glibc headers) and the Vulkan SDK.

**C++ dependencies** (CPM):
- `glfw` — windowing and input
- `volk` + `VulkanMemoryAllocator` — Vulkan loader and allocator
- `imgui` (docking branch) — editor UI (**locked choice**, no custom UI framework)
- `imnodes` — node-graph editor widgets for visual scripting projection
- `imguizmo` — viewport manipulation gizmos
- `imgui_md` (MD4C-based) — markdown rendering in BLD chat panel
- `spdlog` — logging (evaluated; may be replaced by in-house logger in Phase 2)
- `doctest` — unit testing
- `glm` — math (candidate; will be retired if in-house math library lands ahead of schedule)

**Rust/C++ bridge tooling** (build-time):
- **Corrosion** — CMake ↔ Cargo integration
- **`cxx`** (dtolnay) — Rust/C++ binding generator, C++ side of the boundary
- **`cbindgen`** — Rust → C header generation

**Rust dependencies** (Cargo, BLD crate only):
- `rmcp` — official Rust Model Context Protocol SDK
- `tree-sitter` + grammars (C++, GLSL, HLSL, JSON, vscript IR, `.gwscene`) — AST-aware chunking for RAG
- `petgraph` — symbol graph + PageRank for RAG ranking
- `rusqlite` + `sqlite-vss` bindings — local vector index (alt: `lancedb` embedded)
- `candle` — pure-Rust ML runtime for local embeddings and inference (alt: `llama-cpp-rs`)
- Per-provider HTTP client crates (pinned; one per alternate cloud LLM we support)
- Claude-API Rust client (per-provider crates pinned; in-house wrapper where needed)
- `reqwest` + `eventsource-stream` — HTTP/SSE transport for cloud LLM providers
- `serde` + `serde_json` — serialization
- `tokio` — async runtime (single, shared across the BLD crate)
- `keyring` — OS keychain access for API keys
- `notify` — filesystem watcher for incremental RAG index updates
- Nomic Embed Code (model weights, fetched at first run) — default code-aware embedding model

### 4.4 Code Quality & Automation

**C++:**
- **`clang-format`** with a committed `.clang-format`. Enforced on every PR.
- **`clang-tidy`** with a curated checkset, including a custom check that forbids OS headers outside `platform/`.
- **Sanitizer matrix**: ASan, UBSan, and TSan builds run nightly on Linux. One failure blocks the release train.

**Rust (BLD crate):**
- **`rustfmt`** with a committed `rustfmt.toml`. Enforced on every PR.
- **`clippy`** with a curated lint set; `-D warnings` in CI. `unsafe` blocks require a `// SAFETY:` comment (custom lint).
- **`cargo-deny`** for license and advisory checks on every Rust dep.
- **`cargo-audit`** nightly for security advisories.
- Miri runs on the `bld-ffi` crate nightly to catch undefined behavior at the FFI boundary.
- Contract tests for every `ILLMProvider` implementation.

**Shared:**
- **`sccache`** on all CI runners and recommended for developer machines (works for both Rust and C++).
- **`cmake-format`** on all `CMakeLists.txt` files — because infrastructure is code.
- **GitHub Actions** matrix: `{windows-latest, ubuntu-latest} × {debug, release}`, plus nightly sanitizer + Miri jobs.

---

## 5. Phased Execution Plan

**Twenty-four build phases plus one ongoing LTS sustenance phase. Total horizon: 162 weeks (~37 months).** Each phase ends at a **gated milestone** or, for phases without a named milestone, at a set of exit criteria signed off by the Technical Director. No phase begins until the prior phase's exit criteria are met.

Phases 1–18 build the engine and its general-purpose subsystems. Phases 19–23 build **Blacklake + *Sacrilege*** subsystems (deterministic genesis, arena/GPTM, Sacrilege frame, Martyrdom combat stack, host/boss pipeline — see `docs/02_ROADMAP_AND_OPERATIONS.md`). Phase 24 hardens and ships. Phase 25 sustains.

The operational week-by-week schedule lives in `docs/02_ROADMAP_AND_OPERATIONS.md`. This section defines deliverables and exit criteria at the blueprint level.

### Phase 1 — Scaffolding & CI/CD · 4 weeks · Engine Infrastructure Lead

**Deliverables**
1. Repository layout (`/engine`, `/editor`, `/sandbox`, `/runtime`, `/gameplay`, `/bld`, `/tools`, `/docs`, `/third_party`), root `CMakeLists.txt`, `CMakePresets.json`, Clang toolchain files for both OSes.
2. CPM.cmake for C++ deps (pinned by SHA); Rust toolchain pinned via `rust-toolchain.toml`; `bld/` Cargo workspace scaffolded; Corrosion wired into CMake.
3. `cxx` + `cbindgen` plumbing: one hello-world Rust function callable from the editor via C-ABI.
4. `.clang-format`, `.clang-tidy`, `rustfmt.toml`, `.editorconfig`, `.gitattributes` committed.
5. GitHub Actions: `{windows, linux} × {debug, release}` matrix plus nightly ASan/UBSan/TSan and nightly Miri on `bld-ffi`.
6. Contributor docs: `BUILDING.md`, `CONTRIBUTING.md`, `CODING_STANDARDS.md`.

**Exit Criteria (*First Light*)** — fresh-clone build on either OS produces a hello-triangle sandbox **and** a hello-BLD smoke test via a single preset invocation; CI is green on `main`; `cargo clippy` and `cargo-deny` pass on the empty `bld/` workspace.

### Phase 2 — Platform & Core Utilities · 5 weeks · Engine Core Lead

**Deliverables**
1. `engine/platform/` — windowing, input plumb, timing, filesystem, dynamic-library loading for Windows and Linux.
2. `engine/memory/` — frame, arena, pool, free-list allocators with ASan-clean unit tests.
3. `engine/core/` — logger, assertions, `Result<T, E>`, error taxonomy.
4. `engine/core/` containers — `span`, `array`, `vector`, `hash_map`, `ring_buffer`. STL-free hot path.
5. `engine/core/string` — UTF-8 with small-string optimisation.
6. Custom `clang-tidy` OS-header check active across the repo.

**Exit Criteria** — allocator unit tests pass under ASan, UBSan, TSan; gameplay-module hot-reload demonstrated on both platforms; zero sanitizer warnings on nightly.

### Phase 3 — Math, ECS, Jobs & Reflection · 7 weeks · Engine Core Lead

**Deliverables**
1. `engine/math/` — Vec, Mat, Quat, Transform with SIMD behind a feature flag.
2. `engine/core/command/` — `ICommand`, `CommandStack`, transaction brackets (pulled forward because BLD and editor both depend on it).
3. `engine/jobs/` — custom fiber-based work-stealing scheduler, submission/wait/parallel_for APIs.
4. `engine/ecs/` — archetype chunk storage, generational entity handles, typed queries with Read/Write tags, system registration, scheduler on top of `engine/jobs/`.
5. Reflection primitives (macro-based for Phase 3, with tagged-header extension points for later libclang codegen); serialization framework primitives.

**Exit Criteria (*Foundations Set*)** — sandbox simulates 10 000 entities at a stable frame time under a defined CPU budget; `engine/jobs/` benchmarks within 5 % of enkiTS on the reference multi-threaded workload; unit coverage ≥ 80 % on core/math/memory/jobs/ECS; zero sanitizer warnings.

### Phase 4 — Renderer HAL & Vulkan Bootstrap · 8 weeks · Rendering Lead

**Deliverables**
1. `engine/render/hal/` — device, queue, surface, swapchain interface; ~30 types in total.
2. Vulkan 1.3 backend bootstrap — instance, physical-device selection, feature flagging, device creation, validation-layers-clean init.
3. HAL primitives — command buffers, fences, timeline semaphores.
4. VMA (VulkanMemoryAllocator) integration; Buffer/Image HAL wrappers.
5. Descriptor management — pools, sets, bindless via `VK_EXT_descriptor_indexing`.
6. Transfer queue + staging buffer management; dynamic rendering primitives.
7. Pipeline objects, pipeline cache, graphics + compute pipelines.

**Exit Criteria** — HAL contract documented; a stub WebGPU backend compiles (does not necessarily run) against the same interface; descriptor indexing + BDA + synchronization-2 paths verified.

### Phase 5 — Frame Graph & Reference Renderer · 8 weeks · Rendering Lead

**Deliverables**
1. Frame graph with pass declaration, resource reads/writes, topological sort, automatic barrier insertion (`synchronization_2`), transient resource aliasing.
2. Multi-queue scheduling with async-compute path.
3. Shader toolchain — HLSL → DXC → SPIR-V, content-hashed cache, editor hot-reload via file-watch.
4. Forward+ clustered lighting pass; light-grid build on compute queue.
5. Cascaded shadow maps (directional) + atlas shadows (spot/point).
6. IBL with split-sum approximation; tonemapping pass.
7. Tracy profiler integration.

**Exit Criteria (*Foundation Renderer*)** — the reference indoor-outdoor test scene renders at ≥ 60 FPS @ 1080p on RX 580 reference hardware; zero Vulkan validation warnings in debug builds; async-compute path demonstrably active on the reference renderer.

### Phase 6 — Asset Pipeline & Content Cook · 4 weeks · Tools Lead

**Deliverables**
1. `tools/cook/gw_cook` CLI — content-hashed incremental cook, parallel, deterministic output (bytewise-identical across OSes).
2. Mesh import: glTF 2.0 → runtime format; skeleton import → Ozz offline pipeline.
3. Texture import: KTX2 with BC7 + ASTC encoders.
4. Runtime asset database, virtual-mount filesystem, hot-reload watchers.

**Exit Criteria** — cook determinism is a CI gate (same inputs produce same content hashes on Windows and Linux); `gw_cook` callable from editor, CI, and BLD tooling.

### Phase 7 — Editor Foundation · 6 weeks · Tools Lead

**Deliverables**
1. `greywater_editor` executable — Dear ImGui + docking + viewports + ImGuizmo. **UI stack is locked; no custom UI framework is permitted.**
2. Scene hierarchy + selection model + outliner.
3. Reflection-driven inspector panel; asset browser.
4. Viewport with camera controls, 3D gizmos, debug-draw hooks.
5. Play-in-editor mode launching `gameplay_module`.
6. Public C-ABI registration API on the editor command stack so Phase 9 BLD tools can submit `ICommand` instances from outside.

**Exit Criteria (*Editor v0.1*)** — designer can author a scene, save, reopen, and see identical state; ImGuizmo gizmos operable; play-in-editor launches the gameplay module; dogfooded by the gameplay team for one full sprint.

### Phase 8 — Scene Serialization & Visual Scripting · 5 weeks · Tools Lead

**Deliverables**
1. `.gwscene` binary format — versioned, with migration hooks per schema bump.
2. Scene view over ECS — parent/child transforms, dirty-flag propagation.
3. Visual-scripting IR v1 — typed text IR + runtime interpreter in `engine/vscript/`.
4. vscript AOT cook to bytecode with deterministic-execution validation.
5. Node-graph projection (ImNodes) over the IR with a sidecar `.layout.json` for node positions.

**Exit Criteria** — `.gwscene` loads identically in editor, sandbox, and runtime; a visual script authored as text IR executes correctly in both editor (interpreted) and runtime (AOT); same script opened in the node-graph view renders a layout that round-trips through save/reload; IR-level merge-ability validated in a simulated conflict.

### Phase 9 — BLD (Brewed Logic Directive) — Rust · 12 weeks · AI Systems Lead

BLD is the engine's primary competitive differentiator. Implemented as a Rust crate (`bld/`) linked into the editor via Corrosion and a narrow C-ABI.

**Sub-phase 9A — Rust crate & C-ABI plumbing (2w).** Cargo workspace with the 8 sub-crates (`bld-mcp`, `bld-tools`, `bld-provider`, `bld-rag`, `bld-agent`, `bld-governance`, `bld-bridge`, `bld-ffi`). `rmcp`-backed MCP server. Claude cloud provider via `reqwest` + `eventsource-stream`. Keyring-backed credential store. ImGui chat panel streaming tokens from Rust via callback; `imgui_md` markdown. One hello-world read tool (`project.list_scenes`).

**Sub-phase 9B — Tool surface via proc macros (2w).** `#[bld_tool]` proc macro generating MCP schema + JSON decoder + dispatch + registry entry at compile time. `#[bld_component_tools]` macro generating `component.*` tools from the reflection registry. Mutating tools submit opcode+payload via C-ABI callback; editor enqueues as `ICommand`. MCP `elicitation` handler for diff-rendering approval dialogs.

**Sub-phase 9C — RAG (2w).** `tree-sitter` with grammars for C++, GLSL, HLSL, JSON, vscript IR, `.gwscene`. Symbol graph via `petgraph` + PageRank. Nomic Embed Code via `candle` (offline). `rusqlite` + `sqlite-vss` index. Engine-docs index pre-built; user-project index built incrementally via `notify`. Hybrid retrieval (ripgrep lexical + tree-sitter symbol + vector).

**Sub-phase 9D — Build, runtime & vscript tools (2w).** `build.*` wrapping CMake, parsing Clang JSON diagnostics. `runtime.*` exposing profiler snapshots. `code.*` for read/patch/symbol. `vscript.*` on the IR. Hot-reload pipeline end-to-end: agent writes gameplay code → build → reload → verify.

**Sub-phase 9E — Governance & polish (2w).** Three-tier permission system (`read`/`mutate`/`execute`). Append-only JSONL audit log. Session replay (`gw_agent_replay`). Multi-session tab UI. Slash commands as MCP Prompts. Cross-panel integration (right-click entity / compile error / profiler spike). Secret filter in every file-reading tool (compile-time-constant). `agent_ignore` support.

**Sub-phase 9F — Local hybrid & offline (2w).** `candle` (or `llama-cpp-rs` fallback) with GBNF grammars compiled from tool schemas. Hybrid router (local for embeddings / `docs.search`; cloud for plan-execute-verify). Offline-mode banner — never silently degrade. Contract tests per `ILLMProvider`. Miri clean on `bld-ffi`.

**Exit Criteria (*Brewed Logic*)** — full agent session (scene → components → code → hot-reload → verify) runs through the chat panel alone with every action on the command stack; Ctrl+Z reverses agent moves; external MCP clients (Claude Code, Cursor) drive the engine over the same protocol; offline mode works for RAG + `docs.*` + `scene.query`; zero secrets leak into any captured session log.

### Phase 10 — Runtime I: Audio & Input · 6 weeks · Runtime Systems Lead

**Deliverables**
1. `engine/audio/` — miniaudio mixer + format decoders (Opus, Vorbis, WAV) + streaming music.
2. Spatial audio via Steam Audio (HRTF + occlusion); `AudioSource` / `AudioListener` / `AudioZone` ECS components.
3. Cooked runtime audio format via `gw_cook`.
4. `engine/input/` — SDL3 device layer (gamepad/HID/haptics); keyboard/mouse through platform layer.
5. Action-map system — rebindable, chord-capable, TOML config + user-override persistence, accessibility defaults (hold-to-toggle, single-switch).

**Exit Criteria** — sandbox scene accepts controller + keyboard input through rebindable actions on both OSes; 3D spatial audio plays correctly on both; audio latency stays within budget under load.

### Phase 11 — Runtime II: UI, Events, Config, Console · 6 weeks · Runtime Systems Lead

**Deliverables**
1. `engine/core/events/` — type-safe pub/sub, zero-alloc in-frame buffer, cross-subsystem message bus.
2. `engine/core/config/` — typed CVar registry, TOML persistence, dev-vs-user tagging.
3. `engine/console/` — in-game dev console overlay (debug builds only), cvar get/set + command exec.
4. `engine/ui/` — RmlUi integration, HarfBuzz + FreeType text shaping, Vulkan-HAL backend for RmlUi.
5. Basic HUD and menu templates in the sandbox.

**Exit Criteria (*Playable Runtime*)** — sandbox renders a HUD through RmlUi with localized strings (placeholder locale table); dev console toggles in debug and is absent from release builds; event bus handles cross-subsystem traffic with zero in-frame allocation under Tracy profile.

### Phase 12 — Physics · 6 weeks · Simulation Lead

**Deliverables**
1. Jolt Physics integration — rigid bodies, colliders, triggers, ECS bridge.
2. Character controller (Jolt's).
3. Constraints, joints, vehicle.
4. Raycasts + shape queries + physics debug draw.
5. Deterministic mode with lockstep validation, fixed-step integration.
6. Physics playground scene used as regression test.

**Exit Criteria** — physics debug-draw visible in the editor viewport; deterministic mode passes a 60-second 4-client zero-drift test; regression scene runs stably at target frame time.

### Phase 13 — Animation & Game AI · 6 weeks · Simulation Lead

**Deliverables**
1. Ozz-animation runtime + ACL decompression.
2. ECS-backed blend trees; glTF → Ozz cook pipeline.
3. FABRIK + CCD IK; morph-target support.
4. Game AI: custom ECS-backed Behavior Tree executor.
5. Recast/Detour pathfinding integration + navmesh bake tool.

**Exit Criteria (*Living Scene*)** — a character walks a navmeshed scene driven by a BT, with blended animations and deterministic physics; BT authored in editor graph view and via BLD chat produce identical runtime behaviour.

### Phase 14 — Networking & Multiplayer · 12 weeks · Networking Lead

**Deliverables**
1. GameNetworkingSockets transport — encryption, NAT punch, congestion control, connection lifecycle.
2. ECS replication — delta compression, relevancy/interest management, per-component reliability tiers, priority scheduling.
3. Lag compensation — server-side rewind-and-replay (200 ms default window).
4. Netcode harness — latency injection, packet-loss simulation, desync detection, replay capture.
5. Voice — Opus codec, GNS transport, Steam Audio spatialisation; mute/block lists.
6. `ISessionProvider` with a dev-local backend; Steam/EOS backend stubs to be completed in Phase 16.
7. Deterministic-lockstep mode over Jolt's deterministic physics (Phase 12).

**Exit Criteria (*Two Client Night*)** — 2c + dedicated server at 60 Hz under 120 ms injected RTT for 10 minutes with < 1 % desync; voice round-trip < 150 ms on LAN with audibly correct spatialisation; 4-client lockstep runs 60 s with zero drift.

### Phase 15 — Persistence & Telemetry · 6 weeks · Production Systems Lead

**Deliverables**
1. Versioned binary save format (ECS snapshot), SQLite local DB, migration hooks with a save-compat regression suite.
2. `ICloudSave` abstraction with Steam Cloud + EOS + S3 backends.
3. Telemetry via Sentry Native SDK for crashes + custom event pipeline (SQLite-queued, opt-in).
4. Compliance: GDPR + CCPA + COPPA, in-game data-collection policy UI.

**Exit Criteria** — save written in v1 loads cleanly after a v2 schema migration; crash report round-trips to Sentry with symbolicated stack traces; telemetry compile-time-disabled in certified builds unless explicitly enabled.

### Phase 16 — Platform Services, Localization & Accessibility · 6 weeks · Production Systems Lead

**Deliverables**
1. `IPlatformServices` with Steam backend first, then EOS — achievements, leaderboards, presence, cloud save, Workshop.
2. Localization — ICU runtime, binary string tables, XLIFF export/import workflow.
3. Text shaping — HarfBuzz for complex scripts, RTL, ligatures; per-locale font-stack fallback.
4. Accessibility — color-blind modes (lighting-integrated), text scaling, subtitle system with speaker attribution, screen-reader hooks, motion-reduction, photosensitivity safe mode.

**Exit Criteria (*Ship Ready*)** — Steam + EOS dual-unlock of a placeholder achievement from a single gameplay event; WCAG 2.2 AA audit of the sample HUD passes; color-blind mode renders without visual regression.

### Phase 17 — Shader Pipeline Maturity, Materials & VFX · 6 weeks · Tools & Content Lead

**Status (2026-04-21): completed** — See `docs/02_ROADMAP_AND_OPERATIONS.md` §Phase 17 and ADRs 0074–0082. `dev-win` CTest 711/711; exit gate: `apps/sandbox_studio_renderer` + `STUDIO RENDERER` marker; `gw_perf_gate_phase17` + `docs/09_NETCODE_DETERMINISM_PERF.md` (**Phase 17 performance budgets — Studio Renderer**).

**Deliverables (landed)**
1. Shader-pipeline maturity — orthogonal permutation matrix (≤ 64 / template), content-hash cache, Slang opt-in, SPIRV-Reflect layout inference; DXC default.
2. Material system — `MaterialWorld` PIMPL, `MaterialTemplate` / `MaterialInstance`, `.gwmat` v1, glTF PBR + KHR clearcoat/specular/sheen/transmission; hot-reload on `material_reload` job lane; ImGui material-browser.
3. VFX + post — GPU particle lifecycle, ribbons, deferred decals; `engine/render/post/` dual-Kawase bloom, k-DOP TAA, McGuire motion blur, CoC DoF, chromatic aberration, grain, Khronos PBR Neutral + ACES.

**Exit Criteria** — *Studio Renderer* marker green: shader cache warm hit ≥ 95 %, `sandbox_studio_renderer` prints configured budgets, frozen render sub-phase order in `runtime/engine.cpp::tick` (see ADR-0082).

### Phase 18 — Cinematics & Mods · 5 weeks · Tools & Content Lead

**Deliverables**
1. Cinematics — timeline runtime with transform/property/event/camera/audio/subtitle tracks; ImGui docked timeline editor; `.gwseq` format.
2. Mod support — package format (vscript IR + assets + optional signed native dylib); sandboxed vscript tool tier; native-mod consent flow; Workshop integration behind `IPlatformServices`.

**Exit Criteria (*Studio Ready*)** — a designer-authored cutscene plays identically in editor and runtime with localized subtitles; a mod installs from the Workshop flow, runs sandboxed, authored via BLD.

---

### *Sacrilege* / Blacklake phases (19–23)

Phases 19–23 build engine-side capabilities for **Blacklake** procedural genesis and the ***Sacrilege*** debut title. They consume prior platform work (Phases 1–18) and extend `engine/world/`, gameplay facades, and content pipelines. **Authoritative week rows:** `docs/02_ROADMAP_AND_OPERATIONS.md`. **Flagship spec:** `docs/07_SACRILEGE.md`. **Procedural depth:** `docs/08_BLACKLAKE_AND_RIPPLE.md`.

### Phase 19 — Blacklake Core & Deterministic Genesis · 8 weeks · World Systems Lead

**Deliverables**
1. `engine/world/universe/` — 256-bit Σ₀, coordinate model, **HEC-style** hierarchical sub-seeds (arena/circle scope + optional large-scale graph).
2. `engine/world/streaming/` — chunk streaming (LRU, time-sliced jobs) for arena and open templates.
3. Noise pipeline — **SDR** integration target + classical composition (Perlin/Simplex/Worley/warp/fBm) on GPU where applicable.
4. Macro masks — temperature, humidity, **corruption**, circle-theme weights.
5. `engine/world/resources/` — placement rules keyed to circle archetypes.
6. Constraint synthesis — WFC (or equivalent) for structured arenas.

**Exit Criteria (*Infinite Seed*)** — same Hell Seed + coordinate → byte-identical chunk content on Windows and Linux; determinism suite passes; streaming holds RX 580 frame budget under stress.

### Phase 20 — Arena Topology, GPTM & Sacrilege LOD · 8 weeks · World Systems Lead

**Deliverables**
1. **GPTM** slice — hybrid heightfield / sparse-volume modules for large inverted-circle arenas.
2. GPU mesh generation for terrain and organic tunnel walls.
3. Tessellation / LOD handoff for close brutal detail.
4. `engine/world/origin/` — floating origin for km-scale arenas.
5. Materials — circle themes (ice, gut, forge, maze, void).
6. Instancing / impostors for crowds and distant geometry.
7. Fluids / blood pools as budget allows (Tier B).

**Exit Criteria (*Nine Circles Ground*)** — multi-layer arena fly-through at target FPS @ 1080p RX 580; no visible precision or LOD failures in the demo path.

### Phase 21 — Sacrilege Frame — Rendering, Atmosphere, Audio · 8 weeks · Rendering + Audio Leads

**Deliverables**
1. Forward+ stress — many locals + emissive horror reads.
2. Local volumetrics — smoke, fog, censer clouds.
3. Screen-space blood/grime composite; film damage optional.
4. GPU decal persistence hooks for Martyrdom feedback.
5. Dynamic music mixer — intensity-driven layers.
6. VO + sting bus; environmental beds per circle.

**Exit Criteria (*Hell Frame*)** — one-circle vertical slice hits visual/audio intent with perf gates in `docs/09_NETCODE_DETERMINISM_PERF.md`.

### Phase 22 — Martyrdom Combat & Player Stack · 6 weeks · Gameplay Systems Lead

**Deliverables**
1. ECS **Martyrdom** state — Sin, Mantra, Rapture, Ruin.
2. Blasphemies ability system + tuning tables.
3. Profane/Sacred weapon framework + alt-fires.
4. Locomotion — sprint, slide, wall-kick, grapple, blood-surf hooks.
5. Melee / glory-kill sync events.

**Exit Criteria (*Martyrdom Online*)** — sandbox proves full resource loop and weapon swaps under net + determinism bars for combat.

### Phase 23 — Damned Host, Circles Pipeline & Boss · 6 weeks · Gameplay + Content Tools Lead

**Deliverables**
1. Enemy archetypes v1 — data-driven roster + AI wiring.
2. Encounter director — spawn budgets + perf caps.
3. Circles pipeline — procedural base + designer markup + **BLD** authoring hooks.
4. Boss framework — phases, weak points, cinematic hooks.
5. Seed-sharing / demo packaging.

**Exit Criteria (*God Machine RC*)** — boss + two circles + full Martyrdom loop in nightly build.

---

### Phase 24 — Hardening & Release · 8 weeks · Engineering Director

**Deliverables**
1. Testing expansion — perf regression gates in CI (> 5 % fails the build); libFuzzer harnesses for asset parsers, scene deserializer, network packet handling; `cargo-mutants` on `bld-governance` and `bld-ffi`; BLD eval harness scoring canned tasks.
2. Packaging — CPack installers (MSIX, AppImage, DEB, RPM); Authenticode + GPG signing; symbol-server upload (PDB + split DWARF).
3. Anti-cheat boundary — `IAntiCheat` interface finalized; EAC integration documented in a studio-internal addendum (not in the open repo); content-integrity hashing on cooked assets.
4. Documentation — engineering handbook, onboarding guide, LTS runbook; all indexable by BLD's RAG.
5. Board-room demo — end-to-end gameplay vertical slice running on the finished engine, profiled under Tracy, demonstrating BLD authoring, multiplayer, audio, accessibility.

**Exit Criteria (*Release Candidate*)** — RC installer passes certification smoke on Windows and Linux; LTS branch cuts cleanly; hotfix backport pipeline demonstrated; zero sanitizer warnings, zero Miri UB, zero clippy `deny` across the full build; engineering handbook returns correct answers to ≥ 95 % of a canned question set via BLD; 10-minute board-room demo runs unattended on reference hardware.

### Phase 25 — LTS Sustenance · ongoing · Engineering Director

Not time-bounded — the steady state after RC.

| Activity                             | Cadence                    |
| ------------------------------------ | -------------------------- |
| Hotfix backports to active LTS       | as-needed, weekly SLA      |
| New LTS branch cut                   | quarterly                  |
| Security advisory response           | 24-hour SLA (P0/P1)        |
| Dependency upgrades                  | monthly review             |
| Engineering-handbook updates         | continuous                 |
| BLD RAG index rebuild                | on every merge to `main`   |
| Deprecation announcement             | two LTS cycles ahead       |
| Removal of deprecated APIs           | one LTS cycle after announcement |

LTS branches are supported for **12 months** from cut. Two LTS branches are live at any time (current and previous). Titles pin to a specific LTS.

---

## 6. Risks & Mitigations

| Risk                                                            | Likelihood | Impact | Mitigation                                                                                 |
| --------------------------------------------------------------- | ---------- | ------ | ------------------------------------------------------------------------------------------ |
| Clang on Windows diverges from Linux (ABI, SDK drift)           | Medium     | High   | CI matrix blocks drift; Clang version pinned and upgraded in lockstep.                     |
| Vulkan driver variance across vendors                            | High       | Medium | Validation layers on in debug; reference GPU list published; RenderDoc captures archived.  |
| ECS performance does not beat object-graph baseline              | Low        | High   | Bench gates in Phase 2; if missed, escalate before Phase 3 begins.                         |
| Scope creep from "nice to have" editor features                  | High       | Medium | Phase 4 scope is frozen at sign-off. New asks go to Phase 5 backlog.                       |
| Talent concentration on ECS and renderer                         | Medium     | High   | Design docs and decision records (ADRs) mandatory in `docs/10_APPENDIX_ADRS_AND_REFERENCES.md`. No undocumented tribal knowledge. |
| MCP spec drift between protocol versions                         | Medium     | Medium | Vendor one spec version; upgrade on studio cadence, not upstream's. `ai/mcp/` is a thin facade.    |
| Cloud LLM provider API breakage or policy change                  | High       | Medium | `ILLMProvider` abstraction; contract tests per provider; at least two providers live at all times. |
| BLD authors gameplay code that reaches production unreviewed      | Medium     | High   | Pre-commit hook checks audit trail; mutate-tier actions require human approval; agent commits tagged. |
| Local model tool-use quality too weak for real agent loops       | High       | Low    | Offline mode ships with explicit "reduced capability" banner; degrade gracefully, never silently.   |
| Tool-surface explosion confuses the model                         | Medium     | Medium | Cap public tools at ~80; compose finer operations server-side; measure via eval harness in CI.     |
| Secrets leak into model context                                    | Low        | Critical | Hard-coded filter list in file-read tool; audit-log scanner blocks merges if a secret pattern appears. |
| Rust/C++ FFI boundary refactoring cost compounds over time         | Medium     | Medium | Keep the C-ABI narrow and stable; any boundary change requires an ADR; Miri coverage over `bld-ffi`. |
| Rust game-engine hiring pool is small                              | High       | Medium | Restrict Rust to BLD only; hire C++ engine engineers and grow them into Rust for BLD maintenance.   |
| Cross-language debugger friction slows BLD development             | Medium     | Low    | Standardize on LLDB with Rust support; document the debugging workflow in `docs/DEBUGGING.md`.      |
| Rust crate compile time bloats CI matrix                           | Medium     | Low    | `sccache` + Cargo's incremental + CI caching of `target/`; keep BLD crate lean (no mega-dependencies). |
| `cxx`/`cbindgen` upstream breakage on a toolchain upgrade          | Low        | Medium | Pin Rust toolchain in `rust-toolchain.toml`; upgrade on studio cadence, not upstream's.            |
| `rmcp` (Rust MCP SDK) lags spec updates                            | Medium     | Low    | `bld-mcp` is a thin facade; if `rmcp` drifts, we can fork or re-implement the transport.           |
| Jolt Physics determinism breaks across platform/compiler updates  | Medium     | High   | Pin Jolt version; determinism contract tests gate every Jolt upgrade; replay-based CI check.        |
| GameNetworkingSockets API changes between upstream SDK releases   | Low        | Medium | Vendor a pinned version; upgrade on studio cadence with a compatibility harness.                    |
| Steam Audio licensing restricts distribution (platform-specific)  | Low        | Medium | Abstract behind `engine/audio/`; platform fallback to panning-only spatial if license unavailable. |
| RmlUi maintenance burden (smaller community than commercial UI)   | Medium     | Medium | Contribute upstream; keep a studio fork for studio-critical patches; UI module is well-scoped.      |
| Save-format migration complexity on live titles                    | High       | High   | Versioned schema + explicit migration hooks per bump; save-compat test suite runs every PR.         |
| Platform services vendor lock-in (Steam-specific features)        | Medium     | Medium | Engine depends only on `IPlatformServices`; Steam-only features are opt-in per-title, not per-engine.|
| Voice-chat latency exceeds budget in competitive multiplayer      | Medium     | Medium | Per-region voice servers; Opus at 20ms frames; latency regression tests in netcode harness.         |
| Localization workflow falls behind engineering cadence             | High       | Medium | Translator-visible keys frozen per-phase; CI fails on untranslated keys in release branches.        |
| Fiber-based job system hits platform limitations (Linux fiber quirks) | Low    | High   | Abstract fiber primitives behind `jobs::IFiber`; `makecontext`/`swapcontext` with ABI-guarded tests.|
| Mod sandbox escape                                                  | Low        | Critical | vscript sandboxed by default; native-mod opt-in per user with signing; competitive MP disables mods. |
| Sentry outage blocks crash reporting                                | Medium     | Low    | Local SQLite queue (§3.16) buffers reports; retry with exponential backoff; no data lost.           |
| Content pipeline non-determinism (cook produces different bytes) | Medium     | High   | Cook determinism is a CI gate — same inputs must produce same content hashes across OSes.          |
| Recast/Detour navmesh bake time on large worlds                    | Medium     | Medium | Incremental navmesh tiles; off-thread bake via §3.18; editor progress indicator.                    |
| Anti-cheat integration leaks into open repo                        | Low        | High   | `IAntiCheat` interface only in open repo; AC SDK integrations live in studio-internal addendum.    |

---

## 7. Approval

This blueprint is submitted for stakeholder sign-off. Approval authorizes the Engineering Infrastructure Lead to begin Phase 1 on the date of signature.

| Role                             | Name | Signature | Date |
| -------------------------------- | ---- | --------- | ---- |
| CEO, Cold Coffee Labs            |      |           |      |
| Technical Director               |      |           |      |
| Principal Engine Architect       |      |           |      |
| Head of Production               |      |           |      |

---

*End of document.*


# Greywater_Engine × Cold Coffee Labs — Core Architecture

> *A narrative walkthrough of the engine we are building, who it is for, and the decisions behind it.*

**Author:** Principal Engine Architect, Cold Coffee Labs
**Date:** 2026-04-21
**Companion document:** [`grey-water-engine-blueprint.md`](grey-water-engine-blueprint.md) (the formal stakeholder blueprint)

---

## Chapter 1 — Intro

Welcome to the Greywater_Engine project.

At Cold Coffee Labs, we are building a game engine from scratch. Not as a tutorial exercise, and not as a thin wrapper over someone else's runtime — as a real, production-intent technology stack that we own end to end. The purpose of this document is to lay out, plainly and in one place, what the engine is, how it is architected, and the engineering principles we will hold ourselves to while building it.

If you are a gameplay programmer, a tools engineer, a renderer specialist, or a stakeholder trying to understand what the team is actually doing — this is the document to read first.

The name is **Greywater_Engine**. It is the engine that powers Cold Coffee Labs titles. Nothing more clever than that.

---

## Chapter 2 — Series / Project Overview

Greywater_Engine is a long-horizon project. It is not a six-month sprint to a demo. It is the foundation we expect to build and ship multiple games on over multiple years, and every early decision is weighed against that horizon.

What we are **not** doing:
- We are not trying to out-feature the large commercial engines. If shipping a game quickly is the only goal, those exist.
- We are not building a tutorial engine. Clarity matters, but "easy to follow in a YouTube video" is not a design constraint.
- We are not building an educational toy. Every subsystem has to hold up under a real production workload.

What we **are** doing:
- Building a **data-oriented, cache-aware, native C++23** runtime. C++ is the language for the engine core, editor, sandbox, runtime, and gameplay module.
- Building an editor that is **the same language, the same toolchain, and the same process boundary** as the runtime it edits. UI is locked to **Dear ImGui + docking + viewports + ImNodes + ImGuizmo** — no custom UI framework.
- Building a **cross-platform-from-day-one** pipeline so that "it works on my machine" is never a credible bug report.
- **Targeting the RX 580 (Polaris, 2016, 8 GB, Vulkan 1.2/1.3)** as the baseline. Every Tier A visual feature runs at 1080p / 60 FPS on this card. No ray tracing, no mesh shaders, no GPU work graphs as baseline — all are Tier G (hardware-gated, post-v1).
- **Baking a first-class, native AI copilot — the Brewed Logic Directive (BLD) — into the engine as a Rust crate**, linked into the editor via a narrow C-ABI. Rust is used **only for BLD**; the rest of the engine is C++23.
- **Visual scripting is text-first, AI-native, and graph-visualized.** A typed text IR is the ground-truth serialized form; the node graph is a reversible projection with a sidecar layout file. BLD reads and writes the IR directly.
- **Purpose-built for *Sacrilege* first.** Greywater_Engine is designed around the debut title’s needs (Martyrdom loop, Blacklake hellscapes, BLD-authored content) — with a clean engine/game code boundary so the engine remains reusable for future Cold Coffee Labs titles.
- **Flagship art direction (*Sacrilege*).** Crunchy low-poly horror, high-contrast lighting, engine capability for GW-SAC vision — see `docs/07_SACRILEGE.md`.
- Building it in the open within the studio — every architectural decision gets a written record under `docs/10_APPENDIX_ADRS_AND_REFERENCES.md`.

---

## Chapter 3 — Platform Support

We support **Windows and Linux from the first commit.** Not "Windows first, Linux later." Both, simultaneously, gated by CI.

The reasoning:
- Retrofitting cross-platform support later is always more expensive than building it in from the start.
- Our CI runners are cheaper on Linux, our shipping targets are heavier on Windows. Supporting both keeps us honest.
- A platform abstraction layer that is exercised on two OSes on every PR is a platform abstraction layer that actually works. One that is only exercised on one OS is wishful thinking.

**macOS, consoles, mobile, and WebGPU-in-browser** are not in scope for the initial phases. The HAL is designed to accept a WebGPU backend and the platform layer is designed to accept additional OS targets — but we will not commit to those platforms until a business case names them.

---

## Chapter 4 — Tech Stack

The stack is short on purpose. Every tool in it earns its place.

| Concern                 | Choice                                                              | Why                                                                                     |
| ----------------------- | ------------------------------------------------------------------- | --------------------------------------------------------------------------------------- |
| **Language (engine)**   | C++23 (moving to C++26 as Clang lands features)                     | Zero-cost abstractions, `constexpr`/`consteval`, concepts, modules. No runtime tax.     |
| **Language (BLD only)** | Rust stable (2024 edition)                                          | Proc macros for tool surfaces, `rmcp`, and Rust's ML ecosystem outclass the C++ alternatives. Used nowhere else. |
| **C++ compiler**        | Clang / LLVM 18+ (only)                                             | Identical frontend on every OS. `clang-cl` on Windows gives MSVC-ABI parity without MSVC. |
| **Rust toolchain**      | Cargo, pinned via `rust-toolchain.toml`                             | Cargo builds the `bld/` workspace; CMake orchestrates via Corrosion.                    |
| **Build generator**     | CMake 3.28+ with **Presets** and **file sets** (+ **Corrosion** for Rust) | CMake is the single source of truth; Corrosion invokes Cargo as a sub-build.        |
| **Rust/C++ bridge**     | `cxx` (C++ side) + `cbindgen` (Rust → C header)                     | Narrow, well-tested bindings. Firefox-scale production proof.                            |
| **Dependency manager**  | CPM.cmake (C++) + Cargo `Cargo.lock` (Rust), both pinned            | Reproducible, version-locked, no system-library roulette.                               |
| **Graphics API**        | Vulkan 1.3 (dynamic rendering, timeline semaphores, bindless)       | Modern, explicit, and the best fit for a frame graph.                                   |
| **Editor UI**           | **Dear ImGui (docking + viewports) + ImNodes + ImGuizmo — LOCKED**  | No custom UI framework. Proven, mature, correct tool for an engine editor. Permanent.    |
| **Visual scripting UI** | ImNodes over a typed text IR (IR is ground-truth, graph is a view)  | AI-native, diff-friendly, merge-resolvable. No Blueprint-style graph-as-ground-truth.   |
| **Profiler**            | Tracy (+ Rust bindings via `tracy-client` for BLD)                  | Frame-level, low overhead, remote-capable, language-neutral.                             |
| **Testing**             | doctest (C++), `cargo test` + Miri (Rust)                           | Per-language native tooling. Miri validates the C-ABI for UB.                            |
| **CI**                  | GitHub Actions, matrix `{win, linux} × {debug, release}` + nightly sanitizers + Miri | Both languages on both OSes; zero-tolerance policy on nightly failures. |
| **Formatting & Lint**   | `clang-format`, `clang-tidy`, `cmake-format`, `rustfmt`, `clippy`, `cargo-deny` | Enforced in CI; non-conforming PRs auto-blocked.                           |
| **Agent protocol**      | **Model Context Protocol (MCP) 2025-11-25** via `rmcp` (Rust)       | BLD *is* an MCP server. Editor chat panel is one client; Claude Code, Cursor, Zed, CI are others. |
| **Cloud LLM (default)** | Claude Opus 4.6+                                                    | Best-in-class tool-use for our agent workloads. Alternate cloud LLMs configurable via `ILLMProvider`. |
| **Local LLM**           | `candle` (pure-Rust) primary, `llama-cpp-rs` fallback, with GBNF grammars | Offline inference; structurally valid tool calls even on small models.              |
| **Code chunking (RAG)** | `tree-sitter` (Rust bindings) — grammars: C++, GLSL, HLSL, JSON, vscript IR, `.gwscene` | AST-aware chunks; one function/class/component per chunk.              |
| **Embeddings (default)**| `nomic-embed-code` model via `candle`                               | Offline, no API keys. Cloud embedding providers pluggable for studio upgrade.             |
| **Vector index**        | SQLite + `sqlite-vss` via `rusqlite`                                | Embedded, no server process. LanceDB embedded as alternate.                              |
| **HTTP/SSE**            | `reqwest` + `eventsource-stream` (Rust)                             | Used by `ILLMProvider` backends.                                                         |
| **Markdown (editor UI)**| `imgui_md` (MD4C-based)                                             | Streaming token rendering in the BLD chat panel, driven by callbacks from Rust.         |
| **Secret storage**      | OS keychain via `keyring` crate (Windows Credential Manager / libsecret) | API keys never plaintext on disk, never reach model context.                        |

Tools explicitly **not** in the stack: MSBuild, `.sln`/`.vcxproj` files, Visual Studio as a build source, GCC, MSVC's compiler, CMake's non-Ninja generators, any managed runtime, any scripting language embedded in the engine (beyond hot-reloaded native C++).

---

## Chapter 5 — Project Structure

Greywater_Engine ships as **four linkable artifacts** plus one hot-reloadable module. The boundary between them is enforced by CMake target visibility — it is not a suggestion.

```
cold-coffee-labs/
├── engine/                 # greywater_core — C++23 static library
│   ├── platform/           # OS abstraction (the only place OS headers are allowed)
│   ├── memory/             # Arena, frame, pool allocators
│   ├── core/               # Logger, asserts, containers, strings, errors
│   │   ├── command/        # Undo/redo command bus (ICommand, CommandStack, transactions)
│   │   ├── events/         # Type-safe pub/sub; in-frame + cross-subsystem bus
│   │   └── config/         # CVar registry, TOML persistence
│   ├── math/               # Vec, mat, quat, transform
│   ├── jobs/               # Custom fiber-based work-stealing scheduler
│   ├── ecs/                # Archetype ECS, scheduler (atop jobs)
│   ├── render/             # HAL + Vulkan backend + frame graph
│   │   ├── shader/         # HLSL/GLSL authoring, DXC, cooked permutations
│   │   └── material/       # Material instance system
│   ├── assets/             # Import, cook, pack pipeline
│   ├── scene/              # Scene graph view over ECS, serialization
│   ├── scripting/          # Dynamic library loader for gameplay hot reload
│   ├── vscript/            # Visual-scripting IR runtime (typed text IR, interpreter, AOT cook)
│   ├── audio/              # miniaudio + Steam Audio; mixer, DSP, streaming
│   ├── net/                # GameNetworkingSockets + ECS replication + voice
│   ├── physics/            # Jolt Physics integration
│   ├── anim/               # Ozz + ACL runtime, blend trees, IK
│   ├── input/              # SDL3 device layer + action-map system
│   ├── ui/                 # RmlUi for in-game UI (HUD, menus) — NOT ImGui
│   ├── i18n/               # ICU + HarfBuzz localization
│   ├── a11y/               # Accessibility (color-blind, text scaling, captions, screen-reader)
│   ├── persist/            # Save system, SQLite, cloud-save abstraction
│   ├── telemetry/          # Sentry crash reporting + custom event pipeline
│   ├── console/            # In-game dev console (debug builds only)
│   ├── gameai/             # Behavior trees + Recast/Detour pathfinding
│   ├── vfx/                # GPU particle system, ribbon trails, decals, post
│   ├── sequencer/          # Cinematics timeline runtime
│   ├── mod/                # Mod package format, sandbox, signing
│   └── platform_services/  # IPlatformServices (Steam, EOS, Galaxy backends)
│
├── bld/                    # BLD — Rust crate (Cargo workspace), linked into editor via Corrosion
│   ├── Cargo.toml          # workspace root
│   ├── bld-mcp/            # rmcp-based MCP server
│   ├── bld-tools/          # #[bld_tool] proc macro, registry, dispatcher, tier/permission
│   ├── bld-provider/       # ILLMProvider trait + Claude cloud + alternate-cloud + candle backends
│   ├── bld-rag/            # tree-sitter chunking, embeddings, sqlite-vss, symbol graph + PageRank
│   ├── bld-agent/          # plan/act/verify loop, sessions, memory, audit log, replay
│   ├── bld-governance/     # tier policy, HITL elicitation handlers, secret filter
│   ├── bld-bridge/         # cbindgen-generated C header + cxx-facing exported API
│   └── bld-ffi/            # C-ABI: opaque handles, POD messages, callback function pointers
│
├── editor/                 # greywater_editor — C++ executable, statically linked to core + bld
│   ├── agent_panel/        # Docked ImGui chat panel — streaming tokens, markdown, diff UI
│   ├── bld_bridge/         # C++ glue for the Rust BLD crate (cxx bindings, command-bus adapter)
│   └── vscript_panel/      # ImNodes node-graph view over the vscript IR (projection, not ground-truth)
├── sandbox/                # engine feature test bed — executable
├── runtime/                # game_runtime — the shipping executable
├── gameplay/               # gameplay_module — hot-reloadable .dll / .so
│
├── tools/                  # asset cookers, shader compilers, CLI utilities
│   └── cook/               # gw_cook — deterministic content cooker
├── tests/                  # integration tests, perf regression, fuzz harnesses, BLD evals
├── content/                # source assets (gitignored — Blender, Substance, DAW projects)
├── assets/                 # cooked, deterministic runtime assets (tracked)
├── third_party/            # CPM-managed, pinned by SHA
├── cmake/                  # toolchain files, preset fragments, dependency declarations
│   └── packaging/          # CPack config — MSIX, AppImage, DEB, RPM
├── docs/                   # architecture, ADRs, build guides, engineering handbook
└── CMakePresets.json
```

### The five artifacts

1. **`greywater_core`** — the engine. A static library. No executable. It cannot depend on the editor or the game. If it builds and passes its tests in isolation, it is healthy.
2. **`greywater_editor`** — the authoring tool. A native C++ executable. Statically links `greywater_core`. Uses ImGui docking + viewports. Writes the same binary scene format the runtime reads.
3. **`sandbox`** — the engine's own test bed. A native C++ executable used to exercise engine features directly, without the weight of the editor or the game. This is where new subsystems prove themselves first.
4. **`game_runtime`** — the shipping game executable. Statically links `greywater_core`. Loads `gameplay_module` at startup. This is what reaches players.
5. **`gameplay_module`** — the game's own code. The **only** dynamic library in the system. It is hot-reloadable in both the editor and the runtime: change gameplay code, recompile, and the running process picks up the new module without shutdown.

The editor is **not** shipped inside the game runtime. Editor code never reaches players. This is a hard boundary, enforced by the fact that `game_runtime` has no dependency on the editor target.

---

## Chapter 6 — Feature List

This is not an exhaustive roadmap — it is the feature set that defines what "Greywater_Engine Phase 1-4 complete" means. Everything here is scheduled in the companion blueprint.

**Build & Infrastructure**
- Reproducible build on Windows and Linux via a single preset
- GitHub Actions CI with matrix builds and nightly sanitizer runs (ASan, UBSan, TSan)
- `sccache`-accelerated compile on developer machines and CI

**Platform Layer**
- Window creation, input (keyboard, mouse, gamepad), timing, high-resolution clocks
- Filesystem abstraction with virtual mount points
- Dynamic library loading with hot-reload file watching
- Threading primitives (threads, atomics, lock-free queues)

**Core**
- Structured logger with compile-time levels and runtime sinks
- Assertion framework with stack traces and graceful debug breaks
- Result/error types (no exceptions in runtime code paths)
- Cache-friendly containers: `span`, `array`, `vector`, `hash_map`, `ring_buffer`
- UTF-8 string handling with small-string optimization

**Memory**
- Frame allocator (per-frame linear reset)
- Arena allocators (scoped, O(1) teardown)
- Pool allocators (fixed-size, for hot component types)
- No global `new`/`delete` in engine code. Allocators are always passed explicitly.

**Math**
- Vectors, matrices, quaternions, affine transforms
- SIMD path behind a feature flag
- `constexpr` where the standard permits

**ECS**
- Archetype-based, SoA storage
- Generational entity handles (index + generation, 64-bit total)
- Systems are plain functions with declared read/write sets
- Scheduler runs non-conflicting systems in parallel, lock-free

**Renderer**
- Hardware Abstraction Layer: ~30 types, backend-agnostic
- Vulkan 1.3 backend: dynamic rendering, timeline semaphores, bindless descriptors, VMA for allocation
- Frame graph with explicit resource lifetimes and automatic barrier insertion
- Forward+ reference renderer capable of sponza-class scenes at 144 FPS @ 1440p on reference hardware
- Shader pipeline: GLSL/HLSL → SPIR-V, content-hashed cache

**Assets**
- Binary, versioned, engine-native asset format
- glTF 2.0 mesh import
- KTX2 texture import (BC7, ASTC)
- Cook step separate from runtime load — runtime never parses JSON

**Scene**
- Scene graph as a **view** over the ECS, not a parallel hierarchy
- Binary scene serialization (`.gwscene`), forward- and backward-migratable
- Undo/redo in the editor backed by a command stack over ECS mutations

**Scripting (Hot Reload)**
- Gameplay code lives in `gameplay_module` (`.dll` / `.so`)
- File watcher detects rebuild, reloads symbols, re-binds component vtables
- State is preserved across reload via opaque handles

**Tools**
- ImGui docking editor with multi-viewport support
- Scene hierarchy, inspector, asset browser, gizmo-driven viewport
- Play-in-editor mode that launches `gameplay_module` against the current scene
- Tracy integration for frame-level profiling

**BLD — Brewed Logic Directive (Native AI Copilot, implemented in Rust)**
- Implemented as a Rust crate (`bld/`), built via Corrosion, linked into the editor through a narrow C-ABI (`cxx` + `cbindgen`)
- Agentic chat copilot with plan/act/verify loop, docked ImGui panel (C++ side), streaming tokens from Rust via callbacks
- **Model Context Protocol (MCP) server** via `rmcp` — any compliant client can drive the engine
- **Tool surface auto-generated at compile time** by `#[bld_tool]` proc macros on tagged Rust functions
- **Every mutating tool is an `ICommand`** on the C++ editor's command stack (via C-ABI callback) — Ctrl+Z reverts agent moves atomically
- Three-tier permission system in `bld-governance`: `read` auto-approved, `mutate` gated, `execute` always prompts
- `tree-sitter` + `petgraph` symbol graph + PageRank RAG over engine source, engine docs, and user project
- Cloud LLM providers via Rust HTTP/SSE clients: Claude cloud default; alternate cloud LLMs configurable via `ILLMProvider`
- Local inference via `candle` (pure-Rust) or `llama-cpp-rs`; GBNF grammars for structurally valid tool calls; offline mode
- Cross-panel integration — right-click entity → "ask BLD"; click compile error → "fix this"; right-click profiler spike → "diagnose"
- Append-only audit log; session replay tool for QA and incident review
- Hard secret filter in every file-reading tool — `.env`, `*.key`, OS-keychain entries never reach model context

**Visual Scripting**
- **Typed text IR is ground-truth.** Node graph is a reversible projection with a sidecar layout file.
- **ImNodes** editor widget renders the graph view; node positions persisted in `.layout.json` (never in the IR file)
- BLD's `vscript.*` tools read and write the IR directly — AI-authored gameplay logic is a first-class workflow
- IR executes in both editor (interpreted, fast iteration) and runtime (AOT-cooked to bytecode on asset build)
- Diffs are meaningful, merges are resolvable, undo/redo happens at IR level
- No Blueprint-style "graph is ground-truth" complexity; no "nativization" step

**Audio**
- `miniaudio` device/mixer backend (MIT, cross-platform, single-header)
- Steam Audio for HRTF spatial, occlusion, reflection baking
- Opus (voice/streaming), Ogg Vorbis (music), WAV (source SFX)
- ECS components: `AudioSource`, `AudioListener`, `AudioZone`
- Voice chat shares the same codec, transported by the networking layer

**Networking & Multiplayer**
- **GameNetworkingSockets** transport (battle-tested at large-scale multiplayer)
- Authoritative client-server default; deterministic lockstep mode for fighting-game-class titles
- Custom ECS delta replication with relevancy/interest management and per-component reliability tiers
- Server-side rewind-and-replay lag compensation (200ms default window)
- Opus voice chat with Steam Audio spatialization
- `ISessionProvider` abstracts matchmaking; Steam + EOS backends

**Physics**
- **Jolt Physics** (open-source, production-proven)
- Rigid bodies, character controllers, constraints, triggers, raycasts, vehicles
- ECS components: `RigidBody`, `Collider`, `CharacterController`, `Trigger`, `PhysicsConstraint`
- Deterministic mode for lockstep multiplayer
- Threaded via our custom job scheduler

**Animation**
- **Ozz-animation** runtime + **ACL** compression
- glTF authoring pipeline → Ozz runtime format via cook step
- Data-driven blend trees on ECS
- FABRIK + CCD IK built-in
- Morph targets via glTF

**Input**
- **SDL3** device layer (gamepad, HID, haptics) — platform layer handles keyboard/mouse directly
- Logical action-map system, rebindable per user, TOML default + user override
- Chord support, accessibility-conscious (hold-to-toggle, single-switch, full rebind)
- ECS component `PlayerInput`; actions fire events

**In-Game UI (HUD, Menus)**
- **Deliberately not ImGui.** ImGui is editor-only.
- **RmlUi** (MIT, HTML/CSS-inspired, native C++, production-proven)
- HarfBuzz + FreeType for shaping (complex scripts, RTL, ligatures, COLR color fonts)
- Vector rendering through the Vulkan HAL — RmlUi's backend is rewritten to emit through our render layer
- Input via the action system, not raw events
- Strings via the localization layer

**Localization & Accessibility**
- Binary string tables loaded per-locale at runtime
- ICU for pluralization, date/number/currency, collation, Unicode
- HarfBuzz for complex-script text shaping; per-locale font stacks with auto-fallback
- XLIFF export/import workflow for external translators
- Color-blind modes (lighting-integrated), text scaling, subtitle + speaker attribution, screen-reader hooks, motion-reduction, photosensitivity safe mode
- WCAG 2.2 AA compliance goal

**Persistence & Save System**
- Versioned binary ECS snapshot saves, with explicit migration hooks per version
- **SQLite** local database for settings, achievements mirror, telemetry queue, save metadata, mod registry
- `ICloudSave` abstraction over Steam Cloud / EOS / S3
- Config: TOML user-facing, binary runtime

**Telemetry, Analytics & Crash Reporting**
- **Sentry Native SDK** for crash reporting with minidump upload and server-side symbolication
- Custom event pipeline, batched locally in SQLite, shipped to a studio-operated ingest endpoint
- Opt-in only; GDPR/CCPA/COPPA compliant; fully compile-time-disabled in certified builds by default

**Job System & Threading**
- Custom **fiber-based work-stealing scheduler** (fibers preserve stack state across yields)
- One worker per logical core minus one; main thread is not a worker
- ECS scheduler, render frame graph, physics, animation, cook, RAG indexing all consume it
- Hard rule: no `std::async`, no raw `std::thread` in engine code

**Event System**
- Type-safe pub/sub over ECS events
- In-frame: zero-allocation, double-buffered, end-of-frame drain
- Cross-subsystem: typed message bus
- BLD `event.*` tools let the agent subscribe and emit

**Config, CVars & Dev Console**
- Typed CVar registry; runtime-settable; persist-optional; tagged `dev-only` vs `user-facing`
- In-game dev console overlay (debug builds only) — separate from BLD
- TOML (user) + binary (runtime) config formats

**Shader Pipeline, Materials & Lighting**
- HLSL primary (DXC → SPIR-V), GLSL supported
- Permutation matrix generated at cook time; no runtime shader compilation in shipped builds
- Editor hot-reload via file-watch
- Data-driven material instances over shader templates (node-graph material editor: later phase)
- Clustered forward+, cascaded shadow maps, atlas spot/point shadows, IBL with split-sum
- Placeholder SSGI v1; ReSTIR or baked probes as future phase

**VFX & Particles**
- GPU-driven compute-based particle simulation
- Ribbon trails, decals, screen-space effects (bloom, DoF, motion blur, chromatic aberration, film grain)
- Data-driven authoring v1; node-graph VFX editor (shares vscript IR machinery) later

**Game AI (distinct from BLD)**
- Custom ECS-backed **Behavior Trees**
- **Recast/Detour** pathfinding
- Utility AI and GOAP as optional overlays
- BT files serialized as vscript IR — BLD can author them via `vscript.*` tools

**Cinematics & Sequencer**
- ImGui docked timeline editor — tracks for transform, property, event, camera, audio, subtitle
- `.gwseq` file format; same serialization machinery as scenes
- BLD `sequencer.*` tools for conversational cutscene authoring

**Mod Support**
- Mods ship as vscript IR + assets + optional signed native dylib
- vscript mods sandboxed with a reduced tool tier (no filesystem write, no network, no shell)
- Native mods require signing + per-mod user consent; disabled in competitive multiplayer
- BLD can author mods under the sandboxed tier
- Workshop integration via platform services

**Platform Services**
- `IPlatformServices` interface — achievements, leaderboards, cloud save, matchmaking, Workshop, DLC, presence, rich invites
- Backends rolled out progressively: Steam, EOS, GOG Galaxy; consoles when agreements exist
- Dev default: in-memory backend for offline development

**Anti-Cheat Boundary**
- Engine is anti-cheat-agnostic — provides integration points, not an AC
- Server-authoritative networking is the first line of defense
- SHA-256 content integrity hashes on cooked assets
- `IAntiCheat` interface for EAC / BattlEye / Denuvo; AC SDKs never land in the open repo

**Content Pipeline**
- Source assets in `/content/` (gitignored) — Blender, Substance, Photoshop, DAW projects
- Cooked assets in `/assets/` (tracked) — deterministic, content-hashed, incremental, parallel
- Importers: glTF 2.0, KTX2, Ozz, Ogg/Opus/WAV, XLIFF
- `gw_cook` CLI callable from editor, CI, and BLD
- **Cook determinism is a CI gate** — identical inputs must produce identical content hashes across OSes

**Testing Strategy**
- Unit: doctest (C++), `cargo test` (Rust)
- Integration: scene-based — `.gwscene` runs in sandbox with assertion scripts
- Perf regression: Tracy traces vs. per-PR baselines; CI fails on > 5% frame-time regression
- Smoke: every sample scene runs 60 seconds in CI
- Fuzzing: libFuzzer on asset parsers, scene deserializer, network packet handling
- Mutation testing: `cargo-mutants` on `bld-governance` and `bld-ffi`
- BLD eval harness: canned tasks with tool-call accuracy scoring; regressions block release

**Packaging & Release**
- CMake + CPack installer generation — MSIX (Windows), AppImage + DEB + RPM (Linux)
- Symbol servers: PDB (Windows), split DWARF (Linux)
- LTS branches cut quarterly; hotfix backports for 12 months
- SemVer for engine; titles version independently
- Authenticode + GPG signing
- Auto-generated changelog from PR labels; human-edited release notes for LTS cuts

---

## Chapter 7 — Engine Architecture

The engine is organized as concentric layers. Each layer may depend **only on layers below it**. This is enforced by CMake and by a custom `clang-tidy` check that forbids upward includes.

```
  ┌─────────────────────────────────────────────────────────┐
  │  Tools Layer  —  editor, sandbox, runtime, gameplay     │
  ├─────────────────────────────────────────────────────────┤
  │  Systems Layer  —  scene, assets, scripting             │
  ├─────────────────────────────────────────────────────────┤
  │  Render Layer  —  frame graph, HAL front-end            │
  ├─────────────────────────────────────────────────────────┤
  │  Render Back-end  —  Vulkan 1.3 (WebGPU planned)        │
  ├─────────────────────────────────────────────────────────┤
  │  Engine Core  —  ECS, math, memory, containers, logger, │
  │                  command bus (undo/redo)                │
  ├─────────────────────────────────────────────────────────┤
  │  Platform Layer  —  Windows / Linux abstraction         │
  └─────────────────────────────────────────────────────────┘

  ┌─────────────────────────────────────────────────────────┐
  │  BLD — Brewed Logic Directive (Rust crate, separate)    │
  │                                                         │
  │  bld-agent      → plan/act/verify, sessions, audit      │
  │  bld-tools      → #[bld_tool] proc macro, registry      │
  │  bld-mcp        → rmcp MCP server                       │
  │  bld-rag        → tree-sitter + candle + sqlite-vss     │
  │  bld-provider   → Claude cloud + alt-cloud + candle     │
  │  bld-governance → tier policy, HITL gates, secret filter│
  │  bld-ffi        → C-ABI boundary (opaque handles, POD)  │
  │                                                         │
  │  Linked into editor via Corrosion + cxx + cbindgen.     │
  │  Consumes engine state only via the C-ABI callback      │
  │  interface exposed by the editor's bld_bridge module.   │
  └─────────────────────────────────────────────────────────┘
```

### Platform Layer
The *only* place in the engine where `#include <windows.h>` or `<unistd.h>` is allowed. Everything above this line talks to the OS through a small, opinionated API. Windowing uses GLFW under the hood for now; that is an implementation detail behind the platform façade and can be replaced without touching the rest of the engine.

### Engine Core
Logger, assertions, errors, containers, strings, math, memory allocators, and the ECS. This is the layer that most other code in the engine spends its time calling. It has zero external dependencies beyond the C++ standard library and is the most heavily unit-tested module in the codebase.

### Render Back-end & HAL
The HAL is the vocabulary of a modern GPU, expressed in ~30 types: devices, queues, command buffers, pipelines, descriptor sets, synchronization primitives, swapchains, resources. The Vulkan 1.3 backend is the first implementation. A WebGPU backend is planned and the HAL is designed to accept it without front-end changes.

Above the HAL sits a **frame graph**: the front-end declares render passes and their resource reads/writes, and the frame graph inserts barriers, aliases transient resources, and schedules work. Gameplay code and the editor both render through the frame graph — there is no "editor-only" rendering path.

### Systems Layer

The Systems Layer is the broadest layer in the engine. It contains every subsystem that turns the core + renderer into a game-capable runtime. Every subsystem here is a concrete, predefined architectural choice — no open questions, no "TBD."

**Scene graph & assets**
- **Scene** is a view over the ECS — parent/child and transform traversal on top of entity data, but entities are still the underlying truth.
- **Assets** handles import, cook, and runtime load. Runtime code reads only the binary cooked format. JSON/glTF parsing lives in the cook step, not in the shipping runtime.
- **Scripting** is the dynamic-library loader for `gameplay_module`; it owns the hot-reload story.
- **vscript** runs the typed text IR of the visual-scripting system (Chapter 12).

**Simulation**
- **Physics** — Jolt Physics integration; rigid bodies, character controller, constraints, determinism mode.
- **Animation** — Ozz-animation + ACL compression; blend trees on ECS; FABRIK/CCD IK.
- **Game AI** — custom ECS-backed behavior trees; Recast/Detour pathfinding.

**Player-facing I/O**
- **Input** — SDL3 device layer + action-map system (rebindable, chord-capable, accessibility-conscious).
- **Audio** — miniaudio mixer + Steam Audio spatial + Opus/Vorbis/WAV decoders.
- **In-game UI** — RmlUi (deliberately not ImGui); HarfBuzz + FreeType; Vulkan backend.
- **Localization & accessibility** — ICU + HarfBuzz; binary string tables; WCAG 2.2 AA goal.

**Multiplayer & services**
- **Networking** — GameNetworkingSockets transport; custom ECS delta replication; lag compensation; voice chat.
- **Platform services** — `IPlatformServices` with Steam / EOS / Galaxy backends.
- **Anti-cheat boundary** — `IAntiCheat` interface; engine is AC-agnostic.

**Data & observability**
- **Persistence** — versioned binary save format + SQLite for local structured data + cloud-save abstraction.
- **Telemetry & crash reporting** — Sentry Native SDK + custom event pipeline; opt-in, GDPR-compliant.
- **Config & CVars** — typed registry; TOML user / binary runtime.
- **Dev console** — in-game overlay for debug builds only (separate from BLD).

**Authoring & content**
- **Cinematics/sequencer** — timeline runtime + editor tracks.
- **VFX** — GPU-driven particles, trails, decals, post-process.
- **Shader pipeline** — HLSL/GLSL authoring → DXC → SPIR-V; cooked permutation matrix.
- **Mod support** — vscript-IR + assets + signed-native-dylib package format; sandboxed tier for mods.
- **Content pipeline** — `gw_cook` CLI, deterministic cook, BLD-callable.

**Infrastructure**
- **Events** — type-safe pub/sub over ECS; zero-alloc in-frame bus.
- **Jobs** — custom fiber-based work-stealing scheduler.
- **Command bus** — undo/redo primitive shared by editor, BLD, and any authoring tool.

Every subsystem above has a concrete library choice or a custom implementation plan in Chapter 6 and in the companion blueprint (§3.9 through §3.30). Nothing is left to "we'll figure it out later."

### Tools Layer
The editor, sandbox, runtime, and gameplay module. These are the only executables and the only dynamic library. They consume the engine; the engine does not know they exist.

---

## Chapter 8 — Why a Decoupled, Native Editor

The editor is a native C++ executable that statically links `greywater_core` and renders with ImGui docking + viewports. This single decision eliminates an entire category of problems that plague engines built around managed-runtime tools:

- **No marshalling.** Editor code reads ECS data directly — no P/Invoke, no COM, no serialization across a process boundary.
- **One debugger.** A gameplay bug surfaced in the editor is debugged in the same LLDB/Visual Studio session as the runtime.
- **One build.** Changing an engine header rebuilds the editor. No "engine devs" and "tools devs" waiting on each other's binaries.
- **One language.** Any engine programmer can contribute to the editor. Any tools programmer can step into the renderer.
- **Serialization is engine-owned.** The editor does not "export" scenes — it writes the same `.gwscene` the runtime loads, byte-for-byte.

Native ImGui tooling is the right default for an engine editor in 2026 and will remain the right default for as long as we can foresee. It is mature, battle-tested, and does not require us to maintain our own UI framework.

---

## Chapter 9 — Repo & Licensing

The canonical repository is `git@github.com:SL1C3D-L4BS/cold-coffee-labs.git`. Branch protection is enforced on `main`: PR required, CI must pass, one approving review from an engineering lead.

Licensing is a **studio-internal** matter — Greywater_Engine is not open source. It is Cold Coffee Labs intellectual property. Contributor agreements are on file for every committer. Third-party dependencies brought in via CPM are audited for license compatibility before their pin lands in `cmake/dependencies.cmake`.

---

## Chapter 10 — What Happens Next

With this architecture signed off:

**Engine phases (1–18)**

- **Phase 1 — Scaffolding & CI/CD (4w).** Repo layout, `bld/` Rust workspace, Corrosion, CI green on both OSes. Hello-triangle + hello-BLD smoke test → *First Light*.
- **Phase 2 — Platform & Core Utilities (5w).** Windowing, memory allocators, logger, assertions, `Result`, core containers, UTF-8 strings, OS-header lint.
- **Phase 3 — Math, ECS, Jobs & Reflection (7w).** Vec/Mat/Quat, command bus, custom fiber-based scheduler, archetype ECS, reflection + serialization primitives → *Foundations Set*.
- **Phase 4 — Renderer HAL & Vulkan Bootstrap (8w).** HAL, Vulkan device/swapchain/command buffers, VMA, bindless descriptors, dynamic rendering, pipeline cache.
- **Phase 5 — Frame Graph & Reference Renderer (8w).** Frame graph with barrier insertion, DXC shader toolchain, forward+ lighting, cascaded shadows, IBL → *Foundation Renderer*.
- **Phase 6 — Asset Pipeline & Content Cook (4w).** `gw_cook` CLI, glTF/KTX2/Ozz import pipelines, deterministic incremental cook, virtual-mount filesystem.
- **Phase 7 — Editor Foundation (6w).** ImGui shell, docking + viewports + ImGuizmo, scene hierarchy, reflection-driven inspector, play-in-editor → *Editor v0.1*.
- **Phase 8 — Scene Serialization & Visual Scripting (5w).** `.gwscene` format, vscript IR + runtime, AOT cook, ImNodes node-graph projection over IR.
- **Phase 9 — BLD (Brewed Logic Directive, Rust, 12w).** Native AI copilot as a Rust crate linked via Corrosion: MCP server, `#[bld_tool]` proc macros, tree-sitter + candle RAG, build/runtime tools, governance, offline mode → *Brewed Logic*.
- **Phase 10 — Runtime I: Audio & Input (6w).** miniaudio + Steam Audio, SDL3 + action maps, accessibility defaults, cooked audio format.
- **Phase 11 — Runtime II: UI, Events, Config, Console (6w).** RmlUi + HarfBuzz, event bus, CVars, dev console → *Playable Runtime*.
- **Phase 12 — Physics (6w).** Jolt integration, character controller, constraints, deterministic mode, physics debug draw.
- **Phase 13 — Animation & Game AI Framework (6w).** Ozz + ACL, blend trees, FABRIK/CCD IK; custom BT + Recast/Detour pathfinding → *Living Scene*.
- **Phase 14 — Networking & Multiplayer (12w).** GameNetworkingSockets, ECS replication, lag compensation, Opus voice, deterministic lockstep → *Two Client Night*.
- **Phase 15 — Persistence & Telemetry (6w).** Versioned save format, SQLite, cloud-save abstraction, Sentry crash reporting, opt-in event pipeline.
- **Phase 16 — Platform Services, i18n & a11y (6w).** Steam + EOS backends, ICU + HarfBuzz localization, accessibility features → *Ship Ready*.
- **Phase 17 — Shader Pipeline Maturity, Materials & VFX (6w).** Shader permutation maturity, material system, GPU particles, post-FX.
- **Phase 18 — Cinematics & Mods (5w).** Timeline sequencer, mod sandbox, Workshop integration → *Studio Ready*.

***Sacrilege* / Blacklake phases (19–23)** — see `docs/README.md` and `docs/02_ROADMAP_AND_OPERATIONS.md` for week rows.

- **Phase 19 — Blacklake Core & Deterministic Genesis (8w).** Σ₀, HEC-style seeds, streaming, SDR + noise masks, WFC/constraint content → *Infinite Seed*.
- **Phase 20 — Arena Topology, GPTM & Sacrilege LOD (8w).** Large arena GPTM slice, GPU mesh, tessellation/LOD, floating origin, circle-themed materials → *Nine Circles Ground*.
- **Phase 21 — Sacrilege Frame — Rendering, Atmosphere, Audio (8w).** Horror-forward rendering, volumetrics, decals, dynamic score → *Hell Frame*.
- **Phase 22 — Martyrdom Combat & Player Stack (6w).** Sin/Mantra/Rapture/Ruin, Blasphemies, weapons, locomotion → *Martyrdom Online*.
- **Phase 23 — Damned Host, Circles Pipeline & Boss (6w).** Roster, encounters, authoring pipeline, boss phases → *God Machine RC*.

**Release phases (24–25)**

- **Phase 24 — Hardening & Release (8w).** Perf regression gates, libFuzzer, cargo-mutants, BLD eval harness, CPack + signing, anti-cheat boundary, LTS process, board-room demo → *Release Candidate*.
- **Phase 25 — LTS Sustenance (ongoing).** Quarterly LTS cuts, 12-month backport SLA, security advisories, dependency upgrades, deprecation policy.

**Total buildout: 162 weeks (~37 months)** from first commit to RC, plus ongoing LTS. The phases and their exit criteria live in [`grey-water-engine-blueprint.md`](grey-water-engine-blueprint.md). That document is the one stakeholders sign. This document is the one engineers read on day one.

---

## Chapter 11 — BLD: The Brewed Logic Directive

BLD is what makes Greywater_Engine different.

Every major engine in 2026 either has an AI feature bolted onto a mature editor, or a third-party plugin riding on reflection-based automation. None of them treat an agentic AI copilot as a core engine subsystem with compile-time-generated tool surfaces and undo-stack integration. BLD does — and the architectural consequences compound into a materially different developer experience.

### What BLD is

BLD is a **native, in-engine AI copilot**, implemented as a **Rust crate** (`bld/`) and linked into the editor through a narrow C-ABI boundary. The rest of the engine — core, editor, sandbox, runtime, gameplay module — stays C++23. BLD lives behind a docked chat panel in the editor, but that panel is just one consumer. BLD itself is a Model Context Protocol server. Any compliant client can drive it — Claude Code from a terminal, Cursor in a separate window, a CI automation script, an in-game debug console in a shipping debug build. The editor panel is one implementation.

The copilot has **authoritative tool-use over the engine's public surface**. It can create entities, edit components, author shaders, run builds, read profiler captures, diagnose crashes, answer documentation questions, patch gameplay code, author visual scripts at the IR level, and verify its own work against the running runtime. Every destructive action is gated by a human-in-the-loop approval primitive built into MCP itself (`elicitation`). Every mutating action is an `ICommand` on the editor's undo stack — indistinguishable from a user's click for Ctrl+Z purposes.

### Why Rust — but only for BLD

This is a deliberate, scoped choice. The engine core is **C++23** for all the usual reasons (template metaprogramming across editor↔core, mature graphics ecosystem, hot-reload story, AAA shipping precedent, hiring pool). Rust is used **only for BLD**, because four specific advantages land exactly in this subsystem:

1. **`#[bld_tool]` proc macros beat libclang codegen.** A tagged Rust function automatically emits its MCP schema, JSON decoder, dispatch handler, and registry entry at compile time. No external codegen tool, no CMake integration pass, no header-parsing edge cases, no invalidation bookkeeping.
2. **`rmcp` (the official Rust MCP SDK) is ahead of any C++ MCP SDK.** If BLD is our differentiator, we want the better protocol substrate.
3. **The Rust ML ecosystem leads for our use case.** `candle` (pure-Rust ML runtime), `llama-cpp-rs`, and the Rust HTTP/SSE clients that let us talk to cloud LLM APIs are all more mature than the C++ equivalents we would otherwise piece together from libcurl and simdjson.
4. **`tree-sitter` has parity** in both languages — no loss moving RAG into Rust.

The Rust↔C++ boundary runs through a single narrow C-ABI (generated by `cbindgen`, consumed via `cxx`). Only opaque handles, POD messages, and function-pointer callbacks cross. No Rust generics, no trait objects, no `Result`/`Option`, no C++ templates or exceptions. This narrowness is deliberate — it bounds the refactoring tax that every Rust/C++ FFI project eventually pays.

**Why not Rust for the whole engine?** Because the costs compound where the boundary runs through hot paths. For a full Rust-core + C++-editor split, editor↔core calls (per-frame scene reads, inspector queries, preview rendering) hit the FFI continuously, and you design your APIs around what `cxx` can express rather than what the language can express. BLD doesn't have this problem — agent tool calls are low-frequency and already serialized (MCP is JSON at the wire level), so the FFI cost is bounded by design. Rust earns its keep in this subsystem; it would cost more than it pays back elsewhere.

### The seven commitments that make it real

1. **The engine's public surface is the agent's tool surface.** Schemas are generated at compile time by `#[bld_tool]` proc macros. No hand-written JSON, no translation-layer drift. Each tool either implements itself entirely in Rust or calls back into the editor via a C-ABI callback.
2. **The undo stack is the agent's commit log.** Mutating tools submit opcode + payload messages to the C++ command bus via C-ABI callback. The editor enqueues them as `ICommand` instances. Ctrl+Z works identically on agent moves and user moves. Transaction brackets collapse multi-step actions into a single undo.
3. **MCP is the wire from day one.** `rmcp` hosts the server (spec 2025-11-25). Editor panel, Claude Code, Cursor, Zed, and CI automation are all clients over the same protocol.
4. **BLD is embeddable, not editor-only.** The Rust static library can link into sandbox, runtime debug builds, or CI tools — anywhere that wants an embedded agent.
5. **HITL approval is protocol-native.** MCP's `elicitation` primitive drives the approval UX; any compliant client renders it correctly.
6. **Tool surface auto-synchronizes with the engine.** Adding a `#[bld_tool]` function rebuilds the tool registry on `cargo build`.
7. **The agent stress-tests the exposed engine API every day.** If BLD cannot accomplish a task, the C-ABI callback surface is missing an entry — the engine's exposed API is incomplete. A deliberate forcing function on API design.

### How it sits in the architecture

BLD is eight Rust crates under `bld/` — `bld-mcp`, `bld-tools`, `bld-provider`, `bld-rag`, `bld-agent`, `bld-governance`, `bld-bridge`, `bld-ffi` — built by Cargo as a workspace and imported into CMake via Corrosion as a single static library. The editor contains a small `bld_bridge/` module that holds the `cxx` bindings, wraps the C-ABI handle types in C++ RAII, and adapts BLD's command submissions to the editor's `CommandStack`.

The RAG layer (`bld-rag`) indexes two corpora: the engine's source and docs (shipped pre-built) and the user's project (built incrementally on filesystem-watch events). `tree-sitter` provides AST-aware chunking; `petgraph` builds the symbol graph and runs a structural-importance ranking. Embeddings default to `nomic-embed-code` via `candle` — offline, no API keys required on first run. Storage is SQLite + sqlite-vss via `rusqlite`. No server process.

LLM providers are pluggable behind an `ILLMProvider` trait. Claude Opus 4.6+ is the default for cloud work because it leads on agentic tool-use for our workloads. Alternate cloud LLMs configurable via `ILLMProvider`. For offline work, `candle` (or `llama-cpp-rs` as fallback) drives a local model with a GBNF grammar derived from the tool registry's schemas at startup — small local models emit structurally valid tool calls. Offline mode shows an explicit "reduced capability" banner; we never silently degrade.

Governance (`bld-governance`) is a three-tier permission system. Read-tier tools auto-approve. Mutate-tier tools gate on the MCP elicitation primitive. Execute-tier tools prompt on every call. Every tool invocation is appended to a JSONL audit log; `gw_agent_replay` can re-run a session against a clean checkout for QA and incident review.

Secret handling is load-bearing. Every file-reading tool has a compile-time-constant exclusion list — `.env`, `*.key`, OS-keychain entries — that cannot be bypassed. API keys are read via the `keyring` crate only and never enter model context. A pre-commit hook can be configured to block any commit containing audit-log evidence of a secret-path access.

### Why this changes the developer experience

Because the tool surface is the public API:
- A gameplay programmer asks BLD "what entities have a `RigidBody` but no `Collider`?" and gets an answer sourced from the same `World::query` they would call themselves.
- A tools programmer right-clicks a compile error and BLD patches the gameplay module, triggers a hot-reload, and verifies the fix — using the same `build.*` and `runtime.*` tools any developer has access to.
- A designer asks BLD to "make this projectile bounce three times"; BLD writes to the vscript IR, and the node graph view updates as a projection.
- A QA engineer writes an MCP-driven test script that drives the editor through a complex authoring scenario using the same primitives an interactive session would use.
- A modder on a shipped title uses an embedded BLD panel in the game's debug build to author new content, because the agent is a linkable static library — not locked to the editor.

BLD is not a feature added to an engine. It is an engine designed with the assumption that an AI agent is a first-class user of every API we ship. That assumption is load-bearing from the first commit in Phase 1.

---

## Chapter 12 — Visual Scripting: Text IR as Ground-Truth

Visual scripting in Greywater_Engine is **text-first, AI-native, and graph-visualized**. This is a deliberate departure from the Blueprint convention.

### The rule

A typed text IR is the serialized source of truth. The node graph is a reversible projection with a sidecar layout file. BLD reads and writes the IR directly.

### Why not Blueprint-style (graph-as-ground-truth)

The dominant visual-scripting systems made the node graph itself the canonical serialized form. That decision pays dividends in the editor (direct manipulation, no translation) and costs in every other dimension:
- Diffs are meaningless — binary graph blobs don't merge.
- Source-control conflicts are unresolvable without a specialized merge tool.
- Text-based code assistance (and AI in particular) can't operate on the representation.
- "Nativization" (compiling Blueprint to C++) becomes a necessary escape hatch that adds a parallel representation.

We avoid all of that by inverting the relationship. The IR is the file. The graph is a view.

### How it works

- **The IR** is a typed textual representation — expressions with known types, execution edges, stable-ID references to engine types and component members. Compact and diff-friendly. (Concrete DSL chosen during Phase 4 kickoff; leading candidates are a custom MIR-shaped DSL or an annotated Rhai subset.)
- **The node graph** is drawn with ImNodes on top of a live IR. Opening the IR regenerates a graph. Re-saving writes the IR plus a `.layout.json` sidecar that remembers node positions. Node positions are **never** part of the IR file.
- **The `vscript/` runtime** (C++, engine-core) interprets the IR directly in the editor for fast iteration, and AOT-cooks it to bytecode at asset-cook time for the runtime.
- **BLD's `vscript.*` tools** operate on the IR text. A designer can author by drag-and-drop in the graph view, by chat ("make this projectile bounce three times"), or by typing IR directly. All three produce identical artifacts.
- **Undo/redo happens at the IR level**, integrated with the editor command bus — the same undo stack that scene edits and BLD actions use.

This is the architectural pay-off of treating AI as a first-class user: the script representation that is easiest for the agent to manipulate turns out to be the one that is easiest for humans to diff, merge, and reason about. We get both by choosing well at the design stage.

---

---

## Chapter 13 — The Editor UI Architecture

The editor is the face of the engine. Everyone who ships a title on Greywater spends more time in the editor than in the game. The editor UI is not a skin on top of functionality — it is the functionality, presented.

This chapter defines that presentation end-to-end: the visual system, the layout, the panels, the interaction patterns, the performance budget, and the explicit commitments and refusals that shape each. It applies to the entire `editor/` target.

### 13.1 The reference layout

The default workspace, rendered as ASCII. Anyone reading this should be able to recognise every region at a glance.

```
┌──────────────────────────────────────────────────────────────────────────────────┐
│ ≡ File  Edit  View  Go  Help                              Workspace: Scene ▾ ⌘K │
├─────────────────────────┬─────────────────────────────────────┬──────────────────┤
│ Scene Hierarchy         │                                     │ Inspector        │
│ ┌───────────────────┐   │                                     │ ┌──────────────┐ │
│ │ ▾ World           │   │                                     │ │ 〔Entity〕    │ │
│ │   ▸ Player        │   │                                     │ │ Transform    │ │
│ │   ▸ Terrain       │   │           3 D   V I E W P O R T     │ │ Mesh         │ │
│ │   ▸ Atmosphere    │   │          (multi-viewport capable)   │ │ RigidBody    │ │
│ │   ▸ Lights        │   │                                     │ │ AudioSource  │ │
│ │   ▸ Cameras       │   │                                     │ └──────────────┘ │
│ └───────────────────┘   │                                     │                  │
│                         │                                     │ Render Passes    │
│ Asset Browser           │                                     │ ┌──────────────┐ │
│ ┌──────┬──────┬──────┐  │                                     │ │ Forward+     │ │
│ │ ▢    │ ▢    │ ▢    │  │                                     │ │ CSM 4-cascade│ │
│ │ mesh │ tex  │ mat  │  │                                     │ │ IBL          │ │
│ └──────┴──────┴──────┘  │                                     │ │ Tonemap      │ │
│                         │                                     │ └──────────────┘ │
├─────────────────────────┴─────────────────────────────────────┴──────────────────┤
│  BLD │ Console │ Profiler │ Render Debug │ Stats │ Config │ Timeline │ Command ▼ │
│                                                                                  │
│  > BLD: how do I make this light flicker with the ship hum?                      │
│    BLD: proposing diff — gameplay/src/ship/engine_hum.cpp (+12 / -3)             │
│         [view diff] [approve] [reject]                                           │
│                                                                                  │
├──────────────────────────────────────────────────────────────────────────────────┤
│ Phase 20 · Week 128 · frame 16.2 ms · GPU 7.1 ms · RX 580 · 1920×1080  ●●●       │
└──────────────────────────────────────────────────────────────────────────────────┘
```

The dockspace is a single ImGui root dockspace with the multi-viewport branch enabled so panels can be detached to a second monitor without losing docking semantics.

### 13.2 The palette, in full

Six principal colours, fixed. Every widget, every panel, every bit of chrome picks from this palette.

| Token      | Hex       | Primary role                                                 |
| ---------- | --------- | ------------------------------------------------------------ |
| Obsidian   | `#0B1215` | Main workspace background; the void the editor sits in       |
| Bean       | `#2A1F1A` | Deepest nested-panel background; well depth                  |
| Greywater  | `#6B7780` | Dividers, disabled text, inactive borders, meta labels       |
| Crema      | `#E8DDC8` | Primary text on dark                                         |
| Brew       | `#5D4037` | Warm accent; branding, non-interactive highlights            |
| Signal     | `#5865F2` | Interactive: focus ring, selection, hyperlink, active tab    |

Semantic layer on top (earned, not decorated):

| Token      | Hex       | Use                                                          |
| ---------- | --------- | ------------------------------------------------------------ |
| Success    | `#3E9A5C` | Completion, valid state, "passed"                            |
| Warn       | `#C29243` | Caution, stale state, non-blocking issue                     |
| Error      | `#C34A4A` | Failure, invalid state, blocking                             |
| Info       | `#4A8FC3` | Neutral notification                                         |

**Every text-on-background pair meets WCAG 2.2 AA contrast.** Text on panel backgrounds, selection text on Signal, label text on Greywater divider — all measured, all pass. A 1080p screenshot with the colour-picker dropped on any element must report a ratio ≥ 4.5 : 1 for body text and ≥ 3 : 1 for large text / non-text interactive elements.

### 13.3 Typography

**Two typefaces, both ship with the editor** (loaded from `editor/ui/fonts/` via `ImFontAtlas::AddFontFromFileTTF`):

- **Sans** — a humanist sans in the Inter / IBM Plex Sans family. Used for labels, headers, panel titles, body prose.
- **Mono** — a coding monospace in the JetBrains Mono / Fira Code family. Used for numeric data in inspectors, console output, code snippets, asset paths, hex colour values.

**Scale** (pixel sizes, fixed): 10 · 11 · 12 · 13 · 14 · 16 · 20 · 24. Nothing outside this scale is permitted. Panel body is 11, panel title is 13, section header in a dialog is 16, large dialog header is 20, toast / breaking-news banner is 24.

**Weights:** 400 (regular, 95 % of text) · 500 (medium, panel titles and section labels) · 600 (bold, reserved for warnings and the command-palette leading keyword). No italic except for filesystem paths, asset type names, and literal-text code.

**Line height:** 1.40 × for body prose; 1.20 × for dense data tables; 1.10 × for console-style mono dumps where vertical density matters.

**Letter-spacing:** +1 % on all-caps labels (there are few of these). Default everywhere else.

The atlas is built once at editor startup and cached. Hot-reload swaps the atlas when the TTF file changes in debug builds — useful when iterating on typography.

### 13.4 Spacing

The entire UI lives on a **4-pixel grid**. Values derived from the grid, in order of frequency:

- **4 px** — dense control gap (adjacent inputs in a row).
- **8 px** — standard widget gap (labels to inputs, rows in a table).
- **12 px** — section separator (between unrelated widget groups within a panel).
- **16 px** — panel padding (between panel border and first content).
- **24 px** — dialog padding / large section break.

Off-grid values are a review-blocking smell. If `style.ItemSpacing.x` gets set to 6 px anywhere, the PR gets rejected.

### 13.5 Panel catalogue

The editor ships with the following panels. Each is a registered subsystem with a unique ID, a title, a category, a default dock position, and a set of shortcuts.

**Core (always available)**
- **Scene Hierarchy** — entity tree. Search, drag-to-reparent, right-click menu, keyboard nav. Multi-select.
- **Inspector** — reflection-driven component editor for the current selection. Sections collapse; values support undo/redo.
- **Asset Browser** — grid of thumbnails + tree of folders. Search, tag filter, drag to viewport or inspector slot.
- **Viewport** — 3D scene. Supports up to four split viewports (quad-view). Gizmos via ImGuizmo. Camera modes: orbit, fly, top-down, first-person.
- **BLD Chat** — docked by default in the bottom tab group. Streaming token output, slash commands, diff approvals, session tabs.
- **Console** — dev console (debug builds only). Command prompt with fuzzy completion; log tail with filter-by-level and filter-by-subsystem.
- **Command Palette** — Ctrl+K overlay. Fuzzy-searches every registered command across every subsystem.

**Diagnostic**
- **Profiler** — Tracy-style flame graph. CPU lanes, GPU lanes. Frame scrubber. Zone search.
- **Render Debug** — G-buffer visualiser (base colour, normals, roughness, metalness, depth, emission), light grid overlay, shadow cascade view, frame-graph inspector. This is the successor to the bottom-bar RT strip in the early prototype — same concept, better structured (collapsible, named, scrollable).
- **Stats** — real-time frame time, FPS, draw calls, triangles, GPU memory, CPU memory, job-system utilisation, network round-trip.
- **Config / CVars** — searchable CVar list with type-aware editors (bool, int, float slider, enum dropdown, string).
- **ECS Inspector** — archetype browser, entity count per archetype, system execution graph, per-system timing.
- **Log** — structured log viewer with level filter, subsystem filter, regex search, timestamp navigation.

**Authoring**
- **Material Editor** — opens when a material asset is selected in the asset browser. Node-graph editor with live preview.
- **VScript Node Graph** — ImNodes projection over the vscript IR. Save writes the IR; open re-builds the graph from the IR.
- **Timeline / Sequencer** — ImGui docked timeline with transform/property/event/camera/audio/subtitle tracks.
- **Shader Editor** — text editor for HLSL/GLSL with hot-reload. Syntax highlighting; error annotations inline.
- **Texture Inspector** — opens on texture selection. Channel isolation (R/G/B/A), mip-level slider, compression preview.
- **Animation Editor** — skeletal pose, blend-tree graph, curves.

**Blacklake / `engine/world` tooling** *(full engine capability; *Sacrilege* production may prioritize a subset)*
- **Universe Browser** — hierarchical view of cluster → galaxy → system → planet (when Blacklake runs at universe scale). Click a body to load / preview.
- **Arena / Circle Authoring** — seed, circle theme, GPTM slice parameters, encounter markup hooks (flagship pipeline).
- **Planet Inspector** — planet-type selector, atmosphere parameters, biome weights, terrain noise layers, ocean level, seed override; live preview.
- **Atmospheric Parameter Editor** — per-scene atmosphere definition (Rayleigh, Mie, ozone, sun colour, scattering coefficients).
- **Orbital Trajectory Preview** — when a body is selected, renders predicted trajectory in the viewport for the next N minutes of sim time.
- **Chunk Visualizer** — debug view of chunks in view: LOD, generation state, memory footprint, cache hit/miss.
- **Structural Integrity Debug** — visualises integrity graph for building pieces (if survival/building modules active).
- **Population / faction views** — optional; for ecosystem/utility-AI debugging when those modules are enabled.

### 13.6 Workspaces

Four built-in workspaces, switchable via menu or Ctrl+1–4. Each loads a dockspace preset and a selection of visible panels.

| Shortcut | Workspace        | Primary panels visible                                                                    |
| -------- | ---------------- | ----------------------------------------------------------------------------------------- |
| Ctrl+1   | **Scene**        | Hierarchy, Inspector, Asset Browser, Viewport, BLD Chat                                   |
| Ctrl+2   | **Materials**    | Asset Browser, Material Editor, Texture Inspector, Shader Editor, Viewport, BLD Chat      |
| Ctrl+3   | **Profiling**    | Profiler, Stats, Render Debug, ECS Inspector, Viewport                                    |
| Ctrl+4   | **Debug**        | Log, Console, Config/CVars, Render Debug, Viewport                                        |

Users can define their own workspace presets via Workspace → Save Current… The preset is stored as `.greywater/editor/workspaces/<name>.layout.ini`. Studio-wide workspaces ship in `editor/ui/workspaces/`.

### 13.7 Interaction patterns

**Command palette (Ctrl+K).** Fuzzy search over every registered command in the editor. Commands come from every subsystem: file operations, selection operations, BLD actions, build commands, workspace switches, panel toggles, camera controls. The palette is the keyboard-first answer to "I know the thing exists, I don't know the menu."

**Keyboard shortcuts.** Ctrl+Z / Ctrl+Shift+Z for undo/redo (wired to the command bus). Ctrl+S to save the scene. Ctrl+B to build. Ctrl+P to play in editor. Ctrl+` to toggle dev console. F11 to toggle immersive (fullscreen viewport) mode. F to frame selected. G / R / S to translate / rotate / scale. All shortcuts are discoverable via the menu and the command palette.

**Drag and drop.** Asset thumbnails drag to viewport (places in scene), to inspector slots (assigns), to material slots (binds). Entities drag to reparent within the hierarchy. Colour swatches drag between colour fields.

**Context menus.** Right-click on any selectable element opens a context menu. **Every context menu includes an "Ask BLD about this" item** that sends the selection to BLD with structured context. Right-click on a compile error, a profiler spike, a failing assertion — all route to BLD.

**Multi-select.** Ctrl-click / Shift-click for multi-select in every list-like view (hierarchy, asset browser, log). Inspector shows shared-properties-only for multi-select; changes apply to all selected.

**Undo/redo.** Wired to the editor's command bus. Every mutating action is an `ICommand`. BLD's actions go through the same stack; Ctrl+Z reverts them identically.

### 13.8 Data visualisation standards

We ship custom widgets for the data shapes an engine editor needs. Living in `editor/ui/widgets/`.

- **Drag-float-matrix** — 4 × 4 transform matrix with drag-to-edit per cell, plus a decomposed view (position / rotation-as-Euler / scale). The decomposed view is editable; the matrix updates live.
- **Curve editor** — for tonemap curves, animation curves, easing. Bezier control points, interpolation preview.
- **Histogram** — rolling real-time histogram (the luminance histogram from the early prototype is the canonical example). Anti-aliased rendering; auto-scaled axes.
- **Colour picker** — HSV wheel + RGB sliders + hex entry + eyedropper. Invokable as a modal from any colour field via the little swatch button. Not a persistent panel — it opens, does its job, closes.
- **Vector editors** — Vec2 / Vec3 / Vec4 with optional lock/link between axes (for uniform scale, etc.).
- **Flame graph** — hierarchical timing breakdown for the Profiler panel.
- **G-buffer viewer** — grid of render-target thumbnails with channel isolation, tone-map toggle, click-to-enlarge.
- **Node graph widget** — wraps ImNodes for both the material editor and the vscript panel.
- **Selectable tree** — for the hierarchy, asset folder tree, etc. With expand/collapse, drag/drop, multi-select, search-highlight.

### 13.9 BLD integration surface

BLD is docked. Default position is the bottom tab group, sharing space with Console, Profiler, Log. Users can pin it to a different dock location in their workspace.

Beyond the chat panel itself, BLD reaches into every other panel through right-click "Ask BLD about this" context menu items:

- Right-click an entity in the hierarchy → BLD gets the entity ID + serialized component data.
- Right-click a compile error in the Log → BLD gets the diagnostic + the offending file region.
- Right-click a profiler spike → BLD gets the frame capture.
- Right-click a texture in the asset browser → BLD gets the texture's metadata + preview.
- Right-click a shader in the shader editor → BLD gets the shader source and the cook error.

These context menus are registered by each panel via `gw::editor::ui::register_context_action(panel_id, predicate, prompt_template)`.

Approval dialogs for BLD mutations use a **diff viewer**: unified diff for code, tree diff for scenes, thumbnail before/after for assets.

### 13.10 Performance budget

The UI is a subsystem and obeys the same discipline as the rest of the engine.

| Budget item                                        | Target                                                       |
| -------------------------------------------------- | ------------------------------------------------------------ |
| UI rendering per frame @ 1080p on RX 580           | ≤ 2 ms                                                       |
| Default workspace total frame cost (RX 580, 1080p) | ≤ 16.6 ms (60 FPS)                                           |
| Panel update latency on state change               | < 1 frame                                                    |
| Command palette time to first result               | < 50 ms for a query of a 500-command registry                |
| Asset-browser thumbnail generation                 | Deferred; never on the frame that requests it                |
| Live-histogram rebuild (e.g., luminance)           | GPU compute pass, < 0.5 ms                                   |
| BLD streaming token insertion into chat            | Per-chunk parse only at 5 Hz max; render every frame is text |

A panel that drops frames to render itself is broken. Perf regression on the default workspace gates CI (see `docs/02_ROADMAP_AND_OPERATIONS.md` exit criteria for Phase 7).

### 13.11 Accessibility

All WCAG 2.2 AA commitments apply to the editor as well as to the in-game HUD.

- Text scaling: 80 % – 200 %, live-switchable without restart.
- Colour contrast: every text / background pair measured; none under threshold.
- Keyboard navigation: every action reachable without the mouse.
- Screen-reader hooks: panel titles, panel contents, and inspector values exposed via the platform accessibility API on supported OSes.
- Motion reduction: respects the OS setting; all animations shorten to 0 ms.
- Focus indicator: 2 px Signal ring with 4 px offset, never hidden.

### 13.12 What we explicitly refuse

The following are not features of the Greywater editor UI. If any of them is proposed, the proposal is rejected at review — they are not "we haven't gotten to it yet," they are "never."

- **Skeuomorphic styling** — no gradients, no drop shadows, no bevels, no chrome, no glass, no neon glow.
- **Per-panel custom themes** — one editor, one palette. Panels do not get their own colour scheme.
- **Animated chrome** — animated panel borders, breathing highlights, pulsing focus rings, aurora gradients. No.
- **Tutorial overlays and tooltip marketing** — no "tip of the day," no first-run carousel, no sparkles-and-confetti microinteractions.
- **Font choices per panel** — two fonts in the entire editor. Ever.
- **Custom window chrome** — OS window chrome is what the OS provides. We do not draw our own title bars and minimise buttons.
- **Hidden gestures without keyboard equivalents** — every gesture has a keyboard shortcut. Mouse-only discovery is forbidden.
- **Third-party UI frameworks** — ImGui + ImNodes + ImGuizmo + imgui_md. The stack is locked.

### 13.13 Module structure

```
editor/
├── ui/
│   ├── core/            # dockspace, style, fonts, theme application
│   ├── panels/          # every panel in §13.5, one file per panel
│   ├── widgets/         # custom widgets (§13.8)
│   ├── command_palette/ # palette registry + fuzzy search + invocation
│   ├── workspaces/      # built-in presets + persistence
│   ├── fonts/           # shipped TTFs (sans + mono)
│   └── theme/           # palette tokens + semantic mappings
├── panels/              # alias for ui/panels (imported by workspace presets)
├── bld_bridge/          # C++ side of the BLD Rust crate (separate from UI)
└── main.cpp             # the editor entry point
```

Each panel is a class implementing a tiny interface:

```cpp
struct IEditorPanel {
    virtual ~IEditorPanel() = default;

    virtual PanelId id() const noexcept = 0;
    virtual std::string_view title() const noexcept = 0;
    virtual PanelCategory category() const noexcept = 0;

    virtual void on_attach() {}
    virtual void on_detach() {}

    virtual void tick(f32 dt) {}
    virtual void render_ui() = 0;    // ImGui render only — no GPU state mutation
};
```

Panels are registered at editor init through a generated registry (macros + source-of-truth list in `editor/ui/panels/registry.hpp`). Adding a new panel is a single file + a single registry entry.

---

*Greywater × Cold Coffee Labs.*
*C++ engine. Rust agent. ImGui surface. IR-native scripting. Restrained, deliberate UI.*
*Owned end-to-end. Built on purpose. Brewed with BLD.*