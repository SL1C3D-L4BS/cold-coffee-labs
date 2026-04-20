# Greywater — Architectural Blueprint & Phased Execution Plan

| Field              | Value                                                   |
| ------------------ | ------------------------------------------------------- |
| **Project**        | Greywater (engine + game)                               |
| **Engine**         | Greywater_Engine                                        |
| **Game**           | Greywater                                               |
| **Hardware baseline** | AMD Radeon RX 580 8GB @ 1080p / 60 FPS              |
| **Art direction**  | Realistic low-poly                                      |
| **Document Owner** | Principal Engine Architect, Cold Coffee Labs            |
| **Audience**       | Executive Stakeholders, Engineering Leads, Technical Ops |
| **Status**         | **DRAFT — Pending Stakeholder Sign-Off**                |
| **Version**        | 0.2                                                     |
| **Date**           | 2026-04-19                                              |
| **Supersedes**     | All prior informal engine proposals                     |

---

## 1. Executive Summary

Cold Coffee Labs is chartering **Greywater** — a two-part program comprising a proprietary game engine (`Greywater_Engine`) and its flagship title (`Greywater`, a persistent procedural universe simulation). The engine is **purpose-built** for the game, targets the **AMD Radeon RX 580 8GB at 1080p / 60 FPS** as its baseline hardware, and ships with a **realistic low-poly** art direction. The engine maintains a clean engine/game code separation so that future Cold Coffee Labs titles can also ship on it.

Greywater is not an attempt to compete feature-for-feature with the large commercial engines. It is a focused, **data-oriented, tool-agnostic, hardware-accessible** technology stack designed to give Cold Coffee Labs the following durable advantages:

1. **Engineering sovereignty.** Every byte of the runtime is ours. No per-seat licenses, no royalty ceilings, no vendor-imposed roadmaps.
2. **Deterministic, reproducible builds** on every developer machine and CI runner — identical toolchain on Windows and Linux, enforced by policy.
3. **Performance headroom** through Data-Oriented Design (DOD) and a native Entity Component System (ECS) at the core. Our cache-line layout is a deliberate decision, not an accident of object graphs.
4. **A single-language, single-process editor experience.** No managed-runtime bridges, no inter-language marshalling, no "editor team" and "engine team" drifting apart.
5. **Hardware-accessible GPU strategy** — a thin Hardware Abstraction Layer (HAL) over **Vulkan 1.2 baseline with 1.3 features opportunistic**, targeting the RX 580 Polaris architecture at 1080p / 60 FPS. Ray tracing, mesh shaders, and GPU work graphs are **hardware-gated** (Tier G, post-v1). A WebGPU backend slots in for web and sandboxed targets, post-v1.
6. **Brewed Logic Directive (BLD)** — a first-class, native AI copilot agent baked into the engine as a **Rust crate**, linked into the editor via a narrow C-ABI boundary. Chat-driven, MCP-protocol, with authoritative tool-use over the engine's public surface. BLD is **not an editor plugin** — it is an engine subsystem. Rust is chosen **only for BLD** (not the engine core) because the Rust ecosystem decisively wins on three things that matter to the agent: proc-macro-generated tool surfaces (`#[bld_tool]`) beat libclang codegen outright, `rmcp` (the official Rust MCP SDK) is ahead of any C++ MCP SDK, and the Rust ML ecosystem (`candle`, `llama-cpp-rs`, `async-openai`) leads on embeddings and inference. **This is our primary competitive differentiator.** No other engine in 2026 treats an agentic AI copilot as a core subsystem with undo-stack integration and compile-time-generated tool surfaces.

This document is the blueprint for Phase 1 through Phase 24 of the Greywater construction (plus the ongoing LTS Sustenance phase, Phase 25). Phases 1–18 build the engine and its general-purpose subsystems. Phases 19–23 build the Greywater-game-specific subsystems. Phase 24 hardens and ships. Approval of this document authorizes the team to scaffold the repository, stand up CI/CD, and begin implementation against the milestones in §5.

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
- **Module:** `engine/vfx/`.

