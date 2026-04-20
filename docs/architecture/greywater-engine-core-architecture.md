# Greywater_Engine × Cold Coffee Labs — Core Architecture

> *A narrative walkthrough of the engine we are building, who it is for, and the decisions behind it.*

**Author:** Principal Engine Architect, Cold Coffee Labs
**Date:** 2026-04-19
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
- **Purpose-building for Greywater the game.** Greywater_Engine is designed around the needs of the Greywater persistent-procedural-universe simulation — but with a clean engine/game code boundary so the engine remains reusable for future Cold Coffee Labs titles.
- **Realistic low-poly art direction.** Reduced polygon counts with realistic physically-based lighting and atmospheric scattering. See `docs/games/greywater/20_ART_DIRECTION.md`.
- Building it in the open within the studio — every architectural decision gets a written record under `docs/adr/`.

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

**Greywater-game phases (19–23)**

- **Phase 19 — Greywater Universe Generation (8w).** Seed management, hierarchical procedural gen, chunk streaming, biomes, resources, WFC → *Infinite Seed*.
- **Phase 20 — Greywater Planetary Rendering (8w).** Spherical planets, quad-tree cube-sphere, GPU compute mesh, hardware tessellation, floating origin, vegetation, oceans → *First Planet*.
- **Phase 21 — Greywater Atmosphere & Space (8w).** Atmospheric scattering, volumetric clouds, orbital mechanics, re-entry, space rendering → *First Launch*.
- **Phase 22 — Greywater Survival Systems (6w).** Crafting, base building, structural integrity, territory → *Gather & Build*.
- **Phase 23 — Greywater Ecosystem & Faction AI (6w).** Predator/prey, faction utility AI, per-planet navmesh → *Living World*.

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

**Greywater-game-specific**
- **Universe Browser** — hierarchical view of galaxy cluster → galaxy → system → planet. Click a planet to load it.
- **Planet Inspector** — planet-type selector, atmosphere parameters, biome weights, terrain noise layers, ocean level, seed override. Real-time preview regenerates the planet as values change.
- **Atmospheric Parameter Editor** — per-planet-type atmosphere definition (Rayleigh, Mie, ozone, sun colour, scattering coefficients).
- **Orbital Trajectory Preview** — when a ship or celestial body is selected, renders its predicted trajectory in the viewport for the next N minutes of game time.
- **Chunk Visualizer** — debug view of chunks in view: shows LOD level, generation state, memory footprint, cache hit/miss.
- **Structural Integrity Debug** — visualises the integrity graph for a selected building. Coloured support values per piece, cascade preview on piece-destroy.
- **Ecosystem Population View** — per-biome herbivore / carnivore / flora density over time, with population-dynamics graph.
- **Faction State View** — current utility-AI scoring, territory claims, active patrols, diplomacy matrix.

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

A panel that drops frames to render itself is broken. Perf regression on the default workspace gates CI (see `docs/05_ROADMAP_AND_MILESTONES.md` exit criteria for Phase 7).

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