### 3.23 Game AI (distinct from BLD)

- **Behavior Trees:** custom, ECS-backed. No external dep. Editor support in Phase 10.
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

The editor UI is the face the engineer sees every day. It is engineered with the same rigor as every other subsystem. Full narrative treatment lives in `docs/architecture/greywater-engine-core-architecture.md` Chapter 13; this section is the formal spec for the build.

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

**Panels that must ship by the *Editor v0.1* milestone (Phase 7, week 42):** Scene Hierarchy, Inspector, Asset Browser, Viewport, Console, Log, Stats. All others (BLD Chat, Profiler, Render Debug, Material Editor, VScript Node Graph, Timeline, Shader Editor, Texture Inspector, Animation Editor, Config/CVars, ECS Inspector, and all eight Greywater-game panels) ship in their owning phase; most are integrated by the end of Phase 10 or Phase 18.

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

Phases 1–18 build the engine and its general-purpose subsystems. Phases 19–23 build the Greywater-game-specific subsystems (universe generation, planetary rendering, atmosphere/space, survival, ecosystem/factions). Phase 24 hardens and ships. Phase 25 sustains.

The operational week-by-week schedule lives in `docs/05_ROADMAP_AND_MILESTONES.md`. This section defines deliverables and exit criteria at the blueprint level.

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

**Deliverables**
1. Shader-pipeline maturity — cooked permutation matrix; no runtime shader compilation in shipped builds.
2. Material system — data-driven instances over shader templates; editor material browser with hot-reload.
3. VFX — GPU compute particle system; ribbon trails; decals; screen-space post (bloom, DoF, motion blur, CA, grain).

**Exit Criteria** — a technical artist ships a hero particle effect in under one day; shader permutations cook deterministically; editor material-browser hot-reload works end-to-end on both OSes.

### Phase 18 — Cinematics & Mods · 5 weeks · Tools & Content Lead

**Deliverables**
1. Cinematics — timeline runtime with transform/property/event/camera/audio/subtitle tracks; ImGui docked timeline editor; `.gwseq` format.
2. Mod support — package format (vscript IR + assets + optional signed native dylib); sandboxed vscript tool tier; native-mod consent flow; Workshop integration behind `IPlatformServices`.

**Exit Criteria (*Studio Ready*)** — a designer-authored cutscene plays identically in editor and runtime with localized subtitles; a mod installs from the Workshop flow, runs sandboxed, authored via BLD.

---

### Greywater-game-specific phases (19–23)

Phases 19–23 build the subsystems specific to the Greywater game. They consume the engine's public API and live under `engine/world/` (engine-side capabilities the game uses). Details of the algorithms live in `docs/games/greywater/12_*` through `20_*`.

### Phase 19 — Greywater Universe Generation · 8 weeks · World Systems Lead

**Deliverables**
1. `engine/world/universe/` — 256-bit seed, coordinate model, seed-derivation chain (cluster → galaxy → system → planet).
2. `engine/world/streaming/` — 32 m chunk streaming with LRU cache and time-sliced generation on the job system.
3. Noise composition — Perlin, Simplex, Worley, domain warping, fBm — all GPU compute.
4. `engine/world/biome/` — biome-gradient maps (temperature + humidity + soil).
5. `engine/world/resources/` — procedural placement with geological rules.
6. Wave-function-collapse for structured procedural content (ruined cities, stations).

**Exit Criteria (*Infinite Seed*)** — same universe seed + same coordinate produces byte-identical chunk content on Windows and Linux. 1 000 randomly-sampled coordinates pass the determinism gate. Chunk streaming under stress maintains the frame-time budget on RX 580.

### Phase 20 — Greywater Planetary Rendering · 8 weeks · World Systems Lead

**Deliverables**
1. `engine/world/planet/` — quad-tree cube-sphere subdivision with LOD metric.
2. GPU compute terrain-mesh generation per chunk.
3. Hardware tessellation for close-range terrain.
4. `engine/world/origin/` — floating-origin recenter + event propagation for subsystems that cache world positions.
5. `engine/world/biome/` — terrain shader with biome-blend weights driving albedo, normal, and vegetation.
6. `engine/world/vegetation/` — GPU instancing + 4 LOD + indirect draw.
7. Ocean rendering — ocean sphere + FFT waves (gated on compute budget per reference hardware).

**Exit Criteria (*First Planet*)** — an Earth-class procedural planet renders at ≥ 60 FPS @ 1080p on RX 580. Camera can fly from 2 km altitude to ground without visible seams or precision jitter. Floating-origin recenter is imperceptible.

### Phase 21 — Greywater Atmosphere & Space · 8 weeks · World Systems Lead

**Deliverables**
1. `engine/world/atmosphere/` — physically-based atmospheric scattering (single compute dispatch, no LUTs).
2. Volumetric clouds — raymarched, Perlin-Worley 3D density texture, adaptive step size.
3. Five-layer atmospheric model — drag/lift/heating/audio-filter interpolation.
4. Re-entry physics — heat + drag + structural stress model.
5. Re-entry plasma VFX — per-hull-surface fragment shader keyed on heating rate.
6. `engine/world/orbit/` — Newtonian gravity with `f64` positions/velocities; trajectory prediction.
7. `engine/world/space/` — deterministic starfield + nebulae impostors + planets-at-distance (three distance tiers).

**Exit Criteria (*First Launch*)** — a player (or scripted camera) launches from the surface of the First Planet and ascends continuously through all five atmospheric layers to orbit. No loading screen. Atmospheric scattering, cloud density, audio filter, and physics drag all interpolate smoothly. Re-entry plasma VFX activates on descent.

### Phase 22 — Greywater Survival Systems · 6 weeks · Gameplay Systems Lead

**Deliverables**
1. `engine/world/crafting/` — recipe database + ECS crafting system + ingredient flow + 5 workbench tiers.
2. `engine/world/buildings/` — modular grid placement + piece types (foundation, wall, floor, door, window, roof, stair, ceiling) across 5 material tiers.
3. `engine/world/buildings/` — structural-integrity graph with ground-rooted support propagation; destruction cascades collapse unsupported components (Jolt physics debris).
4. `engine/world/territory/` — claim flags + contested zones + upkeep + decay.
5. Resource gathering flow end-to-end (node → tool → inventory → workbench → crafted item → placed building).

**Exit Criteria (*Gather & Build*)** — a player walks up to a resource node, mines it, returns to a workbench, crafts a wall, places it against a foundation, sees it connect through the integrity graph. Destroying the foundation collapses the wall. Full loop runs on RX 580 at target frame rate.

### Phase 23 — Greywater Ecosystem & Faction AI · 6 weeks · Gameplay Systems Lead

**Deliverables**
1. `engine/world/ecosystem/` — chunk-cell predator/prey dynamics with flora density, herbivore population, carnivore population.
2. Fauna spawning + migration + behavior-tree wiring (extending the Phase 13 game-AI framework).
3. `engine/world/factions/` — utility-AI action scorer with per-faction consideration sets (pirates, traders, military, cultists).
4. Faction territories, patrol routes, engagement logic.
5. Per-chunk navmesh baking (extending Phase 13 navigation to planetary scale).

**Exit Criteria (*Living World*)** — a biome chunk is populated with herbivores grazing, a carnivore hunting, and a faction patrol on a scheduled route. Predator-prey populations stabilize over a 60-minute simulation without extinction or explosion.

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
| Talent concentration on ECS and renderer                         | Medium     | High   | Design docs and decision records (ADRs) mandatory in `docs/adr/`. No undocumented tribal knowledge. |
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
