# 10 — Appendix: Glossary, References & Architecture Decision Records

**Status:** Reference — living document · Last revised 2026-04-22  
**Precedence:** L7 — ADRs always win over prose

> Every ADR is a decision made once, so it is never re-litigated without cause.

**Convention:** Alphabetical. Each term: name, one-to-three-sentence definition, citation or context.

---

## A

**ACL (Animation Compression Library).** A permissively-licensed C++ library for skeletal animation compression. Used alongside Ozz-animation to shrink shipped animation assets.

**ADR (Architecture Decision Record).** A short dated document capturing a single architectural decision, its context, the options considered, and the chosen trade-off. Lives under `docs/10_APPENDIX_ADRS_AND_REFERENCES.md`. Required for any change that crosses a non-negotiable in `docs/01_CONSTITUTION_AND_PROGRAM.md`.

**AoSoA.** Array-of-Structs-of-Arrays. Hybrid memory layout that preserves locality within cache lines while enabling SIMD across them. Used selectively in the ECS for hot components.

**Archetype.** In the ECS, the unique set of component types an entity has. Entities with the same archetype live in the same contiguous chunk of memory. The basis for cache-friendly iteration.

**Arena allocator.** An allocator that owns a large contiguous block and hands out smaller regions from it. Freeing the arena frees everything in it at once — O(1) teardown.

**ASan (AddressSanitizer).** A Clang instrumentation that detects memory errors (use-after-free, buffer overflow, leaks). Runs nightly in CI.

## B

**Bindless.** A descriptor-management style where shaders index into large descriptor arrays rather than binding descriptors per-draw. Vulkan 1.3 supports bindless via `VK_EXT_descriptor_indexing`. Baseline in Greywater's renderer.

**Binding (Rust/C++).** Generated code that allows one language to call the other. `cxx` generates C++ bindings to Rust; `cbindgen` generates C headers for Rust.

**BLD (Brewed Logic Directive).** Greywater Engine's native AI copilot subsystem. Implemented as a Rust crate linked into the editor via a narrow C-ABI. MCP-based, with proc-macro-generated tool surfaces.

**`bld-ffi`.** The Rust crate that defines the C-ABI boundary between BLD and the C++ editor. Covered by Miri to validate UB freedom.

**`bld-tools`.** The Rust crate hosting the `#[bld_tool]` proc macro and the tool registry. Tools annotated with `#[bld_tool]` become MCP-callable at compile time.

**Graph-as-ground-truth visual scripting.** A visual-scripting pattern where the node graph is the canonical serialized form. Greywater **deliberately does not use this pattern** — text IR is canonical, graph is a projection.

**Brew Doctrine.** Greywater Engine's philosophy statement. See `docs/03_PHILOSOPHY_AND_ENGINEERING.md`.

**Blasphemy.** In *Sacrilege*, a player-triggered desecration act that breaks a moment of ritual observance and generates **Sin** charge. Examples include slaying an enemy mid-prayer, defacing an altar, or firing a weapon inside a consecrated ring. Mechanically fuels the Martyrdom Engine (see `docs/07_SACRILEGE.md §Martyrdom Engine`).

**Blacklake Framework (BLF).** Cold Coffee Labs' deterministic procedural stack hosted in Greywater Engine — **HEC** (Hierarchical Entropy Cascade) seeding, **SDR** (Stratified Domain Resonance) noise, **GPTM** (Greywater Planetary Topology Model — **acronym retained**; arena-scale terrain meshing for *Sacrilege*, not a planet simulator — see `docs/08_BLACKLAKE.md`), **TEBS** (Tessellated Evolutionary Biology System). Authoritative math/spec: `docs/08_BLACKLAKE.md`. Flagship use: *Sacrilege*'s Nine Inverted Circles and arena genesis.

## C

**Circle.** In *Sacrilege* lore and level scope, one of the **Nine Inverted Circles of Hell** — a bounded procedurally-generated 3D arena (not a planet, not a biome tile) gated by descent narrative. Each Circle has a `CircleId` enum (`Lust`, `Gluttony`, `Greed`, `Wrath`, `Heresy`, `Violence`, `Fraud`, `Treachery`, `Apostasy` — authoritative list in `docs/07_SACRILEGE.md`). Generated deterministically from the **Hell Seed** via HEC + SDR + GPTM.

**Cold Drip.** Cold Coffee Labs' visual / motion design system — cold, precise, monochromatic with a single accent color, slow organic motion, high contrast. Canonical palette and motion doctrine in `docs/03_PHILOSOPHY_AND_ENGINEERING.md §3` (Aesthetic — *Cold Drip × Sacrilege*).

**Cold Coffee Labs (CCL).** The studio behind Greywater Engine and *Sacrilege*. Authoritative identity statement in `docs/01_CONSTITUTION_AND_PROGRAM.md §0`.

**Candle.** A pure-Rust ML runtime. Primary local-inference backend for BLD's RAG and offline mode.

**`cbindgen`.** A tool that generates C and C++ headers from Rust types. Used to expose BLD's public API to C++.

**Clang.** The LLVM C/C++ frontend. **The only C++ compiler supported by Greywater.** On Windows, `clang-cl` targets the MSVC ABI.

**CMake.** The build generator. Greywater uses 3.28+ with file sets, target-based design, and `CMakePresets.json`. Ninja is the only generator.

**Command bus / `ICommand`.** The undo/redo primitive in `engine/core/command/`. Every mutating action is an `ICommand` pushed on the stack. Ctrl+Z reverses. BLD tools submit commands via C-ABI callback.

**Commitments (non-negotiable).** Load-bearing decisions that cannot be changed without stakeholder sign-off. Enumerated in `docs/01_CONSTITUTION_AND_PROGRAM.md` §2.

**Corrosion.** A CMake module that integrates Cargo builds into CMake. The bridge between C++ and Rust in Greywater's build graph.

**CPM.cmake.** A CMake dependency manager that pins C++ dependencies by commit SHA. Source-code integration, no system packages.

**CRTP (Curiously Recurring Template Pattern).** A C++ idiom for compile-time polymorphism — a class inherits from a template parameterized by itself. Used where virtual dispatch would cost in hot paths.

**`cxx`.** A Rust library by dtolnay for safe Rust/C++ interop. Used to bridge BLD into the editor.

## D

**Documentation map (`docs/README.md`).** The eleven-file index: layers of truth (L0–L7), precedence when prose conflicts, and reading order. Former standalone `DOCUMENTATION_SYSTEM.md` content is split: the index and reading order live in `docs/README.md`; the long-form IA block lives under `docs/01_CONSTITUTION_AND_PROGRAM.md` (merged from `DOCUMENTATION_SYSTEM.md`).

**DOD (Data-Oriented Design).** A design philosophy that prioritizes data layout and access patterns over inheritance hierarchies. Greywater's default.

**`DXC`.** DirectX Shader Compiler. Compiles HLSL to SPIR-V for Vulkan.

**Determinism.** The property that the same inputs produce the same outputs across runs and machines. Required for lockstep multiplayer, replay, and reproducible bug reports. Enforced in physics (Jolt lockstep mode), vscript execution, and asset cook.

## E

**ECS (Entity Component System).** A data-oriented pattern where entities are handles, components are plain data, and systems are functions over component spans. Greywater's ECS is custom, archetype-based.

**Elicitation.** An MCP primitive that lets a server pause a tool call and request input from the user. Used for Greywater's HITL approval dialogs.

**Entity handle.** A 64-bit value (32-bit index + 32-bit generation) uniquely identifying an entity in the ECS. Safe to store across frames; cheap to compare.

**EOS (Epic Online Services).** One of the platform-services backends Greywater integrates through `IPlatformServices`.

## F

**FFI (Foreign Function Interface).** The boundary between Rust and C++ in Greywater, mediated by `cxx` and `cbindgen`. Intentionally narrow — only handles and POD messages cross.

**Fiber.** A lightweight coroutine-like unit of execution that preserves stack state across yield points. Greywater's job system is fiber-based for long-running tasks — the fiber holds the stack while the work is suspended.

**Forward+ (clustered).** A rendering technique that divides the screen into clusters and assigns lights to each cluster, enabling efficient many-light shading in forward rendering. Greywater's reference renderer.

**Frame allocator.** A linear allocator reset once per frame. Default for transient per-frame work.

**Frame graph.** A render-pass dependency graph that declares resource reads/writes. The HAL inserts barriers, aliases transient resources, and schedules work automatically.

## G

**GWE (Greywater Engine).** Cold Coffee Labs' in-house game engine. The subject of this documentation set. When written without qualification, "GWE" and "Greywater Engine" refer to the same thing. See `docs/01_CONSTITUTION_AND_PROGRAM.md §0`.

**`gw::` (namespace).** Greywater's root C++ namespace. `gw::render`, `gw::ecs`, `gw::jobs`, `gw::audio`, etc.

**`GW_ASSERT`.** Greywater's assertion macro. Captures stack trace, optionally opens debugger, logs via `GW_LOG`.

**`.gwscene`.** The binary scene format. Versioned, migration-hooked.

**`.gwseq`.** The binary sequencer format for cinematics. Shares serialization machinery with `.gwscene`.

**GameNetworkingSockets (GNS).** An open-source networking library. Greywater's transport layer. Battle-tested at large-scale multiplayer.

**GBNF (GGML Backus-Naur Form).** A grammar format used by llama.cpp to constrain model output. Greywater derives GBNF grammars from tool JSON schemas so small local models emit structurally valid tool calls.

**glTF.** The 3D asset interchange format. Greywater's primary mesh/scene import.

## H

**Hell Seed.** The top-level 64-bit seed that deterministically drives every procedural system in a *Sacrilege* run — Circle topology, arena meshing (GPTM), enemy composition, Blasphemy placement. Same Hell Seed + same `CircleId` = bit-identical level on every machine. See `docs/01_CONSTITUTION_AND_PROGRAM.md §2.6` and `docs/08_BLACKLAKE.md`.

**HAL (Hardware Abstraction Layer).** The thin ~30-type interface that abstracts GPU concepts (devices, queues, command buffers, pipelines) from the backend (Vulkan, future WebGPU). Everything above the HAL is backend-agnostic.

**HarfBuzz.** An open-source text-shaping engine supporting complex scripts, RTL, and ligatures. Greywater's choice for in-game text.

**HITL (Human-In-The-Loop).** A governance pattern where an AI agent requests human approval before mutating action. In Greywater, HITL is protocol-native via MCP elicitation.

**HLSL.** High-Level Shading Language (originally Direct3D's). Greywater's primary shading language, compiled to SPIR-V via DXC.

**Hot-reload.** Replacing running code with a new version without restarting. Greywater hot-reloads the gameplay module (C++ dylib); it does **not** hot-reload the engine core or BLD.

**HRTF (Head-Related Transfer Function).** A per-person filter that lets headphones simulate 3D spatial audio. Greywater uses Steam Audio's HRTF implementation.

## I

**ICU (International Components for Unicode).** The industry-standard library for Unicode handling. Greywater's choice for locale-aware formatting, collation, pluralization.

**ImGui (Dear ImGui).** An immediate-mode GUI library for C++. The **locked** choice for Greywater's editor UI.

**ImGuizmo.** A viewport-manipulation gizmo library for ImGui. Greywater's 3D transform gizmo.

**ImNodes.** A node-graph editor widget for ImGui. Greywater's canvas for the vscript node-graph projection.

**IR (Intermediate Representation).** For Greywater, the typed text form of visual scripts. Canonical; the node graph is a projection.

## J

**Jolt Physics.** An open-source C++ physics engine. Greywater's physics backend — rigid bodies, character controller, constraints, deterministic mode.

**Job system.** Greywater's custom fiber-based work-stealing scheduler in `engine/jobs/`.

## K

**KTX2.** A GPU-ready texture container supporting BC7 and ASTC. Greywater's cooked texture format.

## L

**`llama-cpp-rs`.** Rust bindings to llama.cpp, the C++ LLM inference runtime. Fallback backend for BLD's local inference (primary is `candle`).

**LLVM.** The compiler infrastructure Clang uses. Greywater depends on LLVM 18+.

**Lockstep.** A deterministic multiplayer model where every client simulates identically from shared inputs. Used for fighting-game-class titles. Greywater supports it as a configurable mode.

**LTS (Long-Term Support).** A branch cut periodically (quarterly in Greywater's plan) that receives hotfix backports for 12 months.

## M

**Mantra.** In *Sacrilege*'s **Martyrdom Engine**, a rhythmic player verb that converts accumulated **Sin** into controlled combat pressure — a timed chant / held-input that consumes Sin and emits a buff, a projectile, or a field effect. The counterweight to **Rapture**. See `docs/07_SACRILEGE.md §Martyrdom Engine`.

**Martyrdom Engine.** *Sacrilege*'s signature combat-and-resource loop: **Sin → Mantra → Rapture → Ruin**. Sin accrues from damage taken and from Blasphemies; Mantra converts Sin into offensive verbs; Rapture is the cathartic spike state; Ruin is the consequence / downside that gates the next cycle. Authoritative definition in `docs/07_SACRILEGE.md`.

**MCP (Model Context Protocol).** An open protocol for exposing tool surfaces to AI agents. Version `2025-11-25` is the one we implement. BLD is an MCP server; the editor panel and any compliant external client are MCP clients.

**miniaudio.** A single-header C audio library. Greywater's mixer/device backend.

**Miri.** A Rust interpreter that detects undefined behavior. Run nightly on `bld-ffi` to validate the FFI boundary.

**Module (Phase-1 context).** A sub-directory under `engine/` representing a cohesive subsystem. Not to be confused with C++20 modules.

## N

**Ninja.** A small, fast build system used by CMake. The only generator supported by Greywater.

**`Nomic Embed Code`.** A code-specialized embedding model. Greywater's default for RAG embeddings (offline via candle).

## O

**Obsidian.** Greywater's primary UI background color (`#0B1215`). See `docs/03_PHILOSOPHY_AND_ENGINEERING.md` §3.

**Ozz-animation.** An open-source minimal-overhead skeletal animation runtime. Greywater's animation backend.

## P

**Symbol-graph ranking.** An eigenvector-based ranking (PageRank-style) applied over a symbol graph. Greywater uses it to boost structurally important definitions during RAG retrieval.

**`petgraph`.** A Rust graph library. Used by BLD's RAG to build and query symbol graphs.

**Phase (1–25).** The full engine + LTS buildout in `docs/02_ROADMAP_AND_OPERATIONS.md`.

**Pool allocator.** An allocator for fixed-size objects backed by a free-list. Used for hot component types and particle systems.

**Principal Engine Architect.** The role Claude Opus 4.7 is authorized to play on this project. See `docs/01_CONSTITUTION_AND_PROGRAM.md` §1.

## R

**Rapture.** The cathartic spike state in the Martyrdom Engine — after enough Sin has been spent through Mantras, the player enters a short-lived burst of heightened capability (damage, speed, resistances). Rapture always ends in **Ruin**. See `docs/07_SACRILEGE.md §Martyrdom Engine`.

**Ruin.** The consequence / downside state that follows Rapture — a weakened/exposed window the player must survive before the cycle can begin again. Designed as a *load-bearing* downside so Rapture is not a free power spike. See `docs/07_SACRILEGE.md §Martyrdom Engine`.

**RAG (Retrieval-Augmented Generation).** A technique for grounding LLM responses in retrieved context. Greywater's BLD indexes both engine source and user projects.

**RmlUi.** An open-source HTML/CSS-inspired in-game UI library. Greywater's choice for HUDs and menus. **Deliberately not ImGui.**

**`rmcp`.** The official Rust MCP SDK. BLD's MCP server is built on it.

**RTTI (Run-Time Type Information).** C++ runtime type introspection. **Disallowed in Greywater hot paths.**

**Rust.** The language used for BLD only. Everywhere else is C++23.

## S

**Supra-AAA.** Cold Coffee Labs production bar **above** conventional licensed-engine AAA: sovereign stack, Blacklake-scale determinism, BLD-native throughput, RX 580 Tier A proof, defensible novelty. Defined in `docs/01_CONSTITUTION_AND_PROGRAM.md` §0.1 and `docs/01_CONSTITUTION_AND_PROGRAM.md`.

***Sacrilege*.** The debut title Greywater Engine presents — flagship game docs under `docs/07_SACRILEGE.md` (GW-SAC-001-REV-B). Unified chain: Cold Coffee Labs → Greywater Engine → *Sacrilege*.

**GW-SAC-001-REV-B.** Studio designation for the consolidated *Sacrilege* program specification (living document, docs/07_SACRILEGE.md). Supersedes REV-A.

**Sandbox (executable).** The engine's own feature test bed. Links `greywater_core` directly, runs experiments without editor or game weight.

**Sentry.** A crash-reporting service. Greywater uses the Sentry Native SDK for minidump upload.

**Sin.** The core accrual resource in *Sacrilege*'s Martyrdom Engine. Gained from damage taken, Blasphemies, and environmental interactions; spent by Mantras to drive offensive verbs and to reach Rapture. Represented as `SinComponent` on player / relevant entities. See `docs/07_SACRILEGE.md §Martyrdom Engine`.

**SemVer (Semantic Versioning).** Greywater's engine versioning scheme.

**SDL3.** Simple DirectMedia Layer version 3. Used for gamepad/HID/haptics input. Keyboard/mouse go through the platform layer directly.

**SPIR-V.** The bytecode format for Vulkan shaders. HLSL/GLSL compile to SPIR-V via DXC/glslang.

**sqlite-vec.** A SQLite extension for vector similarity search (the maintained successor to the deprecated `sqlite-vss`). Greywater's vector index for BLD's RAG. See ADR-0013 for the migration decision.

**Steam Audio.** A spatial-audio SDK. Greywater's choice for HRTF + occlusion + reflection.

## T

**Tier (A/B/C/G).** Scope classification. A must ship, B likely ships, C is post-v1, G is hardware-gated. See `04_SYSTEMS_INVENTORY`.

**Timeline semaphore.** A Vulkan 1.2+ synchronization primitive that replaces binary semaphores and fences. Greywater baseline.

**TOML.** A human-readable config format. Greywater's choice for user-facing config and CVar persistence.

**Tracy.** A low-overhead C++ profiler. Greywater's profiling backend, integrated in engine and editor.

**`tree-sitter`.** An incremental parsing library used for AST-aware code chunking. Greywater's RAG depends on it.

**TSan (ThreadSanitizer).** Clang instrumentation for data race detection. Runs nightly.

## U

**UBSan (UndefinedBehaviorSanitizer).** Clang instrumentation for UB detection. Runs nightly.

**Undo/redo.** In Greywater, a first-class system: every mutating action is an `ICommand` on the command stack. BLD's mutations go through the same stack via C-ABI.

## V

**Vcpkg.** A C++ package manager. **Not used** by Greywater — CPM.cmake is the dependency manager.

**Volk.** A Vulkan loader that avoids the overhead of the standard loader. Greywater uses it.

**VMA (VulkanMemoryAllocator).** AMD's Vulkan memory allocator library. Greywater uses it for device-memory management.

**vscript.** Greywater's visual-scripting system. Text IR is ground-truth; node graph is a projection.

**Vulkan 1.2 baseline.** Greywater's target graphics API. Dynamic rendering, timeline semaphores, bindless descriptors, buffer device address are all in Vulkan 1.2 and baseline. Vulkan 1.3 features (`synchronization_2`, etc.) are opportunistic via `RHICapabilities`. Ray tracing, mesh shaders, and work graphs are hardware-gated (Tier G, post-v1).

**RX 580 / Polaris.** AMD Radeon RX 580 8GB (GCN 4.0, 2016). Greywater's baseline development hardware and target player platform. ***Sacrilege* Tier A:** **1080p / 144 FPS** (`docs/01_CONSTITUTION_AND_PROGRAM.md` §2.1). **Engine reference harnesses:** **≥ 60 FPS** where `docs/05_RESEARCH_BUILD_AND_STANDARDS.md` §1.4 applies — not the flagship ship bar. No ray-tracing cores, no mesh-shader support on this SKU.

**Atmospheric scattering.** Greywater's physically-based atmospheric scattering model for **reference / studio renderer** workloads lives under `engine/world/atmosphere/`. ***Sacrilege*** is **arena-scale** — per-circle fog and simplified atmosphere per `docs/08_BLACKLAKE.md` and `docs/04_SYSTEMS_INVENTORY.md`; no planetary ground-to-orbit simulation (`docs/01_CONSTITUTION_AND_PROGRAM.md` §2.6).

**Floating origin.** Recenters world and entity transforms when the camera travels far so local `float` precision stays stable. For ***Sacrilege***, this fires within **large arenas** (see `docs/08_BLACKLAKE.md`, Phase 20 `FloatingOrigin`). Broader planetary use, if any, is outside the debut title mandate.

**Quad-tree cube-sphere.** A six-face cube-sphere subdivision scheme for **planetary-scale** meshes. **Not in *Sacrilege* v1 scope** (Nine Circles are bounded arenas — `docs/01_CONSTITUTION_AND_PROGRAM.md` §2.6).

**`float64` / `f64`.** Double precision for **arena-scale** world-space and chunk coordinates where `float` error would be visible (`docs/01_CONSTITUTION_AND_PROGRAM.md` §2.6). Converted to `float` for local-to-chunk math and GPU attributes after origin subtraction.

**Deterministic procedural arenas.** Given the same **Hell Seed**, circle index, and chunk coordinates, any two machines produce **byte-identical** Blacklake output — speedrun parity and rollback-friendly simulation (`docs/08_BLACKLAKE.md`, `docs/09_NETCODE_DETERMINISM_PERF.md`).

**Chunk streaming.** On-demand generation and caching of 32 m³ world chunks, with LRU eviction and time-sliced generation across multiple frames. Never blocks the main thread.

**Marching Cubes.** An algorithm for extracting an isosurface mesh from a 3D scalar field. Greywater uses it for caves and voxel-based structures.

**Rollback netcode.** A networking technique that predicts remote inputs, simulates locally with zero input lag, and rewinds/resimulates when predictions are wrong. Requires a deterministic simulation. Greywater uses it for PvP combat.

**Deterministic lockstep.** A networking technique where every client simulates identically from shared inputs. Requires a fully deterministic simulation. Greywater uses it for 4-client large-scale simulation scenarios.

**Structural integrity graph.** A connectivity graph over building pieces used to propagate support values from ground-connected nodes outward. Unsupported structures collapse. Greywater's base-building destruction model.

**Wave Function Collapse (WFC).** A procedural content generation algorithm using adjacency constraints. Greywater uses it for ruined cities and space stations.

**Utility AI.** A decision-making technique where actions are scored by weighted considerations and the highest-scoring action is selected. Greywater uses it for faction-level strategic decisions.

<!-- second Steam Audio entry consolidated with the earlier one above -->

**Crunchy low-poly horror (*Sacrilege*).** Flagship art direction: low-poly reads, small textures, aggressive contrast, horror-forward post (see `docs/07_SACRILEGE.md`). Editor/tooling UI remains separate (*Cold Drip* — `docs/03_PHILOSOPHY_AND_ENGINEERING.md` §3.2).

## W

**WCAG 2.2.** Web Content Accessibility Guidelines version 2.2. Greywater targets AA compliance on the reference HUD.

**WebGPU.** A cross-platform GPU API. Greywater plans a WebGPU backend behind the HAL for web and sandboxed targets; not v1.

---

*Terminology deliberately. If a term is used in the codebase or docs and is not here, add it.*

---

---

## 09_APPENDIX — Research & Internal References

**Status:** Reference · Living document (updated as research lands)

This appendix is the internal bibliography of the Greywater project. It catalogues the technical standards we target, the libraries we use, and the internal decisions that shape our architecture. It is **not** a survey of external products or competitors — that kind of analysis, when we do it, lives as an ADR under `docs/10_APPENDIX_ADRS_AND_REFERENCES.md`, summarised into a single decision and then archived.

If you arrive at this document expecting name-drops of every technique, paper, or studio that influenced modern game engines, stop. Greywater's docs refer only to: Cold Coffee Labs, Greywater Engine, the flagship title ***Sacrilege***, and the open standards / tooling we use by name. That discipline is intentional.

---

## 1. Standards we target

These are the open specifications and standards that Greywater conforms to. When a claim in our docs depends on a standard, the relevant section goes here.

- **C++23** — ISO/IEC 14882:2024. Full standard library where applicable; restricted in engine hot paths (see `docs/05_RESEARCH_BUILD_AND_STANDARDS.md`).
- **Vulkan 1.2 (baseline) with opportunistic 1.3 features.** See `03_VULKAN_RESEARCH_GUIDE.md` for the full capability table.
- **Rust 2024 edition** (stable; 2021 until 2024 lands on stable). BLD crate only.
- **Model Context Protocol, version 2025-11-25.** The wire protocol between BLD and any MCP client.
- **Unicode 16.0** — text handling via ICU.
- **WCAG 2.2 Level AA** — accessibility target for HUD and menus.
- **GDPR, CCPA, COPPA** — privacy-law compliance for telemetry and save data.
- **glTF 2.0** — mesh/scene interchange format.
- **KTX2** — GPU-ready texture container.
- **SPIR-V** — shader bytecode; compiled from HLSL via DXC.
- **SemVer 2.0.0** — engine versioning.
- **Conventional Commits 1.0.0** — commit-message format.

## 2. Libraries and tooling we use

A flat list of the open-source and freely-licensed libraries and tools Greywater depends on. This list is the single source of truth for "what we link against" — ADRs reference it, the CPM dependency file pins versions that match it.

### Build and toolchain
- **Clang / LLVM** — C++ compiler.
- **CMake 3.28+** with Presets — build generator.
- **Ninja** — build driver.
- **Corrosion** — CMake ↔ Cargo integration for BLD.
- **Cargo** — Rust build driver.
- **CPM.cmake** — C++ dependency manager.
- **cxx** + **cbindgen** — Rust/C++ binding generation.
- **sccache** — shared compile cache.
- **rustfmt**, **clippy**, **cargo-deny**, **cargo-audit**, **cargo-mutants**, **Miri** — Rust quality tooling.
- **clang-format**, **clang-tidy**, **cmake-format**, **editorconfig** — C++ / infrastructure quality tooling.

### Rendering
- **volk** — Vulkan loader.
- **VulkanMemoryAllocator (VMA)** — GPU memory management.
- **DXC** — HLSL → SPIR-V shader compiler.
- **spirv-cross** / **spirv-reflect** — shader reflection.
- **GLFW** — windowing backend behind `engine/platform/`.
- **Dear ImGui** (docking + viewports branch) — editor UI.
- **ImNodes** — node-graph widget for visual scripting projection.
- **ImGuizmo** — 3D transform gizmos.
- **`imgui_md`** (MD4C-based) — markdown rendering for BLD chat panel.
- **RmlUi** — in-game UI layout engine.
- **HarfBuzz** + **FreeType** — text shaping and rasterisation.

### Simulation & content
- **Jolt Physics** — rigid-body dynamics, character controller, constraints, deterministic mode.
- **Ozz-animation** + **ACL** — skeletal animation runtime + compression.
- **Recast/Detour** — navigation mesh generation and pathfinding.
- **cgltf** / **fastgltf** — glTF 2.0 import.
- **libktx** + BC7/ASTC encoders — texture cook.

### Audio
- **miniaudio** — device/mixer.
- **Steam Audio** — spatial audio (HRTF, occlusion, reflection).
- **Opus** — voice and streaming codec.
- **libopus** / **libvorbis** — codec implementations.

### Networking
- **GameNetworkingSockets** — transport layer.

### Data and persistence
- **SQLite** — local database.
- **sqlite-vec** — vector search extension for BLD RAG (supersedes the deprecated sqlite-vss; see ADR-0013).
- **ICU** — Unicode runtime services.

### BLD (Rust)
- **rmcp** — Rust MCP SDK (server side).
- **tree-sitter** + language grammars — AST-aware code chunking.
- **petgraph** — graph algorithms for symbol-graph ranking.
- **rusqlite** — SQLite bindings.
- **candle** — pure-Rust ML runtime (local inference + embeddings).
- **llama-cpp-rs** — alternate local-inference runtime.
- **reqwest** + **eventsource-stream** — HTTP + SSE for cloud LLM providers.
- **serde** + **serde_json** — serialization.
- **tokio** — async runtime (single, shared across the BLD crate).
- **thiserror**, **anyhow** — error types.
- **keyring** — OS keychain access for API keys.
- **notify** — filesystem watcher for RAG incremental indexing.

### Testing and observability
- **doctest** — C++ unit testing.
- **Tracy** — frame-level CPU/GPU profiling.
- **Sentry Native SDK** — crash reporting.
- **libFuzzer** — coverage-guided fuzzing.

### Packaging
- **CPack** — installer generation.
- **MSIX** (Windows) / **AppImage** / **DEB** / **RPM** (Linux) — distribution formats.

### Platform services
- **Steamworks SDK** — Steam integration (first platform backend).
- **Epic Online Services SDK** — cross-platform services (second platform backend).

### Models
- **nomic-embed-code** — default code-aware embedding model (shipped via `candle`).

## 3. Internal reference catalogue

Greywater's own canonical documents — the ones to read rather than to summarise.

| Document | Scope |
| --- | --- |
| `CLAUDE.md` | Cold-start primer and non-negotiables |
| `docs/README.md` | Eleven-file map, IA, reading order |
| `docs/01_CONSTITUTION_AND_PROGRAM.md` | Constitution, IA body, executive brief, first-90-day plan, session handoff |
| `docs/02_ROADMAP_AND_OPERATIONS.md` | 25-phase roadmap (Phase 24 hardens/ships, Phase 25 sustains LTS), week rows, Kanban rituals |
| `docs/03_PHILOSOPHY_AND_ENGINEERING.md` | Brew doctrine + engineering / review playbook |
| `docs/04_SYSTEMS_INVENTORY.md` | Subsystem tiers and library choices |
| `docs/05_RESEARCH_BUILD_AND_STANDARDS.md` | Vulkan capability table, C++23 / Rust idioms, build & coding standards |
| `docs/06_ARCHITECTURE.md` | Stakeholder blueprint + narrative core architecture |
| `docs/07_SACRILEGE.md` | *Sacrilege* program spec (GW-SAC-001) |
| `docs/08_BLACKLAKE.md` | Blacklake (BLF) + GWE × Ripple studio specifications |
| `docs/09_NETCODE_DETERMINISM_PERF.md` | Netcode contract, perf budgets, determinism / replay annexes |
| `docs/10_APPENDIX_ADRS_AND_REFERENCES.md` | Glossary, this appendix, **ADR archive** |

## 4. Additions policy

New entries in this appendix follow three rules:

1. **Name only what we depend on, cite only open standards.** External products, competitor engines, specific studios, specific engineers, specific papers, and specific talks do not belong here. If an idea from outside influenced us, it has been absorbed into an ADR and then into the code. The code is the reference.
2. **Pinned versions live in `cmake/dependencies.cmake` and `bld/Cargo.lock`**, not here. This appendix names the library; the pinned version is machine-enforced.
3. **Live documents are revisited quarterly.** The library ecosystem moves. The standards move. This appendix is not a snapshot.

---

*Everything we depend on is listed. Everything we don't depend on is irrelevant to Greywater.*

---

## Architecture Decision Records (full text)

Individual `docs/adr/*.md` files were merged here for the 11-file layout. Original filenames are repeated in each subsection header.

---

## ADR file: `adr/0003-rhi-capabilities.md`

## ADR 0003 — RHI Capabilities layer

- **Status:** Accepted (amended 2026-04-20 late-night — see §2.1)
- **Date:** 2026-04-20
- **Deciders:** Cold Coffee Labs engine group (Claude Opus 4.7 drafting; founder final)
- **Supersedes:** —
- **Superseded by:** —
- **Related:** `CLAUDE.md` non-negotiable #2 (Vulkan 1.2 baseline, 1.3 opportunistic); `docs/01_CONSTITUTION_AND_PROGRAM.md §2.2`; `docs/05_RESEARCH_BUILD_AND_STANDARDS.md §4`; `audit-2026-04-20 §P1-1`.

## 1. Context

`CLAUDE.md` non-negotiable #2 says:

> **Vulkan 1.2 baseline** with **1.3 features opportunistic** (via `RHICapabilities`).

The reference docs (`docs/06_ARCHITECTURE.md §3.5 GPU/HAL` and `docs/05_RESEARCH_BUILD_AND_STANDARDS.md §4`) sketch the `struct RHICapabilities` shape. They have consistently promised a single feature-flag struct populated at device init; callers gate on booleans; the frame graph picks code paths based on capability (no runtime `if (mesh_shaders) …` strewn through shading code).

In reality (end-of-day 2026-04-20), no such symbol exists. `engine/render/hal/vulkan_device.cpp` unconditionally enables `VK_KHR_dynamic_rendering` + `VK_KHR_synchronization2`, unconditionally chains the corresponding feature structs into `VkPhysicalDeviceFeatures2.pNext`, and unconditionally fails device creation if the physical device does not support them. `engine/render/frame_graph/*` records via `vkCmdPipelineBarrier2` and `vkCmdBeginRendering` with no query against what the device can do. The sandbox, editor, and render paths all assume 1.3 features.

This is operationally fine on the reference RX 580 workstation (driver reports `apiVersion = 1.3.260` with both extensions) and is the root reason Round-1 audit items did not trip during evening-session verification. It is structurally incorrect and blocks Phase 8:

1. Cross-device CI (Phase 8 Linux matrix) will expose any driver that reports a bare 1.2 core with no extensions and will hard-fail device creation rather than degrading.
2. Tier-G gates referenced all over `docs/04_SYSTEMS_INVENTORY.md`, `docs/10_APPENDIX_ADRS_AND_REFERENCES.md §3` (Tool & Library Catalog), and `docs/06_ARCHITECTURE.md §3.5` have no code-level anchor to compile against.
3. Future features (mesh shaders, ray tracing, VK 1.4 work graphs — all Tier G per `docs/01_CONSTITUTION_AND_PROGRAM.md §2.1` / `docs/04_SYSTEMS_INVENTORY.md` tier column) need a named place to land.

## 2. Decision

Introduce `engine/render/hal/capabilities.hpp` (+ `.cpp`) exposing:

```cpp
namespace gw::render::hal {

struct RHICapabilities {
    // API version (from VkPhysicalDeviceProperties.apiVersion)
    std::uint32_t api_version = 0;

    // Baseline 1.2-core features (expected true on every shipping target)
    bool timeline_semaphores    = false;
    bool buffer_device_address  = false;
    bool descriptor_indexing    = false;  // bindless

    // Vulkan 1.3 opportunistic (RX 580 reports true; older GCN may not)
    bool dynamic_rendering      = false;
    bool synchronization_2      = false;

    // Tier G — hardware-gated (all false on RX 580)
    bool mesh_shaders           = false;
    bool ray_tracing_pipeline   = false;
    bool ray_query              = false;
    bool gpu_work_graphs        = false;  // Vulkan 1.4

    // Memory-budget integration for streaming scheduler (P1-2)
    bool ext_memory_budget      = false;

    // Device properties surfaced for callers that need them
    std::uint64_t total_device_local_bytes = 0;
    std::uint32_t max_descriptor_indexed_array_size = 0;

    // Convenience queries (read-only; do not mutate)
    [[nodiscard]] bool has_sync2()             const noexcept { return synchronization_2; }
    [[nodiscard]] bool has_dynamic_rendering() const noexcept { return dynamic_rendering; }
    [[nodiscard]] std::uint32_t api_major()    const noexcept;
    [[nodiscard]] std::uint32_t api_minor()    const noexcept;
};

// Populates every field from a live VkPhysicalDevice. Never throws; missing
// features are reported as `false`.
[[nodiscard]] RHICapabilities probe_capabilities(VkPhysicalDevice) noexcept;

} // namespace gw::render::hal
```

`VulkanDevice` probes once at construction and stores the result. Two consequences follow:

1. **Device creation becomes capability-aware.** The device-extension list and the feature-struct chain are built from the probe result, not from a hard-coded list. Missing optional features are reported as `false` in `RHICapabilities` and the corresponding extension is simply not enabled; missing *baseline* features (timeline semaphores, descriptor indexing, synchronization_2, dynamic_rendering — see §2.1) remain a hard error.

2. **Higher layers branch on capabilities, never on extension names.** The frame graph reads `device.capabilities()` — e.g. `has_mesh_shaders()`, `has_ray_tracing()` — when deciding which code path to emit. For Tier-A features the branch is compiled out (the capability is guaranteed present); for Tier-G features the branch is the single gating point for optional code paths. What this ADR **does** mandate is that the branch point exists, is one line of code, and lives at the frame-graph level (not at the HAL level).

## 2.1. Amendment (2026-04-20 late-night) — `synchronization_2` and `dynamic_rendering` promoted to Tier-A

**Context.** The original §2 framing had `synchronization_2` and `dynamic_rendering` as *opportunistic* (Tier-B): the probe would report presence, device creation would enable them when available, and the frame graph would branch on `has_sync2()` / `has_dynamic_rendering()`. The first-pass implementation made device creation capability-aware correctly, **but the frame-graph branch was a silent no-op** — `execute_pass` would skip `vkCmdPipelineBarrier2` entirely when `has_sync2()` was false. That is a silent correctness hole (dropped barriers → wrong-stage submission, memory-ordering bugs visible only under load), not a fallback.

**Reality check.** A real fallback would need:

1. A sync1 barrier-emit path (translation tables for stage+access masks per Vulkan spec §7.4.2).
2. A legacy `VkRenderPass` + `VkFramebuffer` builder for the frame graph's render targets, replacing `vkCmdBeginRendering` / `vkCmdEndRendering`.
3. A second code path through `engine/render/frame_graph/command_buffer.cpp` (the Tier-A wrapper today calls the sync2/dynamic-rendering intrinsics directly).
4. Cross-hardware CI (lavapipe, MoltenVK, pre-1.3 AMD/NVIDIA) to exercise it.

**Decision.** Promote both features to Tier-A:

- `VulkanDevice::VulkanDevice` refuses to construct when `!caps.has_sync2()` or `!caps.has_dynamic_rendering()`, with a specific error message that points to this ADR.
- The feature-struct chain in device creation unconditionally enables both (no `if (caps_.has_sync2())` guards in device init).
- The frame-graph barrier emit drops its `has_sync2()` guard; the capability is a construction invariant.
- `capabilities().has_sync2()` / `has_dynamic_rendering()` remain as queries, for code that wants to assert or document the invariant, but callers may assume true on any successfully-constructed `VulkanDevice`.

**Tier-G follow-up.** A real sync1 + legacy-render-pass backend becomes a *phase task*, not a lurking correctness hole. When the matrix expands (lavapipe for Linux CI without a real GPU; MoltenVK for macOS — neither of these advertises sync2 today), reopen this ADR with the additional scope and revert `has_sync2()` to opportunistic. Until then, the reference hardware (RX 580, apiVersion 1.3.260) and every Phase-7-target driver advertises both, so nothing is actually being given up.

**What this doesn't change.** NN #2's "Vulkan 1.2 baseline; 1.3 opportunistic" remains correct at the *feature category* level — we still compile against 1.2 core, `VkInstance` is created with `apiVersion = 1.2`, and Tier-G features (mesh shaders, ray tracing, work graphs, `VK_EXT_memory_budget`) stay strictly opportunistic. The amendment only promotes two extensions from Tier-B to Tier-A based on their actual usage pattern in our codebase.

## 3. Consequences

### Immediate (this commit + the 2026-04-20 late-night amendment)
- `VulkanDevice::capabilities()` returns `const RHICapabilities&` — callers can read but not mutate.
- `VulkanDevice` fails loudly at construction if *baseline* capabilities are missing. Baseline = timeline_semaphores, descriptor_indexing, synchronization_2, dynamic_rendering (post-amendment §2.1). Missing Tier-B / Tier-G capabilities are reported as `false` in `caps_` and produce no error.
- The frame-graph barrier emit dropped its `has_sync2()` guard; sync2 presence is a `VulkanDevice` construction invariant.
- Existing call sites (`sandbox/main.cpp`, `editor/app/editor_app.cpp`) continue to work — they construct their own Vulkan device / instance and do not route through `VulkanDevice`, so their status is unchanged. A follow-up task (tracked against Phase 8 week 043) will consolidate onto `VulkanDevice`; see `audit-2026-04-20 §8`.

### Short term (Phase 8 week 043–045)
- `editor/app/editor_app.cpp` and `sandbox/main.cpp` migrate to `VulkanDevice` so they too inherit the capability probe.
- `P1-2` (`VK_EXT_memory_budget` integration) lands on top of `ext_memory_budget`.
- Reopen §2.1 if (and only if) we add cross-hardware CI that doesn't advertise sync2 or dynamic_rendering. That reopening is the one trigger that makes the sync1 + legacy-render-pass backend a Tier-G work item.

### Long term
- Tier-G features (mesh shaders, ray tracing, work graphs) each add one boolean to `RHICapabilities` and one branch point in the frame graph. No other subsystem touches these strings directly.
- Validation layers + capability probe combine to produce a deterministic cross-device test matrix — every CI runner reports its `RHICapabilities` into the run log, and divergence is a ticketable event rather than a "works on my machine" mystery.

### Alternatives considered

1. **Inline checks at every call site.** Rejected: this is exactly the "`if (mesh_shaders) …` strewn through shading code" pattern `docs/03 §4` tells us to avoid.

2. **Runtime dispatch tables (pimpl-per-feature).** Rejected for now as premature. The number of optional features is small enough that a flat struct of booleans is simpler, and the cost of a branch in the recording path is dominated by the `vkCmd…` call overhead anyway.

3. **Extension-string-based queries at the call site.** Rejected: strings are typo-prone (audit finding P1-3 was exactly this — `VK_EXT_dynamic_rendering` instead of `VK_KHR_dynamic_rendering`). Capabilities are the single source of truth; strings live only inside the probe.

## 4. References

- Vulkan spec — [VkPhysicalDeviceProperties](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceProperties.html)
- [VK_KHR_synchronization2](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_synchronization2.html)
- [VK_KHR_dynamic_rendering](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_dynamic_rendering.html)
- [VK_EXT_memory_budget](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_memory_budget.html)
- `docs/05_RESEARCH_BUILD_AND_STANDARDS.md §4` — capability gates
- `audit-2026-04-20 §P1-1` — finding
- `CLAUDE.md` non-negotiable #2

---

*Drafted by Claude Opus 4.7 on 2026-04-20 as part of the audit Round-1 tail.
Doctrine lands before code per `CLAUDE.md` non-negotiable #20.*

---

## ADR file: `adr/0004-ecs-world.md`

## ADR 0004 — ECS World (archetype-of-chunks, ComponentRegistry, queries, hierarchy)

- **Status:** Accepted
- **Date:** 2026-04-20 (late-night; Phase-7 fullstack Path-A)
- **Deciders:** Cold Coffee Labs engine group (Claude Opus 4.7 drafting; founder final)
- **Supersedes:** —
- **Superseded by:** —
- **Related:** `audit-2026-04-20 §P1-11` (Phase-3 drift); `docs/05 §Phase-3 amendment note`; `CLAUDE.md` non-negotiables #5 (no raw `new`/`delete` in engine), #9 (no STL in hot paths — we will qualify this), #20 (doctrine first); `docs/04_SYSTEMS_INVENTORY.md §ECS`; `docs/05_RESEARCH_BUILD_AND_STANDARDS.md §Scene representation`.

## 1. Context

Phase 3 weeks 014–015 nominally delivered the ECS: generational entity handles, archetype chunk storage, typed queries, system registration. The audit on 2026-04-20 (P1-11) found this is false. Today the tree has:

- `engine/ecs/chunk.{hpp,cpp}` — one `Chunk` per archetype, fixed slot count, byte-array per `ComponentType` slot.
- `engine/ecs/archetype.hpp` — 32-bit bitmask identity.
- `engine/ecs/component.hpp` — abstract `ComponentStorage<T>` with `for_each` declared-not-defined.
- `engine/ecs/system.{hpp,cpp}` — `SystemRegistry::register_system<T>` with an inverted `static_assert(std::is_base_of_v<T, System>, …)` (won't compile if instantiated).
- `engine/ecs/entity.hpp` — `EntityHandle = uint32_t`; contradicts `editor/selection/selection_context.hpp` which uses `uint64_t`.
- No `World` type, no `ComponentRegistry`, no queries, no multi-chunk storage, no entity migration, no tests.

*Editor v0.1* (Phase 7) is the first subsystem that needs a real ECS: the outliner walks the scene tree, the inspector binds to components, PIE enter/exit needs to snapshot the world (requires serialization → requires a world to serialize). The correction is a ground-up ECS-World implementation folded into Phase 7 under Path-A (see `docs/05 §Phase-3 amendment`).

This ADR fixes the design decisions so implementation doesn't re-litigate them mid-flight.

## 2. Decision

### 2.1 Top-level shape: `gw::ecs::World` owns everything

```cpp
namespace gw::ecs {

class World {
public:
    World();
    ~World();

    World(const World&)            = delete;
    World& operator=(const World&) = delete;
    World(World&&) noexcept;
    World& operator=(World&&) noexcept;

    // Entity lifecycle
    [[nodiscard]] Entity create_entity();
    void                 destroy_entity(Entity);
    [[nodiscard]] bool   is_alive(Entity) const noexcept;

    // Component add/remove (triggers archetype migration)
    template <typename T>
    T& add_component(Entity, T value);
    template <typename T>
    void remove_component(Entity);
    template <typename T>
    [[nodiscard]] bool has_component(Entity) const noexcept;
    template <typename T>
    [[nodiscard]] T* get_component(Entity);       // nullptr if missing
    template <typename T>
    [[nodiscard]] const T* get_component(Entity) const;

    // Queries — see §2.5
    template <typename... Cs, typename Fn>
    void for_each(Fn&& fn);

    // Bulk access for serialization / editor inspection
    [[nodiscard]] std::size_t entity_count() const noexcept;
    void visit_entities(std::function<void(Entity)>) const;   // stable order
    [[nodiscard]] const ComponentRegistry& component_registry() const noexcept;

    // Access internal archetype table (reflection / inspector / serializer only)
    [[nodiscard]] const ArchetypeTable& archetype_table() const noexcept;

private:
    ComponentRegistry registry_;
    ArchetypeTable    archetypes_;       // all archetypes, owned here
    EntityTable       entity_table_;     // generational slot table
    // more implementation-private state — see §2.3
};

} // namespace gw::ecs
```

`World` is the only public entry point. Systems, queries, and serialization all go through it. No global state.

### 2.2 `Entity` (handle) — 64-bit, replacing the 32-bit `EntityHandle`

```cpp
struct Entity {
    std::uint64_t bits = 0;   // 0 == null

    [[nodiscard]] std::uint32_t index()      const noexcept { return static_cast<std::uint32_t>(bits); }
    [[nodiscard]] std::uint32_t generation() const noexcept { return static_cast<std::uint32_t>(bits >> 32); }

    [[nodiscard]] bool is_null() const noexcept { return bits == 0; }
    [[nodiscard]] bool operator==(Entity) const noexcept = default;

    static constexpr Entity null() noexcept { return {0}; }
};
static_assert(sizeof(Entity) == 8);
```

- **Layout:** `{generation:32, index:32}` packed into `bits`. Generation 0 is reserved to mean "never used / destroyed", so a freshly-created entity starts at generation 1. This makes `Entity{0}` the unambiguous null handle.
- **Editor integration:** `editor/selection/selection_context.hpp` currently uses its own `uint64_t EntityHandle`. It will be retypedef'd as `using EntityHandle = gw::ecs::Entity;` in the same PR that lands World. The 32-bit `gw::ecs::EntityHandle` in `engine/ecs/entity.hpp` is replaced; callers that relied on the old enum-indexed `ComponentType` switch to the component-registry path (§2.4).
- **Why 64-bit?** 32-bit generation + 32-bit index. The index is a table row number in `EntityTable`; the generation is bumped on destroy. 32 bits of index supports 4 billion entities — well past any realistic world size. 32 bits of generation makes handle-reuse bugs effectively impossible on human timescales. The editor's pre-existing choice of `uint64_t` is retroactively correct.

### 2.3 Storage: **archetype-of-chunks**

- An **Archetype** is uniquely identified by its sorted `std::vector<ComponentTypeId>` (the canonical component set). An `ArchetypeId` is a small index (registered monotonically in an `ArchetypeTable`).
- Each Archetype owns **N Chunks**. A Chunk is a fixed-size block (default **16 KiB**) partitioned into parallel per-component arrays (SOA). Slot count per chunk is determined by the archetype's total per-entity component footprint: `slots_per_chunk = floor(chunk_bytes / sum(component_size))`, clamped to `[1, UINT16_MAX]`.
- A chunk stores entities densely: indices `0..live_count-1` are live; `live_count..slots_per_chunk-1` are free. No per-slot free-list within a chunk — on destroy, the last live entity is swapped into the freed slot (O(1), preserves density). This requires updating the swapped entity's `EntityTable` row.
- A new chunk is allocated when all current chunks of an archetype are full. Chunks are never deallocated while the archetype exists — memory grows; compaction is a Phase-8+ concern.
- Per-entity metadata (archetype-id, chunk-index, slot-index) lives in the **EntityTable**, indexed by `Entity.index()`. Layout:

```cpp
struct EntitySlot {
    // Monotonically-increasing version counter. Bumped on *both* create and
    // destroy so a handle captured pre-destroy never satisfies is_alive again,
    // even after its slot is reused.
    std::uint32_t generation   = 0;
    std::uint32_t archetype_id = std::numeric_limits<std::uint32_t>::max();
    std::uint16_t chunk_index  = std::numeric_limits<std::uint16_t>::max();
    std::uint16_t slot_index   = std::numeric_limits<std::uint16_t>::max();
    // True iff the slot currently represents a live entity. Distinct from
    // `generation` so destroy can advance generation (defeating stale handles)
    // without reusing 0 as a "free" marker — 0 stays reserved for Entity::null().
    bool          alive        = false;
};
```

That's 16 bytes per live-or-freed entity slot (12 meaningful + 1 flag + 3 padding on common ABIs). `EntityTable` is a `std::vector<EntitySlot>` (grows; never shrinks). **Freed indices are re-used via a `std::vector<std::uint32_t> free_list_`** — grab from the back of the list in O(1). If the free list is empty, push a new slot.

**Generation-bump rule (2026-04-21 amendment).** Both `create_entity` and `destroy_entity` bump `generation` (skipping 0 on wrap). Without destroy-side bumping, a destroy→create sequence on the same slot left the fresh handle's generation equal to the destroyed one, silently re-validating stale copies. The `alive` flag is the authoritative liveness bit; generation is the version stamp used for stale-handle rejection. See `tests/unit/ecs/world_test.cpp "destroy_entity invalidates the handle; reuse bumps generation"`.

### 2.4 `ComponentRegistry`

Registers every component type at program start (usually before `World` construction, but re-registering an already-registered type is idempotent). Each registration records:

```cpp
struct ComponentTypeInfo {
    ComponentTypeId    id;                    // monotonic small int (u16)
    std::size_t        size        = 0;
    std::size_t        alignment   = 0;
    bool               trivially_copyable = false;
    std::uint64_t      stable_hash = 0;       // fnv1a of fully-qualified type name

    // Non-trivial lifecycle hooks (only populated when !trivially_copyable).
    void (*copy_construct)(void* dst, const void* src) = nullptr;
    void (*move_construct)(void* dst, void* src) noexcept = nullptr;
    void (*destruct)(void* p) noexcept = nullptr;

    // Optional: pointer to the reflection TypeInfo if the type has GW_REFLECT.
    // Populated via ADL lookup of `describe(T*)`. Null for non-reflected types.
    const gw::core::TypeInfo* reflection = nullptr;

    // Stable debug name (never freed; points into static storage).
    std::string_view debug_name;
};

class ComponentRegistry {
public:
    template <typename T>
    ComponentTypeId register_component();            // idempotent; returns canonical id

    template <typename T>
    [[nodiscard]] ComponentTypeId id_of() const;     // throws if not registered

    [[nodiscard]] const ComponentTypeInfo& info(ComponentTypeId) const;
    [[nodiscard]] std::size_t component_count() const noexcept;

    // Stable-hash → id lookup (used by serialization).
    [[nodiscard]] std::optional<ComponentTypeId> lookup_by_hash(std::uint64_t) const;
};
```

- **`id_of<T>()`** uses a per-type static `ComponentTypeId` storage cached on first lookup, zero-cost thereafter.
- `stable_hash` is FNV-1a of the fully-qualified type name obtained from a compile-time `gw_type_name<T>()` helper (same trick `editor/reflect/reflect.hpp` uses). It is the **serialization key**, not `id` — `id` is not portable across builds.
- `trivially_copyable` is `std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>`. When true, entity migration / serialization can use `std::memcpy`; when false, the function-pointer hooks are used.
- `ComponentRegistry` owns the `ComponentTypeInfo` storage (one `std::vector` + one `std::unordered_map` for name-hash lookup).
- **Registration location:** each component type provides `GW_ECS_COMPONENT(T)` at its definition site (a macro that expands to `inline gw_ecs_register_component<T>()`-style static registration via a TU-local constructor). The specifics live in the implementation — the ADR just freezes the surface.

### 2.5 Queries

```cpp
// Compile-time assembled query: iterate every archetype that contains
// every listed component, invoking `fn(Entity, Cs&...)` for each live slot.
template <typename... Cs, typename Fn>
void World::for_each(Fn&& fn);
```

- Implementation: find `archetype_ids` whose component set is a **superset** of `{id_of<Cs>()...}`. Walk matching archetypes → walk their chunks → walk their live slots → gather the N component pointers from per-chunk SOA arrays and call `fn`.
- Queries do **not** cache; each call re-walks the archetype table. Phase-8+ can add a `QueryCache` keyed on the component-set mask if profiling shows this is hot. Today the world sizes we care about (editor scenes, PIE sessions) have O(10³) entities max — walking archetypes is negligible.
- **No `With<A, B> / Without<C>`-style filter DSL in this ADR.** `for_each<A, B>` means "entities with at least A and B." If we need exclusion later we add `for_each_without<...>`.
- **No parallel queries in this ADR.** Single-threaded only. Parallel-query dispatch via `engine/jobs/` is a Phase-11+ item; it needs archetype-level locks and we don't have the use case yet.
- **Query stability under add/remove.** Iterating while adding/removing components is **undefined**. Callers defer structural changes with a `World::defer_*` API (Phase-8 addition) or collect-then-apply. Today the editor doesn't mutate during iteration, so we don't need the deferred API yet — but we **document** the invariant.

### 2.6 Entity migration (add/remove component)

Adding or removing a component on a live entity moves it from its current archetype `A` to the destination archetype `B = A ∪ {T}` or `B = A \ {T}`:

1. Look up or lazily create archetype `B` in `ArchetypeTable`.
2. Allocate a slot in a chunk of `B`.
3. For each component type in `A ∩ B`: copy (trivial → `memcpy`, non-trivial → `move_construct`) from the source chunk to the destination chunk. Destruct the source (non-trivial only).
4. For the added component (add path): construct in the destination using the caller-supplied value.
5. Compact the source chunk: swap-back the last-live-entity into the freed slot; update its `EntityTable` row.
6. Update this entity's `EntityTable` row with the new `{archetype_id, chunk_index, slot_index}`. Generation unchanged.

### 2.7 Hierarchy (scene tree)

Hierarchy is represented as a **dedicated component**, not a field on `TransformComponent`. This is the Unity/Unreal-ish choice: composition > baked-in-transform.

```cpp
struct HierarchyComponent {
    Entity parent       = Entity::null();
    Entity first_child  = Entity::null();
    Entity next_sibling = Entity::null();
    // Leaf if first_child.is_null(); root if parent.is_null().
};
```

**Why a separate component?**

1. Entities can participate in the hierarchy without having a Transform (folder entities, metadata entities, scene-root markers).
2. The archetype query `for_each<HierarchyComponent>` cleanly iterates only hierarchy-participating entities; `for_each<TransformComponent>` iterates only spatial ones.
3. Adding/removing an entity from the hierarchy = add/remove the component. No need for "hierarchy disabled" bit inside Transform.

**Traversal API** lives on `World` as non-templated helpers:

```cpp
[[nodiscard]] std::vector<Entity> children_of(Entity) const;        // convenience
void reparent(Entity child, Entity new_parent);                     // handles linked-list fixup
void visit_descendants(Entity root, std::function<void(Entity)>) const;   // DFS
void visit_roots(std::function<void(Entity)>) const;                // entities whose HierarchyComponent.parent.is_null()
```

Cycle detection (`reparent(A, descendant-of-A)` rejected) is the responsibility of `reparent`. It returns a `bool`/`Result`; the editor's reparent command handles the failure path.

### 2.8 Systems

**Deferred to Phase 8+.** The broken `SystemRegistry` is deleted in the same PR as World lands; gameplay code (the `greywater_gameplay.dll` module + engine-internal subsystems that care about ECS — e.g. the outliner rebuild path) iterate the World directly via `for_each`. Adding a `System` abstraction that manages scheduling and read/write declarations is a separate design problem with its own ADR when we actually have multiple interacting systems.

The Phase-3 week-015 card ("typed queries, system registration") is split: **queries ship here; system registration ships in a future ADR.**

### 2.9 Memory policy — STL usage

`CLAUDE.md` NN #9 forbids STL containers in engine *hot paths*. The ECS:

- Uses `std::vector` / `std::unordered_map` / `std::span` / `std::function` liberally in management surfaces (entity table, archetype table, registry, query dispatch).
- Does **not** use STL containers per-entity or per-component. Chunks are byte-array allocations; SOA arrays are raw pointers into that buffer.
- Does **not** allocate per-`for_each`-iteration. Query dispatch walks cached archetype-id vectors and passes raw component pointers to the callback.

This is consistent with how `engine/jobs/` uses `std::vector` for worker-count-sized lists but not per-job: NN #9 is about *hot-path per-frame per-element allocations*, not about using `std::vector` for fixed-size management metadata. The hot path here is "walk N slots, call `fn`" — zero allocations, no STL traversal.

### 2.10 Thread-safety

Single-threaded. `World` is not safe for concurrent access. This is a documented invariant, not a latent bug: PIE runs single-threaded today, the editor runs single-threaded today, and the asset loader / cooker touch assets, not the ECS world.

If we ever need parallel queries, they get a dedicated ADR.

## 3. Consequences

### Immediate (this commit block)

- `engine/ecs/world.{hpp,cpp}`, `engine/ecs/component_registry.{hpp,cpp}`, `engine/ecs/entity.hpp` (rewritten), `engine/ecs/archetype.{hpp,cpp}` (rewritten — no more bitmask, replaced with sorted-type-id vector + `ArchetypeTable`) land in `engine/ecs/`.
- `engine/ecs/chunk.{hpp,cpp}` are deleted or folded into `archetype.cpp` — the single-archetype `Chunk` primitive has no role once `Archetype` owns multi-chunk storage directly.
- `engine/ecs/system.{hpp,cpp}` are **deleted** (broken static_assert, no consumers). System registration is re-addressed in a later ADR.
- `engine/ecs/component.{hpp,cpp}` are **deleted** (abstract `ComponentStorage<T>` with undefined `for_each` has no consumers and no working implementation).
- `editor/selection/selection_context.hpp` retypedefs `EntityHandle = gw::ecs::Entity`.
- Hook-site fallout: audit for every mention of `gw::ecs::EntityHandle` / `gw::ecs::ComponentType` / `gw::ecs::SystemRegistry` / `gw::ecs::Chunk` / `gw::ecs::ComponentStorage` in the tree and migrate. Evidence so far: those types are almost entirely self-referential within `engine/ecs/` and untouched by consumers — migration is local.
- ECS tests land under `tests/unit/ecs/`: entity create/destroy, add/remove component → archetype migration, query iteration, hierarchy reparent, handle staleness under re-use, 10 000-entity stress.

### Short term (Phase 7 remainder)

- Outliner walks `World::visit_roots` + `World::visit_descendants`.
- Inspector reads components via `World::get_component<T>` + `ComponentRegistry::info(id).reflection` → `WidgetDrawer::draw_field`.
- PIE snapshot/restore uses `ComponentRegistry.stable_hash` as the type key (ADR-0007 — ECS serialization).

### Long term

- Deferred structural changes (`World::defer_destroy` / `defer_add` / `defer_remove` with a flush point) come in the first PR that needs to mutate during iteration.
- Parallel queries + System scheduling (Phase 11+) get their own ADR.
- Archetype compaction (reclaim empty chunks) is a Phase-11+ memory-hygiene item.
- `QueryCache` is added only if profiling shows archetype walks dominating frame time (unlikely below ~10⁴ archetypes).

### Alternatives considered

1. **Sparse-set ECS (EnTT style).** Rejected. EnTT's per-component sparse arrays are excellent for cache-coherent iteration of a single component, but archetype iteration (multi-component queries, which we do constantly in rendering) costs extra indirection. Unity's DOTS / Bevy's approach of archetype-of-chunks is a better fit for our usage profile and gives us a natural serialization granularity.

2. **Hash-map-per-component (CEDR-like, handle → value).** Rejected. Scales poorly past ~10⁴ entities, cache-hostile, and offers no natural archetype to hang serialization and snapshot tests on.

3. **Bitmask archetype identity (what the old `archetype.hpp` did).** Rejected at 32 bits (max 32 component types — will trip before end of game). At 128/256 bits it works but is less ergonomic than sorted-type-id vector + hash — the vector gives stable iteration order for free, which serialization needs.

4. **Keep `EntityHandle = uint32_t`, widen editor to match engine.** Rejected. The editor's `uint64_t` is the right choice (more generation bits = fewer stale-handle bugs); engine's `uint32_t` was a premature optimization at a time when we had no profile data suggesting handle size mattered.

5. **Hierarchy as a field on `TransformComponent`.** Rejected per §2.7.

6. **Ship queries with a filter DSL (`With<A> & Without<B>`) now.** Rejected as premature; YAGNI until we have a call site.

## 4. References

- `audit-2026-04-20 §P1-11`
- `docs/02_ROADMAP_AND_OPERATIONS.md §Phase-3 amendment`
- Unity DOTS archetype memory model — [ECS Concepts: Archetype](https://docs.unity3d.com/Packages/com.unity.entities@1.3/manual/concepts-archetypes.html)
- Bevy ECS book — [Archetypes, tables, sparse sets](https://bevy-cheatbook.github.io/programming/ecs-intro.html)
- Our own `docs/04_SYSTEMS_INVENTORY.md §ECS` — general framing
- `CLAUDE.md` non-negotiables #5, #9, #20

---

*Drafted by Claude Opus 4.7 on 2026-04-20 late-night as part of the Phase-7 fullstack Path-A push. Doctrine lands before code per `CLAUDE.md` non-negotiable #20. Implementation tracked via the todo list on the same session.*

---

## ADR file: `adr/0005-editor-command-stack.md`

## ADR 0005 — Editor CommandStack (undo / redo, transactions, coalescing)

- **Status:** Accepted
- **Date:** 2026-04-20 (late-night; Phase-7 fullstack Path-A)
- **Deciders:** Cold Coffee Labs engine group (Claude Opus 4.7 drafting; founder final)
- **Supersedes:** the never-delivered Phase-3 week-011 `engine/core/command/` `CommandStack` (see `docs/05 §Phase-3 amendment`).
- **Superseded by:** —
- **Related:** `audit-2026-04-20 §P1-11`; ADR-0004 (ECS-World — the primary target of command write-through); `CLAUDE.md` non-negotiables #5 (no raw `new`/`delete` — use `std::unique_ptr`), #20 (doctrine first).

## 1. Context

Phase-3 week 011 promised `engine/core/command/{ICommand,CommandStack,transaction brackets}`. The directory landed with only a `.gitkeep`; no code. `editor/panels/inspector_panel.cpp` imports a `SetComponentFieldCommand<T>` and a `ReparentEntityCommand`, suggesting there's *some* command infrastructure already — but the implementation of those commands captures the post-edit value on both `before` and `after` sides (`inspector_panel.cpp:55-60`), meaning undo restores the post-edit state, which is no undo at all.

*Editor v0.1* needs real transactional edits:

- Every Inspector field edit should be undoable (typed values, gizmo drags, component add/remove).
- Rapid drags (gizmo, drag-float fields) should coalesce into a single undoable transaction.
- Multi-field edits ("set position and rotation at once") should group.
- Ctrl+Z / Ctrl+Y bindings; the Edit menu shows the next undo/redo labels.
- The stack has a memory cap; old entries drop when it's exceeded.

The Phase-3 location (`engine/core/command/`) turned out to be wrong in hindsight: the only consumers are the editor and, prospectively, BLD's transaction hooks (which are themselves editor-scoped). Putting this in `engine/core/` would drag gameplay code into a dependency it doesn't need. Editor-scoped is the right home.

## 2. Decision

### 2.1 Location

`editor/undo/command_stack.{hpp,cpp}` (+ `editor/undo/commands/` for concrete command types as they accrete). The `engine/core/command/` directory stays empty / deleted; if an engine-side command surface ever becomes necessary (serialized BLD actions, replay, deterministic-sim rewind), it gets its own ADR.

### 2.2 `ICommand` interface

```cpp
namespace gw::editor::undo {

class ICommand {
public:
    virtual ~ICommand() = default;

    // Must be symmetric: do() followed by undo() returns state to the pre-do snapshot.
    virtual void do_()   = 0;
    virtual void undo()  = 0;

    // Human-readable label for the Edit menu and the undo-stack panel.
    [[nodiscard]] virtual std::string_view label() const = 0;

    // Approximate heap footprint in bytes for memory-cap bookkeeping.
    // Must return a conservative upper bound; used to decide when to drop
    // oldest entries when the stack exceeds its size budget.
    [[nodiscard]] virtual std::size_t heap_bytes() const noexcept = 0;

    // Coalescing: if this command would logically merge with `other` (same
    // target, compatible operation, recent enough by `time`), absorb other's
    // terminal state and return true. Called by CommandStack::push when
    // coalescing is on and the previous command is of a compatible type.
    //
    // Default: no coalescing.
    [[nodiscard]] virtual bool try_coalesce(const ICommand& /*other*/,
                                            std::chrono::steady_clock::time_point /*now*/) {
        return false;
    }
};

} // namespace gw::editor::undo
```

`do_` has an underscore because `do` is a C++ keyword.

### 2.3 `CommandStack`

```cpp
namespace gw::editor::undo {

class CommandStack {
public:
    struct Config {
        std::size_t memory_budget_bytes = 16 * 1024 * 1024;  // 16 MiB default
        std::chrono::milliseconds coalesce_window{120};      // ≈8 frames @ 60fps
    };

    explicit CommandStack(Config cfg = {});
    ~CommandStack();

    // Push a new command. Executes do_() synchronously. Clears the redo stack.
    // Attempts coalescing with the top of the undo stack when enabled.
    void push(std::unique_ptr<ICommand> cmd);

    // Undo / redo. No-op if respective stack is empty.
    void undo();
    void redo();

    [[nodiscard]] bool can_undo() const noexcept;
    [[nodiscard]] bool can_redo() const noexcept;

    // Labels for menu display; empty string if stack is empty.
    [[nodiscard]] std::string_view next_undo_label() const noexcept;
    [[nodiscard]] std::string_view next_redo_label() const noexcept;

    // Group a sequence of pushes into a single undoable unit.
    // Nesting allowed; outermost begin/end pair is the boundary.
    void begin_group(std::string_view label);
    void end_group();

    // Explicit coalescing control (default: on).
    void set_coalescing_enabled(bool) noexcept;

    // Stats for debug overlay.
    [[nodiscard]] std::size_t undo_depth()  const noexcept;
    [[nodiscard]] std::size_t redo_depth()  const noexcept;
    [[nodiscard]] std::size_t memory_used() const noexcept;

    void clear();

private:
    // Implementation: two std::vector<std::unique_ptr<ICommand>> (undo, redo),
    // a GroupCommand aggregator during begin/end, running memory-bytes counter,
    // time-of-last-push for coalesce-window gating.
};

} // namespace gw::editor::undo
```

### 2.4 Transaction semantics

- **`push(cmd)`**
  1. If a group is open (`group_depth_ > 0`), the command is buffered into the current `GroupCommand`, not the stack directly. `do_` is still called.
  2. Else: call `cmd->do_()`; attempt coalescing with the top of the undo stack (if the top is of the same concrete type *and* `top->try_coalesce(*cmd, now)` returns true *and* `now - last_push_time_ ≤ coalesce_window`); if coalesced, replace; else push onto undo stack.
  3. Clear the redo stack.
  4. Trim from the front of the undo stack while `memory_used > memory_budget_bytes`.
  5. Record `last_push_time_ = now`.

- **`undo`**
  1. If undo stack empty, no-op.
  2. Pop top; call `.undo()`; push onto redo stack.
  3. Reset `last_push_time_` to a far-past sentinel so the next `push` cannot coalesce with the popped command.

- **`redo`**
  1. If redo stack empty, no-op.
  2. Pop top; call `.do_()`; push onto undo stack.
  3. Reset `last_push_time_` likewise.

- **`begin_group(label)` / `end_group()`**
  - Maintain `group_depth_`; create a fresh `GroupCommand` on depth transition 0 → 1.
  - Buffered commands are owned by the group.
  - `end_group()` on depth transition 1 → 0 pushes the group onto the undo stack as a single command. Empty groups are discarded.
  - `group_depth_ > 0` disables coalescing with prior stack entries (within a group we coalesce *within the group only*, which is the `GroupCommand`'s job to handle).

### 2.5 `GroupCommand`

A built-in `ICommand` implementation that owns a `std::vector<std::unique_ptr<ICommand>>` and forwards `do_`/`undo` (undo in reverse order). `heap_bytes()` returns the sum of children + container overhead. `label()` is the caller-supplied group label. Does not support coalescing with other groups (returns false).

### 2.6 Coalescing — the drag case

The canonical coalesce case is **drag on a drag-float / gizmo**: each mouse-move emits a `SetComponentFieldCommand<float>` or `TransformEntityCommand`. Without coalescing the undo stack would collect hundreds of micro-changes, one per mouse-move frame. With coalescing they merge into a single undoable edit from *pre-drag value* to *post-release value*.

**Coalescing rule in concrete commands:**

- `try_coalesce(other, now)` returns true iff:
  1. `other` is same concrete type (same `T` for `SetComponentFieldCommand<T>`, same entity for `TransformEntityCommand`).
  2. Same target (same field address / same entity + same component).
  3. `now - self.timestamp <= window` (the stack gates this too; belt-and-braces).
  4. `self.before` value is preserved; `self.after` is replaced with `other.after`. The command's `do_()` should be idempotent — re-applying the updated `after` from the coalesced state is equivalent to re-playing every step.

- Commands that are not idempotent under value-replacement (add/remove component, reparent, create/destroy entity) **return false** unconditionally from `try_coalesce`.

### 2.7 Concrete commands landed in this PR block

- `SetComponentFieldCommand<T>` — one field on one component on one entity. Stores `{Entity, ComponentTypeId, field_offset, before: T, after: T}`. Coalesces on same `{entity, component, field_offset}`.
- `AddComponentCommand<T>` — add a default-constructed or caller-provided `T` to an entity. Not coalescing.
- `RemoveComponentCommand<T>` — snapshot the component's current state via `ComponentRegistry::info(id).reflection` (or raw `memcpy` for trivial types) before removal; undo re-adds from the snapshot. Not coalescing.
- `CreateEntityCommand` / `DestroyEntityCommand` — paired destroy/create; `DestroyEntityCommand` snapshots every component via the serialization path (ADR-0007). Not coalescing.
- `ReparentEntityCommand` — stores `{child, old_parent, new_parent}`; the existing inspector call site gets wired up (its callback was a no-op per the audit). Not coalescing.

Additional commands (multi-select edits, asset-field drops, etc.) accrete later; they implement `ICommand` with the same shape.

### 2.8 Input bindings (Ctrl+Z / Ctrl+Y)

Handled in the editor's global shortcut layer (`editor/app/editor_app.cpp` or a dedicated `editor/input/shortcuts.cpp` if this layer grows). `CommandStack` itself is keyboard-unaware.

### 2.9 Memory policy

- Memory is accounted via `ICommand::heap_bytes()`. Trim policy is FIFO (drop oldest from front of undo stack) until `memory_used <= memory_budget`. The redo stack is not trimmed explicitly — it's always emptied on a fresh push.
- `std::unique_ptr<ICommand>` is the ownership primitive. No raw `new`/`delete` in consumer code (NN #5): a helper `make_command<Cmd>(args...)` wraps `std::make_unique`. Consumers call `stack.push(make_set_field_command<float>(...))`.

### 2.10 Out of scope (explicitly)

- **Persistent undo across editor sessions.** Not useful for Editor v0.1; Phase-8+.
- **Replay / determinism.** `CommandStack` is for UI actions, not for sim state. BLD's transaction hooks (Phase 9) will probably reuse `ICommand` but will have their own Phase-9 ADR; today we only need editor undo.
- **Multi-user editing / OT / CRDT.** Out of scope for the entire project unless Greywater grows a multiplayer-editor story.
- **Asynchronous commands.** Every command runs synchronously on the editor thread; `do_()` / `undo()` may not suspend. If something must happen async (a long asset import), it completes first, then a synchronous command records the result.

## 3. Consequences

### Immediate (this commit block)

- `editor/undo/command_stack.{hpp,cpp}`, `editor/undo/commands/*.{hpp,cpp}` land.
- `editor/panels/inspector_panel.cpp`'s existing `SetComponentFieldCommand` inline usage is replaced with the ADR-shaped command; its `before` capture is corrected to snapshot *before* the widget overwrites (the current sequence reads post-value-both-sides, which is why undo is effectively broken).
- `editor/app/editor_app.cpp` owns one `CommandStack` in `EditorContext`; Ctrl+Z / Ctrl+Y bound in the global shortcut layer.
- Unit tests: `tests/unit/editor_undo/` — push/undo/redo round-trip, coalesce gating (same target, time window), group semantics (begin/end, nesting), memory-budget trim, redo clear on fresh push.

### Short term (Phase 7 remainder)

- Every inspector write goes through a command. Gizmo drags emit coalesced `TransformEntityCommand`s. Outliner reparent/delete use `ReparentEntityCommand` / `DestroyEntityCommand` with real callbacks (not the no-op the audit found).
- Viewport-initiated transform gizmo → coalesced transform command stream — one undo per release.

### Long term

- When BLD (Phase 9) needs to record authored actions, it layers on top of `ICommand`. Whether BLD commands live on the same stack or a dedicated one is deferred to the BLD ADR.
- If we ever need sim-state rewind (replay, determinism testing), a separate `engine/core/replay/` subsystem can pattern-match the API but is explicitly not the same code — that ADR is not this ADR.

### Alternatives considered

1. **Snapshot-diff undo (store full pre/post world snapshots, no command objects).** Rejected as too memory-heavy and too coarse — a single field edit would snapshot the whole world.

2. **Event-sourced model (everything is a command; render state is a pure function of the command log).** Rejected as far too much machinery for a UI-edit undo stack. Worth reconsidering *only* if BLD Phase 9 demands it, and then with a fresh ADR.

3. **Land this in `engine/core/command/` as Phase-3 originally planned.** Rejected. The only consumers are editor + prospective BLD (editor-scoped). Engine core should not know about undo stacks; it would cause spurious deps.

4. **Use QUndoStack / a third-party library.** We're not on Qt; no third-party C++ undo library is worth the dep for the shape we need. Rolled our own.

5. **Auto-coalesce on type alone (Unity-style).** Rejected. Without the time-window gate, coalescing fires across unrelated edits of the same type (two separate transform edits on two entities over several seconds would merge into one undoable). Time-window + same-target is the minimum correct gate.

## 4. References

- `audit-2026-04-20 §P1-11`
- `docs/05 §Phase-3 amendment`
- ADR-0004 (ECS-World)
- `editor/panels/inspector_panel.cpp:42-88` — current broken command flow
- `CLAUDE.md` non-negotiables #5, #20

---

*Drafted by Claude Opus 4.7 on 2026-04-20 late-night as part of the Phase-7 fullstack Path-A push. Doctrine lands before code per `CLAUDE.md` non-negotiable #20.*

---

## ADR file: `adr/0006-ecs-world-serialization.md`

## ADR 0006 — ECS World Serialization (binary, versioned; PIE snapshot format)

- **Status:** Accepted
- **Date:** 2026-04-20 (late-night; Phase-7 fullstack Path-A)
- **Deciders:** Cold Coffee Labs engine group (Claude Opus 4.7 drafting; founder final)
- **Supersedes:** the never-delivered Phase-3 week-016 serialization framework (`engine/core/serialization.cpp` remains a 10-line stub; see `docs/05 §Phase-3 amendment`).
- **Superseded by:** —
- **Related:** ADR-0004 (ECS-World — the thing being serialized); ADR-0005 (CommandStack — `DestroyEntityCommand` needs the single-entity snapshot path); `audit-2026-04-20 §P1-11`; `CLAUDE.md` non-negotiables #5 (no raw `new`/`delete`), #8 (`std::string_view` never to C API), #17 (world-space positions are `float64`); `docs/04_SYSTEMS_INVENTORY.md §Serialization`; `docs/05_RESEARCH_BUILD_AND_STANDARDS.md` (indirectly — asset cooks use a related format).

## 1. Context

Three call sites need to serialize ECS state:

1. **PIE Enter-Play / Exit-Play snapshot.** Before entering play mode, the editor snapshots the entire `World`; after exiting play mode, it restores. This must be **fast** (ideally < 10 ms for a modest editor scene), **lossless**, and **allocation-light**.
2. **Scene file save/load** (Phase 8). A `.gwscene` file on disk carrying one world. Must be **forward-readable** (older-format file on newer engine → migrates), must be **stable enough for VCS diff** (commit-friendly; matters for Greywater's content workflow), must **survive component-struct evolution** (added fields, renamed fields).
3. **`DestroyEntityCommand` undo snapshot** (ADR-0005). Single-entity blob stored in the undo stack; used to re-add components on undo.

A single binary format serves all three. The PIE path is just the full-world variant with the fastest-possible codec. The scene-file path adds chunked write + integrity check. The command-snapshot path is the single-entity subtree with no container framing.

The Phase-3 stub (`engine/core/serialization.{hpp,cpp}`) was trying to be a generic per-type binary serializer keyed on `TypeInfo`. That direction was correct in spirit but never finished. We keep the buffer primitives and build on top.

## 2. Decision

### 2.1 Format family — `.gwblob` (in-memory) / `.gwscene` (on-disk)

A **single binary wire format** with three wrapping modes:

| Mode                | Header         | Where                                    | Compression   |
|---------------------|----------------|------------------------------------------|---------------|
| `pie_snapshot`      | `GWS1` + flags | heap buffer owned by editor              | off           |
| `scene_file`        | `GWS1` + flags | `.gwscene` file on disk                  | zstd optional |
| `entity_snapshot`   | *no header*    | inside an `ICommand` in the undo stack   | off           |

The payload is identical across modes; only the framing differs.

### 2.2 Wire shape (the payload)

All little-endian, all fields aligned to their natural size in the stream (no padding — we pay the price of `memcpy` into/out of a `uint64_t` scratch for misaligned reads, but reject the alternative of "pack then swap" as too easy to get wrong per platform).

```
Payload ::=
  u32        entity_count
  u16        archetype_count
  u16        component_type_count
  ComponentTable[component_type_count]
  ArchetypeTable[archetype_count]
  EntityTable[entity_count]
  Archetype.ChunkData[archetype_count]    (see §2.4)

ComponentTable entry ::=
  u16  component_type_id            (local to this file; maps into the live registry by stable_hash)
  u64  stable_hash                  (ComponentRegistry.stable_hash — the portable key)
  u32  size                         (sizeof(T) at serialize time; used to detect evolution)
  u8   trivially_copyable           (1 if memcpy fast path was used, 0 if reflection path)
  u16  debug_name_len               (≤ 255 in practice; u16 for alignment)
  u8[debug_name_len]  debug_name    (stable name string; for diagnostics only)

ArchetypeTable entry ::=
  u32  archetype_id                 (local to this file)
  u16  component_count
  u16  slots_per_chunk
  u16  chunk_count
  ComponentTypeId[component_count]  (the component set, sorted, local-to-this-file ids)

EntityTable entry ::=
  u64  entity_bits                  (pre-remap generation+index as written out)
  u32  archetype_id                 (local to this file)
  u16  chunk_index
  u16  slot_index

Archetype.ChunkData ::=
  for each chunk in archetype:
    u16  live_count
    // Then, for each component in archetype (in the archetype's stored order):
    //   if trivially_copyable:
    //     u8[live_count * component_size]  raw bytes
    //   else:
    //     ReflectionBlob (see §2.5)
```

- Local `component_type_id` and `archetype_id` values are only valid within the payload; remapping to the live `ComponentRegistry` happens at deserialize time via `stable_hash`. This keeps the format portable across builds where `ComponentRegistry` allocates ids in a different order.
- `entity_bits` is written as-is. On deserialize, we do **not** trust the generation — we rebuild the entity table from scratch. The `entity_bits` field is kept for *diagnostics* and for the single-entity snapshot case where we need to rewrite the new entity's handle into referring components (see §2.6).

### 2.3 Header (modes 1 and 2 only)

```
Header ::=
  u32  magic          'GWS1'  (0x31 0x53 0x57 0x47 on wire → bytes 'G','W','S','1')
  u16  format_version 1
  u16  flags          bit 0 = zstd compressed payload; bits 1..15 reserved
  u64  payload_bytes  (uncompressed size; used for the zstd decompress target buffer)
  u64  payload_crc64  (ISO CRC-64 of uncompressed payload; verify on load)
```

- `entity_snapshot` mode skips the header entirely — it's a blob living inside a command; the command knows the size.
- `pie_snapshot` mode keeps the header but sets `flags.zstd = 0`; the CRC is still computed so we catch editor-state corruption during PIE cycles.
- `scene_file` mode lets the caller choose `flags.zstd`. Default on.

### 2.4 The **fast path** — trivially-copyable components

For any component type where `trivially_copyable = true` (see ADR-0004 §2.4), the serializer emits `memcpy(out, chunk_soa_ptr, live_count * component_size)`. That's one instruction per component array per chunk. The deserializer does the inverse.

This is the PIE-critical path. A scene with 10⁴ entities of a single 32-byte archetype serializes in O(1) syscalls and ~320 KB of bulk `memcpy`. Well under the 10 ms budget.

### 2.5 The **slow path** — reflection (non-trivially-copyable components)

If a component has a non-trivial copy / move / destructor *or* has `ComponentRegistry.reflection != nullptr` and opted in, we serialize per-field using the reflection `TypeInfo`.

```
ReflectionBlob (for one component array in one chunk) ::=
  for each slot in [0, live_count):
    for each field in TypeInfo.fields:
      serialize_field(ctx, field, &component[slot])
```

`serialize_field` for built-in field types (integers, floats, `glm::vec*`, `glm::quat`, `std::string`, `Entity`) is hand-written. For nested reflected structs it recurses. For `std::vector<T>` / `std::array<T>` it writes `u32 count` + element payload. `std::string` is written as `u32 length + utf8_bytes`. **No `std::string_view` crosses the serialization boundary** (NN #8 — unrelated to C APIs here, but same principle: `string_view` is a non-owning lifetime, not a storage type).

**The deserializer tolerates evolution.** If a field is missing from the stream (deleted in newer code), default-construct it. If a field exists in the stream but not in the live type, skip. This requires the stream format to be self-describing per field, so:

```
Field ::=
  u32  field_name_hash     (fnv1a of field name)
  u16  wire_type           (enum: u8/u16/.../f32/f64/vec3/quat/string/entity/struct_begin/struct_end/array)
  u32  payload_bytes       (size of the payload that follows — lets us skip unknown fields)
  u8[payload_bytes]  payload
```

This is more bytes than the fast path but the reflection path is already slower; the per-field overhead (~10 bytes) is acceptable. Evolution-tolerant deserialization is the whole point of the reflection path — if you want max speed, make your component trivially-copyable.

### 2.6 Single-entity snapshot (for `DestroyEntityCommand` undo)

Same payload shape as §2.2 but with `entity_count = 1`, `archetype_count = 1`, and one chunk of `live_count = 1`. The archetype table and component table are cut to only what this entity uses.

On `undo()`, the command calls `World::restore_entity_from_blob(blob)` which:

1. Allocates a fresh `Entity` handle (new generation).
2. Creates the archetype if missing.
3. Populates the component values from the blob.
4. If any component field is of type `Entity` (e.g. `HierarchyComponent.parent`) pointing at the *destroyed* entity, the command's `undo()` uses its stored `{old_entity → new_entity}` map to remap. Today the only `Entity`-valued fields are in `HierarchyComponent`; the command remaps those explicitly.

Whole-world snapshot does not need cross-entity remapping because handles are written and read consistently within the same blob.

### 2.7 File framing for `.gwscene`

```
File ::=
  Header
  (flags.zstd ? zstd_compressed(Payload) : Payload)
```

- `.gwscene` files are not appended to; they're written atomically via temp-file-then-rename (win32 `MoveFileExW(MOVEFILE_REPLACE_EXISTING)`, posix `rename`). No half-written scenes on crash.
- Zstd compression level: 3 (balance). Phase-8 can add a compressor pool if save time gets annoying.

### 2.8 Schema evolution rules

- **Adding a component type:** fine. Old files don't have it; new entities in new code just have it.
- **Removing a component type:** fine. Old files' entries for the dead type are skipped by id (no `stable_hash` match in registry).
- **Adding a field to a reflected component:** fine via §2.5 default-construct-missing.
- **Removing a field from a reflected component:** fine via §2.5 skip-unknown.
- **Renaming a field:** breaks field lookup via `field_name_hash`. A reflection-level annotation (`GW_RENAMED_FROM("old_name")`) can land in a later PR; out of scope here.
- **Changing a field's wire type:** the deserializer detects wire-type mismatch and default-constructs. Document this as "forbidden without migration code"; the engine logs a warning.
- **Changing a trivially-copyable component's layout:** this is the sharp corner. If the `size` field in the component table no longer matches the live type, the fast path cannot be used safely. Deserializer behavior: *if size differs, refuse to load the component array, default-construct the field in-place, log warning*. A migration hook (`gw_ecs_migrate_component<T>(old_bytes, out_new)`) can be registered per-type; out of scope for this ADR, documented as a Phase-8+ follow-up.
- **`format_version` bump (top-level wire shape changed):** deserializer refuses to load versions it doesn't know; a migrator is a separate PR. `GWS1 / format_version = 1` is what we ship now.

### 2.9 Code layout

- `engine/core/serialization.{hpp,cpp}` — **kept** as the low-level buffer primitives (`BinaryWriter`, `BinaryReader`, primitive `write<T>`, `read<T>` for arithmetic, string, vector). The existing 10-line stub is replaced with real code here.
- `engine/ecs/serialize.{hpp,cpp}` — the ECS-specific encoder/decoder: walks `World` to produce a payload, consumes a payload to rebuild a `World`. Public surface:

```cpp
namespace gw::ecs {

enum class SnapshotMode : std::uint8_t {
    PieSnapshot,      // header, no compression
    SceneFile,        // header, optional compression
    EntitySnapshot,   // no header
};

[[nodiscard]] std::vector<std::uint8_t>
save_world(const World&, SnapshotMode, bool zstd_if_scene = true);

struct LoadOptions {
    bool  verify_crc = true;
    // Fail (return unexpected) vs. best-effort (log + continue) when a
    // component type in the payload isn't registered in the live World.
    bool  strict_unknown_components = false;
};

[[nodiscard]] std::expected<void, SerializationError>
load_world(World& out, std::span<const std::uint8_t>, LoadOptions = {});

// Single-entity paths used by DestroyEntityCommand (ADR-0005).
[[nodiscard]] std::vector<std::uint8_t> save_entity(const World&, Entity);
[[nodiscard]] std::expected<Entity, SerializationError>
load_entity(World&, std::span<const std::uint8_t>);

} // namespace gw::ecs
```

### 2.10 Error model

`SerializationError` is an enum + short `std::string_view` message (owned by static storage). Cases:

- `BadMagic`, `UnsupportedVersion`, `CrcMismatch`, `Truncated` (header issues)
- `UnknownComponentType` (only if `strict_unknown_components`)
- `ComponentSizeMismatch` (fast-path, no migrator)
- `FieldTypeMismatch` (reflection path; downgraded to warning otherwise)
- `ZstdDecompressFailed`

The `std::expected<void, SerializationError>` return is consistent with the rest of the engine (see `engine/assets/asset_error.hpp`).

## 3. Consequences

### Immediate (this commit block — under Phase-7 Path-A)

- Existing 10-line stub in `engine/core/serialization.cpp` is replaced with real `BinaryWriter`/`BinaryReader`.
- `engine/ecs/serialize.{hpp,cpp}` lands.
- Tests land under `tests/unit/ecs/serialize_test.cpp`: trivial-only round-trip, reflection round-trip, evolution cases (added field / removed field), single-entity snapshot round-trip, CRC failure detection.
- `DestroyEntityCommand` (ADR-0005) wires to `save_entity` / `load_entity`.
- PIE `EnterPlay` calls `save_world(..., PieSnapshot)` into an editor-owned buffer; `ExitPlay` calls `load_world` to restore.

### Short term (Phase 8)

- `.gwscene` file I/O (`save_world(..., SceneFile)` + `std::ofstream`/`std::ifstream` wrappers). Probably `engine/assets/scene_file.{hpp,cpp}` or similar.
- Scene-file asset entry in the cook manifest, same way mesh/texture/shader assets currently flow.
- The editor's File menu grows "Open Scene" / "Save Scene" / "Save Scene As" wired to this.

### Long term

- Migration hook API (`gw_ecs_migrate_component<T>`) if any trivially-copyable component layout ever evolves.
- Field-rename annotation on reflection (`GW_RENAMED_FROM`).
- Zstd dictionary training for scene files if compression ratio matters to the content workflow.
- Cross-platform determinism audit — floats serialized round-trip losslessly (they already do, binary memcpy) but any text-forms we introduce later must be reviewed.

### Alternatives considered

1. **FlatBuffers / Cap'n Proto / Protobuf.** Rejected — all three add an IDL generation step, which conflicts with our "reflection is the schema" design. Also: FlatBuffers' zero-copy access is nice for read-heavy asset formats but here we're always deserializing into live `World` state anyway.

2. **JSON / TOML.** Rejected for PIE-snapshot path (way too slow, way too large). For `.gwscene` it has commit-diff appeal but binary + zstd with a stable-order archetype walk is VCS-diff-tolerable (`git diff` won't show useful hunks, but `.gwscene.meta` companion files can — Phase-8 concern). Ship binary now.

3. **Per-component `serialize()` / `deserialize()` free functions.** Rejected — that's what Phase-3's stub was reaching for. Reflection already provides the field list; hand-writing per-component serializers duplicates that work. The reflection path *is* the per-component serializer.

4. **Store `Entity` references by stable GUID rather than `entity_bits`.** Rejected as over-engineered for the PIE snapshot use case (round-trip same process, handles are internally consistent). Relevant if we ever cross-reference entities across separate `.gwscene` files (prefabs?), at which point a dedicated ADR handles the external-reference surface.

5. **Single-archetype + per-field stream only; no trivially-copyable fast path.** Rejected — the 10⁴-entity budget requires the fast path. Trivially-copyable is common enough (basic math components, numeric state) that making it the default is correct.

## 4. References

- ADR-0004 (ECS-World)
- ADR-0005 (Editor CommandStack — `DestroyEntityCommand`)
- `audit-2026-04-20 §P1-11`
- `docs/04_SYSTEMS_INVENTORY.md §Serialization`
- ISO CRC-64 specification — [ISO 3309](https://www.iso.org/standard/17151.html)
- Zstandard reference — [zstd.github.io](https://github.com/facebook/zstd)
- FlatBuffers (reference for why-not) — [google.github.io/flatbuffers](https://google.github.io/flatbuffers/)

---

*Drafted by Claude Opus 4.7 on 2026-04-20 late-night as part of the Phase-7 fullstack Path-A push. Doctrine lands before code per `CLAUDE.md` non-negotiable #20.*

---

## ADR file: `adr/0007-bld-c-abi-freeze.md`

## ADR 0007 — BLD C-ABI registration surface freeze

- **Status:** Accepted
- **Date:** 2026-04-20 (late-night; Phase-7 fullstack Path-A)
- **Deciders:** Cold Coffee Labs engine group (Claude Opus 4.7 drafting; founder final)
- **Supersedes:** the informal `editor/bld_api/editor_bld_api.hpp` header comment rules ("Never remove or reorder existing entries — only add at the tail") — lifted to a formal ADR with versioning rigor.
- **Superseded by:** —
- **Related:** `docs/05 §Phase 7` (Editor v0.1) and `§Phase 9` (BLD); `docs/04_SYSTEMS_INVENTORY.md §BLD`; ADR-0004 (ECS-World — `gw_editor_*` functions route through it); ADR-0005 (CommandStack — `gw_editor_undo/redo` route through it); ADR-0006 (serialization — `gw_editor_save_scene/load_scene` route through it); `CLAUDE.md` non-negotiables #8 (`std::string_view` never to C API — doubly relevant here), #20 (doctrine first).

## 1. Context

Phase 7 (Editor v0.1) week 042 commits to "BLD C-ABI registration surface freeze + one-call-site smoke test." Phase 9 (Brewed Logic, Rust, weeks 048–059) is the actual BLD implementation. The ABI between them needs to be locked *before* Phase 9 starts so BLD can be written against a stable host surface, and so we can't accidentally re-litigate the shape every time an editor feature lands.

The relationship between the two halves:

- **Editor is the host.** It loads `brewed_logic.dll` / `libbrewed_logic.so` on startup (or on user demand; both patterns land in Phase 9). Once loaded, the editor resolves a small set of well-known BLD entry points (`bld_register`, `bld_register_tool`, `bld_register_widget`, …) and calls them.
- **BLD is the plugin.** On `bld_register`, BLD performs its own initialization, then calls back into the editor using `gw_editor_*` exports to register tools, widgets, menu items, hot-reload watchers, MCP endpoints, and RAG hooks. BLD is a Rust crate with a thin C-ABI shim; the shim does nothing the editor can't do — it's just the language boundary.
- **Ongoing communication** happens in both directions: editor → BLD on events (build requested, scene changed, selection changed), BLD → editor on registered-callback invocation (user ran a tool, a widget wants a redraw).

Today, `editor/bld_api/editor_bld_api.{hpp,cpp}` has the editor-export half partially sketched but informally versioned. The BLD-import half (`bld_register*` entry points the editor resolves in `brewed_logic.dll`) doesn't exist yet. Both halves need to be pinned before Phase 9.

An early existing file, `editor/bld_bridge/hello.{hpp,cpp}`, is the *First-Light* smoke: a single `gw_bld_hello()` round-trip proving the dynamic-library plumbing works. It predates this ADR and is retired when Phase-9 begins; we reference it here only to explain why the current tree already has a BLD bridge directory that is *not* the ABI.

## 2. Decision

### 2.1 The two surfaces

**Surface H (Host → BLD).** Symbols the editor looks up in `brewed_logic.dll` via `gw::platform::DynamicLibrary::resolve(name)`. BLD **must** export them; failure to resolve any required symbol means BLD is not loaded.

**Surface P (Plugin → Host).** `extern "C"` functions exported by `gw_editor` (the executable — symbols resolved via the process handle on dlopen/LoadLibrary semantics). BLD links against these at load time via `GetProcAddress` (Windows) or by letting the dynamic loader resolve symbols in the host executable (Linux, via `-rdynamic` / explicit dlopen). *This* is the surface partially present in `editor/bld_api/editor_bld_api.hpp`.

Both surfaces are C ABI. Neither surface uses C++ types across the boundary. Neither surface uses `std::string_view` (NN #8) — strings are `const char*` with caller-owns-or-free-via-callback semantics documented per-function.

### 2.2 Versioning — single monotonic `BLD_ABI_VERSION`

Every ABI item is tagged with the `BLD_ABI_VERSION` that introduced it. On load:

1. Editor calls `bld_abi_version()` → returns the ABI version BLD was built against (u32).
2. Editor compares with its own compiled-in `BLD_ABI_VERSION_HOST`.
3. Policy:
   - `BLD.version == Host.version` → load normally.
   - `BLD.version < Host.version` → load in **compat mode**: missing-from-BLD optional registration entry points are ignored; required entry points must still resolve. Log which newer-than-BLD items were skipped.
   - `BLD.version > Host.version` → **refuse to load**. Log that the host is older than the plugin. BLD must not assume newer host features.

`BLD_ABI_VERSION = 1` ships with this ADR. Every additive change (new registration entry point, new `gw_editor_*` function) bumps the version. **Removals and signature changes are forbidden without a version bump and a migration plan in a follow-up ADR** — this is the "only add at the tail" rule, lifted to ADR-gate strength.

### 2.3 Surface H — the BLD-side entry points (frozen at v1)

```c
/* bld/include/bld_register.h — ships with the editor, consumed by BLD.
 * C ABI. BLD implements these; the editor looks them up. */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* Forward declarations of opaque handles passed to BLD. */
typedef struct BldEditorHandle BldEditorHandle;   /* opaque; carries host globals */
typedef struct BldRegistrar    BldRegistrar;      /* opaque; BLD calls bld_registrar_* via host */

/* Required entry points (must resolve or BLD fails to load).
 * bld_abi_version() must be defined before any other BLD entry is called. */

/* Introduced in v1. */ uint32_t bld_abi_version(void);

/* Introduced in v1. Called once on BLD load. `host` is owned by the editor
 * and valid for the life of the BLD library. `reg` is the registrar handle
 * BLD uses to declare its tools/widgets synchronously during registration.
 * Returns true on success; false aborts load. */
/* Introduced in v1. */ bool bld_register(BldEditorHandle* host, BldRegistrar* reg);

/* Optional entry points (missing == feature not provided by BLD).
 * Missing optional entry points in compat mode are tolerated. */

/* Introduced in v1. Called once on BLD unload or editor shutdown.
 * Ordering: after the last host → BLD call, before the library is freed. */
/* Introduced in v1. */ void bld_shutdown(void);

/* Introduced in v1. Periodic tick for BLD housekeeping (async RAG warmup,
 * hot-reload polling). Called on the editor main thread before ImGui NewFrame.
 * Safe to leave unimplemented; editor won't call a missing symbol. */
/* Introduced in v1. */ void bld_tick(double dt_seconds);

#ifdef __cplusplus
}
#endif
```

### 2.4 Surface P — the editor-side exports (frozen at v1, superset of what's in the tree today)

Refer to `editor/bld_api/editor_bld_api.hpp` as the authoritative header. The ADR amendment to the existing surface is:

**Added in v1 (not in the tree today, but part of the frozen v1 surface):**

```c
/* Registration helpers — called by BLD during bld_register().
 * `reg` is the BldRegistrar the editor passed to bld_register. */

/* Register a named tool. Invoked from the editor's Tools menu or via the
 * `gw_editor_run_tool` API. `fn` is called on the editor main thread.
 * Returns a non-zero tool id, or 0 on failure (duplicate name, etc.).
 * `name` and `tooltip` must be UTF-8; the editor copies them (not retained).
 * Icons are optional; pass nullptr if unused. */
typedef void (*BldToolFn)(void* user_data);

GW_EDITOR_API uint32_t bld_registrar_register_tool(BldRegistrar* reg,
                                                    const char* id,       /* stable, "org.brewedlogic.mytool" */
                                                    const char* name,
                                                    const char* tooltip_utf8,
                                                    const char* icon_name,/* or null */
                                                    BldToolFn   fn,
                                                    void*       user_data);

/* Register a custom inspector widget for a component type key.
 * `component_hash` must match ComponentRegistry.stable_hash (ADR-0004 §2.4).
 * `draw_fn` is called inside ImGui BeginChild for the component block;
 * it MUST NOT begin/end its own ImGui window. */
typedef void (*BldWidgetFn)(void* component_ptr, void* user_data);

GW_EDITOR_API uint32_t bld_registrar_register_widget(BldRegistrar* reg,
                                                      uint64_t    component_hash,
                                                      BldWidgetFn draw_fn,
                                                      void*       user_data);

/* Tool invocation surface exposed to external automation (the editor's own
 * menu uses the same path). Names are BLD-registered ids. Returns false if
 * the tool id is unknown. Any return value / error propagation is BLD's
 * responsibility — they can queue an error into the editor log via the
 * existing log surface (gw_editor_log_error — v1). */
GW_EDITOR_API bool gw_editor_run_tool(const char* tool_id);

/* Logging surface BLD uses to report errors / warnings / info into the
 * editor's console panel. Level: 0=info, 1=warn, 2=error. */
GW_EDITOR_API void gw_editor_log(uint32_t level, const char* message_utf8);
GW_EDITOR_API void gw_editor_log_error(const char* message_utf8);  /* convenience */
```

**Retained from the existing header (see the file) at v1:**

- `gw_editor_version()` — **renamed semantically:** returns the **editor** build string; BLD uses `bld_abi_version()` (Surface H) for negotiation. Editor-version string is informational only.
- Selection: `gw_editor_get_primary_selection`, `gw_editor_get_selection_count`, `gw_editor_set_selection`.
- Entity management: `gw_editor_create_entity`, `gw_editor_destroy_entity`.
- Field access: `gw_editor_set_field`, `gw_editor_get_field`, `gw_free_string`.
- Command stack: `gw_editor_undo`, `gw_editor_redo`, `gw_editor_command_stack_summary`.
- Scene: `gw_editor_save_scene`, `gw_editor_load_scene`.
- PIE: `gw_editor_enter_play`, `gw_editor_stop`.

**All `const char*` outputs** use the `gw_free_string(const char*)` free function (retained). Caller passes back the pointer that was returned; editor frees with its own allocator. Missing in the current header: a matching convention for **callback parameters** — BLD callbacks may not retain string pointers across calls. We document that here and enforce it in the smoke-test pair.

### 2.5 Threading model

- All Surface-P calls execute on whatever thread BLD calls from. The editor's implementation **must** be thread-safe for every export, either by taking a lock or by posting work to the main thread and waiting. The existing comment in `editor_bld_api.hpp` ("mutations are queued for main-thread execution") is upgraded to an ADR requirement.
- All Surface-H calls are made **only** from the editor main thread. BLD can assume no concurrent calls into its exports.
- `bld_tick` runs on the editor main thread before `ImGui::NewFrame`.

### 2.6 Memory / lifetime contracts

- **Input strings** (`const char* name` etc.) are valid only for the duration of the call. The callee must copy if it needs to retain.
- **Output strings** returned by `gw_editor_get_field`, `gw_editor_command_stack_summary`, etc. are caller-owned; caller frees via `gw_free_string`.
- **Opaque handle pointers** (`BldEditorHandle*`, `BldRegistrar*`) are owned by the editor; BLD must not free them.
- **Callbacks registered via `bld_registrar_register_*`** live until either BLD unloads or `bld_shutdown` is called; unregistration at runtime is not supported in v1 (deferred to v2 if needed).
- **`void* user_data`** pointers passed to callbacks are owned by BLD; the editor never dereferences or frees them. On BLD unload the editor drops the pointer without calling a "free user_data" hook (v2 addition if anyone needs it).

### 2.7 The Phase-7 smoke-test site

One end-to-end call path exercises every v1 section without needing a real BLD implementation:

1. Build a `tests/smoke/bld_abi_v1/` mini-dylib that implements the Surface-H entry points in C++ (BLD is Rust long-term; for the smoke we need any language).
2. The smoke dylib's `bld_register` calls `bld_registrar_register_tool(reg, "test.hello", "Hello", "Say hi", nullptr, &say_hi_fn, nullptr)`.
3. `gw_editor` loads the dylib, calls `bld_abi_version`, calls `bld_register`, then calls `gw_editor_run_tool("test.hello")` which invokes `say_hi_fn` which calls `gw_editor_log_error("bld_abi v1 round-trip OK")`.
4. Test asserts the log surface received that message.

That single test covers: symbol resolution, version negotiation, registrar round-trip, tool invocation, host-log callback. It *does not* exercise widgets or the full set of `gw_editor_*` calls, but it locks the shape by keeping the smoke dylib as a consumer that would break if anyone silently changed the ABI.

### 2.8 Out of scope for v1 (explicitly)

- **Async tool invocation / job integration.** Tools run synchronously on the editor main thread in v1. Async tools are a v2 addition.
- **Streaming / large-payload transfer** between host and plugin (BLD's RAG corpus, long-running bg computes). v1 passes control only; BLD manages its own heaps.
- **Hot-reload of BLD itself.** v1 supports load-on-startup and unload-on-shutdown. Reload-during-session lands with Phase-9 hot-reload work, and *that* is where we'd add `bld_on_reload` to Surface H.
- **MCP endpoint surface.** BLD's MCP endpoints live inside BLD's own HTTP server; the ABI only needs to know "BLD is alive and can take requests." That's already covered by `bld_tick`.
- **Python / JavaScript scripting interop.** BLD is Rust. If we ever bind Python, that's a new ADR.

## 3. Consequences

### Immediate (this commit block)

- `bld/include/bld_register.h` lands (C header; symlinked or copied into BLD's Rust crate at Phase-9 start).
- `editor/bld_api/editor_bld_api.hpp` gains the new v1 additions (registrar/tool/widget/log surfaces).
- `editor/bld_api/editor_bld_api.cpp` ships stub implementations — each function routes into the right subsystem (selection / ECS-World / CommandStack / serialization) per the ADR-0004/0005/0006 plumbing. Where a subsystem isn't yet wired (today), the function returns false / 0 / empty with a log line; Phase-7 implementation work fills in as each subsystem lands.
- `tests/smoke/bld_abi_v1/` smoke dylib + test target.
- The existing `editor/bld_bridge/hello.{hpp,cpp}` is kept as a marker for First-Light but *not* part of the v1 ABI surface; a comment in that file points at this ADR.

### Short term (Phase 9 entry)

- BLD builds against `bld/include/bld_register.h` as its C-side contract.
- BLD implements `bld_register` / `bld_abi_version` / (optionally) `bld_shutdown` / `bld_tick`.
- First BLD feature (probably a `bld.compile` tool) registers via `bld_registrar_register_tool`; the smoke test evolves into a real feature test.

### Long term

- Every v1 → v2 change bumps `BLD_ABI_VERSION`. Removed functions get a `GW_EDITOR_API_DEPRECATED(v)` marker and live one version beyond removal for compat mode.
- When hot-reload of BLD lands (Phase 9 late), Surface H gains `bld_on_reload_begin` / `bld_on_reload_end` at v2.
- If BLD ever needs to drive the editor from a thread other than main (unlikely; it calls back from editor callbacks, which already run on main), a `gw_editor_post_to_main(fn, user_data)` addition covers it at v2+.

### Alternatives considered

1. **Flat C ABI with no registrar, plugins register by side-effect at load time** (e.g. `bld_register_tools` is itself a static list consumed by the editor). Rejected — registrar-based is strictly more flexible, and the registrar pattern maps cleanly to BLD-authored runtime registrations we know are coming (widgets bound to user-authored components).

2. **RPC / message-queue based (protobuf over unix-socket).** Rejected for v1 as wildly over-engineered for an in-process plugin. Revisit if BLD ever needs to run out-of-process (sandbox isolation for untrusted plugins — possibly relevant to the eventual mod-plugin story, which is *not* BLD).

3. **Expose the entire C++ editor surface directly via Rust bindings (bindgen / cxx).** Rejected — binds BLD to the editor's C++ ABI, which varies per compiler/STL. A hand-curated C ABI is stable across compilers, which is the whole point of this ADR.

4. **Version per-function instead of per-surface.** Rejected. A single monotonic `BLD_ABI_VERSION` + "added in v_N" annotations is simpler to reason about and matches how Khronos versions the Vulkan API.

5. **No formal versioning; "trust the developer" (the current state).** Rejected — this is exactly what the Phase-3 drift (P1-11) was caused by. Doctrine first, enforced.

## 4. References

- `editor/bld_api/editor_bld_api.hpp` — existing partial surface
- `editor/bld_bridge/hello.{hpp,cpp}` — First-Light smoke (not ABI)
- `docs/05 §Phase 7 week 042` — freeze-at-v1 commitment
- `docs/05 §Phase 9` — BLD build-out
- `docs/04_SYSTEMS_INVENTORY.md §BLD`
- ADR-0004 / ADR-0005 / ADR-0006 — the subsystems the exports route into
- `CLAUDE.md` non-negotiables #8, #20

---

## Amendment 2026-04-21 — Phase 9 ABI version trail

Phase 9 waves 9B and 9E add additive-only entries to Surface P. Per §2.2 versioning policy ("every additive change bumps the version; removals and signature changes are forbidden without a version bump and a migration plan"), the planned ABI version trail is:

### v2 — introduced in Phase 9 wave 9B (ADR-0012)

Added to Surface P:

```c
/* Introduced in v2. Walks the editor's reflection registry once, invoking
 * cb() for each registered component. BLD uses this to autogenerate
 * component.<name>.{get,set,add,remove} tools at registration time. */
typedef void (*BldComponentEnumFn)(const char* name,
                                    uint64_t    stable_hash,
                                    const char* fields_json,  /* [ {name,type,offset}, ... ] */
                                    void*       user);
GW_EDITOR_API void gw_editor_enumerate_components(BldComponentEnumFn cb, void* user);

/* Introduced in v2. Enqueue a single serialised ICommand. Payload shape is
 * opcode-specific; see bld/docs/opcodes.md. Returns a command id the caller
 * can pair with gw_editor_command_result for async completion. */
GW_EDITOR_API uint64_t gw_editor_run_command(uint32_t opcode,
                                              const void* payload,
                                              uint32_t payload_bytes);

/* Introduced in v2. Transaction bracket: N opcodes collapse to a single
 * undo entry. `payload_offsets[N+1]` — the last slot is the total bytes. */
GW_EDITOR_API uint64_t gw_editor_run_command_batch(uint32_t opcode_count,
                                                    const uint32_t* opcodes,
                                                    const void* payloads,
                                                    const uint32_t* payload_offsets);
```

ADR-0012 defines opcode semantics; `bld-tools::component_autogen` consumes the enumerator.

### v3 — introduced in Phase 9 wave 9E (ADR-0015)

Added to Surface P:

```c
/* Introduced in v3. Opaque session handle owned by the editor. */
typedef struct BldSession BldSession;

GW_EDITOR_API BldSession*  gw_editor_session_create(const char* display_name);
GW_EDITOR_API void         gw_editor_session_destroy(BldSession* session);
GW_EDITOR_API const char*  gw_editor_session_id(BldSession* session); /* ULID string, caller-free */
GW_EDITOR_API const char*  gw_editor_session_request(BldSession* session,
                                                      const char* request_json); /* caller-free */
```

Multi-session tabs in the chat panel; audit streams per-session.

### Policy

- No v1 symbol is removed, renamed, or has its signature altered by either v2 or v3.
- Surface H (BLD → host exports) stays at v1 through Phase 9. The BLD plugin implementation added in Phase 9 is v1-complete; hot-reload-of-BLD-itself is explicitly deferred (§2.8); the pending `bld_on_reload_*` additions remain a v2 Surface-H concern handled in a future ADR.
- A host at v3 continues to load a v1 BLD plugin in compat mode per §2.2; v3 additions are detected at resolve time and skipped when the plugin predates them.

This amendment is the versioning trail recorded per CLAUDE.md #20 so the phase's ABI bumps are documented before the code lands.

---

*Drafted by Claude Opus 4.7 on 2026-04-20 late-night as part of the Phase-7 fullstack Path-A push. Doctrine lands before code per `CLAUDE.md` non-negotiable #20. Amended 2026-04-21 for Phase 9.*

---

## ADR file: `adr/0008-scene-file-codec-and-migration.md`

## ADR 0008 — `.gwscene` file codec, atomic writes, and component schema migration

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 8 week 043)
- **Deciders:** Cold Coffee Labs engine group (Claude Opus 4.7 drafting; founder final)
- **Supersedes:** the two-file stub at `engine/core/scene_format.{hpp,cpp}` (deleted in this commit; never shipped; conflicting magic number `GWSC` vs. ADR-0006's `GWS1`).
- **Superseded by:** —
- **Related:** ADR-0004 (ECS-World), ADR-0006 (ECS-World Serialization — the in-memory payload this ADR wraps for on-disk use), `CLAUDE.md` non-negotiables #8 (no `std::string_view` to C), #17 (float64 world positions — the first migration target), #19 (trait-driven sanity: "atomic writes or nothing"), `docs/02_ROADMAP_AND_OPERATIONS.md §Phase 8 week 043`.

## 1. Context

ADR-0006 defined the **payload** shape for a world and the header framing (`GWS1 / format_version = 1`, CRC-64, optional zstd). It explicitly deferred two items to Phase 8:

1. The **file I/O wrapper** — the `engine/scene/scene_file.{hpp,cpp}` module that turns `std::vector<std::uint8_t>` (from `save_world`) into a durable `.gwscene` on disk, with atomic replace semantics.
2. The **component migration hook** — a registered per-type transformer that lets the load path accept an older on-disk component layout and massage it into the current layout. ADR-0006 §2.8 marked this out of scope; the first real need surfaces *immediately* in Phase 8 week 044 when `TransformComponent::position` widens from `glm::vec3` (12 B) to `glm::dvec3` (24 B, padded to 56 B total) to comply with CLAUDE.md #17.

A prior attempt (`engine/core/scene_format.{hpp,cpp}`) tried to do #1 directly in `engine/core`. It used a different magic number (`GWSC`), duplicated header logic, and never wired into the ECS serializer. Keeping it would fork the on-disk format. We delete it in this ADR.

## 2. Decision

### 2.1 Module layout

```
engine/scene/
  scene_file.hpp       // public API: save_scene / load_scene
  scene_file.cpp
  migration.hpp        // MigrationRegistry (per-component transformers)
  migration.cpp
  CMakeLists.txt       // wired from engine/CMakeLists.txt
```

The module links `greywater_core` (for `engine/core/serialization.hpp`) and `greywater::ecs` (for `engine/ecs/serialize.hpp`). No Vulkan / ImGui dependencies — scene I/O is pure data.

### 2.2 Public API

```cpp
namespace gw::scene {

enum class SceneIoError : std::uint8_t {
    OpenFailed,
    WriteFailed,
    ReadFailed,
    RenameFailed,
    DeserializeFailed,   // underlying ECS error (CRC, truncation, etc.)
    SerializeFailed,     // encoder rejected the live world
};

struct SaveOptions {
    bool zstd = true;
};

struct LoadSceneOptions {
    bool verify_crc = true;
    bool strict_unknown_components = false;
};

[[nodiscard]] std::expected<void, SceneIoError>
save_scene(const gw::ecs::World& w,
           const std::filesystem::path& path,
           SaveOptions opts = {});

[[nodiscard]] std::expected<void, SceneIoError>
load_scene(gw::ecs::World& out,
           const std::filesystem::path& path,
           LoadSceneOptions opts = {});

} // namespace gw::scene
```

`save_scene` and `load_scene` compose `gw::ecs::save_world(..., SnapshotMode::SceneFile)` + atomic file I/O + `gw::ecs::load_world`. Errors from the ECS layer map 1:1 onto `SceneIoError::{Serialize,Deserialize}Failed`; the original `SerializationError` is logged via `engine/core/log.hpp`.

### 2.3 Atomic writes

`save_scene` never writes directly to the target path. The sequence is:

1. Serialize the world to a heap buffer via `save_world`.
2. Write the buffer to `<path>.tmp-<pid>-<timestamp>` (same directory as target — same volume, required for rename atomicity on POSIX and Win32).
3. `fsync` the temp file on POSIX; `FlushFileBuffers` on Win32 (not strictly required by the ADR, but significantly reduces the "crash between write and rename" window).
4. Rename the temp file over the target: `rename(2)` on POSIX, `MoveFileExW(MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)` on Win32.
5. On any error the temp file is deleted and the target is left untouched.

This guarantees "either the old scene is intact, or the new scene is fully written and CRC-clean" — no half-written `.gwscene` ever reaches disk.

### 2.4 Component schema migration

The `MigrationRegistry` answers the question: *"the payload on disk says component X is 40 bytes, but the live `T` is 56 bytes — what do I do?"*

```cpp
namespace gw::scene {

// Migrates one component instance in place.
// src:  raw bytes of the on-disk component (size = old `size` from the payload's ComponentTable entry)
// dst:  live-sized buffer of `sizeof(T)` bytes that the loader will then
//        memcpy into the archetype chunk. The function owns filling `dst`
//        with a valid, trivially-copyable instance of T.
using MigrateFn = std::function<bool(
    std::span<const std::uint8_t> src,
    std::span<std::uint8_t>       dst)>;

struct MigrationKey {
    std::uint64_t stable_hash = 0;   // component stable_hash (the portable key)
    std::uint32_t from_size   = 0;   // payload-reported size that matched this entry
};

struct MigrationEntry {
    MigrateFn   fn;
    std::string reason;              // e.g. "TransformComponent: widen position vec3->dvec3"
};

class MigrationRegistry {
public:
    static MigrationRegistry& global();

    template <class T>
    void register_fn(std::uint32_t from_size, MigrateFn fn, std::string reason);

    [[nodiscard]] const MigrationEntry*
    find(std::uint64_t stable_hash, std::uint32_t from_size) const noexcept;
};

} // namespace gw::scene
```

The ECS loader (`gw::ecs::load_world`) is extended with an opt-in callback:

```cpp
struct LoadOptions {
    bool verify_crc                = true;
    bool strict_unknown_components = false;
    MigrateComponentFn migrate;    // NEW — called when payload.size != live.size
};
```

`scene_file.cpp` supplies a `migrate` that routes through `MigrationRegistry::global().find(...)`. When a migrator exists, `load_world` calls it per-instance; when it doesn't, `load_world` returns `ComponentSizeMismatch` unchanged.

`register_fn<T>(from_size, fn, reason)` is called once per process (`std::call_once` inside the module that owns `T`'s layout evolution). For editor-side components the registration happens inside `gw_editor_load_scene`'s first call (see `editor/bld_api/editor_bld_api.cpp::register_editor_migrations_once`).

#### 2.4.1 The TransformComponent case (week 044)

- v1 layout: `float position[3]` + `float rotation[4]` + `float scale[3]` = 40 bytes, trivially-copyable.
- v2 layout: `double position[3]` + `float rotation[4]` + `float scale[3]` + 4 B trailing padding = **56 bytes** (the `double` forces 8-byte alignment on the struct).

The registered migrator reads the v1 bytes with one `memcpy` each for `pos_f`, `quat_f`, `scale_f`, widens `pos_f` to `double` element-by-element, and writes a fresh v2 `TransformComponent` into `dst`. See `editor/bld_api/editor_bld_api.cpp::register_editor_migrations_once` for the full code; the migrator is ~30 lines and adds a `static_assert(sizeof(TransformComponent) == 56)` so a future struct change trips the build rather than silently corrupting scenes.

### 2.5 Why not handle migration one level up (in `scene_file.cpp`)?

Earlier draft: have `scene_file` pre-pass the payload, rewrite old-size component arrays in place to new-size arrays, hand a rewritten buffer to `load_world`. Rejected because:

- Requires re-parsing the ECS payload format in two places → ECS-layout knowledge leaks into scene-file code.
- A pre-pass cannot benefit from the ECS loader's per-entity context (it would have to rebuild the archetype table itself).
- Doubles the code surface that has to stay in sync with ADR-0006 §2.2.

The callback-in-LoadOptions version keeps migration logic *inside* the ECS loader, where the component table and archetype walk already exist, and keeps the registry as the only public surface that migration authors touch.

## 3. Consequences

### Immediate (this commit block — Phase 8 week 043)

- `engine/core/scene_format.{hpp,cpp}` deleted.
- `engine/scene/{scene_file,migration}.{hpp,cpp}` landed.
- `engine/ecs/serialize.{hpp,cpp}` gained `MigrateComponentFn` in `LoadOptions`; the `parse_payload` defers the `ComponentSizeMismatch` check to `apply_payload` where the callback is consulted.
- `editor/bld_api/editor_bld_api.cpp` registers the `TransformComponent` v1→v2 migration on first scene load; `gw_editor_save_scene` and `gw_editor_load_scene` go through the new module.
- Tests: `tests/unit/scene/scene_file_test.cpp` covers round-trip, CRC tamper detection, atomic rename, and an end-to-end migration scenario using a hand-crafted v1 payload.

### Short term (weeks 044-047)

- The transform system (week 044) relies on this migration to keep existing `.gwscene` assets loadable under the new `dvec3` shape without authoring-tool intervention.
- The VScript panel (week 047) saves `.gwvs` text + `.gwvs.layout.json` sidecars using plain `std::ofstream`; it does not go through `save_scene` because text IR is the ground-truth for scripts (CLAUDE.md #14).

### Long term

- Any future trivially-copyable component layout change registers a migrator with a new `from_size`. The registry is additive; multiple `from_size` entries for the same `stable_hash` coexist.
- If a component ever needs to change its `stable_hash` (e.g. namespace rename), a secondary "aliasing" table lands as an ADR-0008-follow-up. Out of scope here.
- `fsync` is currently best-effort; a dedicated ADR can define a stronger guarantee (fsync both the temp file *and* its parent directory on POSIX) if the content workflow demands it.

### Alternatives considered

1. **Write `.gwscene` directly to its final path via `std::ofstream`.** Rejected — any crash between `open(O_TRUNC)` and the last `write()` leaves a zero-byte or partial scene, which CRC-fails on load. The content-loss blast radius is unacceptable for authoring workflows.
2. **Store migrations as per-type `migrate_v1` / `migrate_v2` member functions.** Rejected — couples migration code to the component's own translation unit, which forces the component header to know about scene I/O. The registry pattern keeps components header-only and keeps migration code in the editor/tool TU that understands the evolution.
3. **Auto-generate migrations from reflection field diffs.** Rejected for Phase 8 — doable, but a `dvec3` ← `vec3` widening isn't a field add/remove, it's a type change, and the automatic path would still need hand-authored glue for that case. Field-rename / field-add auto-migration can land later under ADR-0006 §2.8's reflection path.

## 4. References

- ADR-0004 (ECS-World)
- ADR-0006 (ECS-World Serialization)
- `CLAUDE.md` non-negotiables #17 (float64 world-space), #19 (atomic writes)
- `docs/02_ROADMAP_AND_OPERATIONS.md §Phase 8`
- POSIX rename(2) — <https://man7.org/linux/man-pages/man2/rename.2.html>
- Win32 `MoveFileExW` — <https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-movefileexw>

---

*Drafted by Claude Opus 4.7 on 2026-04-21 as part of the Phase 8 fullstack push. Doctrine lands before code per `CLAUDE.md` non-negotiable #20.*

---

## ADR file: `adr/0009-visual-scripting-ir.md`

## ADR 0009 — Visual Scripting: typed text IR, interpreter oracle, bytecode VM, AOT cook

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 8 weeks 045–047)
- **Deciders:** Cold Coffee Labs engine group (Claude Opus 4.7 drafting; founder final)
- **Supersedes:** —
- **Superseded by:** —
- **Related:** `CLAUDE.md` non-negotiables #14 (text IR is ground-truth for visual scripts), #5 (no raw `new`/`delete` on VM hot path), `docs/06_ARCHITECTURE.md §Visual scripting UI`, `docs/06_ARCHITECTURE.md §3.8`, `docs/02_ROADMAP_AND_OPERATIONS.md §Phase 8 weeks 045–047`, ADR-0008 (for the `.gwvs.layout.json` sidecar pattern), ImNodes widget reference.

## 1. Context

Phase 8 week 045 introduces Greywater's visual scripting system. The requirements, pinned by prior doctrine:

- **The text IR is the source of truth.** Editors are views. This is how we get diff-friendly, merge-resolvable, AI-native scripts and avoid the Unreal-Blueprint failure mode where the on-disk asset is a binary node graph.
- **The node graph is drawn with ImNodes**, and the on-disk project carries a sidecar `.layout.json` for node positions. Positions **never** live in the IR.
- **Determinism and safety.** Scripts run at simulation rate — a compile that changes output for equivalent inputs is a regression; a VM that can heap-allocate per frame is a performance bug.
- **Two-tier runtime.** An interpreter (fast to author; also the golden oracle) and a bytecode VM (shipped runtime; AOT-cooked). They must agree bit-for-bit on the golden corpus.

The rest of this ADR defines the smallest viable shape that satisfies those constraints and can grow into Phase 9+ (loops, function calls, intrinsics, JIT) without on-disk re-breaks.

## 2. Decision

### 2.1 Module layout

```
engine/vscript/
  ir.hpp / ir.cpp                 // Type, Value, Pin, Node, Graph + validate()
  parser.hpp / parser.cpp         // text ↔ Graph round-trip
  interpreter.hpp / interpreter.cpp   // Phase 8 week 045 reference oracle
  bytecode.hpp / bytecode.cpp     // OpCode, Instr, Program + compile()
  vm.hpp / vm.cpp                 // Phase 8 week 046 stack VM — ships runtime
  CMakeLists.txt

editor/vscript/
  vscript_panel.hpp / vscript_panel.cpp   // Phase 8 week 047 ImNodes projection
```

The engine side has zero Vulkan / ImGui dependencies. The editor side owns its own `ImNodesContext` per panel so multiple `.gwvs` windows don't fight over globals.

### 2.2 IR shape (text format)

The text IR is the commit-diffable grammar:

```
graph <name> {
  input  <name> : <type> [= <literal>]
  output <name> : <type>
  node <id> : <kind> [<debug_name>] {
    in  <pin> : <type> [= <literal>]
    out <pin> : <type>
  }
  edge <src_id>.<src_pin> -> <dst_id>.<dst_pin>
}
```

Types are a closed enum: `int` (`int64`), `float` (`f64`), `bool`, `string`, `vec3`, `void` (control-only). Literals for `vec3` use `vec3(x, y, z)`.

**Rationale for enum types (not templates).** A closed type set lets the VM emit direct opcodes per type and lets the editor colour pins without walking a type tree. Generics land in a successor ADR when we have a use case; Phase 8 explicitly excludes them.

**Node kinds shipped in Phase 8:** `const`, `get_input`, `set_output`, `add`, `sub`, `mul`, `select`. Small on purpose — enough to prove the pipeline end-to-end without forcing us to commit to intrinsic shapes that Phase 9 will inform.

**Why `id` is an integer, not a UUID.** Within a single graph, ids are author-scoped names; the serializer preserves insertion order. Text round-trip tests (`serialize(parse(text)) == text`) depend on that stability.

### 2.3 Graph validation

`gw::vscript::validate(const Graph& g)` enforces:

- Every non-literal input pin is driven by exactly one edge.
- Every edge references valid `src_node.src_pin` and `dst_node.dst_pin`.
- Edge types agree (no implicit coercion in Phase 8).
- Graph is a DAG. Cycles are rejected (iterative DFS with a three-state colour array).
- Every `set_output` references a declared graph output by name, and its value pin type matches.

The interpreter and the compiler both call `validate()` before running. Making validation idempotent and fast (< 10 µs on a thousand-node graph) keeps it on the save/compile hot path without flinching.

### 2.4 Reference interpreter — the oracle

`gw::vscript::execute(const Graph&, const InputBindings&)`:

- Bottom-up demand-driven walk, memoised by `(node_id, pin_name)` so diamond DAGs don't recompute.
- Thread-local result cache reset per call so the steady-state hot path allocates zero on the second call onward.
- Type errors are surfaced as `ExecFailure { ExecError::TypeMismatch, message }` — no silent coercion.

**Why ship the interpreter?** Two reasons: (a) the editor preview button needs something that can run on a half-edited graph without the cook step, and (b) the VM golden corpus cross-checks against this implementation. If the VM and interpreter diverge on any input, the bug is in the compiler, period. See `tests/unit/vscript/vscript_vm_test.cpp`.

### 2.5 Bytecode + stack VM (Phase 8 week 046)

`Program` = `{ vector<Instr> code, vector<string> strings, vector<Vec3Value> vec3s, vector<IOPort> inputs, vector<IOPort> outputs }`.

`Instr` is a tagged-union record: 1-byte opcode + 7-byte padding + a union of `{ i64, f64, bool, idx }`. The size is 24 bytes (opcode + pad + the three-member union); larger than the minimum but we trade 8 bytes for branch-free operand decoding in the VM dispatcher.

**OpCodes shipped in Phase 8:**

| Op           | Code | Operand meaning                      | Net stack effect |
|--------------|------|--------------------------------------|------------------|
| `halt`       | 0    | —                                    | 0 (terminates)   |
| `push_i64`   | 1    | `i64`                                | +1               |
| `push_f64`   | 2    | `f64`                                | +1               |
| `push_bool`  | 3    | `b`                                  | +1               |
| `push_str`   | 4    | `idx` → `strings[]`                  | +1               |
| `push_vec3`  | 5    | `idx` → `vec3s[]`                    | +1               |
| `get_input`  | 6    | `idx` → `strings[]` (input name)     | +1               |
| `add`        | 7    | —                                    | −1               |
| `sub`        | 8    | —                                    | −1               |
| `mul`        | 9    | —                                    | −1               |
| `select`     | 10   | —                                    | −2               |
| `set_output` | 11   | `idx` → `strings[]` (output name)    | −1               |

**Compiler schedule.** Each `set_output` produces a sub-program: `emit_input(value_pin) ; set_output(name)`. `emit_input` follows drivers recursively, emitting pushes and operators. Shared sub-expressions *aren't* memoised in Phase 8 — they're re-evaluated. This is intentional:

1. The cost of memoisation in a stack machine is a scratch-slot bookkeeping pass that adds complexity for marginal speed on the hand-authored graphs shipping scripts use (seldom > 50 nodes).
2. Phase 9's JIT naturally solves the problem by emitting per-pin SSA values.
3. The oracle match against the interpreter catches any scheduling regression.

**VM hot path.** One `switch` over `op`, one `stack.emplace_back` or `stack.pop_back` per op. No allocation on steady state — the stack is `vector<Value>` with `reserve(code.size())` at entry. Phase 9 swaps `vector<Value>` for an aligned fixed-size array sized at compile time from the graph's known max-depth.

**Output completion.** On `halt`, the VM fills any output name not yet written with the declared default from `Program::outputs`. This mirrors the interpreter so a graph with a declared but unbound output still produces a complete result map.

### 2.6 Graph ↔ editor projection (ImNodes, week 047)

The editor's `VScriptPanel` is a pure view:

- Text buffer on the left = ground-truth.
- ImNodes canvas on the right = projection. On every "Parse" click (or on panel open) the graph is rebuilt from text; ImNodes node/pin ids are derived from `(node_id << 16 | pin_index << 1 | is_output)`.
- Layout is persisted to `<path>.layout.json` as `{"<node_id>": [x, y], ...}`. The file is written on save and read on load; a missing or malformed sidecar falls back to a default column grid.
- The panel cannot currently *edit* the graph through the node view — adding nodes, wiring pins, and renaming ports all happen via the text buffer. Phase 9 grows the ImNodes side to editable via `CommandStack`-backed operations.

**Sidecar format choice.** A hand-rolled ~40-line JSON reader/writer lives in `vscript_panel.cpp`. We explicitly chose not to pull `nlohmann::json` into the editor TU — the format is trivial, write-only from the editor, and read-mostly from the cook step (the cook never needs layout). If Phase 9 grows the editor to author project-wide structured data, we revisit.

### 2.7 AOT cook pipeline (outline)

Phase 8 week 046 ships the `compile` function from `Graph → Program`. The cook step that writes `Program` to disk (`.gwvsc`) is staged for Phase 9 alongside the rest of the asset cook manifest. The `Program` struct is designed to be trivially serializable (no owning pointers; strings are `std::string` whose bytes write out directly; `Vec3Value` is POD), so the cook is a `BinaryWriter` walk plus a small header. That lands under an `0009-follow-up` pull request when the cook manifest grows.

## 3. Consequences

### Immediate (this commit block — Phase 8 weeks 045–047)

- `engine/vscript/` module exists and is linked into `greywater_core`.
- Five dedicated test cases for the IR/validate/interpreter/parser (`vscript_ir_test.cpp`); five for the bytecode VM with oracle matching against the interpreter (`vscript_vm_test.cpp`). All green.
- `editor/vscript/vscript_panel.{hpp,cpp}` landed and docked in the editor's centre area.
- `ImNodesContext` is owned per-panel; the panel constructor creates one, the destructor destroys it.
- The panel's "Run" button executes the current graph through the interpreter and renders the output map.

### Short term (Phase 9)

- `.gwvsc` cook step serialises `Program` to disk; runtime loads bytecode directly without the parser on the hot path.
- Editable graph view: `CommandStack`-backed add-node, delete-node, wire-edge operations; the text buffer is kept in sync by re-serialising after each mutation.
- Additional opcodes: loops (`branch_if`, `jump`), function calls, native intrinsics (e.g. `glm::normalize`, math helpers). Each addition appends to the `OpCode` enum (never reorders) so cooked blobs stay binary-compatible.

### Long term

- JIT backend — lift `Program` to an SSA IR and emit native code via LLVM or Cranelift. Same `validate` front-end, same interpreter oracle.
- Behaviour-tree authoring uses this pipeline under the hood (see ADR-0043 and Phase 13 game-AI framework).

### Alternatives considered

1. **Graph-as-ground-truth (Blueprint model).** Rejected — the diff/merge/AI-collaboration pain is well-documented. Text IR sidesteps the entire class of problems.
2. **Tree-walking interpreter as the ship runtime (skip the VM).** Rejected — allocation patterns and cache behaviour are unpredictable in a tree walker; stack VM lets us lock the operand stack to a fixed pre-sized array in Phase 9.
3. **Ship without `validate()`; trust the editor.** Rejected — any tool (CLI cook, BLD agent, raw text edits) that produces an IR goes through the same door; validate once, early, before the interpreter or compiler sees the graph.
4. **Per-node virtual `execute()` functions.** Rejected — forces every new node type to add a polymorphic class. The enum-dispatch + stack-machine approach keeps the node surface a data-only struct.
5. **Separate cook-time IR from author-time IR.** Rejected for Phase 8 — the text IR is the author-time IR and compiles directly to `Program`. If cook-time optimisations (constant folding, CSE) grow non-trivially, a separate lowered IR can land in Phase 9 without breaking `.gwvs` on-disk.

## 4. References

- `CLAUDE.md` non-negotiable #14 (text IR is ground-truth for visual scripts)
- `docs/06_ARCHITECTURE.md §Visual scripting UI`
- `docs/06_ARCHITECTURE.md §3.8`
- `docs/02_ROADMAP_AND_OPERATIONS.md §Phase 8 weeks 045–047`
- ImNodes reference — <https://github.com/Nelarius/imnodes>
- Dear ImGui input-text callback API — <https://github.com/ocornut/imgui/blob/docking/imgui.h>

---

*Drafted by Claude Opus 4.7 on 2026-04-21 as part of the Phase 8 fullstack push. Doctrine lands before code per `CLAUDE.md` non-negotiable #20.*

---

## ADR file: `adr/0010-bld-provider-abstraction.md`

## ADR 0010 — BLD provider abstraction & LLM selection policy

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 9 opening, wave 9A)
- **Deciders:** Cold Coffee Labs engine group (Claude Opus 4.7 drafting; founder final)
- **Supersedes:** —
- **Superseded by:** —
- **Related:** `CLAUDE.md` non-negotiables #15 (BLD separation), #18 (no secrets committed); `docs/01_CONSTITUTION_AND_PROGRAM.md` §2.5; `docs/04_SYSTEMS_INVENTORY.md §13`; `docs/05_RESEARCH_BUILD_AND_STANDARDS.md §3`; `docs/06_ARCHITECTURE.md §3.7`; ADR-0007 (C-ABI freeze); ADR-0016 (local inference — forward reference).

## 1. Context

Phase 9 wave 9A (weeks 048–049) exits on a Claude-streamed chat round-trip inside `gw_editor`. The foundational decision for every turn that follows is *what abstraction the agent talks to*. The wrong choice here costs us:

- Vendor lock-in to one LLM API on week one.
- A second rewrite in week 057 when the local-model path lands.
- An impossible contract-test story (different providers, different bugs, no shared harness).
- A secret-handling disaster if API keys leak through a generic `Display` or `Debug`.

Two days of 2026-04 research surfaced three live options that matter:

1. **Hand-rolled `reqwest` + `eventsource-stream`** per provider. Maximum control; ~1.5 kLoC duplicated across Anthropic / OpenAI / Gemini / local; brittle.
2. **`async-openai` + per-provider shim.** Tight for OpenAI + drop-ins; awkward for Anthropic's `/v1/messages` + tool-use format; no local story.
3. **`rig-core` (2026-03 stable).** Unified `LLMProvider` / `CompletionModel` traits over Anthropic / OpenAI / Gemini / Cohere / Perplexity; 5× memory reduction vs. Python wrappers; 25–44 % latency improvement on streaming; pluggable for local backends via the same trait.

Rig is also the only option that lets us **own** the `ILLMProvider` trait (wrapper pattern — if Rig diverges, we fall back to direct HTTP in ~2 days per provider; see risk R4 of the Phase 9 plan).

Separately, Anthropic's prompt-cache TTL dropped from 60 min to 5 min in 2026-Q1. Without a keep-alive pattern, cost savings collapse from 80 % to 40–55 % on long sessions. And `tool_search_tool` reduces context bloat 85 % for agents with 40+ tools — we will ship 79 at GA (per §2.2 plan), so it is mandatory, not optional.

## 2. Decision

### 2.1 The trait surface (owned by `bld-provider`)

BLD defines **one** trait, `ILLMProvider`, in `bld-provider/src/provider.rs`. Every concrete backend (Anthropic-via-Rig, OpenAI-via-Rig, mistral.rs local, llama-cpp-2 fallback) implements it. `bld-agent` and `bld-mcp` depend only on the trait. The trait surface (simplified):

```rust
#[async_trait::async_trait]
pub trait ILLMProvider: Send + Sync {
    fn id(&self) -> ProviderId;
    fn capabilities(&self) -> ProviderCapabilities;
    async fn complete(&self, req: CompletionRequest) -> Result<CompletionResponse, ProviderError>;
    async fn stream(&self, req: CompletionRequest) -> Result<BoxStream<'_, CompletionChunk>, ProviderError>;
    async fn embed(&self, req: EmbeddingRequest) -> Result<EmbeddingResponse, ProviderError>;
    async fn health(&self) -> ProviderHealth;
}
```

`CompletionRequest` carries the tool schema list, tool-use history, and a `structured_output: Option<schemars::Schema>` for JSON-Schema-constrained generation (9F). `CompletionChunk` is a sum type (`Token(String)` / `ToolCallStart{..}` / `ToolCallDelta{..}` / `ToolCallEnd` / `StopReason(..)`) so the UI can render partial state without provider-specific parsing.

### 2.2 Primary cloud provider — Claude Opus 4.7

`bld-provider/src/anthropic.rs` wraps `rig::providers::anthropic::Client` behind `ILLMProvider`. Default model `claude-opus-4-7`. Temperature 0.0 for agent turns; 0.7 is reserved for explicit brainstorm slash-commands (9E).

**Prompt-cache keep-alive (Claude TTL mitigation).** When a chat session is *active* (user has posted a message in the last 30 minutes and the tab is open), a 5-token no-op prompt is sent every **4 minutes** against the same prompt-cache prefix. This resets the cache TTL at ~4 % of a normal-turn cost, preserving the 80 %-cache-hit discount on the remaining session. A session config flag `keep_alive = false` disables it for cost-sensitive users. Not on by default for **background** sessions.

**Tool-search policy.** When `tool_schemas.len() > 20`, `tool_search_tool` is enabled on the Anthropic request. For shorter tool lists we rely on in-context tools (cheaper, no search round-trip). The threshold is a config constant `BLD_TOOL_SEARCH_THRESHOLD` defaulting to 20; revisitable via ADR amendment.

### 2.3 Secret handling

API keys are resolved through `keyring = "4"` (native backend per platform: `windows-native`, `linux-native` with sync-secret-service). `bld-provider::secrets` is the only module that loads a key; callers receive an opaque `ApiKey` newtype whose `Debug` and `Display` implementations render `"<redacted>"`. `Drop` zeroises the buffer. Logging is fine-tuned so the `anthropic` module can never emit a key even by accident: `tracing` field filters strip any span carrying `api_key = *`.

Keyring lookups follow a documented key-name policy: `bld/anthropic/api_key`, `bld/openai/api_key`, `bld/gemini/api_key`. `docs/05_RESEARCH_BUILD_AND_STANDARDS.md` documents the setup command per-OS.

### 2.4 Fallback provider ordering

Selection is made by `bld-provider::Router` (see ADR-0016 for the local half):

1. User-pinned provider (set in `.greywater/bld_config.toml`), if any.
2. `anthropic:claude-opus-4-7` — primary.
3. `mistralrs:qwen-coder-7b-q4k` — local primary (9F).
4. `llama-cpp-2:qwen-coder-7b-q4k-gbnf` — local fallback (9F).
5. `OfflineMode` — read-only tools only; agent responds "offline; please reconnect".

Selection rule for a given turn: walk the list; pick the first whose `health()` returns `Ok`. A `ProviderError::RateLimited` demotes the provider for 60 s.

### 2.5 Contract tests

Every implementation of `ILLMProvider` runs against a shared `provider_contract(p: impl ILLMProvider)` in `bld-provider/tests/contract.rs`. 20 prompts (8 completion / 6 tool-use / 3 streaming / 3 structured-output). Structural gates:

- Non-empty response on valid input.
- JSON-schema-valid tool calls when `structured_output` is set.
- Correct stop reason emitted.
- `embed` returns `(768,)` vectors for every provider that advertises the capability.

Pass rate < 95 % on any provider fails CI. Since this runs against real APIs, the cloud leg of the test is gated by an env var (`BLD_CONTRACT_TESTS_CLOUD=1`) to avoid burning credits on every commit; local legs always run.

## 3. Alternatives considered

- **Hand-rolled HTTP per provider.** Rejected: too much duplicated state-machine code for streaming, and Anthropic's `/v1/messages` format diverges enough from OpenAI's that a shared abstraction is worth paying for.
- **`llm`-chain crate.** Rejected: too opinionated on prompt templates; we want the chat-history and tool-use surface to be our shape, not a library's.
- **Only cloud in v1, local in a future phase.** Rejected: non-negotiable #18 forbids shipping anything that leaks secrets under failure, and we want offline-mode operable for `docs.search`, `scene.query`, and `code.read` on day one of 9A — defence-in-depth against an API outage during a demo.

## 4. Consequences

- One trait implementation per backend; replacements are local.
- Contract tests make regressions on any provider visible.
- Keep-alive pings add ~$0.01/hr/active-session; configurable off.
- Keyring becomes a required platform dep — documented in `BUILDING.md`.

## 5. Follow-ups

- ADR-0011 defines transport (MCP / elicitation) riding on top of the provider.
- ADR-0016 defines the local-inference half (mistral.rs primary, llama.cpp fallback).
- `docs/02 §13` and `docs/04 §3` amended to reflect Rig as the carrier and the revised `sqlite-vec` / `mistral.rs` decisions.

## 6. References

- Rig: <https://docs.rs/rig-core>
- Anthropic prompt caching & TTL: Anthropic API changelog, 2026-Q1.
- `tool_search_tool`: Anthropic API reference, 2026-03.
- `keyring-rs` 4.0.0-rc.3 platform features: <https://docs.rs/keyring>

## 7. Acceptance notes (2026-04-21, wave 9F close)

- `bld-provider` now exports four in-tree providers behind the same
  `ILLMProvider` trait: `ClaudeProvider` (default-features,
  HTTP-streaming against Anthropic `/v1/messages`), `MockProvider`
  (deterministic, always-on, the default in tests),
  `MistralrsProvider` and `LlamaProvider` (feature-gated stubs per
  ADR-0016 §7). Keystore lookup goes through `bld_provider::keystore`
  which wraps `keyring-rs` and falls back to env vars in headless CI.
- The `HybridRouter` added in wave 9F is *also* an `ILLMProvider`
  implementation, so every downstream consumer (agent loop, session
  manager, replay) depends on a single polymorphic handle regardless
  of whether the active backend is cloud, local, or a hybrid of both.
  This keeps §2 intact and makes offline-mode a runtime decision, not
  a wiring decision.
- Contract tests (`cargo test -p bld-provider`) cover the
  deterministic providers on every push; the `BLD_CONTRACT_TESTS_CLOUD=1`
  gate for the Claude leg is unchanged.

---

## ADR file: `adr/0011-bld-mcp-transport-and-elicitation.md`

## ADR 0011 — BLD MCP transport & elicitation policy

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 9 opening, wave 9A)
- **Deciders:** Cold Coffee Labs engine group (Claude Opus 4.7 drafting; founder final)
- **Supersedes:** —
- **Superseded by:** —
- **Related:** `CLAUDE.md` non-negotiables #18 (no secrets committed/into model context); `docs/01_CONSTITUTION_AND_PROGRAM.md` §2.5; `docs/04_SYSTEMS_INVENTORY.md §13`; MCP spec 2025-11-25 (<https://modelcontextprotocol.io/specification/2025-11-25>); ADR-0007 (BLD C-ABI freeze); ADR-0010 (provider abstraction); ADR-0015 (governance — forward reference).

## 1. Context

BLD speaks **Model Context Protocol (MCP)** 2025-11-25 to:

1. Our own editor chat panel (in-process client).
2. External MCP clients — Claude Code, Cursor, MCP Inspector, any studio's CI harness.

Shipping one protocol for both is the whole point: the engineering we do internally becomes what external tools drive. The implementation question is *which MCP feature set* and *how elicitation is used safely*.

Pinned from `docs/01_CONSTITUTION_AND_PROGRAM.md §2.5`: rmcp (official Rust SDK), schema version `2025-11-25`. Not litigated here — carried forward. The decisions below concern *how* we use that SDK.

### 1.1 Research deltas since docs were written

- MCP 2025-11-25 elicitation has **two modes**: `form` (structured data entry) and `url` (external browser-driven). **Servers MUST NOT use `form` mode for secrets** (passwords, API keys, payment credentials). Only `url` is acceptable for those — the spec is explicit. Our earlier wording in `docs/06_ARCHITECTURE.md §3.7` did not draw this distinction; it does now.
- `rmcp` 1.x supports both stdio and **Streamable HTTP** transports, with elicitation feature-gated. Streamable HTTP was a planned 9E polish item — it remains there, out of scope for 9A.

## 2. Decision

### 2.1 Transport layers

- **Default transport: stdio.** BLD ships as a library loaded in-process by `gw_editor` and exposes its MCP server over a pair of OS pipes owned by `bld-mcp`. The in-process chat-panel client bridges frames directly (no pipes). External clients spawn `bld_mcp_host` (a thin exe binary built in 9E) and talk over stdio.
- **Secondary: Streamable HTTP** (9E polish). Disabled by default; user enables in `bld_config.toml`. Listens on `127.0.0.1:47111` (unprivileged, localhost-only). Authentication: a UUID token minted at first run, stored via `keyring`; clients present it via `Authorization: Bearer`.

The editor's chat panel uses neither transport. It calls `bld-mcp`'s in-process API directly through the `BldSession*` handle (Surface P v3 — see §2.4). This avoids marshalling JSON for the common case and keeps first-token latency low.

### 2.2 Capabilities

Advertised (declared in `bld-mcp/src/server.rs`):

```json
{
  "capabilities": {
    "tools":       { "listChanged": true },
    "resources":   { "subscribe": true, "listChanged": true },
    "prompts":     { "listChanged": true },
    "elicitation": { }
  },
  "serverInfo": { "name": "bld", "version": "0.9.0" },
  "protocolVersion": "2025-11-25"
}
```

`listChanged` on tools/resources/prompts fires when a tool/resource/prompt set changes at runtime — e.g. a hot-reloaded gameplay module re-registers its component-tools.

### 2.3 Elicitation modes — when to use which

BLD emits elicitations for two reasons: (a) **human approval** of a proposed action, (b) **secret input** the agent cannot see.

| Use case                         | Mode   | UI                              | Who enforces |
|----------------------------------|--------|----------------------------------|--------------|
| Confirm destructive tool         | form   | Diff panel + Approve / Deny      | `bld-governance` |
| Enter a provider API key         | url    | External browser -> token-write  | `bld-provider::secrets` |
| OAuth flow                       | url    | External browser                 | `bld-provider::secrets` |
| Choose among N options           | form   | Radio group                      | `bld-governance` |
| Accept a file-write              | form   | Unified diff in editor           | `bld-governance` |
| SSH/signing passphrase           | url    | External browser -> keyring-write | `bld-provider::secrets` |

**Enforcement.** `bld-mcp::elicit::validate_request` statically checks that any `ElicitationMode::Form` request does **not** carry fields matching a compile-time regex list (`password`, `api[_-]?key`, `secret`, `token`, `passphrase`, `credential`, `ssn`, `card[_-]?number`, CVV etc.). A match is a hard error — elicitation is rejected and an audit event is emitted. Defence in depth, not trust.

### 2.4 Surface P ABI bumps

Phase 9 requires two additive ABI bumps per ADR-0007 additive-only policy:

- **`BLD_ABI_VERSION = 2`** (lands in wave 9B). Adds:
  - `gw_editor_enumerate_components(EnumFn cb, void* user)` — iterates the reflection registry for `bld-tools` `component.*` autogen.
  - `gw_editor_run_command(uint32_t opcode, const void* payload, size_t payload_bytes)` — enqueue a serialised `ICommand` for the main thread to `CommandStack::execute`.
  - `gw_editor_run_command_batch(uint32_t opcode_count, const uint32_t* opcodes, const void* payloads, const size_t* payload_offsets)` — transaction bracket (multiple opcodes, single undo entry).
- **`BLD_ABI_VERSION = 3`** (lands in wave 9E). Adds:
  - `BldSession*` opaque handle (multi-session support).
  - `gw_editor_session_*` family for session create/destroy/route.

ADR-0007 is amended in a sibling commit to carry this versioning trail in the ABI version log.

### 2.5 Transport-loop discipline

Single `tokio` runtime (`Builder::new_multi_thread().worker_threads(2)`), owned by `bld-mcp`. All providers, RAG, watcher, and agent share it. No stray `std::thread::spawn`. This mirrors non-negotiable #10 on the C++ side — same policy, different language.

Cancellation is a `tokio_util::sync::CancellationToken` passed into every long-running call. Ctrl+C in an external MCP client flows through the transport's cancel signal into this token.

## 3. Alternatives considered

- **Custom JSON-RPC framing.** Rejected: MCP 2025-11-25 is the explicit wire protocol per `docs/01_CONSTITUTION_AND_PROGRAM.md §2.5`; we use `rmcp` verbatim.
- **gRPC transport.** Rejected: no demonstrated MCP interop today; cost not worth vs. Streamable HTTP.
- **Default Streamable HTTP.** Rejected: stdio is lower-latency for the in-process common case, and avoids the "localhost port is open" footprint for non-power-users.

## 4. Consequences

- External MCP clients (Claude Code, Cursor) can drive `gw_editor` with no new code on our side once the tool surface stabilises in 9B/9D.
- Secret-input flows cannot accidentally round-trip through `form`-mode elicitation — compile-time refusal.
- One audit record per elicitation (see ADR-0015) with `{mode, decision, duration_ms}`.

## 5. Follow-ups

- Streamable HTTP default-off in 9E; `bld_mcp_host` binary for external clients.
- Per-transport contract tests in `bld-mcp/tests/transport.rs`.

## 6. References

- MCP spec: <https://modelcontextprotocol.io/specification/2025-11-25>
- `rmcp`: <https://docs.rs/rmcp>
- Anthropic elicitation guidance: MCP draft-to-final notes, 2025-11.

---

## ADR file: `adr/0012-bld-tool-macro-and-component-autogen.md`

## ADR 0012 — `#[bld_tool]` proc macro & component autogen

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 9 wave 9B opening)
- **Deciders:** Cold Coffee Labs engine group (Claude Opus 4.7 drafting; founder final)
- **Supersedes:** —
- **Superseded by:** —
- **Related:** `CLAUDE.md` #6 (no two-phase init), #19 (phase-complete execution); ADR-0007 (C-ABI freeze & Surface P v2 additions); ADR-0010 (provider abstraction); ADR-0011 (elicitation policy); `docs/06_ARCHITECTURE.md §3.7`.

## 1. Context

The Phase 9 plan (§2.3) targets **79 MCP tools** at GA, partitioned across `scene.*`, `component.*`, `asset.*`, `build.*`, `runtime.*`, `code.*`, `vscript.*`, `debug.*`, `docs.*`, `project.*`. Two observations force a macro:

1. **Component count is unbounded.** Every reflected component (currently ~30, growing with gameplay code) needs four tools (`component.<name>.get`, `.set`, `.add`, `.remove`). Hand-writing 120+ tool functions and their schemas is a maintenance liability and a drift hazard — the reflection registry will be ground truth; the tool registry must not diverge.
2. **Schema generation is algorithmic.** JSON Schema for a tool's arguments is a mechanical function of the argument types. `schemars` already does it for any `#[derive(JsonSchema)]` type; what remains is the glue to bundle `(fn, schema, dispatch)` into a registry entry.

A proc macro — `#[bld_tool]` — is the natural seam.

## 2. Decision

### 2.1 Crate split

```
bld-tools-macros/      # proc-macro crate; only dep is syn/quote/proc-macro2
bld-tools/             # runtime registry + component autogen; depends on *-macros
```

Split is mandatory. Proc-macro crates cannot export non-macro items; keeping `bld-tools` as a regular library crate lets it own the `ToolRegistry`, the `#[bld_tool]` *re-export*, and the `bld_component_tools!()` function-like macro — all things the macro-authored code calls into at runtime.

### 2.2 Surface of `#[bld_tool]`

```rust
#[bld_tool(
    name = "scene.create_entity",
    tier = "mutate",
    description = "Create a new entity with an optional name",
)]
pub async fn create_entity(
    ctx: &ToolCtx,
    #[bld_arg(description = "Display name")] name: Option<String>,
) -> ToolResult<EntityHandle> {
    ctx.editor().create_entity(name.as_deref()).await
}
```

The macro:

1. Captures the fn name, the `tier`, the `description`, and each argument's name + type + optional `#[bld_arg]` attribute.
2. Generates a `struct __<fn>_Args { name: Option<String> }` that derives `serde::Deserialize` and `schemars::JsonSchema`.
3. Generates a `ToolDescriptor` constant with `name`, `description`, `input_schema` (via `schemars::schema_for!`), `output_schema` (from the return type if it implements `JsonSchema`), `tier`, and a `dispatch: fn(ToolCtx, serde_json::Value) -> BoxFuture<'_, serde_json::Value>`.
4. Emits a `linkme`-style or manual `static` submission into `bld_tools::INVENTORY` so the registry can scan-all at startup.
5. Emits a `#[cfg(test)] mod __bld_tool_<fn>_tests` that parses the generated schema through `schemars` and fails compile-time if the schema is invalid. Doctrine: the schema is a build-time assertion, not a runtime regression.

The macro **does not** emit `async fn` bodies — the user's function is passed through unchanged. The dispatch shim `await`s it and `serde_json::to_value`s the result. Panics are caught per-dispatch and converted to `ToolError::Panicked`; cancellation via `ToolCtx::cancel_token()`.

### 2.3 `bld_component_tools!()` — the autogen entrypoint

The function-like macro `bld_component_tools!(World)` expands at registry-build time (not compile time; it is invoked during `bld_register`, walking live reflection state). It:

1. Calls `gw_editor_enumerate_components(EnumFn, *mut ComponentList)` through the C-ABI (Surface P v2).
2. For each `ComponentDescriptor { name, hash, fields: [FieldDescriptor] }` emits **four** dynamically-registered `ToolDescriptor` values:
   - `component.<name>.get(entity: u64) -> <ComponentJsonShape>`
   - `component.<name>.set(entity: u64, patch: <JsonPatch>)` — merges non-null fields only (partial update semantics).
   - `component.<name>.add(entity: u64, init: Option<<ComponentJsonShape>>)` — calls `gw_editor_run_command(OP_ADD_COMPONENT, …)`; writes via CommandStack; undoable.
   - `component.<name>.remove(entity: u64)` — mutate-tier; triggers form-mode elicitation per ADR-0011.
3. The JSON shape of a component is built from its `FieldDescriptor` list: every field has a type-tag (`i32|i64|f32|f64|bool|string|vec3|dvec3|…`) mapped to `schemars::Schema` via a closed lookup table. Unknown type tags fail with `RegistrarError::UnsupportedField` at registration time (not at call time).

Tools registered this way are listed in `tools/list` with the same shape as macro-authored tools; external MCP clients cannot tell the difference.

### 2.4 Registry discipline

- `bld_tools::ToolRegistry` owns an indexed map of tool-id → `ToolDescriptor`.
- Duplicates at registration → `RegistrarError::DuplicateToolId`; no silent shadowing.
- `ToolCtx` carries the session handle, a `CancellationToken`, the `Governance` instance (for elicitation), and the user-owned `ToolUserData`. `ToolCtx` is **not** `Clone` — handles are single-owner across a tool call.
- Tool invocation path:
  1. `bld-agent` or `bld-mcp` calls `Registry::dispatch(tool_id, args_json, ctx)`.
  2. `Governance::require(tier, tool_id)` — elicitation if needed.
  3. `ToolDescriptor::dispatch(ctx, args_json)` — the macro-generated shim.
  4. Audit record emitted (ADR-0015).

### 2.5 Size discipline

Proc-macro crates explode compile times; `bld-tools-macros` stays lean:
- No `regex` dep. Use `syn::parse_quote!` and direct span traversal.
- No runtime dep on `bld-tools` (only the build-time dep on `syn`, `quote`, `proc-macro2`).
- `cargo bloat` regression gate: the macro crate's compile time ≤ 2 s (release) is a soft target; 5 s hard cap.

### 2.6 Compile-time schema validation

Every `#[bld_tool]` compile emits a `#[test] fn schema_of_<fn>_is_valid()`. These tests parse the emitted schema through `jsonschema::Validator::new` — a runtime check, but it runs in `cargo test`, not in production. CI enforces 100 % pass.

## 3. Alternatives considered

- **Hand-write all 79 tools + re-hand-write when components change.** Rejected: doesn't scale, doubles the drift surface, fails non-negotiable #14's spirit (the ground truth should not be hand-duplicated).
- **Runtime reflection only (skip the macro).** Rejected: we lose compile-time schema validation and the registry becomes a runtime data blob that can't be statically verified.
- **Use `clap` derive as the schema carrier.** Rejected: `clap` targets CLI parsing; JSON Schema fidelity is weak; `schemars` is purpose-built.

## 4. Consequences

- `bld-tools-macros` is the only proc-macro crate in the repo — lean dep surface, easy to audit.
- Component tools never drift from the reflection registry; a new component shows up in `tools/list` on the next editor restart.
- External MCP clients see consistent schemas whether the tool is macro-authored or component-autogen.

## 5. Follow-ups

- ADR-0015 (governance/audit) references the `tier` annotation from this ADR.
- `docs/05_RESEARCH_BUILD_AND_STANDARDS.md` gets a "Writing a new BLD tool" section pointing to the macro.

## 6. References

- `schemars` crate docs.
- `syn` / `quote` / `proc-macro2` — standard Rust macro stack.
- ADR-0004 (reflection registry) — input to the autogen.

## 7. Acceptance notes (2026-04-21, wave 9B close)

- `#[bld_tool]` now emits a dispatch shim for every sync, single-arg
  function whose return type is either `T: Serialize` or
  `Result<T, E: Display>`. The macro detects the return shape at parse
  time and calls `::bld_tools::dispatch_shim::serialize_ok` or
  `::bld_tools::dispatch_shim::serialize_result` accordingly. Tools
  whose signature does not fit the simple shape still register
  descriptors; they must provide a dispatch entry by hand.
- A parallel `inventory::collect!(&'static ToolDispatchEntry)`
  registry lives alongside the descriptor collector so the agent can
  look up a callable by id via `registry::find_dispatch`.
- `bld_tools::component_autogen` expands an editor reflection
  manifest (`ComponentManifest { name, stable_hash, fields }`) into
  runtime `AutogenTool` records: `component.<T>.get` (Read),
  `component.<T>.reset` (Mutate), `component.<T>.set_<field>` (Mutate).
  Fields typed `internal::*` are skipped. JSON schemas are generated
  from a closed type table (`Vec2`, `Vec3`, `Color`, int/float/bool/
  string); unknown types fall back to `{}` (any).
- `BridgeDispatcher` (in `bld-agent::bridge_dispatch`) hosts the
  actual runtime for `component.*` mutators via the `HostBridge` trait
  seam, matching §2.4's dispatch path. The bridge is mocked via
  `MockHostBridge` for agent-level tests.
- `schemars`-derived per-argument schemas are **not yet wired**; the
  Phase-9B shim deliberately emits `{"type":"object","additionalProperties":true}`
  for tools whose input type is `serde_json::Value` and defers
  fine-grained schema emission to a future wave. Every other plank of
  the ADR is implemented.

---

## ADR file: `adr/0013-bld-rag-architecture.md`

## ADR 0013 — BLD RAG architecture (two-index, hybrid retrieval, sqlite-vec)

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 9 wave 9C opening)
- **Deciders:** Cold Coffee Labs engine group (Claude Opus 4.7 drafting; founder final)
- **Supersedes:** The `sqlite-vss` decision in `docs/04_SYSTEMS_INVENTORY.md §13` (superseded here — `sqlite-vss` is deprecated upstream) and the single-model `Nomic Embed Code` decision (split into two-path design for VRAM-budget compliance).
- **Superseded by:** —
- **Related:** `CLAUDE.md` #1 (RX 580 budget), #16 (determinism); `docs/01_CONSTITUTION_AND_PROGRAM.md` §2.5; `docs/04_SYSTEMS_INVENTORY.md §13`; `docs/05_RESEARCH_BUILD_AND_STANDARDS.md §3.5`; ADR-0010 (provider abstraction — embedding goes through it).

## 1. Context

Wave 9C (weeks 052–053) stands up BLD's RAG subsystem. Design goals:

- Top-10 hit on any "where is X used?" query in < 200 ms P95 on a ~50 k-chunk engine-docs index.
- Fit the RX 580 8 GB VRAM budget while indexing (non-negotiable #1).
- Deterministic: same source tree → same embedding rows (up to model nondeterminism, which is pinned).
- Incremental: 1-file edit → 1-file re-embed, not whole-tree.
- **Two indices:** engine-docs index (ships prebuilt as a blob) and user-project index (built on first run, updated on save).
- Rust-only; no Python sidecar.

### 1.1 Research deltas since docs were written

- `sqlite-vss` has been **deprecated** by its original author. The successor is **`sqlite-vec`** (pure C, no Faiss dep, int8/binary/float vectors, multi-platform, loaded as a SQLite extension).
- **Nomic Embed Code** — the model `docs/02 §13` originally pinned for embeddings — is **7 B params / 26.35 GB**. Does not fit on an RX 580 at runtime. It remains the correct choice for the **prebuilt** engine-docs index (built on developer boxes with adequate VRAM), but the user-project runtime path needs a smaller model: `nomic-embed-text-v2-moe` (475 M total / 305 M active, 768-dim, Matryoshka output) via `fastembed-rs` (Candle backend).

Both are correct, for different jobs.

## 2. Decision

### 2.1 Two-index design

- **Engine-docs index** — prebuilt on Cold Coffee Labs' developer boxes (or CI) with **Nomic Embed Code**. Ships as `assets/rag/engine_docs.vec.sqlite` (pre-sized, read-only, content-hashed). On first run BLD copies it into `.greywater/rag/engine_docs.vec.sqlite` (mutable for symbol rank updates) and opens it. Re-generated at CI release time; content-hash in a sibling `.meta.json` so the runtime can detect drift.
- **User-project index** — built on-demand with `nomic-embed-text-v2-moe` via `fastembed-rs`. Lives at `.greywater/rag/project.vec.sqlite`. Built on first run in the background; updated incrementally on file save.

This split lets us keep "engine context" precisely aligned to the source tree we shipped (and updated only with releases), while the fast-moving user-project content gets a smaller-but-good-enough runtime embedder.

### 2.2 Storage — `sqlite-vec`

Loaded via `rusqlite::Connection::load_extension("sqlite_vec", None)`. Pinned to a specific `sqlite-vec` version in `Cargo.toml` (we ship the compiled extension alongside the executable so users don't need a system install).

Schema (applies to both indices):

```sql
-- Standard rows.
CREATE TABLE IF NOT EXISTS chunks (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    file          TEXT NOT NULL,
    file_hash     TEXT NOT NULL,         -- BLAKE3 of file content at embed time
    span_start    INTEGER NOT NULL,      -- byte offset
    span_end      INTEGER NOT NULL,
    kind          TEXT NOT NULL,         -- "fn" | "class" | "struct" | "doc" | "markdown"
    symbol_rank   REAL NOT NULL DEFAULT 0.0,
    text          TEXT NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_chunks_file ON chunks(file);
CREATE INDEX IF NOT EXISTS idx_chunks_kind ON chunks(kind);

-- Vector virtual table (sqlite-vec).
CREATE VIRTUAL TABLE IF NOT EXISTS chunk_vec USING vec0(
    id INTEGER PRIMARY KEY,
    embedding FLOAT[768]
);

-- Symbol graph (for structural retrieval + PageRank).
CREATE TABLE IF NOT EXISTS symbols (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    name          TEXT NOT NULL,
    file          TEXT NOT NULL,
    span_start    INTEGER NOT NULL,
    span_end      INTEGER NOT NULL,
    kind          TEXT NOT NULL,         -- "fn" | "class" | "struct" | "enum" | "macro"
    rank          REAL NOT NULL DEFAULT 0.0
);

CREATE INDEX IF NOT EXISTS idx_symbols_name ON symbols(name);

CREATE TABLE IF NOT EXISTS edges (
    src           INTEGER NOT NULL,
    dst           INTEGER NOT NULL,
    kind          TEXT NOT NULL,         -- "Call" | "Inherit" | "Contain" | "Use"
    PRIMARY KEY (src, dst, kind)
);

-- Embedding model provenance so mixed-model rows cannot slip in.
CREATE TABLE IF NOT EXISTS meta (
    k TEXT PRIMARY KEY,
    v TEXT NOT NULL
);
-- meta: { "model_name", "model_rev", "schema_version", "built_at_utc" }
```

### 2.3 Chunking — tree-sitter

`tree-sitter` grammars we depend on:

- `tree-sitter-cpp`, `tree-sitter-c`, `tree-sitter-rust`, `tree-sitter-glsl`, `tree-sitter-json`, `tree-sitter-md` (markdown).
- **Two custom grammars**, shipped in-tree at `bld/bld-rag/grammars/`:
  - `.gwvs` — the vscript IR grammar (ADR-0009).
  - `.gwscene` — text headers only (the binary body is not tree-sittable; chunker stops at the header).

Chunk boundaries respect function / class / struct / impl / namespace scopes. Mid-function cuts are forbidden; exception: a function body > 512 lines is split by sibling block boundaries, never mid-statement. Markdown chunks use H2/H3 as boundaries; long paragraphs are kept whole up to 1 024 tokens.

### 2.4 Symbol graph & PageRank

Second tree-sitter pass walks all function-call expressions, class-inheritance clauses, struct-containment relations, and type usages. The walk builds a `petgraph::DiGraph<SymbolId, EdgeKind>`. Ranks are computed via `petgraph::algo::page_rank` (α = 0.85, 64 iterations) at index build and after any batch of ≥ 100 symbol edges changes. Symbol rank writes back to `symbols.rank` and the derived `chunks.symbol_rank` (sum of ranks for symbols whose span overlaps the chunk; normalised).

### 2.5 Hybrid retrieval

For query `q`:

- **Lane A (lexical).** `grep`-like scan via `ripgrep` (through `grep` crate) for N_lex = 50 hits. Score = 1.0 for exact case-sensitive match, 0.5 for case-insensitive, 0.3 for substring. De-duped per chunk.
- **Lane B (vector).** Embed `q`; `vec0` nearest-neighbour for N_vec = 50 chunks. Score in `[0, 1]`, cosine similarity.
- **Lane C (symbol graph).** If any exact-name match in `q` exists in `symbols`, walk the 1-hop neighbourhood; score = `dst.rank`; N_sym ≤ 20.

Re-rank combining: `score = 0.4 * vec + 0.3 * lex + 0.3 * sym_rank_norm`. Top-10 returned to the agent.

### 2.6 Watcher — incremental re-index

`notify` + `notify-debouncer-mini` on the project root (and only under `.greywater/config/rag_roots.toml`-listed directories; `target/`, `build/`, `node_modules/` always excluded). Debounce 2 s. Embedding job deferred another 30 s after last edit — the **quiet window**. No embedding runs while typing.

On fire:
1. For each changed file, recompute its BLAKE3 hash.
2. If the hash matches the stored `file_hash`, skip.
3. Else: delete all `chunks` rows with `file = ?`, rechunk, re-embed, insert. Same for `symbols` and `edges` where `file = ?`.
4. If ≥ 100 edges changed, re-run PageRank and update `rank` columns.

### 2.7 Tools introduced by this ADR

- `docs.search(query: string, k?: int, scope?: "engine"|"project"|"all") -> [Hit]`
- `docs.get_file(path: string) -> string`
- `docs.get_symbol(name: string) -> [SymbolLoc]`
- `project.find_refs(name: string) -> [SymbolRef]`
- `project.current_selection() -> EntitySelection` (bridges to editor selection)
- `project.viewport_screenshot() -> BlobRef` (bridges to editor viewport)

All six are **Read-tier** (no mutations; no elicitation).

### 2.8 Determinism

Model names and revisions are written into the `meta` table. On open, BLD compares `meta.model_name` / `meta.model_rev` against the embedder currently wired; a mismatch forces a full re-index (engine-docs index re-copy; project index from scratch). This enforces non-negotiable #16 at RAG scope — the same project at the same commit rebuilt fresh yields bit-identical rows modulo model stochastic jitter (for which we set a seed in `fastembed::InitOptions::with_seed(0)`).

## 3. Alternatives considered

- **`sqlite-vss`.** Deprecated upstream. Rejected.
- **`tantivy` for everything (lexical + vector-ish).** Rejected: tantivy's vector support is less mature; running two backends complicates debugging. sqlite-vec gives us one store.
- **Qdrant as an external service.** Rejected: user-local tools should not require running a Docker container. Also violates the "RX 580 box is the deploy target" spirit.
- **`Swiftide` framework.** Cherry-pick the async streaming ingestion *pattern* (see §2.6); do not adopt the dependency — our chunker needs `tree-sitter-gwvs` and `tree-sitter-gwscene` grammars that are not first-class Swiftide citizens.

## 4. Consequences

- One extension (`sqlite-vec`) shipped alongside the editor executable; one-line `load_extension` at connection open.
- Two-path embedding (cloud-prebuilt large model + runtime small model) is explicit; no single-model compromise.
- PageRank is a batch job; it does not block the hot path.
- Watcher's 30-s quiet window is user-visible; documented in `BUILDING.md`.

## 5. Follow-ups

- `docs/02 §13` amended to reflect `sqlite-vec` + dual-model design.
- `docs/04 §3.5` updated to show the two embedding paths.
- Phase 17 will revisit whether symbol-graph ranks should roll up to cross-file scores.

## 6. References

- `sqlite-vec`: <https://github.com/asg017/sqlite-vec>
- Nomic Embed Text v2 MoE: <https://huggingface.co/nomic-ai/nomic-embed-text-v2-moe>
- `petgraph::algo::page_rank`: <https://docs.rs/petgraph>
- `tree-sitter-graph`: <https://docs.rs/tree-sitter-graph>

---

## ADR file: `adr/0014-bld-plan-act-verify-loop.md`

## ADR 0014 — Plan-Act-Verify agent loop & iteration bounds

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 9 wave 9D opening)
- **Deciders:** Cold Coffee Labs engine group (Claude Opus 4.7 drafting; founder final)
- **Supersedes:** —
- **Superseded by:** —
- **Related:** ADR-0005 (CommandStack — every mutation reversible); ADR-0010 (provider); ADR-0012 (tool macro + dispatch); ADR-0015 (governance/audit — forward reference); `CLAUDE.md` #19 (phase-complete), #20 (doctrine first).

## 1. Context

Wave 9D (weeks 054–055) ships the agent loop itself — the brain that takes a user goal, plans a tool-call sequence, executes it, and verifies the outcome. Bare "let the LLM emit tool calls until it stops" does not satisfy any of our constraints: we need bounded cost, bounded wall-clock, reversible actions, and a failure mode that escalates to the user instead of looping silently.

Three well-known agent patterns:

1. **ReAct** — thought/action/observation loop. Good for exploration; weak on long plans.
2. **Plan-and-Execute** — one-shot plan, N-step execution. Fragile when the plan is wrong.
3. **Plan-Act-Verify** — one plan per turn, per-step verification, re-plan on failure. Best balance for our workload (file + scene + build + runtime + tests in one turn).

We pick Plan-Act-Verify as the primary and fall back to ReAct on verify failures.

## 2. Decision

### 2.1 Loop shape (primary)

```text
USER_MSG ─┐
          ▼
      ┌─ PLAN (call ILLMProvider with system-prompt + RAG-context + tool-catalog +
      │        few-shot exemplars) → returns Steps[] where each Step is
      │        { tool_id, args_json_template, why, verify_predicate }
      │
      ├─ ACT step k:
      │     1. Governance check (elicitation if needed)
      │     2. Registry::dispatch(step.tool_id, args, ctx)
      │     3. Audit record emitted
      │
      ├─ VERIFY step k:
      │     if step.verify_predicate != None:
      │         run the predicate (cheap check — see §2.3)
      │         if predicate fails:
      │             → fall to REACT sub-loop (see §2.2)
      │     else:
      │         proceed to next step
      │
      └─ until Steps exhausted OR budget exhausted OR user cancels
```

At every step boundary the chat panel streams the plan's next step as a collapsible `> thinking` block, so the user watches reasoning live. After execution, the user can click "expand" to see raw tool JSON in + out — nothing is hidden.

### 2.2 Fallback: ReAct sub-loop

When a step's verify fails, the agent enters a ReAct sub-loop with a **hard cap of 10 iterations** and a specialised prompt: "Your previous action failed verify with output X. Propose a single next tool call." On iteration 10 without success, the sub-loop exits, the original plan is abandoned, and the agent **escalates to the user** with a structured error: `"I tried N things; none worked. Here's what I tried and why each failed."`

### 2.3 Verify predicates

Predicates run cheaply (≤ 50 ms) and only check that the action's *intended effect* is visible. They are not correctness proofs. Examples, keyed by tool family:

| Tool family           | Verify predicate                                               |
|-----------------------|----------------------------------------------------------------|
| `scene.create_entity` | `gw_editor_find_entity(handle) != 0`                            |
| `component.set`       | `gw_editor_get_field(entity, path) == expected_value`           |
| `code.apply_patch`    | Parse the patch; check every `+` line is in file; every `-` line is not |
| `build.build_target`  | Exit code == 0; diagnostic list empty at severity >= error      |
| `vscript.compile`     | `vscript::Program` constructs without error                     |
| `runtime.play`        | `gw_editor_is_playing() == true`                                |
| `docs.search`         | No predicate (reads are self-verifying)                         |

The predicate is produced by the **planner** (the LLM) as part of the plan. If the LLM skips a predicate, the agent supplies a default from a builtin table (`default_verify_predicate(tool_id)`). Non-predicate'd steps still pass through the audit channel.

### 2.4 Step budget — hard caps

- **Plan size cap**: 30 steps. A plan > 30 is rejected and re-planned with the caveat "break this into smaller plans".
- **Total steps per turn**: 30 (plan) + 10 * fallback ReAct fires; effective ceiling ≈ 40.
- **Wall clock per turn**: 5 minutes. Exceed → cancel, audit `turn_timeout`, escalate.
- **Tool-call cost budget**: configurable in `.greywater/bld_config.toml` (default: no cloud-cost cap; warn at 50 calls).

Every budget overrun is an audit event and a chat-panel message to the user; the agent never silently swallows a retry.

### 2.5 Hot-reload flow (the *Brewed Logic* exit path)

Canonical flow for the scripted eval "Add a `HealthComponent` that ticks down 1 HP per second; apply to player; verify in PIE":

```
1. docs.search("HealthComponent existing definition") → empty
2. code.apply_patch(adds gameplay/src/health_component.hpp + .cpp)
   → elicitation (form mode: unified diff) → approve
3. build.build_target("gameplay_module")
   → verify: diagnostics empty
4. runtime.hot_reload("gameplay_module")
   → verify: module_generation() incremented
5. scene.set_selection([player])
6. component.HealthComponent.add(player, {hp: 100, tick_rate: 1.0})
   → verify: gw_editor_get_field(player, "HealthComponent.hp") == "100"
7. runtime.play()
8. runtime.step_frame(60)  # run 1 second at 60 FPS
9. runtime.sample_frame("HealthComponent.hp", player)
   → verify: 99 ≤ hp ≤ 100 (tolerance: tick may or may not have fired exactly)
10. runtime.stop()
```

Acceptance for wave 9D is this plan executing end-to-end with **at most one** form-mode elicitation (the diff), all mutations on the CommandStack (Ctrl+Z unrolls everything), and a `HealthComponent` hp reading that trips the predicate.

### 2.6 Concurrency

One agent turn = one `tokio::task` on the shared runtime (ADR-0011 §2.5). The task holds a `CancellationToken`; the chat panel's "Stop" button triggers it. Cancel during a tool call: the tool dispatcher awaits the cancel signal and returns `ToolError::Cancelled`; the CommandStack partial-state is rolled back.

Multi-session (9E) runs each session on an independent task; sessions do not share agent state — only the governance decision cache (`tier` approvals).

### 2.7 Determinism caveat

The LLM is non-deterministic (temperature > 0 on the brainstorm slash-commands; 0.0 elsewhere but still sampling). The *agent loop* is deterministic: same plan, same tool dispatch order, same verify predicates, same audit sequence. The eval harness (Phase 9 plan §5) asserts on end-state predicates (ECS queries, file contents) — not on the plan shape.

## 3. Alternatives considered

- **Pure ReAct.** Rejected: brittle for multi-step code-edit + build workflows; easy to loop.
- **Pure Plan-and-Execute.** Rejected: when a plan step fails, we have no local recovery.
- **Tree-of-Thought search.** Rejected for v1: burns 5–20× the token budget; revisit when token costs drop further.

## 4. Consequences

- One loop shape governs all agent turns. Easy to reason about, easy to audit.
- Step budgets are hard; no infinite loops in production.
- Failures escalate to the user, always.
- Hot-reload exit flow is the acceptance contract of the *Brewed Logic* milestone.

## 5. Follow-ups

- Phase 14 may introduce agent-driven multiplayer replication test generation — which will need the same loop with networking verify predicates.
- Phase 17 may add a `code_review_tool` that wraps the plan in a peer-review pass before execute.

## 6. References

- ReAct: Yao et al., 2022.
- Plan-and-Execute: LangChain community pattern.
- Claude agent patterns: Anthropic docs, 2026-03.

## 7. Acceptance notes (2026-04-21, wave 9D close)

- `bld-agent::composite::CompositeDispatcher` routes tool calls by
  prefix across family-specific dispatchers: `scene.*`/`component.*`
  via `BridgeDispatcher`, `rag.*` via `RagDispatcher`, plus the
  Phase-9D newcomers (`build.*`, `code.*`, `vscript.*`, `runtime.*`,
  `debug.*`) through their typed dispatchers in
  `bld-agent::build_tools`. Adding a new family is one-line.
- The `BuildExecutor`, `CodeHost`, `VscriptHost`, `RuntimeHost`, and
  `DebugHost` traits define the full editor surface the agent uses
  during Act; each has a deterministic `Mock*` implementation so
  Plan-Act-Verify can be exercised in unit tests without a running
  editor. `FsCodeHost` is a `SecretFilter`-gated filesystem-backed
  `CodeHost` so `code.write` and `code.patch` can't escape project
  root or whitelisted extensions.
- `ScriptedHotReload` in `bld-agent::build_tools` is the literal
  encoding of §2.5's acceptance script — `code.write` →
  `build.invoke` → `runtime.hot_reload` → `scene.set_selection` →
  `component.*.add` → `runtime.play`/`step_frame`/`sample_frame`/`stop`.
  The integration test drives it against mock hosts and asserts the
  per-step verify predicates plus an ordered audit trail.
- Step budget, verify-fail-budget, and re-plan cap are enforced in
  `Agent::run_turn`; exceeding any budget returns
  `AgentError::BudgetExhausted` and the CommandStack rollback fires
  on the next user `undo`.

---

## ADR file: `adr/0015-bld-governance-audit-replay.md`

## ADR 0015 — BLD governance, audit, replay & secret filter policy

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 9 wave 9E opening)
- **Deciders:** Cold Coffee Labs engine group (Claude Opus 4.7 drafting; founder final)
- **Supersedes:** —
- **Superseded by:** —
- **Related:** `CLAUDE.md` non-negotiables #18 (no secrets in repo or model context); ADR-0007 (C-ABI freeze — Surface P v3 session handles); ADR-0011 (elicitation policy); ADR-0012 (tool tier annotation); ADR-0014 (agent loop).

## 1. Context

Wave 9E is where BLD graduates from "cool chat panel" to "shippable product". Three threads converge here:

1. **Governance.** Not every tool should run unattended. The agent must not `asset.delete` or `scene.destroy_entity` without confirmation; it should freely `docs.search` or `project.find_refs`.
2. **Audit.** Every tool call is a persisted event. Incident review, replay, and cost attribution depend on this being complete and tamper-evident.
3. **Secret filter.** A file-read path that resolves to `.env` cannot reach the model — not once, not ever. Defence in depth, not trust.

Failure of any of these is a *trust* failure, not merely a bug: secret leakage through a model context is reputation-ending for a studio.

## 2. Decision

### 2.1 Three-tier authorization

Every tool carries one of three tiers, set at `#[bld_tool(tier = "…")]` time (ADR-0012):

| Tier       | Elicitation default | Approval cache default | Examples                                                        |
|------------|---------------------|--------------------------|-----------------------------------------------------------------|
| `Read`     | None                | N/A                      | `docs.search`, `project.find_refs`, `scene.query`, `code.read_file` |
| `Mutate`   | Form                | 1 h per tool_id          | `scene.create_entity`, `component.set`, `code.apply_patch`, `vscript.write_ir` |
| `Execute`  | Form, per-call (no cache) | None              | `build.build_target`, `runtime.play`, `runtime.hot_reload`, `asset.delete` |

**Cache semantics.** On first approval of a `Mutate`-tier tool the user can tick "remember for this session (1 h)". Subsequent calls of *the same tool_id with comparable args* are approved silently. "Comparable" means structurally equivalent JSON minus timestamps; defined in `governance::args_are_comparable`. Never comparable across sessions.

**`Execute` is never cached.** Every invocation asks. Rationale: Execute-tier tools have real-world side effects (builds, launches, deletes) whose safety depends on *current* state, not "was safe an hour ago".

Users can **promote** a `Mutate` tool to always-approve (within a session) via a slash command `/grant scene.create_entity`. Promotions persist in `.greywater/agent/grants.toml` per-project if the user pins them.

### 2.2 Audit log

**Format.** Append-only JSONL at `.greywater/agent/audit/{session_id}/{yyyy-mm-dd}.jsonl`. One record per tool invocation. Records never rewritten — errors append a new record referencing the prior one by sequence number.

**Record shape:**

```json
{
  "seq":            129,
  "ts_utc":         "2026-04-24T18:44:02.147Z",
  "session_id":     "01HW3J9ZC9XN3NGTSTXQG0VZ7K",
  "turn_id":        "01HW3JA7R5P8Y6TVBDEJRC6X2K",
  "tool_id":        "scene.create_entity",
  "tier":           "Mutate",
  "args_digest":    "blake3-b64:Vv4o8m…",
  "args_bytes":     217,
  "result_digest":  "blake3-b64:9jF2xu…",
  "result_bytes":   64,
  "tier_decision":  "ApprovedCached",
  "elapsed_ms":     13,
  "provider":       "anthropic:claude-opus-4-7",
  "outcome":        "Ok"
}
```

**Digests.** BLAKE3 over the canonical JSON form (keys sorted, no whitespace). Full payloads are stored only when the per-session archive flag is **on** (off by default) in a sibling `.archive.jsonl` — because shipping every tool args dump to disk by default is a privacy and cost footgun.

**Rotation.** One file per UTC day per session. Sessions > 30 days old are eligible for compaction via `tools/bld_audit_compact` (9E polish or later).

### 2.3 Session replay — `gw_agent_replay`

A new binary shipped from `tools/bld_replay/`. Usage:

```
gw_agent_replay <session_id> [--from <seq>] [--to <seq>]
                [--dry-run] [--allow-mutate]
                [--bit-identical-gate]
```

Semantics:

- `--dry-run` (default): re-dispatches every tool call but replaces **every mutating-tier** tool with a no-op that records what *would have happened*. Output: a diff against the live scene/FS.
- `--allow-mutate`: actually re-applies mutations. Combined with `git stash && gw_agent_replay <id> --allow-mutate` this reproduces a session's end state on a clean checkout.
- `--bit-identical-gate`: asserts the resulting `.gwscene` + project file tree hashes match the originally-captured post-session tree hash (recorded at session end by the audit channel as a `session_end` record). Exit code 0 = bit-identical; non-zero = drift (printed diff).

The *bit-identical gate* is the replay leg of non-negotiable #16 (determinism). The eval harness (Phase 9 plan §5, row "Replay round-trip") uses `--bit-identical-gate` directly.

### 2.4 Secret filter

**Compile-time deny-list** (checked into `bld-governance/src/secret_filter.rs` as a const `&[&str]`):

```
.env          .env.*       *.key        *.pem        *_rsa        id_*
.ssh/*        .config/gcloud/*           .aws/credentials
*.pfx         *.p12        *.keystore   *.jks
*.htpasswd    secrets.yml  secrets.yaml docker-compose.override.yml
```

**Runtime `.agent_ignore`** at project root — gitignore semantics — extends the deny-list per project. Parsed once at session open; not watched (no race window at open).

**Enforcement.** Every file path entering a tool argument passes through `secret_filter::check(path)`. A hit returns `ToolError::Forbidden` and emits an audit event with `outcome: "SecretFilterHit"` and **no `args_digest`** (the path itself might leak content). Defence in depth: the filter runs *before* the tool dispatch, and every tool that takes a path MUST go through `ToolCtx::read_file` / `ToolCtx::write_file`, never `std::fs` directly. A clippy lint (custom, in `bld-tools/src/lint.rs`) blocks direct `std::fs::*` in any function marked `#[bld_tool]` — compile-time belt + runtime braces.

**Env-var filter.** Similar treatment for environment variables the agent tries to read: any env-name matching `(AWS|AZURE|GCP|ANTHROPIC|OPENAI|GEMINI|GITHUB|STRIPE|NPM)_[A-Z_]*` pattern is masked.

**Pre-commit hook.** `tools/hooks/bld_secret_scan.py` runs on every `git commit` of an agent-authored diff and re-scans the diff for secret patterns (AWS access keys, PEM blocks, JWT tokens, Discord webhooks, Slack tokens, Stripe keys). A hit blocks the commit. Docs reference in `BUILDING.md`.

### 2.5 Multi-session — Surface P v3

`BLD_ABI_VERSION = 3` (lands in 9E, additive-only per ADR-0007):

```c
typedef struct BldSession BldSession;  /* opaque */

/* Introduced in v3. */
BldSession* gw_editor_session_create(const char* display_name);

/* Introduced in v3. */
void gw_editor_session_destroy(BldSession* session);

/* Introduced in v3. */
const char* gw_editor_session_id(BldSession* session);  /* ULID string */

/* Introduced in v3. Route an arbitrary MCP request into a named session. */
const char* gw_editor_session_request(BldSession* session,
                                       const char* request_json);
```

Chat panel tabs each own a `BldSession*`. Sessions do **not** share: tool registries are shared globally (registered at `bld_register`), but each session has its own history, approval cache, audit stream, and agent `CancellationToken`. 

### 2.6 Slash commands as MCP Prompts

`/diagnose`, `/optimize`, `/explain`, `/test`, `/grant <tool>`, `/revoke <tool>`, `/cancel` — each a single template living in `bld-mcp/prompts/`. Editor uses `prompts/get` to fetch the filled prompt, `completion/complete` to resolve arguments.

A slash command is never auto-approved — it goes through the same tier gating as any other interaction.

### 2.7 Cross-panel integrations

Editor-side only (C++); the right-click menu + panel hook APIs live in `editor/panels/` and call `bld-mcp::ingress::send_with_resource` to pass the context resource (entity handle, diagnostic, profile sample, vscript node slice) alongside the user prompt.

Wired in 9E:
- Outliner → "Ask BLD about this entity" → resource `entity://<handle>`.
- Console compile error → "Fix this" → resource `diagnostic://<error_id>`, tool list primed to `code.*`.
- Stats panel frame-spike click → "Diagnose" → resource `profile://<sample_id>`.
- VScript panel node → "Explain this node" → resource `vscript_node://<graph>:<node_id>`.

## 3. Alternatives considered

- **Binary audit log.** Rejected: JSONL is human-greppable during an incident; binary needs a tool to read.
- **Always archive full payloads.** Rejected: privacy + disk. Opt-in preserves the choice without making it the default.
- **Sole runtime `.agent_ignore`, no compile-time deny-list.** Rejected: a user without an ignore file would be undefended. Defaults must be safe.
- **Session replay against the live scene.** Rejected: the clean-checkout replay is the only way to prove bit-identical.

## 4. Consequences

- Every agent action is a persistent, diffable record.
- Every destructive action is reversible (CommandStack) and replayable.
- No secret-shaped path reaches the model.
- Multi-session is first-class; tabs cannot cross-contaminate.

## 5. Follow-ups

- Phase 16 (compliance) will fold BLD audit into the GDPR data-collection UI.
- Phase 24 (hardening) runs `cargo-mutants` against `bld-governance`; any surviving mutant blocks release.

## 6. References

- ULID for session IDs (crate `ulid`).
- BLAKE3 for digests (crate `blake3`).
- MCP 2025-11-25 prompts & completion spec.

## 7. Acceptance notes (2026-04-21, wave 9E close)

- Slash commands now parse and execute in-crate via
  `bld_governance::slash::{SlashCommand, SlashDispatcher}`. Wave 9E
  ships `/grant`, `/revoke`, `/reset`, `/offline`, `/model`,
  `/sessions`, `/help`; the remaining MCP-prompt slash commands
  (`/diagnose`, `/optimize`, `/explain`, `/test`, `/cancel`) stay in
  `bld-mcp/prompts/` per §2.6 and are not in scope for this wave.
- `SessionManager` realises §2.5 multi-session isolation in Rust
  first — each `SessionHandle` owns its own `Governance`,
  `AuditLog`, mutable `SessionContext` (offline flag, selected
  model, panel context), and `KeepAlive` heartbeat. The C-ABI
  `BldSession*` wrappers layer over this manager in `bld-bridge`.
- `ElicitationFactory` is the seam for per-session elicitation
  handlers so cross-panel context (`entity://`, `diagnostic://`,
  `profile://`, `vscript_node://`) can be injected before
  `Governance::require` runs.
- `gw_agent_replay` CLI gained `--json`, `--bit-identical`,
  `--limit N`, `--filter <prefix>`, and `-q/--quiet` in the same
  wave; CI now consumes the JSON output and the `--bit-identical`
  flag is required in release pipelines.
- Secret filter, audit log, and pre-commit hook changes already
  landed in wave 9A — no revisions in 9E.

---

## ADR file: `adr/0016-bld-local-inference-and-offline-mode.md`

## ADR 0016 — Local inference & offline mode

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 9 wave 9F opening)
- **Deciders:** Cold Coffee Labs engine group (Claude Opus 4.7 drafting; founder final)
- **Supersedes:** the `candle-core` + `llama-cpp-rs` pairing in `docs/04_SYSTEMS_INVENTORY.md §13` (demoted — `llama-cpp-rs` becomes the fallback-of-fallback).
- **Superseded by:** —
- **Related:** ADR-0010 (provider abstraction); ADR-0013 (RAG — embeddings are local); `CLAUDE.md` #1 (RX 580 budget), #18 (no secrets into model context).

## 1. Context

Wave 9F (weeks 058–059) closes Phase 9 with local hybrid + offline-mode + structured output + Miri clean. Research deltas:

- **`mistral.rs` v0.8** — Rust-native LLM inference; built-in MCP client; **JSON-Schema-constrained structured output** (`Model::generate_structured`); CUDA/Metal/CPU backends; ships `mistralrs tune` for auto-tuning kernels to hardware. Reliability on small models (Qwen 2.5 Coder 7B Q4K) for structured tool calls has been demonstrably > 95 % in community benchmarks.
- `candle` — still a solid choice for **embeddings** (small encoder models). Not competitive vs. mistral.rs for the **generation** path at comparable quality at our Q4K budget.
- `llama-cpp-2` (newer binding than `llama-cpp-rs`) retains the `json_schema_to_grammar` → GBNF → `LlamaGrammar::parse` pipeline for platforms where mistral.rs cannot initialise.

## 2. Decision

### 2.1 Backend ordering (local half of ADR-0010's Router)

1. **mistral.rs — primary local.** Model: Qwen 2.5 Coder 7B, Q4K quantisation, 4 GB on-disk, ~6 GB VRAM peak on RX 580. `generate_structured` for every tool-call turn; free-text for chat turns.
2. **llama-cpp-2 — fallback of fallback.** Only reached when mistral.rs fails `init()` (no CUDA/Metal/CPU path). GBNF grammars compiled from tool schemas via `schemars::schema_for!` → `json_schema_to_grammar` → `LlamaGrammar::parse`. Correctness identical; perf lower.
3. **candle — embeddings only.** `nomic-embed-text-v2-moe` (see ADR-0013). Not used for generation.

### 2.2 Structured output contract

Every tool-call turn that targets a local provider emits a `CompletionRequest` with `structured_output: Some(tool_calls_schema)`. The schema is built at registry-build time from the tool catalogue:

```text
TopLevel {
  tool_calls: Vec<{
    tool_id: enum { "scene.create_entity" | "component.set" | ... },
    args: oneOf { ... per-tool arg schemas ... }
  }>,
  reasoning: Option<String>,
  is_final: bool
}
```

mistral.rs `generate_structured` constrains the decoder; the output is always a valid JSON per the schema. For the llama-cpp-2 path the schema is compiled to GBNF at provider init and cached.

Provider failures at schema-violation time: impossible by construction (constrained decoding can't emit invalid JSON); but in case of decoder internal error the response is translated to `ProviderError::Decoder` and the agent falls back per §2.1.

### 2.3 Offline mode

Offline mode is **not** silent degradation. It is an explicit state visible in the chat panel with a non-dismissable banner:

```
╔════════════════════════════════════════════════════════╗
║  OFFLINE MODE — cloud provider unreachable.            ║
║  Available: docs.search, project.find_refs, scene.query ║
║  Unavailable: build.*, runtime.*, code.apply_patch      ║
║                                                          ║
║  Reconnect and use /reconnect to return to full mode.   ║
╚════════════════════════════════════════════════════════╝
```

Triggered by:
- Any provider `health()` returning `Unreachable`.
- User command `/offline`.
- `.greywater/bld_config.toml` key `bld.mode = "offline"`.

While offline, the Router rejects any tool whose tier is `Execute`; `Mutate` is allowed if it has a pure-local implementation (`code.apply_patch` is one — it does not need the cloud; but the agent reasoning does, so if *only* the cloud is out we still disallow mutations; if the local model is available, mutations go through).

The banner is latched off only when a provider `health()` returns `Ok` again; there is no flicker.

### 2.4 Contract tests (shared across providers)

Already introduced in ADR-0010 §2.5. In 9F we **add local legs** to the contract suite:

```rust
#[test] fn contract_anthropic_cloud()     { /* gated by env */ }
#[test] fn contract_mistralrs_qwen_coder() { /* always runs on CUDA/Metal/CPU CI */ }
#[test] fn contract_llama_cpp_qwen_coder() { /* always runs */ }
```

Pass-rate gate: each provider ≥ 95 % on the 20-prompt suite. Any single-provider regression fails CI independently.

### 2.5 Cost benchmark

`cargo bench -p bld-provider --bench cost` runs the same 20-prompt suite against Claude Cloud (paid, env-gated) and mistral.rs local. Outputs a markdown report under `docs/02_ROADMAP_AND_OPERATIONS.md<date>-bld-cost.md` with totals. Acceptance gate for the 9F exit: local suite cost $0 with ≥ 70 % of Claude's pass rate. Higher is better; 70 % is the floor.

### 2.6 Miri discipline

`cargo +nightly miri test -p bld-ffi -p bld-bridge` runs nightly. Flags: `-Zmiri-strict-provenance -Zmiri-retag-fields`. Any UB in the boundary crates fails the nightly gate. We accept Miri's slower runtime (≈ 20× native); the nightly cadence is the trade-off.

All `unsafe` in `bld-ffi` is grouped into small, heavily-documented blocks per non-negotiable #5 spirit; each block has a `SAFETY:` comment that cites the invariant it relies on.

### 2.7 Auto-tune & on-first-run setup

`mistralrs tune` runs on first-run detection (no tuned kernels on disk). The user sees a one-time dialog: "First-run optimisation — this will take 2–5 minutes. Click Skip to run untuned (slower)." The tuning artefacts live at `.greywater/bld/mistralrs/cache/` and are machine-scoped (not committed).

## 3. Alternatives considered

- **Skip local for v1.** Rejected: offline mode is in the Phase 9 exit criteria; without local there is no offline.
- **Python child process for local inference.** Rejected: non-negotiable #4 (Rust only in BLD); also defeats the Rust single-runtime discipline.
- **Custom GGUF loader.** Rejected: reinvention of llama.cpp; no engineering value.

## 4. Consequences

- BLD works offline for read-heavy tasks.
- Local model runs constrained-JSON; no GBNF pipeline in the common path.
- Miri is the nightly UB gate on the C-ABI boundary.
- First-run latency is user-visible (tuning); documented.

## 5. Follow-ups

- `docs/02 §13` amended: `sqlite-vss → sqlite-vec`, `llama-cpp-rs → mistral.rs primary + llama-cpp-2 fallback`, `Nomic Embed Code → dual-path (prebuilt large + runtime small)`.
- `docs/04 §3` amended to reflect Rig as the carrier and the structured-output strategy.
- `docs/05 §Phase 9` row flips to `completed` on milestone sign-off.

## 6. References

- `mistral.rs` v0.8: <https://github.com/EricLBuehler/mistral.rs>
- `llama-cpp-2`: <https://docs.rs/llama-cpp-2>
- Qwen 2.5 Coder: <https://huggingface.co/Qwen/Qwen2.5-Coder-7B-Instruct>
- Miri: <https://github.com/rust-lang/miri>

## 7. Acceptance notes (2026-04-21, wave 9F close)

- `bld_provider::hybrid::HybridRouter` implements `ILLMProvider` and
  delegates across an ordered vector of `(priority, ILLMProvider)`
  entries. It reads its primary/fallback decision from the shared
  `OfflineFlag` and from each provider's `health()`:
  - Offline ⇒ only `ProviderKind::Local` candidates are considered.
  - Online ⇒ Cloud is preferred unless it reports `Unhealthy`, in
    which case the router falls through to Local and records the
    demotion in the audit stream.
  - `OfflineFlag` is toggled at runtime by the `/offline` slash
    command (see ADR-0015 §7) and surfaces to the editor via the
    session context.
- `MistralrsProvider` and `LlamaProvider` ship as feature-gated stubs
  (`--features local-mistralrs` / `--features local-llama`). The
  stubs advertise `ProviderHealth::Unhealthy { reason: "not
  configured" }` until a model path is attached, and return a
  `ProviderError::NotConfigured` on any call. This keeps the base
  `cargo test --workspace` lean while keeping the router's
  integration points real.
- Constrained JSON grammar, auto-tune cache wiring, and the
  benchmark harness per §2.4 / §2.5 / §2.7 stay gated behind the
  same local-inference features and are not activated in the
  default-feature CI matrix. They will land with wave 9G (model
  manager + first-run dialog).
- Miri gate: `bld-ffi`'s current exports are Miri-clean under
  `-Zmiri-strict-provenance -Zmiri-retag-fields`; the nightly job
  consumes the same target set described in §2.6.

---

## ADR file: `adr/0017-engine-audio-service-and-mixer-graph.md`

## ADR 0017 — Engine Audio Service & Mixer Graph (Wave 10A)

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 10 opening)
- **Deciders:** Cold Coffee Labs engine group (Claude Opus 4.7 drafting; founder final)
- **Supersedes:** —
- **Superseded by:** —
- **Related:**
  - `CLAUDE.md` non-negotiables #5 (RAII, no raw new), #9 (no STL in hot paths), #10 (no std::thread), #11 (OS headers in platform only)
  - `docs/04_SYSTEMS_INVENTORY.md` (audio subsystem rows) + flagship audio intent in `docs/07_SACRILEGE.md` — budgets to be tightened in follow-on *Sacrilege* audio docs
  - ADR-0018 (Steam Audio), ADR-0019 (`.kaudio` streaming), ADR-0022 (Phase 10 cross-cutting)

## 1. Context

Phase 10 opens the Runtime I track with audio + input. Wave 10A (week 060) establishes the mixer foundation. The audio runtime must:

1. Be a **single RAII owner** — `AudioService` is constructed by the engine; its destructor leaves miniaudio cleanly shut down.
2. Produce **zero allocations on the audio callback thread** — all voices pre-allocated at service init.
3. Be **deterministic** — same input clips + same listener trajectory produce identical PCM frames.
4. Be **deferred-backend** — the code path that talks to miniaudio lives behind an `IAudioBackend` interface so CI can run a null backend (no device), and a future backend swap (e.g. WASAPI direct) does not ripple.
5. Keep **OS headers quarantined**: neither `engine/audio/` nor any downstream consumer includes `<windows.h>`, `CoreAudio` headers, or any backend SDK directly. miniaudio's `"miniaudio.c"` TU is a single source file built under `engine/platform/audio/`.
6. Enforce the **bus graph from §5 of the audio bible** as the authoritative topology.

## 2. Decision

### 2.1 Layering

```
engine/audio/
  audio_types.hpp         — enums, handles, config, listener state, stats
  audio_backend.hpp       — IAudioBackend interface
  audio_backend_null.cpp  — deterministic offline backend (CI + tests)
  voice_pool.hpp/.cpp     — lock-free fixed-capacity free-list
  mixer_graph.hpp/.cpp    — Master → {SFX, Music, Dialogue, Ambience} → leaf buses
  atmosphere_filter.hpp/.cpp — biquad LPF cutoff vs density
  emitter.hpp             — AudioEmitter + ListenerState component types
  audio_service.hpp/.cpp  — RAII façade (this ADR's central object)
engine/platform/audio/
  miniaudio_unity.c       — the sole TU that pulls miniaudio amalgamation
  miniaudio_backend.cpp   — IAudioBackend impl calling into miniaudio
```

Downstream code (ECS systems, editor, game) only ever sees `engine/audio/` headers.

### 2.2 Bus graph (mirrors §5 of audio bible, frozen)

```
Master
├── SFX
│   ├── Weapons
│   ├── Environment
│   └── UI
├── Music
│   ├── Combat
│   ├── Exploration
│   └── Space
├── Dialogue
│   ├── NPC
│   └── Radio
└── Ambience
    ├── Wind
    ├── Water
    └── Space
```

`BusId` is a 16-bit enum class; the mixer's 15 leaf + group nodes are stored in a fixed `std::array<BusNode, kBusCount>` inside `MixerGraph`. Bus volumes are `float`; bus mutes are `bool`. The atmosphere LPF node sits between **SFX** and **Master** (SFX only — Dialogue/Music/UI bypass atmospheric filtering per §3 of the bible).

### 2.3 Voice pool

- **32 3D voices** + **64 2D voices**, pre-allocated in `VoicePool` at service construction.
- Acquire/release via a fixed-capacity SPSC free-list (`VoicePool::Slot*` head pointer, `std::atomic` cmpxchg).
- **Steal-oldest** policy on saturation: when all of a class's voices are live, the oldest-started voice is stopped and its slot reissued. Callers observe this via `VoiceHandle::generation` — a stale handle fails `VoicePool::is_live()`.
- `VoiceHandle` is { `u16 index`, `u16 generation` }; 4 bytes, trivially copyable.

### 2.4 AudioConfig

```cpp
struct AudioConfig {
    std::uint32_t sample_rate   = 48'000;
    std::uint32_t frames_period = 512;
    std::uint8_t  channels      = 2;
    std::uint16_t voices_3d     = 32;
    std::uint16_t voices_2d     = 64;
    bool          enable_hrtf   = true;   // Wave 10B
    const char*   device_id     = nullptr; // null = system default
};
```

Determinism gate: when `GW_AUDIO_DETERMINISTIC` is defined (tests, replay), the service pins the null backend and uses a fixed 48 kHz / 2-channel / 512-frame period.

### 2.5 Thread model

- `AudioService::update(dt, listener)` — main thread; submits event commands (play/stop/volume) into a SPSC queue read by the mixer.
- `IAudioBackend::render(frames)` — audio thread; owned by the backend. For miniaudio this is the `ma_device` callback; for `NullAudioBackend` it is pumped manually by tests.
- No `std::thread` / `std::async` anywhere in `engine/audio/`. The audio thread is owned by the platform backend.

### 2.6 Hotplug

`IAudioBackend` reports rerouting via an in-process `Events<DeviceReroutedEvent>` queue. `AudioService::update` drains this queue onto the main-thread event bus; the mixer graph is preserved across reroutes (no state loss).

### 2.7 Decoders

miniaudio ships with WAV/FLAC/MP3 built in. We extend via `engine/audio/decoder_registry.hpp`:

- `WavRawDecoder` — baked f32 (our primary SFX path; part of `.kaudio`)
- `VorbisDecoder` (Wave 10C) — via miniaudio's `miniaudio_libvorbis.c` extra
- `OpusDecoder` (Wave 10C) — via `libopusfile`

## 3. Non-goals

- No reflections/reverb baking — deferred to Wave 10B's Phase-12 continuation (ADR-0018 §4).
- No per-instance DSP graphs — authored effects (reverb preset, filter) live on buses, not voices.
- No Web Audio compatibility.
- No MIDI.

## 4. Consequences

Positive:
- Determinism-first surface: the `NullAudioBackend` can render an N-frame golden buffer in CI without a device.
- Bus graph cannot drift — it is a frozen topology in code with tests that pin it.
- Swap-in seam for Phase 12 (physics-driven reflections) is `MixerGraph::set_reverb_preset(BusId, ReverbPreset)`, not a new API.

Negative:
- Every new bus is a code change (not data-driven). We accept this: the bus graph is a product design decision, not tuning data.
- Voice-pool saturation forces steal-oldest. We document this in the developer-facing audio guide.

## 5. Verification

- `voice_pool_test.cpp` — fill to capacity, observe graceful steal, handle generation check.
- `mixer_graph_test.cpp` — bus topology equality against a golden vector.
- `atmosphere_filter_test.cpp` — LPF cutoff monotonic in distance × density; biquad coefficients stable.
- `audio_service_null_backend_test.cpp` — 10 000 play/stop cycles, zero heap churn after warmup (tracked via an allocator hook).
- Build smoke: `greywater_core` links; `gw_tests` runs all audio cases green.

## 6. Rollback

The audio module is ring-fenced — deleting `engine/audio/` and `engine/platform/audio/` leaves the rest of the engine compiling (only the sandbox demo loses audio).

---

## ADR file: `adr/0018-engine-audio-steam-integration.md`

## ADR 0018 — Steam Audio spatial integration (Wave 10B)

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 10 Wave 10B opening)
- **Deciders:** Cold Coffee Labs engine group
- **Related:** ADR-0017 (audio service); spatial mix and propagation requirements in `docs/04_SYSTEMS_INVENTORY.md` + *Sacrilege* GW-SAC-001 (Part IV audio).

## 1. Context

Greywater's audio identity is built on **object-based spatial audio** with HRTF binaural rendering and geometry-aware propagation. Wave 10B adds the Steam Audio layer on top of the Wave-10A mixer foundation.

Steam Audio (v4.8.1, 2026-02-11) is **fully open-source**, C-ABI (`phonon.h`), supports **runtime mesh / material changes** since 4.8.0, and integrates via a custom audio node. Licence permits linking against Greywater's static build; the SOFA HRTF dataset carries its own licence which we vendor separately under `assets/audio/hrtf/default/`.

## 2. Decision

### 2.1 Abstraction

`engine/audio/spatial.hpp` declares `ISpatialBackend`:

```cpp
class ISpatialBackend {
public:
    virtual ~ISpatialBackend() = default;
    virtual void set_listener(const ListenerState&) = 0;
    virtual SpatialHandle acquire(const EmitterState&) = 0;
    virtual void release(SpatialHandle) = 0;
    virtual void update(SpatialHandle, const EmitterState&) = 0;
    // Audio-thread call: in [mono] → out [stereo], deterministic per handle.
    virtual void render(SpatialHandle, std::span<const float> mono_in,
                        std::span<float> stereo_out) = 0;
};
```

Two implementations land in Wave 10B:

1. **`SpatialStubBackend`** — deterministic pan + ITD + distance attenuation, no HRTF. Used in CI, the `NullAudioBackend`, and on systems where Steam Audio cannot initialise. Golden PCM for tests is generated from this backend.
2. **`SpatialSteamBackend`** — production. Owns `IPLContext`, one shared `IPLHRTF` (default SOFA), a pool of 32 `IPLBinauralEffect` + `IPLDirectEffect` + `IPLReverbEffect`, all pre-allocated.

Selection via `AudioConfig::enable_hrtf` + runtime capability probe (Steam Audio DLL presence).

### 2.2 Listener / emitter contract

- `ListenerState` carries position (`Vec3_f64`), forward, up, velocity — fed each frame by `AudioService::update`.
- World-space positions are `float64`; Steam Audio consumes `float`. Conversion happens **once** per voice per frame inside `SpatialSteamBackend::update` using the listener position as the origin (floating-origin-safe).
- `EmitterState` carries the same plus directivity (cardioid cone params), spatialBlend (0 = 2D, 1 = full 3D), distance-attenuation curve, occlusion/obstruction coefficients.

### 2.3 Pipeline per voice (3D path)

```
voice mono PCM → Steam Audio DirectEffect (distance + occlusion + directivity)
              → Steam Audio BinauralEffect (HRTF, ITD, ILD)
              → 2-ch output into Master mixer node
```

Air absorption is **disabled inside Steam Audio** — we author atmospheric filtering ourselves (atmosphere LPF, §3 of the bible) because game design needs authoritative control (vacuum, CO₂ atmosphere, etc.). Doppler is computed engine-side from float64 velocity and applied as a pitch trim before the direct effect (Steam Audio's Doppler uses float positions).

### 2.4 Occlusion

Physics is offline until Phase 12. Wave 10B ships `IOcclusionProvider`:

- `NullOcclusion` — always returns 1.0 (fully audible).
- `GridVoxelOcclusion` — stub backed by the Phase-5 chunk/material grid; a straight ray segment from listener to emitter samples the grid and tallies `transmission * material_coefficient`.

Phase 12 replaces `GridVoxelOcclusion` with a Jolt raycast-based provider without touching `engine/audio/`.

### 2.5 Reflections / reverb

Full probe-baked reflections are **deferred to Phase 12** (when Jolt mesh data is available). In Phase 10 we ship **preset reverb** on each bus via `IPLReverbEffect` parameters: `cave`, `hall`, `outdoor`, `vacuum`. `MixerGraph::set_reverb_preset(BusId, ReverbPreset)` is the authoring surface.

### 2.6 Vacuum path

When `atmospheric_density < 0.001`:

- SFX bus muted (not paused — the state is recoverable on re-entry to an atmosphere)
- Dialogue/Radio buses held at 1.0
- Master gets a contact-audio LPF (2 kHz cutoff, 12 dB/oct) representing solid-body transfer

Implemented as a `VacuumState` field on the mixer plus declarative rules in `audio_mixing.toml`. Not per-voice.

### 2.7 Mono/stereo toggle

Game-Accessibility-Guidelines "Hearing / Intermediate" line item. Implemented as a `MixerGraph::DownmixMode { Stereo, Mono, Bypass }` on the Master path. Default Stereo; user sets Mono in the accessibility panel (Wave 10E).

## 3. Non-goals

- No reflections baking (Phase 12).
- No pathing / occlusion-by-sound-propagation (Phase 12).
- No ambisonics (deferred; HRTF handles the 3D case for our voice budget).

## 4. Verification

- Golden-PCM test: 2-second tone at (+1, 0, 0) relative to listener, compare FFT against checked-in reference `tests/fixtures/audio/golden_tone_right.fft.bin`.
- 32-voice soak: CPU < 3 %, no heap allocs.
- Determinism: same scene seed + same listener trajectory produces identical PCM (bit-identical on the stub backend; ±0.5 dB on Steam Audio where HRTF interpolation is involved).
- Vacuum transition test: SFX voice pre-transition audible; post-transition muted; dialogue voice preserved.

## 5. Consequences

Positive:
- Accessibility (mono toggle), vacuum design, and future reflections all have a single seam.
- CI runs without Steam Audio installed; determinism is stub-backend.

Negative:
- Two codepaths (stub + Steam Audio) must be kept semantically equivalent for the **direct** component. We accept this; the stub is canonical for tests and the Steam Audio path documents its expected PCM envelope per-frame.

## 6. Rollback

Setting `AudioConfig::enable_hrtf = false` forces the stub backend. No schema / file changes involved.

---

## ADR file: `adr/0019-kaudio-cooked-format-and-streaming.md`

## ADR 0019 — `.kaudio` cooked format & music streamer (Wave 10C)

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 10 Wave 10C opening)
- **Deciders:** Cold Coffee Labs engine group
- **Related:** ADR-0017 (mixer), Phase 6 asset pipeline (`engine/assets/*`); dynamic score / streaming intent in *Sacrilege* GW-SAC-001

## 1. Context

Greywater ships ~500 MB of audio in the v1 demo: ~40 MB baked SFX, the remainder as streamed music and dialogue. Wave 10C lands:

1. A **cooked container** — `.kaudio` — that keeps both the tight baked-f32 SFX path and the seek-friendly streaming codec path under one extension.
2. A **streaming music pipeline** — job-scheduled decode into a SPSC ring buffer, drained by the audio callback, with the **audio thread never blocking on I/O**.
3. A **decoder registry** — plug-in `IDecoder`s so WAV/Vorbis/Opus/FLAC all round-trip identically.

## 2. Decision

### 2.1 `.kaudio` file layout (little-endian, version 1)

```
offset  size  field
0x00    4     magic        "KAUD"
0x04    2     version      u16 = 1
0x06    2     codec        u16 enum { raw_f32, vorbis, opus, flac }
0x08    1     channels     u8
0x09    1     reserved
0x0A    2     flags        u16 bitfield (loop, streaming, seekable)
0x0C    4     sample_rate  u32
0x10    8     num_frames   u64
0x18    8     loop_start   u64 (frames; 0 = none)
0x20    8     loop_end     u64
0x28    8     seek_table   u64 (file offset; 0 = none)
0x30    8     body_offset  u64
0x38    8     body_bytes   u64
0x40    16    content_hash xxHash3-128 of the body (cook key)
...padding to 64...
0x40+   --    body         codec-native bytes
```

Flags:
- `0x01` loop enabled
- `0x02` streaming (force `MusicStreamer`)
- `0x04` seekable (seek table present)

The seek table — when present — sits at `seek_table`:

```
u32 entries
entries × { u64 frame_index, u64 file_offset }
```

Quantised at 250 ms by default. Target seek latency ≤ 5 ms for a cached file.

### 2.2 AssetCooker subcommand

`gw_cook audio IN.(wav|ogg|opus|flac) --out OUT.kaudio --codec (raw|vorbis|opus) [--stream]`

Wired into the Phase-6 cook CLI. Metadata (loop points, bus hint, accessibility flags) comes from a sibling `IN.asset.toml`. The cooker:

- Decodes source to f32 mono/stereo
- Writes the chosen codec
- Builds seek table if `--stream`
- Stamps `content_hash` = xxHash3-128 of the body
- Produces deterministic output (same input → same bytes)

### 2.3 MusicStreamer

```cpp
class MusicStreamer {
public:
    MusicStreamer(std::unique_ptr<IStreamDecoder>, RingBufferCfg);
    ~MusicStreamer() = default;

    // Main-thread API
    void start();
    void stop();
    void seek(u64 frame);

    // Audio-thread pull (drains ring; never blocks)
    std::size_t pull(std::span<float> out);

    MusicStreamStats stats() const noexcept;  // underruns, queued frames, ...
};
```

- Ring buffer default: **500 ms** at 48 kHz stereo = 48 000 samples.
- Decode job pulls 100 ms pages via `engine/jobs/` I/O pool.
- Audio-thread `pull` is wait-free. Underrun → emit silence + increment a counter on `MusicStreamStats`.
- Fault tolerance: decode error marks stream dead; audio-thread gets silence until `start()` re-initialises.

### 2.4 Crossfade

`AudioService::crossfade_music(AudioClipId new_track, float duration_s)` — uses two `MusicStreamer` slots on the Music bus group. Old fades out linearly; new fades in on the same curve; the old is torn down when its volume reaches 0.

### 2.5 Ducking & category controls

Authoring rules live in `assets/audio/audio_mixing.toml`:

```toml
[rule.combat_ducks_music]
when = "combat_intensity > 0.5"
bus  = "music"
delta_db = -6.0
attack_ms = 80
release_ms = 300

[rule.vacuum_mutes_master]
when = "atmospheric_density < 0.001"
bus  = "sfx"
mute = true
```

`AudioMixerSystem::update(GameState)` evaluates rules each frame. Rules are data — changing them does not require an engine rebuild. The rule evaluator is a straight-line condition parser (no branching beyond `>`, `<`, `==`).

Per-category mutes (GAG Hearing/Basic):

```toml
[bus.master]  { volume = 1.0, mute = false }
[bus.sfx]     { volume = 1.0, mute = false }
[bus.music]   { volume = 0.8, mute = false }
[bus.dialogue]{ volume = 1.0, mute = false }
[bus.ambience]{ volume = 0.8, mute = false }
```

## 3. Non-goals

- No streaming SFX (all SFX is baked — 40 MB budget).
- No runtime re-cook.
- No Vorbis/Opus in Wave 10A (added in 10C, gated by CPM option).

## 4. Verification

- Round-trip cook+decode yields f32 PCM within ±1 ULP for `raw_f32`; PSNR ≥ 40 dB for Vorbis/Opus against source.
- 30-minute streaming soak: 0 underruns at 48 kHz stereo, flat RAM (< 1 MB growth after warmup).
- Seek storm: 1 000 random seeks, p99 < 5 ms.

## 5. Consequences

Positive:
- One format, one cooker, one reader; codec lives in the header byte.
- Format has a version field — we can bump it in Phase 20 without breaking old assets (version-aware decoder).
- Ducking is data, not code — audio designer owns it.

Negative:
- Seek table is optional — if a cooker run forgets `--stream`, music becomes non-seekable. Mitigation: cooker warns when `duration > 30 s` without streaming.

## 6. Rollback

Streaming can be disabled by forcing `codec = raw_f32`. Files are naturally fast-path; larger disk but simpler.

---

## ADR file: `adr/0020-engine-input-sdl3-device-layer.md`

## ADR 0020 — Input: SDL3 device layer (Wave 10D)

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 10 Wave 10D opening)
- **Deciders:** Cold Coffee Labs engine group
- **Related:** `CLAUDE.md` #11 (OS headers quarantined), ADR-0021 (action map), ADR-0022

## 1. Context

Phase 2 seeded an `engine/platform/input.cpp` stub that carried only keyboard/mouse structs. Phase 10 needs a full device-capability layer: gamepads, HID (flight sticks, HOTAS, Xbox Adaptive Controller, Quadstick), haptics, sensors, with stable IDs across hotplug and deterministic replay for CI. SDL3 (3.4.0, 2026-01-01) is the smallest credible cross-platform surface that handles all of the above with first-class hotplug and the 2025 DualSense / trigger-haptics API.

## 2. Decision

### 2.1 Layering

```
engine/input/                           (OS-free)
  input_types.hpp         — DeviceId, DeviceCaps, KeyCode, GamepadButton, axes
  input_backend.hpp       — IInputBackend interface
  input_trace.hpp/.cpp    — record/replay .input_trace for CI
  device.hpp/.cpp         — Device, DeviceRegistry, capability probe
  haptics.hpp/.cpp        — cross-device rumble + trigger feedback
  input_service.hpp/.cpp  — RAII façade (poll, snapshot, events)

engine/platform/input/                  (only TUs that touch SDL)
  sdl3_backend.cpp        — SDL3 implementation of IInputBackend
  sdl3_haptics.cpp
```

The existing `engine/platform/input.cpp` (Phase 2 stub) is promoted to a thin include shim so the Phase-2 API compiles unchanged.

### 2.2 IInputBackend

```cpp
class IInputBackend {
public:
    virtual ~IInputBackend() = default;
    virtual void initialize(const InputConfig&) = 0;
    virtual void poll() = 0;                          // one pump, main thread
    virtual std::span<const RawEvent> drain() = 0;    // frame's events
    virtual const RawSnapshot& snapshot() const = 0;  // fused steady state
    virtual void rumble(DeviceId, const HapticEvent&) = 0;
    virtual void set_led(DeviceId, u8 r, u8 g, u8 b) {}
};
```

Three backends:

1. **`NullInputBackend`** — no-op, used on headless CI or tooling.
2. **`TraceReplayBackend`** — drives events from a `.input_trace` file; used in deterministic tests and to reproduce player reports.
3. **`Sdl3InputBackend`** — production. Lives entirely in `engine/platform/input/sdl3_backend.cpp`.

### 2.3 Device identity

`DeviceId` is a 64-bit value:

```
u32 guid_hash    — xxHash32 of SDL's joystick GUID (stable across runs for the same physical device)
u16 player_index — 0-based, assigned in arrival order
u16 flags        — { is_adaptive, is_steam_controller, is_virtual, ... }
```

Hotplug of the same physical device (unplug/replug) keeps `guid_hash`; only the `player_index` may reshuffle. The action-map layer (ADR-0021) binds by `guid_hash`.

### 2.4 Capability mask

```cpp
enum class DeviceCap : u32 {
    Buttons          = 1 << 0,
    Axes             = 1 << 1,
    DPad             = 1 << 2,
    Rumble           = 1 << 3,
    TriggerRumble    = 1 << 4,
    Gyro             = 1 << 5,
    Accel            = 1 << 6,
    Touchpad         = 1 << 7,
    LED              = 1 << 8,
    Adaptive         = 1 << 9,  // Xbox Adaptive Controller auto-flag
    Quadstick        = 1 << 10,
};
```

Deadzone (radial + per-axis) and response curves (linear, quadratic, cubic, custom LUT) are applied **in the backend** — consumers never see raw hardware values.

### 2.5 Hotplug events

SDL emits `SDL_EVENT_GAMEPAD_ADDED/REMOVED/REMAPPED`. The SDL3 backend marshals these into the main-thread event bus as `DeviceConnectedEvent` / `DeviceRemovedEvent`. This is a **cert requirement** for Xbox and Steam Deck and is therefore on the Phase-10 Definition-of-Done checklist.

### 2.6 Haptics

Unified API:

```cpp
struct HapticEvent {
    float low_freq      = 0.0f;  // [0, 1]
    float high_freq     = 0.0f;
    float left_trigger  = 0.0f;  // DualSense / Xbox Series
    float right_trigger = 0.0f;
    u32   duration_ms   = 100;
};
```

`Haptics::play(DeviceId, const HapticEvent&)` queries device capabilities and skips channels the device doesn't support. Global volume scalar + per-device toggle via `haptics.toml`.

### 2.7 Background input policy

Default `SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS=0`. Editor (which has focus juggling) sets it to `1` at init.

### 2.8 Steam init ordering

When Steam is present (`GW_STEAM=1`), `SteamAPI_InitEx` runs before `SDL_Init` so SDL's Steam Input integration sees Steam. The runtime checks `getenv("SteamAppId")` at startup.

### 2.9 `.input_trace` format

Binary little-endian:

```
magic "INTR"  u32  version=1
u64 start_ns
repeated:
  u64 ns_since_start
  u16 kind (device_added, device_removed, button_down, button_up, axis, gyro, ...)
  u16 payload_len
  bytes payload
```

`TraceReplayBackend` consumes one of these and emits deterministic events at replay rate. Recording is enabled via `InputConfig::record_trace_path`.

## 3. Non-goals

- No IME text-input UI (Phase 11).
- No virtual on-screen keyboard (Phase 16 a11y).
- No remote / network input (Phase 14).

## 4. Verification

- `input_trace_roundtrip_test.cpp` — record a synthetic session, replay it, snapshots byte-identical.
- Hotplug fuzz: connect/disconnect every 50 ms for 1 000 iterations, no leaks, no stale handles.
- Deadzone / curve unit tests on known inputs.
- `input_poll_gate` perf test: 4 gamepads connected, poll < 0.1 ms / frame.

## 5. Consequences

Positive:
- CI runs without a physical gamepad.
- Adaptive Controller / Quadstick auto-tagged for the accessibility layer.
- Swapping SDL3 for a future backend is a file-level change.

Negative:
- Two codepaths to maintain (null/trace vs SDL). The trace path is authoritative for determinism tests, which keeps it honest.

## 6. Rollback

Forcing `InputConfig::backend = Null` disables all device input (keyboard/mouse still available through the Phase-2 platform stub). No file changes required.

---

## ADR file: `adr/0021-engine-input-action-map-and-accessibility.md`

## ADR 0021 — Action map & accessibility (Wave 10E)

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 10 Wave 10E opening)
- **Deciders:** Cold Coffee Labs engine group
- **Related:** ADR-0020 (device layer), Game Accessibility Guidelines — Motor (Basic/Intermediate/Advanced), Hearing (Basic/Intermediate), General (Basic/Advanced).

## 1. Context

Wave 10E turns the device layer into a typed, rebindable, context-aware, accessibility-first action system. Accessibility is not a bolt-on — it drives the design of the action map itself. The cross-walk in §3 of this ADR is the Definition-of-Done for Phase 10 accessibility.

## 2. Decision

### 2.1 Action types

```cpp
template <typename T> class Action;

using ButtonAction = Action<bool>;
using AxisAction   = Action<float>;
using StickAction  = Action<math::Vec2f>;
using GyroAction   = Action<math::Vec3f>;
using EnumAction   = Action<u32>;
```

Each `Action<T>` owns:
- A human-readable `name` (stable across saves)
- A list of `Binding`s (primary + alternates)
- A `Processor` chain: `Deadzone → Curve → Invert → Scale → Clamp → Smooth`
- Optional accessibility flags: `hold_to_toggle`, `auto_repeat_ms`, `assist_window_ms`, `scan_index`, `can_be_invoked_while_paused`

### 2.2 Bindings

```cpp
struct Binding {
    BindingSource source;   // kbd/mouse/gamepad/gyro/hid
    u32           code;     // keycode / button / axis id
    ModifierMask  modifiers;
    float         threshold;   // for analog-as-button
    bool          inverted;
};
```

Composite bindings:
- **`Chord`** — N simultaneous bindings (`Ctrl+C`, `L1+R1`). Each chord is one `Action` input emitting `true` only while *all* members are held.
- **`Sequence`** — N sequential bindings with per-step `timeout_ms`. Useful for fighting-game inputs and accessibility "sticky sequence" replacements for chords.
- **`CompositeWASD`** — four Button bindings combined into a Vec2 axis. Implemented as a first-class binding kind so the processor chain runs once.

### 2.3 ActionSet and context stack

- `ActionSet` — named collection: `gameplay`, `menu`, `vehicle`, `editor`, `adaptive_default`, `one_button_simple`.
- At most **one** `ActionSet` is topmost at any time. A context stack enforces push/pop discipline (`InputService::push_set("menu")` / `pop_set()`).
- `consume` / `pass-through` semantics: an overlaid set may "eat" certain bindings (menu eats `toggle_menu`) while passing through others (radio hotkey always fires).

### 2.4 TOML persistence

Default map lives at `assets/input/default_gameplay.toml`. User overrides:

- **Windows**: `%APPDATA%\greywater\input.toml`
- **Linux**: `$XDG_CONFIG_HOME/greywater/input.toml` (fallback `~/.config/greywater/input.toml`)

Round-trip preserves unknown keys under `[extra]`. The schema:

```toml
[action.fire]
output = "bool"
bindings = [
  { device = "mouse",    input = "button_left" },
  { device = "gamepad",  input = "r2",        threshold = 0.3 },
  { device = "keyboard", input = "space",     modifier = "hold_to_toggle" },
]

[action.move]
output = "vec2"
processors = ["deadzone_radial(0.15)", "curve_quadratic", "clamp_unit"]
bindings = [
  { device = "gamepad",  input = "left_stick" },
  { device = "keyboard", input = "composite_wasd" },
]

[accessibility]
hold_to_toggle = true
repeat_cooldown_ms = 500
single_switch_scan_rate_ms = 900
enable_chord_substitution = true
```

Profiles:
- `input.toml` — active
- `input.$PROFILE.toml` — named profile; hot-swap via `InputService::load_profile`.

### 2.5 Rebind UX

- ImGui rebind panel in `editor/input_panel/` (Tier A, editor-only).
- In-game overlay using the temporary ImGui debug layer; replaced by RmlUi in Phase 11.
- Live capture: press any input → bound, with **conflict detection** (dialog: replace / swap / cancel).
- Xbox vs PlayStation prompts auto-swap based on last-used device type.
- Per-device alternate slots.

### 2.6 Accessibility defaults — built in, not opt-in

| Guideline | Tier | Implementation |
|---|---|---|
| Remappable/reconfigurable controls | Motor/Basic | §2.5 |
| Haptics toggle + slider | Motor/Basic | `haptics.toml` |
| Sensitivity slider per axis | Motor/Basic | `Processor::Scale` |
| All UI accessible with same input method as gameplay | Motor/Basic | `menu` set always bound to both kbd + gamepad |
| Alternatives to holding buttons | Motor/Intermediate | `hold_to_toggle` modifier: press converts to latched toggle; double-tap un-latches |
| Avoid button-mashing | Motor/Intermediate | `auto_repeat` modifier: single press treated as repeated input for configurable duration |
| Support multiple devices | Motor/Intermediate | Device fusion in `InputService` (no "primary device lock") |
| Very simple control schemes (switch / eye-tracking) | Motor/Advanced | `single_switch_scanner`: cycles through a configurable action subset on a timer; user triggers the single bound switch to activate current action. Preset `one_button_simple.toml`. |
| 0.5 s cool-down between inputs | Motor/Advanced | `repeat_cooldown_ms` global + per-action |
| Make precise timing inessential | General | `assist_window_ms`; `can_be_invoked_while_paused` |
| Stereo/mono toggle | Hearing/Intermediate | ADR-0018 §2.7 |
| Separate volume/mute per SFX/speech/music | Hearing/Basic | ADR-0019 §2.5 |
| Settings saved/remembered | General/Basic | TOML round-trip per profile |
| Settings saved to profiles | General/Advanced | multiple named `input.$PROFILE.toml` files |

### 2.7 Hold-to-toggle state machine

Every action flagged `hold_to_toggle = true` uses this transducer:

```
Idle --press--> Pressed
Pressed --release-fast--> Latched-on      (press < 250 ms → toggle on)
Pressed --release-slow--> Released        (press ≥ 250 ms → classic momentary)
Latched-on --double-tap--> Idle           (two presses within 350 ms → latch off)
Latched-on --single-tap--> Latched-on     (swallowed)
```

The transducer is stateless per-frame from the caller's perspective — the action outputs `true` continuously while latched. Idempotent under missed-press injection (interrupted frames).

### 2.8 Single-switch scanner

Cycles through a configurable list of N actions at `single_switch_scan_rate_ms` cadence. A single bound "scan_activate" button triggers whichever action is currently highlighted. Reachability: every action is reachable within `ceil(log2 N)` scan cycles when the scanner supports hierarchical groups; otherwise within N cycles linearly.

UI affordance (overlay, later replaced by RmlUi): highlight current action name on-screen; emit audio cue (TTS-ready in Phase 16).

### 2.9 Xbox Adaptive Controller / Quadstick auto-profile

On device connect, if `DeviceCaps::Adaptive` is set (MS VID 0x045E with Adaptive product IDs) or the device name matches the Quadstick fingerprint, the input service:

1. Sets `accessibility.adaptive_default = true`
2. Loads `input.adaptive_default.toml` on top of the active map
3. Enables hold-to-toggle globally
4. Turns on single-switch scanner for secondary menus

User can override via profile at any time.

## 3. Non-goals

- No eye-tracking hardware integration (Phase 16 a11y).
- No on-screen virtual keyboard (Phase 16).
- No voice-command input (Phase 14 voice + Phase 16 a11y).

## 4. Verification

- Replay-trace test: `.input_trace` → deterministic Action emission.
- Round-trip fuzz on TOML map: random bind, save, reload, compare.
- Hold-to-toggle property tests: idempotent under reordering of press/release pairs.
- Single-switch scanner reachability test: 1 000 synthetic hits, every action hit within expected cycles.
- Per-guideline self-check: `docs/a11y_phase10_selfcheck.md` table signed off before Phase-10 exit.

## 5. Consequences

Positive:
- Accessibility is enforced by the action-map type system, not policed by review.
- Profiles are data — QA can ship "Adaptive Controller preset" as a TOML without code.
- Rebind UX is editor-only in Phase 10; Phase 11 picks up RmlUi implementation for the shipping game.

Negative:
- TOML round-trip is strict — hand-edits that break syntax are rejected with a parse error (migration helper included).

## 6. Rollback

The whole action map can be bypassed by reading raw `RawSnapshot` from `InputService::snapshot()`. This is intentional — used by the editor's 3D camera which bypasses rebinding for debug navigation.

---

## ADR file: `adr/0022-phase10-cross-cutting.md`

## ADR 0022 — Phase 10 cross-cutting decisions (summary)

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 10 Wave 10E close / exit gate)
- **Deciders:** Cold Coffee Labs engine group
- **Summarises:** ADRs 0017–0021

This ADR is the **cross-cutting summary** for Phase 10. It records decisions that span more than one wave or that fell out during implementation.

## 1. Dependency pins

| Library | Pin | Notes |
|---|---|---|
| miniaudio | 0.11.25 (2026-03-04) | Vendored via a single-TU wrapper `miniaudio_unity.c` that includes `miniaudio.c` instead of using the `MINIAUDIO_IMPLEMENTATION` macro. This is forward-compatible with the 0.12 split that separates `.c` from `.h`. Optional CPM package (gated by `GW_AUDIO_MINIAUDIO`). |
| Steam Audio SDK | 4.8.1 (2026-02-11) | Binary DLL/SO from GitHub Releases — open-source since 4.5.2. Gated by `GW_AUDIO_STEAM`. |
| SDL3 | 3.4.0 (2026-01-01) | FetchContent from `libsdl-org/SDL@release-3.4.0`. Gated by `GW_INPUT_SDL3`. |
| libopus | 1.6 | Submodule `xiph/opus`. Gated by `GW_AUDIO_OPUS`. |
| libopusfile | 0.12 | Depends on libopus + libogg 1.3.5. Gated by `GW_AUDIO_OPUS`. |

All four `GW_*` gates default **OFF** in Phase 10's initial landing. The null/trace backends are the default production path; gate-on builds are the integration path that Phase 11 consumes for the Playable-Runtime demo.

**Why gate off by default in Phase 10?** The phase's Definition-of-Done asks for *architecture*, *determinism*, and *accessibility* — all of which are testable without the binary dependencies. Gate-on lights up the full production path and does so behind flags that flip in Phase 11 without code changes. This keeps CI quick, avoids vendoring a 12 MB Steam Audio binary across a million git clones, and makes every decision made in ADRs 0017–0021 survive on its own merits.

## 2. Determinism contract

- The null audio backend pumps N frames from a known seed and produces bit-identical PCM across platforms.
- The trace-replay input backend emits identical `RawEvent` streams across platforms.
- Golden-PCM fixtures under `tests/fixtures/audio/` are the canonical reference.
- Steam Audio and miniaudio, when gated on, are *allowed* to differ within ±0.5 dB; tests that gate on SDK paths relax bit-identity to envelope equality.

## 3. Hot-path budgets (CI-enforced via `gw_perf_gate`)

| Gate | Target | Hardware basis |
|---|---|---|
| Audio CPU (32 voices + 10 s occlusion barrage) | ≤ 3 % | i5-8400 equivalent |
| Steam Audio per-frame | ≤ 1.0 ms | at 60 FPS |
| Audio callback alloc/free | 0 / 0 | miniaudio custom alloc hook |
| Music streamer underruns / 10 min | 0 | 48 kHz stereo |
| Input poll | ≤ 0.1 ms / frame | 4 gamepads |
| Rebind round-trip | ≤ 2 ms | TOML save + reload |

## 4. Thread-ownership summary

| Thread | Owns | Calls |
|---|---|---|
| Main | `AudioService`, `InputService`, bus volumes, listener update | backend `poll`, `update(dt)`, rebind, profile swap |
| Audio | `IAudioBackend::render` callback | Voice pool pull, mixer graph process, atmosphere LPF, spatial render |
| I/O (jobs pool) | Music decode page pulls | `IStreamDecoder::decode_page`, ring buffer push |

No `std::thread` / `std::async` introduced. `engine/jobs/` is the only parallelism mechanism.

## 5. What Phase 11 inherits (handoff)

1. `AudioService` + `InputService` already owned by `Engine::Impl`.
2. Device hotplug events flowing on the main event bus.
3. `audio_mixing.toml` + `input.toml` files; Phase 11's CVar system takes ownership of the *same* files.
4. ImGui rebind panel (editor-only); Phase 11 re-implements in RmlUi.
5. Voice-chat scaffolding: `engine/audio/voice_capture.hpp`, `voice_playback.hpp` interfaces (no networking — Phase 14 wires GNS to these).
6. Accessibility cross-walk self-check (`docs/a11y_phase10_selfcheck.md`) — Phase 11 adds the RmlUi-facing items.

## 6. Cross-platform notes

- **Windows**: WASAPI backend via miniaudio; SDL3's Raw Input path; DualSense trigger rumble through SDL's DualSense driver.
- **Linux**: PulseAudio / PipeWire via miniaudio (PipeWire detected at runtime by miniaudio); SDL3's evdev + HIDAPI path. Real-time priority on the audio thread guarded behind `MA_NO_PTHREAD_REALTIME_PRIORITY_FALLBACK` so containerised CI runners don't crash.
- **Steam Deck**: treated as Linux with `SDL_HINT_JOYSTICK_HIDAPI_STEAMDECK=1` hint; trigger rumble works out of the box via SDL3.

## 7. What did *not* land in Phase 10

- Reflections / reverb probe baking — Phase 12 (needs Jolt meshes).
- Full DSP authoring graph per-voice — deferred indefinitely (out of scope for the identity we're building).
- Voice chat transport — Phase 14 (GNS).
- On-screen virtual keyboard — Phase 16 a11y.

## 8. Exit-gate checklist

- [x] Week rows 060–065 all merged behind the `phase-10` tag
- [x] ADRs 0017–0021 land as Accepted
- [x] ADR 0022 land as the summary (this doc)
- [x] `gw_tests` green with ≥ 40 new audio + input cases
- [x] `ctest --preset dev-win` and `dev-linux` green
- [x] Performance gates §3 green
- [x] `docs/a11y_phase10_selfcheck.md` signed off
- [x] `docs/05` marks Phase 10 *shipped* with today's date
- [x] Git tag `v0.10.0-runtime-1` pushed

---

## ADR file: `adr/0023-engine-core-events.md`

## ADR 0023 — Engine core events: typed pub/sub, zero-alloc in-frame bus, cross-subsystem bus

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 11 Wave 11A kickoff)
- **Deciders:** Cold Coffee Labs engine group
- **Supersedes:** none
- **Related:** ADR-0022 §5 (Phase-10 → Phase-11 inheritance), blueprint §3.19

## Context

Phase 10 landed `AudioService` and `InputService` behind null/stub backends, with device-hotplug and audio-cue signals ready to cross subsystem boundaries. Phase 11 must turn those into a *shipping runtime* by introducing the shared **event bus** every subsystem (input, audio, UI, console, CVar registry, BLD over the C-ABI) talks on.

Three distinct event behaviours are needed, and **only** three:

1. **Synchronous direct pub/sub** — UI listens to a CVar change and re-skins immediately; audio listens to `set_bus_volume` and applies; pattern: compile-time dispatch, no heap.
2. **In-frame deferred queue** — gameplay fires 300 `AudioMarkerCrossed` events during a cut-scene; the subtitle system drains them once at end-of-frame; pattern: double-buffered ring, zero-alloc steady-state.
3. **Cross-subsystem (C-ABI) bus** — BLD (Rust) publishes a `ConfigCVarChanged` from a plan-act-verify loop; the engine drains it next frame; pattern: type-erased payload header + opaque bytes, dispatched off the jobs pool.

## Decision

Ship three primitives in `engine/core/events/`:

- `EventBus<T>` — compile-time dispatch. `subscribe(handler) → SubscriptionHandle`; `publish(const T&)` walks a small-buffer vector of handlers. Reentrancy-safe (iteration uses a guarded index; unsubscribe-during-dispatch defers to after the walk). Zero heap on publish once the subscriber table is warm.
- `InFrameQueue<T>` — double-buffered SPMC queue. Writers fill the back buffer under a seqlock; `drain(fn)` swaps front/back and walks the front buffer. Capacity is fixed at registration; overflow increments a dropped-counter and returns `false`. **Zero allocations after registration.**
- `CrossSubsystemBus` — fixed-size (default 256 slots) ring of `{EventKind : u32, size : u16, flags : u16, bytes[248]}` records. Publishers call `publish(kind, std::span<const std::byte>)`; subscribers iterate with a visitor. Used across the C-ABI from BLD.

The engine-level event catalogue lives in `events_core.hpp` and is frozen as Phase 11 ships:

| Event | Carrier | Lands on |
|---|---|---|
| `DeviceHotplug` | `EventBus<DeviceHotplug>` | input service + dev console |
| `ActionTriggered` | `InFrameQueue<ActionTriggered>` | HUD + audio cue system |
| `WindowResized` | `EventBus<WindowResized>` | UI service + render swapchain |
| `FocusChanged` | `EventBus<FocusChanged>` | UI service + input (release-all-keys) |
| `AudioMarkerCrossed` | `InFrameQueue<AudioMarkerCrossed>` | caption channel |
| `ConfigCVarChanged` | `EventBus<ConfigCVarChanged>` | every subsystem |
| `ConsoleCommandExecuted` | `EventBus<ConsoleCommandExecuted>` | audit log + BLD replay |
| `UILayoutComplete` | `InFrameQueue<UILayoutComplete>` | render pass ordering |

A compile-flag `GW_EVENT_TRACE=1` wires an `EventTraceSink` that appends a binary `.event_trace` file per run — same binary shape as Phase 10's `.input_trace`, so the same tooling inspects both.

## Perf contract

| Gate | Target | Notes |
|---|---|---|
| `publish(empty)` single-thread | ≤ 12 ns | no subscribers, warm cache |
| `drain` 1 000 events | ≤ 40 µs / 0 B heap | `InFrameQueue<T>` |
| `CrossSubsystemBus` 1 000 events × 3 subs | ≤ 120 µs / 0 B heap | jobs-dispatched |

All three gates enforced by `gw_perf_gate`'s `events_*` tests.

## Tests (≥ 10)

1. publish-before-subscribe drops the event cleanly (no crash, no alloc)
2. unsubscribe during dispatch defers removal to the end of the walk
3. `InFrameQueue<T>` front/back swap preserves ordering across drains
4. `CrossSubsystemBus` payload alignment is 16-byte for SSE/NEON consumers
5. subscriber-table saturation triggers an `events_full` counter, not an abort
6. `.event_trace` determinism replay across x64/ARM64 (pinned fixtures)
7. reentrancy guard — publishing inside a handler defers the inner publish
8. subscriber lifetime — destroying the subscription during dispatch is safe
9. alignment — every event type satisfies `alignof(T) ≤ 16`
10. cross-platform byte order — event trace bytes are identical on x64 and ARM64

## Consequences

- Phase 10's ad-hoc device-hotplug callback in `InputService::update` gets retired in 11A and routed through `EventBus<DeviceHotplug>`; the callback is kept as a thin forward for one release.
- BLD's Rust side gains a C-ABI `gw_event_bus_publish(kind, ptr, len)` that maps to `CrossSubsystemBus::publish`. The full Surface H v2 wiring lands in Phase 18, but the seam is open now.
- Editor ImGui panels keep their synchronous `on_change` style via `EventBus<T>` — no double-dispatch.

## Alternatives considered

- **entt::dispatcher / sigslot / libcallme** — too much runtime dispatch overhead (`std::function`-based), not zero-alloc.
- **Single giant event union + visitor per frame** — simpler, but loses compile-time type safety and balloons the CPU-cache footprint when few subsystems care about a given event.
- **Broadcast every event over the cross-subsystem bus** — too slow; we measure 120 µs for 1 000 events, so every cheap synchronous edge would cost µs instead of ns.

Rejected. `EventBus<T>` + `InFrameQueue<T>` + `CrossSubsystemBus` is the minimum surface that covers every known 11A–11F use-case and forces new use-cases to motivate themselves against one of the three.

---

## ADR file: `adr/0024-engine-core-config-and-cvars.md`

## ADR 0024 — Engine core config: typed CVar registry + TOML persistence

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 11 Wave 11B)
- **Deciders:** Cold Coffee Labs engine group
- **Related:** ADR-0022 §5 (Phase-10 handoff), ADR-0021 (input action map), ADR-0017 (audio mixing), blueprint §3.14

## Context

Phase 10 shipped three TOML files parsed by handwritten line-oriented code:

- `audio_mixing.toml` — bus volumes, reverb presets, downmix mode
- `input.toml` — active action profile, hotplug policy
- `input.$PROFILE.toml` — per-profile binding tables (`action_map_toml.cpp`)

Phase 11 needs a *single* knob system that:

- the **console** can read/write (`get`, `set`);
- the **settings menu** (RmlUi data-model) can read/write;
- the **BLD agent** can read/write over the C-ABI;
- the **editor inspector** can read/write;
- every subsystem can **subscribe** to via `ConfigCVarChanged`;
- round-trips **byte-identically** to the Phase-10 TOML files (so Phase-10 regressions stay green).

Phase 10's line-oriented parser is not extensible enough for nested tables, inline tables, and typed enums; toml++ 3.4.0 is the Greywater-wide pin for TOML going forward.

## Decision

Ship `engine/core/config/` with:

- `CVar<T>` — a typed handle addressing a slot in a global registry. `T ∈ { bool, int32_t, int64_t, float, double, std::string, enum class }`. PODs only in the runtime slot; strings are small-string-optimised via `gw::core::Utf8String`.
- `CVarFlags` bitmask: `kPersist`, `kUserFacing`, `kDevOnly`, `kCheat`, `kReadOnly`, `kRenderThreadSafe`, `kRequiresRestart`, `kHotReload`.
- `CVarRegistry` — singleton per-process. Lock-free read path via a per-thread snapshot; writes funnel through one mutex + a version bump, and publish `ConfigCVarChanged{id, origin}` on the Phase-11A bus.
- `toml_binding.{hpp,cpp}` — `load_domain(path, registry, domain_prefix) → Result<void,Error>` / `save_domain(path, registry, domain_prefix)`. Uses `toml++` when `GW_CONFIG_TOMLPP=ON` (default ON for `gw_runtime`); falls back to the Phase-10 hand-rolled reader otherwise so CI without deps still round-trips.
- `StandardCVars` — ~40 pre-baked CVars seeded at registry construction. Domain prefixes: `gfx.*`, `audio.*`, `input.*`, `ui.*`, `dev.*`, `net.*`.
- **One TOML file per domain**: `graphics.toml`, `audio.toml`, `input.toml`, `ui.toml`. *Not* one monolithic `user.toml` — saves preserve meaning when a human edits one domain.

Origin tagging on `ConfigCVarChanged` (`kProgrammatic`, `kConsole`, `kTOMLLoad`, `kRmlBinding`, `kBLD`) is mandatory so the RmlUi data-model listener skips its own writes and avoids the feedback loop called out in the Phase-11 risks table.

## Phase-10 handoff

Wave 11B takes ownership of Phase-10's TOML files:

- `action_map_toml.cpp` — rewritten to use `toml++` when gated on. The Phase-10 parser stays compiled as the gate-off fallback; the **regression suite** in `unit/input/action_map_test.cpp` is the merge gate — every fixture round-trips byte-identically or the migration fails.
- `audio_mixing.toml` — becomes a `audio.*` CVar domain file. Existing `AudioService::set_bus_volume` calls stay, but subscribe to `ConfigCVarChanged` for propagation.

## Perf contract

| Gate | Target |
|---|---|
| `CVar<T>::get()` | ≤ 5 ns (snapshot load) |
| `CVar<T>::set() + publish` | ≤ 200 ns |
| `load_domain("ui.toml")` cold | ≤ 2 ms for ~40 entries |
| `load_domain("ui.toml")` warm | ≤ 0.3 ms |
| Heap alloc on hot path | 0 B |

## Tests (≥ 12)

1. every scalar type round-trips (`bool, i32, i64, f32, f64, string, enum`)
2. enum serialisation uses named variants (`"hrtf"`, not `3`)
3. persistence across registry rebuild (kill + reload from disk)
4. hot-reload of external edit publishes a second `ConfigCVarChanged`
5. `kReadOnly` enforcement in non-dev builds
6. `kCheat` gating behind `dev.console.allow_cheats`
7. out-of-range clamp on load with diagnostic emitted to log
8. stale-snapshot detection (reader sees old value until fence)
9. concurrent-reader + single-writer race (TSAN clean)
10. TOML parse error reports line + column
11. Phase-10 `input.toml` backward-compat — fixture round-trips byte-identical
12. `action_map_toml.cpp` regression suite still green after toml++ swap

## Alternatives considered

- **JSON / YAML** — JSON is too verbose for hand-editing; YAML is specification-unstable and error-prone on indentation.
- **ini + hand-rolled** — Phase 10's approach; does not scale to nested tables.
- **Binary profile blob** — fine for release ship, hostile to user-editing and diffing.

Rejected. TOML is the community default for game settings; toml++ is header-only, no-exceptions-mode capable, MIT-licensed, and gives us a clean C++23 API.

---

## ADR file: `adr/0025-engine-console-dev-overlay.md`

## ADR 0025 — Engine console: dev overlay, command registry, debug-only gating

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 11 Wave 11C)
- **Deciders:** Cold Coffee Labs engine group
- **Related:** ADR-0023 (event bus), ADR-0024 (CVars), ADR-0026 (RmlUi), blueprint §3.20

## Context

Every shipping runtime needs a dev console: tweak CVars, call commands, inspect state, reload assets. The console is **RmlUi-backed** per blueprint §3.20 and CLAUDE.md #12 (in-game UI is RmlUi, not ImGui).

## Decision

Ship `engine/console/` with:

- `ConsoleService` — RAII façade. Owns the command registry, a reference to `CVarRegistry`, a 256-line history ring, and an autocomplete trie over `{CVar names} ∪ {command names}`.
- `ICommand` — interface: `void exec(std::span<const std::string_view> argv, ConsoleWriter&)`, optional `complete(prefix, ConsoleWriter&)` for custom tab-completion.
- `ConsoleWriter` — scoped output handle; writes bytes to a ring that the RmlUi overlay drains each frame.
- Built-in commands (all flagged `kDevOnly` when appropriate): `help`, `set`, `get`, `bind`, `audio.duck_music`, `audio.stats`, `input.list_devices`, `ui.reload`, `fs.dump_asset_cache`, `time.scale`, `quit`, `events.stats`, `cvars.dump`, `bld.echo`.
- **Overlay**: a single `console.rml` document; drawn through the shared UI pass (ADR-0027). Toggle binds: backtick, F1, or gamepad Select+Back. The overlay document is *not* compiled into the binary — it is shipped as an asset under `assets/ui/console.rml`.

### Release-build gating

- Compile-flag `GW_CONSOLE_DEV_ONLY=1` (default ON in release) strips every `kDevOnly` command at link time via `[[no_unique_address]]` empty-tag optimisation.
- Release binaries ship with the console code-path compiled, but the command table is empty and the tilde-toggle is guarded behind the CVar `dev.console.allow_release` (default `false`).
- A "CHEATS ENABLED" banner shows whenever any `kCheat` CVar has deviated from its default, matching Valve-source-engine precedent.

### Accessibility

- The overlay respects `ui.text.scale` and `ui.contrast.boost` via the same RCSS theme used by the HUD (ADR-0026 §theming).
- The single text input has a 2-px `signal`-colour focus ring.
- Tab cycles: command line → history → close. Keyboard-only operation is first-class.

## Perf contract

| Gate | Target |
|---|---|
| Console open → first character rendered | ≤ 80 ms |
| `help` over 300 commands | ≤ 2 ms |
| Autocomplete 2-char prefix over 300 items | ≤ 0.5 ms |
| Heap on command dispatch | 0 B |

## Tests (≥ 8)

1. register / deregister commands; names are unique-or-error
2. argv parser handles quoted strings with embedded spaces
3. `set` type-coercion errors print the CVar's expected type
4. autocomplete by prefix and by substring, sorted stably
5. history persists across session via `user/console_history.txt`
6. release-build `kDevOnly` commands are absent from the command table
7. RmlUi data-model round-trip — writing to the model updates the service buffer and vice versa
8. concurrent publish from `ConsoleCommandExecuted` while a command is running does not reenter the command

## Alternatives considered

- **ImGui console** — violates CLAUDE.md #12. ImGui lives only in editor.
- **Terminal-attached REPL** — not usable on Steam Deck / console. UI-in-engine is the only portable answer.
- **Skip the release overlay entirely** — the `--allow-console` flag is a critical QA affordance during bug-bash; striping it out would cost us debug velocity in Phase 14+.

## Consequences

- `audio.*` and `input.*` domains gain one command each (`audio.stats`, `input.list_devices`) that dumps current state. These commands are the first BLD-readable console tap; Phase 18 wires them to the BLD Plan-Act-Verify loop as state-inspection hooks.
- The Phase-10 ad-hoc `std::fprintf(stderr, …)` calls inside `audio_service.cpp` are forbidden going forward; the audio service emits to `engine/core/logger` which the console can mirror.

---

## ADR file: `adr/0026-engine-ui-rmlui-service.md`

## ADR 0026 — `engine/ui/`: RmlUi service, context lifecycle, system/file interfaces

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 11 Wave 11D)
- **Deciders:** Cold Coffee Labs engine group
- **Related:** ADR-0027 (Vulkan-HAL backend), ADR-0028 (text shaping), blueprint §3.20, CLAUDE.md #12

## Context

Blueprint §3.20 and CLAUDE.md #12 name **RmlUi** as the in-game UI library. Phase 11 must make it shippable. Upstream RmlUi 6.2 (released 2026-01-11) carries the semaphore fix from PR #874 and ships native touch + a data-model debugger; 6.2 is our pin.

## Decision

Ship `engine/ui/` with a `UIService` that:

- Owns the single `Rml::Context` per window. A secondary context is allowed for overlays (console, debugger) but they share the one frame-graph pass (ADR-0027).
- Drives `context->Update()` + `context->Render()` from the main thread, after `InputService::update` and before the render-submit.
- Implements `Rml::SystemInterface` (`rml_system.cpp`): time via `gw::core::Time`, logs via `gw::core::logger`, clipboard via a new `engine/platform/clipboard` seam.
- Implements `Rml::FileInterface` (`rml_filesystem.cpp`) against the virtual FS used by cooked assets — so `.rml`, `.rcss`, and font files all go through the same path as textures/meshes.
- Forwards input via Phase-10's `RawSnapshot` through a `rml_input_bridge` that translates `KeyCode`/`MouseButton` to RmlUi key/modifier enums. Gamepad nav uses the same bridge with a synthesised focus-cursor driven by `input.menu.*` actions.
- Exposes `UIService::set_locale(StringView tag)` which routes to the `LocaleBridge` in ADR-0028.
- Integrates the `rml_debugger` plugin in debug builds only; console command `ui.debugger` toggles its overlay.

### Gating

`GW_UI_RMLUI` (default **ON** for the `gw_runtime` preset, **OFF** for `dev-win` / `dev-linux` CI) pulls the RmlUi CPM package. When off, a stub implementation of `UIService` compiles and runs — it registers the same command handlers, accepts the same CVar changes, and exposes the same `update()/render()` entry points, but draws nothing. Every test that asserts "3 UI elements rendered" is skipped (tagged `[skip-if-no-ui]`) when gated off.

### Theming

A single `theme.rcss` under `assets/ui/` defines the **Cold Drip** palette as CSS custom properties (`--obsidian`, `--greywater`, `--brew`, `--crema`, `--signal`, `--bean`). `ui.contrast.boost = true` swaps to a high-contrast token set; no UI code is hand-tinted.

### Context lifecycle and focus

- `FocusChanged` events on the Phase-11A bus steer RmlUi focus via `Rml::ElementDocument::Focus`.
- Gamepad navigation emulates `tab`/`shift-tab` / `space` / `enter` via `input.menu.nav_*` actions. Every focusable element has a visible 2-px `--signal`-coloured ring (WCAG 2.2 AA contrast ≥ 3:1 against every background).
- `WindowResized` resizes every context and reloads the layout.

## Perf contract

See ADR-0028 for text-shape budgets and ADR-0027 for GPU-pass budgets. `UIService::update()` itself (no layout invalidation) is budgeted at **≤ 0.1 ms** per frame at 1080p.

## Tests (≥ 4 in this ADR)

1. `UIService` null-backend constructs and tears down cleanly
2. `set_locale` propagates through `LocaleBridge` and triggers a re-layout
3. `WindowResized` event rebuilds the context dimensions
4. Debugger plugin toggles cleanly when enabled in debug builds

Additional UI tests live in ADR-0027/0028/0029 territory.

## Alternatives considered

- **Noesis GUI** — commercial; license costs + vendor lock.
- **Ultralight** — AGPL / commercial; unclear embedded runtime size, no Vulkan HAL.
- **Write our own** — explicitly forbidden by CLAUDE.md #13.
- **Delay UI integration to Phase 16** — makes the *Playable Runtime* milestone unreachable. Non-starter.

## Consequences

- `engine/platform/clipboard` is introduced in this wave as a thin OS-seam (Win32 `OpenClipboard`, Wayland `wl_data_device`, X11 `XClipboard`). Three-file module; 50 lines each.
- `rml_debugger` lives in a debug-only `.cpp` compiled under `#ifdef GW_UI_RMLUI_DEBUGGER` (default ON in Debug, OFF in Release).
- Phase 12's physics-anchored HUD (world-space → screen-space name tags) needs a `RmlUi::DataModel` feature that's already in 6.2; no retrofit needed.

---

## ADR file: `adr/0027-engine-ui-vulkan-hal-backend.md`

## ADR 0027 — RmlUi render backend via Vulkan HAL

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 11 Wave 11D)
- **Deciders:** Cold Coffee Labs engine group
- **Related:** ADR-0026 (UI service), blueprint §3.20, `engine/render/frame_graph/`

## Context

RmlUi 6.2 ships an optional Vulkan backend as PR #836 (still draft upstream at Phase 11 kickoff). Greywater already owns a Vulkan HAL in `engine/render/hal/` — the RmlUi backend must talk to *that*, never to a raw `VkDevice`.

## Decision

Implement `engine/ui/backends/rml_render_hal.{hpp,cpp}` as an `Rml::RenderInterface`:

- Every `RenderCompiledGeometry` call binds one of a small cache of graphics pipelines keyed by `{vertex_layout, scissor_enabled, texture_bound}`. The cache lives in the HAL and is shared with the debug-draw pipeline cache.
- `CompileGeometry` returns an engine-side `hal::MeshHandle` packed into the RmlUi handle slot. Geometry is uploaded once to a device-local `hal::VulkanBuffer`; updates are rare (layout change) and go through the HAL's transient uploader.
- `LoadTexture` / `GenerateTexture` map to `hal::VulkanImage` creation. Static RmlUi textures (icons, atlas pages) are encoded as **BC7** at cook-time; dynamic glyph atlas (ADR-0028) is `R8_UNORM` live-uploaded. Mipmaps disabled for pixel-exact UI.
- `EnableScissorRegion` / `SetScissorRegion` map to Vulkan dynamic scissor state (`vkCmdSetScissor`) on the HAL command buffer.
- `EnableClipMask` / `RenderToClipMask` implement the stencil-based rounded-corner clipping that landed in RmlUi 6.x; the HAL exposes a depth/stencil dynamic-rendering path that PR #836 requires.
- Transform stack (`SetTransform`) multiplies into a push-constant block; no uniform buffer per draw.

### Frame-graph integration

A new `ui_pass` `FrameGraphNode` is registered at engine init:

- **Reads:** `UIColorInput` (the resolved scene colour, `LOAD_OP_LOAD`)
- **Writes:** `UIColorOutput` (a transient rename of the swapchain colour attachment)
- **Runs after:** `scene_resolve_pass`
- **Runs before:** `present_pass`
- **Command buffer:** allocated by the frame-graph scheduler; RmlUi's backend records into it via HAL primitives.
- **Contexts drained per pass:** HUD (primary), console (debug overlay), rml-debugger (debug overlay). Ordered by z-layer — console on top.

### Vendoring strategy

PR #836 is pulled as a vendored patch (`third_party/rmlui_vulkan_backend.patch`) applied on top of the 6.2 tag in CPM. The patch is rewritten so the backend never touches a raw `VkDevice`; all calls funnel through `hal::VulkanDevice`. An upstream exit path is planned: once upstream merges #836, we open a follow-up PR that routes their code through our HAL interfaces (or loops back to the canonical Vulkan objects, configurable at compile time).

### Gating

Same as ADR-0026: `GW_UI_RMLUI=ON` pulls the backend; `GW_UI_RMLUI=OFF` substitutes a null `Rml::RenderInterface` that records draw-counts for tests but never touches the GPU.

## Perf contract

| Gate | Target | Hardware basis |
|---|---|---|
| UI pass GPU time at 1080p (HUD visible) | ≤ 0.8 ms | GTX 1060 |
| Pipeline-cache hit ratio steady-state | ≥ 98 % | HUD + console open |
| Texture-cache hit ratio steady-state | ≥ 98 % | Cold Drip theme |
| Glyph-atlas upload delta per frame | ≤ 64 KB steady, ≤ 512 KB worst | see ADR-0028 |

## Tests (≥ 3 in this ADR)

1. Pipeline-cache eviction under rapid scissor toggle
2. Texture-cache lifecycle: upload → bind × 100 frames → destroy, zero leaks (VMA stats)
3. Frame-graph pass correctly orders HUD above console above debugger across one full frame

## Alternatives considered

- **Write the backend directly against `VkDevice`** — violates the HAL quarantine (CLAUDE.md #2 non-negotiables).
- **Render RmlUi to an offscreen target and blit** — doubles the fillrate cost at 1080p; measurable on an RX 580.
- **Wait for upstream #836 to merge** — blocks the *Playable Runtime* milestone on an external cadence. Unacceptable.

## Consequences

- A studio fork of RmlUi at `third_party/rmlui/` is introduced. Rebasing on each upstream tag is a Phase-18 maintenance task.
- The `ui_pass` frame-graph node is the first post-resolve pass that does not run at the scene resolution; it always targets the swapchain (to avoid a second resolve). Phase 17's VFX work inherits this pattern.

---

## ADR file: `adr/0028-engine-ui-text-shaping.md`

## ADR 0028 — Text shaping pipeline: FreeType 2.14.3 + HarfBuzz 14.1.0 + SDF atlas + locale bridge

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 11 Wave 11D)
- **Deciders:** Cold Coffee Labs engine group
- **Related:** ADR-0026 (UI service), blueprint §3.20

## Context

RmlUi ships with a bundled FreeType font provider, but for the game we need:

- **SDF glyphs** so `ui.text.scale 0.75×–2.0×` does not re-shape per frame.
- **COLRv1 colour emoji** in captions and chat.
- **CJK + complex-script shaping** so the Japanese locale test passes on day one.
- A **locale bridge** that Phase 16 can replace with ICU without breaking callers.

## Decision

Ship `engine/ui/` with:

- `FontLibrary` — owns a thread-safe cache of `FT_Face` objects keyed by font-id. Faces are instantiated once (`FT_New_Memory_Face`); per-size shapers are cheap derivatives.
- `TextShaper` — wraps `hb_shape` against the FT face. Inputs: script, direction, language tag, a span of UTF-8 code points. Output: `ShapedRun { std::span<GlyphInstance> }` where `GlyphInstance = { glyph_id, x_advance, x_offset, y_offset, cluster }`. Returned spans are backed by a per-shape-call arena (`gw::memory::arena_allocator`); callers copy if they need persistence.
- `GlyphAtlas` — 4 K × 4 K `R8_UNORM` signed-distance-field atlas rasterised via FreeType's `FT_RENDER_MODE_SDF` (introduced in 2.10, stabilised in 2.14). Pages are uploaded on demand through the HAL; eviction policy is LRU per locale-group. Fallback alpha-coverage path for bitmap emoji (COLRv1 / CBDT) via HarfBuzz's `hb_color` API and FT's SVG driver.
- `LocaleBridge` — a simple `StringId → std::string_view` map. Phase 11 ships a TOML-backed placeholder loader (`assets/strings/en-US.toml`, `fr-FR.toml`, `ja-JP.toml`). The interface is frozen so ADR-0044 (Phase 16) can plug ICU + MessageFormat behind it.

### Dependency pins

| Library | Pin | Gate | Why |
|---|---|---|---|
| FreeType | 2.14.3 (2026-03-22) | `GW_UI_FREETYPE` (default ON in `gw_runtime`, OFF in CI) | 2.14 adds dynamic HarfBuzz loading; 2.14.3 patches two CVEs and improves LCD filtering. SDF driver via `FT_CONFIG_OPTION_SDF`. |
| HarfBuzz | 14.1.0 (2026-04-04) | `GW_UI_HARFBUZZ` | 14.x stabilises COLRv1 paint-tree and ships the Slug GPU rasteriser; we consume CPU-only but keep the 14 API for future Slug adoption. |

When either gate is OFF, a placeholder shaper emits monospace-Latin-only runs and a dummy atlas of empty glyphs — enough to compile + run; tests marked `[skip-if-no-fonts]` skip.

### Threading

- `FontLibrary` face-cache mutex-guarded (coarse-grained; N ≤ 16 fonts in a running game).
- `TextShaper` is stateless per call; many can run concurrently.
- `GlyphAtlas` upload path runs on the I/O job pool; rasterisation (`FT_Render_Glyph` with SDF) dispatched to the jobs pool for pages under hot-load.

## Perf contract

| Gate | Target |
|---|---|
| Shape + layout 200-glyph Latin run | ≤ 0.2 ms |
| Shape + layout 100-glyph CJK run | ≤ 0.4 ms |
| Shape + layout 50-glyph Arabic (RTL) | ≤ 0.5 ms |
| Glyph atlas upload delta / frame | ≤ 64 KB steady, ≤ 512 KB worst |
| COLRv1 emoji raster | ≤ 0.1 ms per 72×72 glyph |

## Tests (≥ 8 in this ADR)

1. Latin shaping vs. pinned HarfBuzz oracle corpus
2. CJK shaping against pinned HB fixture (Noto Sans CJK JP)
3. RTL shaping correctness with a right-to-left Arabic fixture (even though RmlUi RTL layout is Phase 16, HB output must already be correct)
4. COLRv1 glyph raster round-trip matches bitmap oracle within 1 ULP
5. SDF atlas eviction under hot-load (1000 random glyphs, cache size = 512)
6. `ui.text.scale = 1.5` preserves layout metrics without re-shaping
7. LocaleBridge resolves `StringId → UTF-8` for en-US / fr-FR / ja-JP fixtures
8. Missing glyph falls back to `.notdef` rather than crashing

## Alternatives considered

- **MSDF instead of SDF** — higher quality at small sizes but needs a preprocessing step; we don't ship a tool for it in Phase 11. SDF is sufficient for our target scale range.
- **RmlUi's bundled FT font provider** — doesn't expose SDF or HarfBuzz path; replacing it is cleaner than forking.
- **Slug (HarfBuzz 14 GPU rasteriser)** — requires fp64 or indirect pixel-shader work that the RX 580 can run but we haven't validated. Defer to Phase 17; consume CPU HB now.
- **ICU now** — 50+ MB binary, full i18n surface, out of scope for Phase 11 motor budget. `LocaleBridge` seam reserves the slot.

## Consequences

- `assets/ui/fonts/` ships three fonts by default: **Inter** (Latin UI), **Noto Sans CJK** (CJK fallback), **Noto Color Emoji** (COLRv1 emoji). Licences: Inter is OFL 1.1; Noto is OFL 1.1; Noto Color Emoji is OFL 1.1 (Google SIL OFL release). All three are redistributable under the repo's license manifest.
- Phase 16's ICU drop replaces `LocaleBridge` internals, keeping the interface.

---

## ADR file: `adr/0029-playable-runtime-wiring.md`

## ADR 0029 — Playable runtime: `Engine::Impl` boot order, sandbox scene, CI self-test

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 11 Wave 11F)
- **Deciders:** Cold Coffee Labs engine group
- **Related:** ADR-0017…0028, blueprint §Phase-11 exit demo

## Context

Phase 11's gate is binary: a QA tester sits down, plays a 60-second scene, adjusts the text scale, rebinds a key, toggles the dev console, quits, and finds their settings persisted. Every ADR in 0023–0028 must bolt together cleanly in `runtime/main.cpp` for that to happen.

## Decision

### Boot order (RAII-ordered, single-threaded main)

```
0. CLI parse + env (docs/06 §runtime flags)
1. CorePlatform  (engine/platform/window, time, fs, dll)
2. JobSystem     (engine/jobs/)
3. EventBus      (engine/core/events)
4. CVarRegistry  (engine/core/config; loads graphics.toml / audio.toml / input.toml / ui.toml)
5. AudioService  (engine/audio; backend from audio.toml)
6. InputService  (engine/input; profile from input.toml)
7. UIService     (engine/ui; font library + RmlUi context)
8. ConsoleService (engine/console; registers commands against 3–7)
9. SceneLoader + FrameGraph
10. Main loop
```

Destruction is the mirror image. Any constructor failure propagates `gw::core::Result<Engine, BootError>` up to `main`, which logs and exits cleanly — no destruction of partially-constructed subsystems.

### `runtime/main.cpp` CLI

- `--config-dir=<path>` (default: platform save dir; tests override)
- `--self-test --frames=N --golden=<png>`
- `--allow-console=<0|1>`
- `--replay-trace=<path>.input_trace`
- `--record-trace=<path>.input_trace`
- `--scene=<path>.gwscene` (default: `assets/scenes/playable_boot.gwscene`)

### Sandbox playable scene

`apps/sandbox_playable/` ships the 60-second demo: a small 3D room, three audio emitters (ambience loop, crackle one-shot, voice line with a caption-channel publish), a first-person action set, a HUD (hp, interact, caption area), and the dev console bound to backtick/F1.

### CI self-test

`ctest --preset playable-windows` / `playable-linux`:

- Launches `gw_runtime --self-test --frames=120 --golden=tests/golden/playable_120f.png`
- Asserts:
  1. The process exits with code 0 within 30 s
  2. ≥ 3 RmlUi elements drew on frame 120
  3. Audio callback pumped ≥ 120 × 1024 frames (NullBackend deterministic)
  4. Input action `Jump` fired ≥ 1× through the replay trace
  5. ≥ 1 `ConfigCVarChanged` observed on the bus (the self-test toggles `ui.text.scale`)
  6. `graphics.toml` + `audio.toml` + `input.toml` + `ui.toml` are written on exit and round-trip byte-identical to the seed

### Determinism knobs

The self-test seeds:

- `--replay-trace=tests/fixtures/input/playable_demo.input_trace`
- `audio.deterministic = true`
- `input.backend = trace`
- `rng.seed = 0xC0LD_C0FFE3`

and asserts the `.event_trace` output byte-matches `tests/golden/playable_events_120f.event_trace`.

## Perf contract

| Gate | Target | Notes |
|---|---|---|
| Runtime boot (menu visible) | ≤ 1.2 s cold, ≤ 0.5 s warm | on NVMe |
| Frame-pacing on the demo scene at 1080p | 60 FPS nominal | RX 580 |
| Steady-state heap growth | 0 B / frame | traced via jemalloc profile in nightly |

## Tests (≥ 4 in this ADR)

1. Boot-order unit test: each stage constructs with a null-stub predecessor (no RAII leak)
2. CLI parser: unknown flag is an error with line-column diagnostic
3. `--self-test --frames=1` exits 0 with no sound card and no GPU
4. `user.toml`s round-trip through boot → frame-loop → shutdown unchanged

## Alternatives considered

- **One giant `runtime::App` class** — doesn't mirror the blueprint's `Engine::Impl` shape; harder to test subsystems in isolation.
- **Run the self-test as a plain unit test** — cannot exercise the full boot path + RmlUi + audio callback + input trace in one process; end-to-end exit-code assertion is the minimum useful signal.

## Consequences

- `docs/05_RESEARCH_BUILD_AND_STANDARDS.md` gains a `Playable Runtime` section documenting the new presets.
- The asset pipeline gains the `assets/scenes/playable_boot.gwscene` fixture; its cook outputs pin the 120-frame golden.

---

## ADR file: `adr/0030-phase11-cross-cutting.md`

## ADR 0030 — Phase 11 cross-cutting decisions (summary)

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 11 exit gate)
- **Deciders:** Cold Coffee Labs engine group
- **Summarises:** ADRs 0023–0029

This ADR is the **cross-cutting summary** for Phase 11. It pins dependencies, perf gates, and thread-ownership decisions that span more than one wave.

## 1. Dependency pins

| Library | Pin | Gate (CMake option) | Default |
|---|---|---|---|
| RmlUi | 6.2 (2026-01-11) | `GW_UI_RMLUI` | ON for `gw_runtime` / `playable-*`, OFF for `dev-*` |
| HarfBuzz | 14.1.0 (2026-04-04) | `GW_UI_HARFBUZZ` | same as above |
| FreeType | 2.14.3 (2026-03-22) | `GW_UI_FREETYPE` | same as above |
| toml++ | 3.4.0 | `GW_CONFIG_TOMLPP` | same as above |

All four gates default **OFF** on the CI pipelines that do *not* target the Playable Runtime milestone. The stub implementations compile and run identically to the production path for every subsystem test that does not require a real font rasteriser or real TOML round-trip.

**Why gate off by default for `dev-*` CI?** The phase's Definition-of-Done is tested by the stubs + by `playable-*`. The stubs exist so a clean dev-preset checkout never needs the heavy deps; the `playable-*` preset opts every flag in. Same principle as Phase 10 (ADR-0022 §1).

## 2. Hot-path budgets (CI-enforced via `gw_perf_gate`)

| Gate | Target | ADR |
|---|---|---|
| `EventBus::publish` empty | ≤ 12 ns | 0023 |
| `InFrameQueue` drain 1 000 | ≤ 40 µs / 0 B heap | 0023 |
| `CrossSubsystemBus` 1 000 × 3 subs | ≤ 120 µs / 0 B heap | 0023 |
| `CVar<T>::get()` | ≤ 5 ns | 0024 |
| `CVar<T>::set() + publish` | ≤ 200 ns | 0024 |
| `load_domain(~40 entries)` cold | ≤ 2 ms | 0024 |
| `load_domain(~40 entries)` warm | ≤ 0.3 ms | 0024 |
| Console open → first char | ≤ 80 ms | 0025 |
| `help` over 300 items | ≤ 2 ms | 0025 |
| Autocomplete 2-char prefix | ≤ 0.5 ms | 0025 |
| Shape + layout 200-glyph Latin | ≤ 0.2 ms | 0028 |
| Shape + layout 100-glyph CJK | ≤ 0.4 ms | 0028 |
| Glyph atlas delta / frame | ≤ 64 KB steady | 0028 |
| UI pass GPU time | ≤ 0.8 ms | 0027 |
| Runtime boot | ≤ 1.2 s cold / 0.5 s warm | 0029 |

## 3. Thread-ownership

| Thread | Owns | New calls in Phase 11 |
|---|---|---|
| Main | EventBus drain, CVarRegistry writes, UIService::update, ConsoleService::dispatch, RmlUi Context::Update+Render | `rml_render_hal` record, font atlas update |
| Render | `ui_pass` FrameGraphNode draw | `rml_render_hal::RenderCompiledGeometry` |
| Audio | no change | — |
| Jobs | CrossSubsystemBus async dispatch, SDF atlas rasterisation | — |
| I/O | `.rml`, `.rcss`, font reads | — |

**No** `std::thread` / `std::async` introduced. All parallelism flows through `engine/jobs/`.

## 4. What Phase 12 inherits

1. The RmlUi data-model is the wire for physics-anchored HUD tags (health bars over 3D bodies), which Phase 12 lands.
2. `CrossSubsystemBus` is the wire for physics-hit audio cues and physics debug-draw toggles via CVars.
3. The `ui_pass` frame-graph node is the pattern Phase 17's VFX passes copy.

## 5. Cross-platform notes

- **Windows**: RmlUi uses Win32 `OpenClipboard` via `engine/platform/clipboard`; high-DPI `PROCESS_PER_MONITOR_DPI_AWARE_V2` at window creation.
- **Linux / Wayland**: clipboard via `wl_data_device`; DPI from `GDK_SCALE` or Wayland `wl_output::scale`.
- **Steam Deck**: treated as Linux-Wayland; glyph atlas budget relaxed 25% (Deck has 16 GB unified RAM). DualSense rumble continues through SDL3's path (Phase 10).
- **HiDPI**: `ui.dpi.scale` CVar; default auto-detect, round to nearest 0.25.

## 6. What did *not* land in Phase 11 (deferred by design)

| Not landing | Target phase | Why deferred |
|---|---|---|
| ICU runtime + XLIFF | Phase 16 | Heavy dep; LocaleBridge seam reserved |
| HarfBuzz-shaped RTL in RmlUi layout | Phase 16 | HB ready; RmlUi RTL lands at i18n gate |
| Screen-reader hooks (MSAA/IAccessible2/AT-SPI) | Phase 16 | Needs i18n + a11y bundle |
| On-screen virtual keyboard | Phase 16 | Same |
| Tracy panels / ImNodes inspector for event graph | Phase 17/18 | Editor tooling, downstream |
| RmlUi data-model ↔ BLD agent UI edits | Phase 18 | Needs BLD context model |
| Physics-anchored HUD | Phase 12 | Needs Jolt |
| Subtitle rendering with speaker attribution | Phase 16 | Localised content; caption channel seam reserved |
| Voice-chat push-to-talk UI | Phase 14 | GNS wires to `IVoiceCapture`/`IVoicePlayback` first |

## 7. Exit-gate checklist

- [x] Week rows 066–071 all merged behind the `phase-11` tag
- [x] ADRs 0023–0029 land as Accepted
- [x] ADR 0030 lands as the summary (this doc)
- [x] `gw_tests` green with ≥ 50 new event / config / console / UI cases
- [x] `ctest --preset dev-win` / `dev-linux` green
- [x] `ctest --preset playable-windows` / `playable-linux` green when run with Phase-11 deps on
- [x] Performance gates §2 green
- [x] `docs/a11y_phase11_selfcheck.md` signed off
- [x] `docs/05` marks Phase 11 *shipped*
- [x] Git tag `v0.11.0-runtime-2` pushed

---

## ADR file: `adr/0031-engine-physics-facade.md`

## ADR 0031 — engine/physics — facade, layers, fixed-step loop

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 12 Wave 12A)
- **Deciders:** Cold Coffee Labs engine group
- **Supersedes / relates:** ADR-0023 (events), ADR-0024 (CVars), ADR-0029 (runtime boot), CLAUDE.md non-negotiables #2, #3, #5, #6, #9, #10, #11

## Context

Phase 11 closed the Playable Runtime with a null event / CVar / UI / console / input / audio stack wired through `runtime::Engine`. Phase 12 layers deterministic rigid-body dynamics onto that stack. The *Living Scene* milestone in Phase 13 (animation + BT + navmesh) is the downstream consumer, so Phase 12 must produce:

1. a stable public C++23 surface that does **not** leak `<Jolt/**>` into any header under `engine/`,
2. a deterministic, fixed-step simulation loop,
3. an RAII owner graph with no hidden globals,
4. a null backend that compiles and ticks identically on Windows + Linux at the `dev-*` preset, and
5. a Jolt backend reachable from `physics-*` and `playable-*` presets, behind `GW_ENABLE_JOLT=ON`.

## Decision

### 1. PIMPL facade

`engine/physics/physics_world.hpp` declares a single public class `PhysicsWorld`. All implementation state lives in a concrete `Impl` class owned via `std::unique_ptr<IBackend>`. Two backends ship:

| Backend | Entry | Gate | Used by |
|---|---|---|---|
| Null | `make_null_backend()` | always available | `dev-*` preset CI, all unit tests that do not explicitly want Jolt |
| Jolt | `make_jolt_backend()` | `GW_PHYSICS_JOLT` | `physics-*` + `playable-*` presets |

`PhysicsWorld::PhysicsWorld(const PhysicsConfig&)` resolves the backend. When `GW_PHYSICS_JOLT=OFF` the Jolt factory is absent from the TU and the null backend is always chosen.

### 2. Object + broadphase layers

```cpp
enum class ObjectLayer : std::uint16_t {
    Static        = 0,
    Dynamic       = 1,
    Trigger       = 2,
    Character     = 3,
    Projectile    = 4,
    Debris        = 5,
    WaterVolume   = 6,
    UserA         = 7,  // reserved for gameplay authors
    UserB         = 8,
    // 9..15 reserved for future engine use
};
enum class BroadPhaseLayer : std::uint8_t {
    Static  = 0,
    Moving  = 1,
};
```

Layer-vs-layer collision matrix is a single `std::uint32_t` per `ObjectLayer` (one bit per opponent). `OBJECT_LAYER_BITS=16` in the Jolt CMake pin matches this; the 16-bit ceiling is intentional (matches `JPH::ObjectLayer` default and keeps the matrix comfortably cache-resident).

### 3. Lifecycle

`PhysicsWorld` is owned by `runtime::Engine::Impl` and constructed **after** `CVarRegistry` is seeded (so gravity / solver iterations / sleep thresholds are read from standard CVars) and **before** any ECS system that wants to spawn bodies. Destruction runs in reverse. No `init()`/`shutdown()` pair exists on the public API — CLAUDE.md non-negotiable #6.

### 4. Fixed-step simulation

`PhysicsWorld::step(double wall_dt)` drains a single-precision accumulator and calls `IBackend::substep(fixed_dt)` zero or more times:

```
accumulator += min(wall_dt, fixed_dt * substeps_max)
while (accumulator >= fixed_dt):
    backend_->substep(fixed_dt)
    accumulator -= fixed_dt
    ++substep_index
interpolation_alpha = accumulator / fixed_dt
```

Rendering reads `PhysicsWorld::interpolation_alpha()` (0..1) to interpolate body transforms between the current and previous substep's cached poses. `fixed_dt` defaults to `1.0 / 60.0` and is bound to the `physics.fixed_dt` CVar; `substeps_max` is bound to `physics.substeps.max`. Phase 12 ships with no substeps below a single fixed step per `step()` call in headless / deterministic mode (`alpha == 0`).

### 5. Ownership + thread rules

| Object | Owner | Thread rules |
|---|---|---|
| `PhysicsWorld` | `runtime::Engine::Impl` | Main thread only |
| `IBackend` (null / Jolt) | `PhysicsWorld` | Implementation-private; Jolt path submits into `engine/jobs/` (ADR-0032) |
| `BodyHandle`, `ShapeHandle`, `ConstraintHandle`, `CharacterHandle` | 32-bit opaque IDs, ref-counted via `PhysicsWorld::retain_*` / `release_*` | Main thread only |
| Contact listener | Internal; pushes into `PhysicsWorld`'s `InFrameQueue<PhysicsContactAdded>` etc. | Jobs thread produces; main drains |

No raw `new`/`delete` in engine/physics — every handle is a typed ID and resources live behind unique_ptr inside the backend. No `<Jolt/**>` outside `engine/physics/impl/jolt_*.cpp` (and the single `impl/jolt_bridge.hpp` TU-local header).

### 6. Boot order (revised `runtime::Engine`)

```
Tier 0: CVarRegistry (+ standard + physics CVars), event buses
Tier 1: LocaleBridge, FontLibrary, GlyphAtlas, TextShaper
Tier 2: AudioService, InputService
Tier 3: PhysicsWorld                        ← new, Phase 12
Tier 4: UIService, SettingsBinder
Tier 5: ConsoleService (+ built-ins including physics.*)
```

Tick order (ADR-0029 revision §3):

```
1. input.update(now_ms)
2. drain CrossSubsystemBus → engine buses
3. physics.step(dt)                         ← new, Phase 12
4. drain physics event queues               ← new
5. ecs::tick()  (transform_sync after physics.step)
6. ui.update(dt)
7. audio.update(dt, listener)
8. ui.render()
9. ++frame_index
```

## Consequences

- Phase 13 animation can drive rigid-body-pose targets without ever touching `<Jolt/**>`.
- Phase 14 networking replicates `BodyHandle` → `TransformState` without replicating physics state; physics re-derives on the client via rewind-and-replay (ADR-0037 is the determinism contract enabling that).
- The CVar-driven solver knobs mean a content creator never edits C++ to tune a scene.
- Header quarantine makes the Jolt version bump a one-file change (ADR-0037 §upgrade-gate then replays the determinism capture).

## Alternatives considered

- **Exposing `JPH::Body*` directly.** Rejected — would couple the editor, gameplay, Phase 13 AI, and Phase 14 replication to the Jolt version.
- **Two-phase `init()`/`shutdown()`**. Rejected — violates non-negotiable #6; RAII is the house pattern.
- **Variable-step simulation.** Rejected — breaks determinism (ADR-0037).
- **Spawning Jolt threads.** Rejected — violates non-negotiable #10; `JoltJobBridge` adapts into `engine/jobs/` (ADR-0032).

---

## ADR file: `adr/0032-physics-ecs-bridge.md`

## ADR 0032 — Physics ↔ ECS bridge — components, transform sync, interpolation

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 12 Wave 12A)
- **Deciders:** Cold Coffee Labs engine + ECS group
- **Relates:** ADR-0004 (ECS World), ADR-0008 (TransformComponent with `dvec3`), ADR-0031 (physics facade), ADR-0037 (determinism contract)

## Context

Every physics-driven entity needs a stable mapping from `ecs::EntityHandle` ↔ `physics::BodyHandle`, with a canonical phase-ordering rule so the render thread sees a consistent transform. The `TransformComponent` uses `dvec3` positions (ADR-0008) to keep planet-scale scenes jitter-free; physics must accept + return the same precision without rounding every tick.

## Decision

### 1. ECS components (new, Phase 12)

```cpp
namespace gw::ecs::physics {

struct RigidBodyComponent {
    physics::BodyHandle body{};          // opaque; 0 = not yet created
    physics::BodyMotionType motion{physics::BodyMotionType::Dynamic};
    float mass_kg{1.0f};
    float linear_damping{0.05f};
    float angular_damping{0.05f};
    float friction{0.5f};
    float restitution{0.1f};
    physics::ObjectLayer layer{physics::ObjectLayer::Dynamic};
    bool  is_sensor{false};              // trigger when true
};

struct ColliderComponent {
    physics::ShapeHandle shape{};
    // Authoring offset applied at body-creation only.
    glm::vec3  local_offset{0};
    glm::quat  local_rot{1, 0, 0, 0};
};

struct CharacterComponent {
    physics::CharacterHandle ctrl{};
    float capsule_half_height{0.9f};
    float capsule_radius{0.3f};
    float step_up_max{0.3f};
    float slope_max_rad{0.872665f};      // 50°
    float jump_impulse_mps{5.0f};
};

struct PhysicsConstraintComponent {
    physics::ConstraintHandle joint{};
    // Authoring lives in `constraint.hpp` (Fixed / Distance / Hinge / Slider / Cone / SixDOF).
};

struct PhysicsVelocityComponent {
    glm::vec3 linear_mps{0.0f};
    glm::vec3 angular_rps{0.0f};
};

} // namespace gw::ecs::physics
```

`RigidBodyComponent` ownership: whoever adds the component is responsible for calling `rigid_body_system::materialize(world, entity)` during the next `physics_system::tick()` — the body is cooked lazily so the world can batch-add on the first substep.

### 2. Transform sync direction

Phase-ordering rule (enforced in `runtime::Engine::tick`):

```
1. input
2. physics.step(dt)                       ← writes body state forward one fixed dt
3. rigid_body_system::write_back_transforms(world, alpha)
   - Dynamic bodies: TransformComponent ← lerp(prev_pose, curr_pose, alpha)
   - Kinematic bodies: body target ← TransformComponent (pre-step path next frame)
4. ecs::tick()                            ← gameplay systems read post-physics transforms
5. ui.update
6. audio.update
7. ui.render
```

For `dvec3` world-space positions, the body's float-precision local position is relative to a per-body "anchor" stored alongside the handle. The anchor tracks the chunk origin (Phase 20 floating-origin) but in Phase 12 every anchor is origin (`dvec3(0,0,0)`) so body positions and `TransformComponent::world_position` are numerically equal modulo `float` vs. `double` precision.

Kinematic + trigger bodies are allowed to push transforms *into* the physics world; dynamic bodies are pull-only from ECS to physics except at initial spawn.

### 3. Interpolation alpha

`PhysicsWorld::interpolation_alpha()` returns the fraction of `physics.fixed_dt` accumulated but not yet simulated. The ECS bridge lerps linear positions, slerps orientations, and lerps linear/angular velocities between `prev` and `curr` substep state. In deterministic / headless mode `alpha == 0` is enforced so render output is a pure function of frame index.

### 4. Lazy materialization + deferred destruction

`physics_system::tick(World& w, PhysicsWorld& pw)` runs in two passes:

1. **Create** — iterate entities that have `RigidBodyComponent` with `body == 0` or `ColliderComponent` with `shape == 0`; create shape first, then body; store handle on the component.
2. **Destroy** — iterate `RigidBodyComponent`s flagged `pending_destroy` (set when the entity is despawned) and issue `pw.destroy_body(...)`. Destruction is deferred one frame so late subscribers still see the final `PhysicsContactRemoved`.

### 5. Systems ordering

```
physics_system::pre_step()          (ECS → body targets for kinematic)
physics_world.step(dt)              (fixed-step integrate)
physics_system::post_step(alpha)    (body → ECS write-back)
trigger_system::drain_events()      (TriggerEntered / TriggerExited → EventBus)
character_system::drain_events()    (CharacterGrounded → EventBus)
```

All five are registered through `ecs::SystemRegistry`; ordering is explicit via `register_after(...)` dependency edges rather than a global order list. (Matches ADR-0004 §6.)

### 6. Jobs granularity

Only `physics_world.step(dt)` is permitted to fan out into the job system (via `JoltJobBridge`). The ECS bridge itself runs on the main thread and iterates archetype chunks sequentially — the cost is bounded by **transform sync ≤ 0.25 ms for 2 048 bodies** (Phase-12 perf gate §7 of the plan).

## Consequences

- Gameplay code never calls `PhysicsWorld` directly; it edits components and lets the bridge do the push/pull.
- Phase 13 animation writes bone poses into kinematic ragdoll-part bodies via the same pre-step path.
- Phase 14 networking replicates `TransformComponent`+`PhysicsVelocityComponent` and re-derives body state on the client with rewind-and-replay.
- Floating origin (Phase 20) rebases the per-body anchor at a chunk boundary; `TransformComponent::world_position` stays `dvec3` and is untouched.

## Alternatives considered

- **Storing body handles in a side-table keyed by `EntityHandle`.** Rejected — a component is the ECS-native home and preserves archetype locality.
- **Writing body poses directly into `TransformComponent` every substep.** Rejected — shreds render interpolation and makes partial substeps non-deterministic per-frame.
- **Hard-realising bodies immediately on component add.** Rejected — makes scene-load a single-threaded blocker; lazy materialization batches.

---

## ADR file: `adr/0033-character-controller.md`

## ADR 0033 — Character controller model

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 12 Wave 12B)
- **Deciders:** Cold Coffee Labs engine + gameplay group
- **Relates:** ADR-0031 (physics facade), ADR-0032 (ECS bridge), ADR-0021 (input actions)

## Context

Greywater's first characters are humanoid survivors, hostile fauna, and small bipedal drones. A full rigid-body character is overkill and brittle (`T`-pose tipping on slopes). Jolt ships `JPH::CharacterVirtual` which is the industry pattern: a capsule swept through the world, resolving penetrations analytically. The null backend mirrors the analytical semantics with a capsule-against-AABB sweep.

## Decision

### 1. Model

| Property | Default | CVar |
|---|---|---|
| Capsule radius | 0.30 m | — (per-character component) |
| Capsule half-height | 0.90 m | — |
| Step-up max | 0.30 m | `physics.char.step_up_max` |
| Slope max | 50° (0.872665 rad) | `physics.char.slope_max_deg` |
| Stick-to-ground distance | 0.05 m | `physics.char.stick_dist` |
| Gravity mode | `World` (world gravity applied) | `physics.char.gravity_mode` (`world`/`custom`/`off`) |
| Jump coyote window | 150 ms | `physics.char.coyote_ms` |
| Ground probe rays | 1 downward + 4 skirt | — |

The character is a `JPH::CharacterVirtual` under Jolt; under the null backend it's a capsule-vs-AABB sweep with analytical depenetration on up to 4 contact manifolds per substep. Both paths return the same `CharacterStepResult` struct so gameplay sees one model.

### 2. Public API (shape)

```cpp
struct CharacterDesc {
    glm::vec3 initial_position{0};
    glm::quat initial_rotation{1, 0, 0, 0};
    float     capsule_radius{0.30f};
    float     capsule_half_height{0.90f};
    float     step_up_max{0.30f};
    float     slope_max_rad{0.872665f};
    float     jump_impulse_mps{5.0f};
    ObjectLayer layer{ObjectLayer::Character};
};

struct CharacterInput {
    glm::vec3 desired_velocity_mps{0};   // horizontal + vertical
    bool      jump_requested{false};
};

struct CharacterStepResult {
    glm::vec3 position_after{0};
    glm::quat rotation_after{1, 0, 0, 0};
    glm::vec3 velocity_after{0};
    bool      is_grounded{false};
    glm::vec3 ground_normal{0, 1, 0};
    std::uint32_t ground_surface_id{0};
    bool      touched_ceiling{false};
    bool      touched_wall{false};
};

class PhysicsWorld {
    CharacterHandle create_character(const CharacterDesc&);
    void            destroy_character(CharacterHandle);
    CharacterStepResult step_character(CharacterHandle, const CharacterInput&, float dt);
    void            set_character_position(CharacterHandle, glm::vec3 pos);
};
```

### 3. Ground detection + events

After each character sub-step:

- If `is_grounded == true` and the previous state was `false` → publish `events::CharacterGrounded{ entity, ground_normal, surface_id }` on the `InFrameQueue<CharacterGrounded>` owned by `PhysicsWorld`.
- `surface_id` is copied from `RigidBodyComponent::surface_id` or zero for geometry that has no dedicated surface ID.

`character_system::drain_events(World&, EventBus&)` pumps these into the per-event-type `EventBus<CharacterGrounded>` on the main thread.

### 4. Jump + coyote window

The controller keeps a `last_grounded_at_ms` timestamp. When `CharacterInput::jump_requested` is observed and `now_ms - last_grounded_at_ms <= physics.char.coyote_ms`, the upward velocity is set to `jump_impulse_mps` regardless of the *current* grounded state. This implements coyote-time without any per-frame bookkeeping on the gameplay side.

Jump peak height (for unit test):

```
h_peak = jump_impulse_mps² / (2 * |gravity|)
       = 5.0²    / (2 * 9.81)   ≈ 1.274 m
```

Wave-12B test `character_jump_peak_height` checks this within 1 cm.

### 5. Slope + step-up tests (unit)

- `character_climbs_30deg`: ramp at 30°, character walks up → position_z_after > position_z_before, grounded.
- `character_refuses_60deg`: ramp at 60° (> slope_max), character slides, does not ascend.
- `character_steps_up_0_3m`: step-up geometry at 0.3 m, character moves over; step-up at 0.31 m, character blocked.
- `character_gravity_mode_off`: `physics.char.gravity_mode = off`, character does not fall in idle.
- `character_wall_blocks`: facing a wall, desired velocity into wall → position_after unchanged on the wall axis.
- `character_grounded_event_fires_once_per_touch`: drop character onto ground; `CharacterGrounded` count == 1 across N frames.

### 6. Input bridge

The Phase-10 `ActionSet` emits `Move`, `Jump`, `Crouch`. The `character_system::apply_input(entity, action_state)` helper converts action deltas to a `CharacterInput`:

```
desired_velocity = rotate_by(Move.xy, yaw) * speed
                 + (Crouch ? -speed*0.3 : 0) (on Z)
jump_requested = Jump.edge
```

Speed constants are CVars:

- `physics.char.walk_speed_mps` default 3.5
- `physics.char.run_speed_mps` default 6.0
- `physics.char.crouch_speed_mps` default 1.5

## Consequences

- Characters never tip over or ragdoll unexpectedly on slopes; ragdolls are explicit and constraint-driven (ADR-0034).
- Coyote-time is built-in; gameplay code does not re-implement it.
- The same controller runs under the null backend; unit tests do not require Jolt.
- Phase 13's `CharacterAnimator` consumes `CharacterStepResult::velocity_after` + `is_grounded` directly.

## Alternatives considered

- **Rigid-body character with constraints.** Rejected — unpredictable on stairs; requires per-scene tuning.
- **Per-bone ragdoll as the primary locomotion model.** Rejected — ragdoll is a death + impact response; locomotion is analytical.
- **Capsule + step-up via a second capsule sweep.** Considered; Jolt's `CharacterVirtual` already handles this internally — we rely on it.

---

## ADR file: `adr/0034-constraints-and-vehicle.md`

## ADR 0034 — Constraint catalogue + vehicle sub-system

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 12 Wave 12C)
- **Deciders:** Cold Coffee Labs engine + gameplay group
- **Relates:** ADR-0031 (facade), ADR-0032 (ECS bridge), ADR-0037 (determinism)

## Context

Jolt exposes a rich constraint catalogue (Fixed, Distance, Hinge, Slider, Cone, SixDOF, Path, Pulley, RackAndPinion, Gear, Point, Swing/Twist). The Phase-12 minimum surface is six primitives that cover 95% of gameplay needs: doors, pulleys, hinges, sliders, ragdoll joints, and ropes. The vehicle sub-system lands as a separately-gated sub-module so pure-walking games never pull vehicle cost.

## Decision

### 1. Constraint catalogue (Phase 12 minimum)

```cpp
namespace gw::physics {

struct FixedConstraintDesc {
    BodyHandle a{}, b{};
    glm::vec3  point_ws{0};              // world-space attach
    glm::quat  rotation_ws{1, 0, 0, 0};
};

struct DistanceConstraintDesc {
    BodyHandle a{}, b{};
    glm::vec3  point_a_ls{0};            // local-space attach
    glm::vec3  point_b_ls{0};
    float      rest_length_m{1.0f};
    float      stiffness_hz{0.0f};       // 0 = rigid
    float      damping_ratio{0.0f};
};

struct HingeConstraintDesc {
    BodyHandle a{}, b{};
    glm::vec3  point_ws{0};
    glm::vec3  axis_ws{0, 1, 0};
    float      limit_lo_rad{-3.1415927f};
    float      limit_hi_rad{+3.1415927f};
    float      motor_target_velocity_rps{0.0f};
    float      motor_max_torque_nm{0.0f};
};

struct SliderConstraintDesc {
    BodyHandle a{}, b{};
    glm::vec3  point_ws{0};
    glm::vec3  axis_ws{0, 0, 1};
    float      limit_lo_m{-1.0f};
    float      limit_hi_m{+1.0f};
    float      motor_target_velocity_mps{0.0f};
    float      motor_max_force_n{0.0f};
};

struct ConeConstraintDesc {
    BodyHandle a{}, b{};
    glm::vec3  point_ws{0};
    glm::vec3  twist_axis_ws{0, 0, 1};
    float      half_angle_rad{0.5f};     // ragdoll-joint swing limit
};

struct SixDOFConstraintDesc {
    BodyHandle a{}, b{};
    glm::vec3  point_ws{0};
    glm::quat  rotation_ws{1, 0, 0, 0};
    glm::vec3  linear_lo{0.0f}, linear_hi{0.0f};     // zero = locked axis
    glm::vec3  angular_lo_rad{0.0f}, angular_hi_rad{0.0f};
};

using ConstraintDesc = std::variant<
    FixedConstraintDesc,
    DistanceConstraintDesc,
    HingeConstraintDesc,
    SliderConstraintDesc,
    ConeConstraintDesc,
    SixDOFConstraintDesc>;

} // namespace gw::physics
```

The `PhysicsWorld` API is:

```cpp
ConstraintHandle add_constraint(const ConstraintDesc&, float break_impulse = 0.0f);
void             destroy_constraint(ConstraintHandle);
bool             get_constraint_broken(ConstraintHandle) const;
```

### 2. Break-force event

If `break_impulse > 0.0` and the resolved contact impulse exceeds that value during a substep, the constraint is destroyed and `events::ConstraintBroken{a, b, impulse}` is published on the `InFrameQueue<ConstraintBroken>`. The constraint handle is invalidated; subsequent `get_constraint_broken` returns `true`.

Unit test `hinge_breaks_on_impulse` asserts the event fires exactly once and the handle is invalid afterwards.

### 3. Vehicle — wheeled reference car

Gated behind `GW_PHYSICS_VEHICLE` (default **ON** under `physics-*`, **OFF** under `dev-*`). When off, the null backend provides stubs that return failure. When on, Jolt's `VehicleCollisionTester` + `WheeledVehicleController` back the API.

```cpp
struct WheelDesc {
    glm::vec3 attach_point_ls{0};
    glm::vec3 steering_axis_ls{0, 1, 0};
    glm::vec3 suspension_up_ls{0, 1, 0};
    glm::vec3 suspension_forward_ls{0, 0, 1};
    float     radius_m{0.30f};
    float     width_m{0.20f};
    float     suspension_min_m{0.0f};
    float     suspension_max_m{0.5f};
    float     suspension_stiffness_hz{1.5f};
    float     suspension_damping_ratio{0.5f};
    float     inertia_kgm2{0.9f};
    bool      drivable{true};
    bool      steerable{false};
};

struct VehicleDesc {
    BodyHandle chassis{};
    std::array<WheelDesc, 8> wheels{};
    std::uint8_t wheel_count{4};
    float max_engine_torque_nm{500.0f};
    float max_brake_torque_nm{1500.0f};
    float max_handbrake_torque_nm{4000.0f};
};

struct VehicleInput {
    float throttle{0};     // [-1, 1]
    float brake{0};        // [0, 1]
    float handbrake{0};    // [0, 1]
    float steer{0};        // [-1, 1]
};

class PhysicsWorld {
    VehicleHandle create_vehicle(const VehicleDesc&);
    void          destroy_vehicle(VehicleHandle);
    void          set_vehicle_input(VehicleHandle, const VehicleInput&);
    bool          wheel_on_ground(VehicleHandle, std::uint8_t wheel) const;
};
```

Unit test `vehicle_wheel_contact_classifier` drops a 4-wheel chassis onto a ground box → all four wheels report grounded after settle.

### 4. Soft-limit + motor events

Hinge + slider motors have `motor_target_velocity_*`. When the error exceeds `motor_max_*`, Jolt publishes a one-shot `ConstraintMotorSaturated{a, b}` on the `InFrameQueue<ConstraintBroken>` (same queue, different kind tag). Null backend: no-op.

## Consequences

- Ragdolls compose from SixDOF + Cone + Hinge primitives — Phase 13 rigs them from authoring data.
- Break-force events are the hook for Phase 22 destruction (walls snap).
- Vehicles are gated, so the `sandbox_physics` exit binary (CTest `physics_sandbox`) must guard the vehicle scene behind `GW_PHYSICS_VEHICLE`.

## Alternatives considered

- **Exposing every Jolt constraint type.** Rejected — the six-primitive set covers Phases 12–22; we add more when content needs them.
- **Vehicle via custom rigid-body suspension.** Rejected — Jolt's wheeled vehicle is the stable reference; DIY re-invents a wheel.
- **Making vehicles always-on.** Rejected — Phase 13's *Living Scene* demo has no vehicles; the gate keeps that path cheap.

---

## ADR file: `adr/0035-physics-query-api.md`

## ADR 0035 — Physics query API — rays, sweeps, overlaps, batching

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 12 Wave 12D)
- **Deciders:** Cold Coffee Labs engine + gameplay group
- **Relates:** ADR-0031, ADR-0032, ADR-0034

## Context

Gameplay relies on spatial queries: ranged attacks (raycast), projectile capsule sweeps, volumetric triggers (overlap), ground detection (short downward sweep). A single-shot API is the common case; batched queries are necessary for AI perception (hundreds of rays per tick) and particle collision (thousands).

## Decision

### 1. Single-shot API

```cpp
namespace gw::physics {

struct RaycastHit {
    BodyHandle  body{};
    glm::vec3   point_ws{0};
    glm::vec3   normal{0, 1, 0};
    float       t{0};                    // 0 = origin, 1 = origin+dir*length
    std::uint32_t surface_id{0};
};

struct ShapeQueryHit {
    BodyHandle  body{};
    glm::vec3   point_ws{0};
    glm::vec3   normal{0, 1, 0};
    float       penetration_m{0};
    std::uint32_t surface_id{0};
};

struct QueryFilter {
    LayerMask layer_mask{kLayerMaskAll};
    // Optional predicate — evaluated per candidate body; return true to accept.
    bool (*predicate)(void* user, BodyHandle) {nullptr};
    void* predicate_user{nullptr};
    // Exclude bodies with these IDs (e.g. the shooter itself).
    std::span<const BodyHandle> excluded{};
};

class PhysicsWorld {
    std::optional<RaycastHit>
    raycast_closest(glm::vec3 origin, glm::vec3 dir, float max_distance,
                    const QueryFilter& filter = {}) const;

    bool raycast_any(glm::vec3 origin, glm::vec3 dir, float max_distance,
                     const QueryFilter& filter = {}) const;

    std::size_t raycast_all(glm::vec3 origin, glm::vec3 dir, float max_distance,
                            std::span<RaycastHit> out, const QueryFilter& filter = {}) const;

    std::optional<ShapeQueryHit>
    shape_sweep_closest(ShapeHandle shape, glm::vec3 start, glm::vec3 end,
                        const QueryFilter& filter = {}) const;

    std::size_t shape_overlap(ShapeHandle shape, glm::vec3 position, glm::quat rotation,
                              std::span<BodyHandle> out, const QueryFilter& filter = {}) const;
};
```

### 2. Batched API

Batched queries submit into `engine/jobs/` through the backend-owned bridge (ADR-0031 §JoltJobBridge). The API is a simple span-in / span-out:

```cpp
struct RaycastBatch {
    std::span<const glm::vec3> origins;
    std::span<const glm::vec3> directions;
    std::span<const float>      max_distances;
    std::span<RaycastHit>       out_hits;
    std::span<std::uint8_t>     out_hit_mask;   // 1 if hit, 0 if miss
    const QueryFilter*          filter{nullptr};
};

void PhysicsWorld::raycast_batch(const RaycastBatch&);
```

Batch size is capped by `physics.query.batch_cap` (default 4096). Over-cap batches are split. The batch is synchronous on the calling thread: `raycast_batch` does not return until all chunks complete. The null backend processes the batch sequentially; the Jolt backend splits into `std::max(1, n / chunk_size)` chunks via `parallel_for`.

### 3. Determinism

All queries read a `const` snapshot of the world. They never mutate body state. Jolt's broadphase query API is `const`-safe; the null backend keeps a sorted-axis-aligned-bounding-box list built lazily on `step()`. Results are ordered by `t` ascending for the `*_all` + `*_closest` variants.

### 4. Excluded bodies + predicate

The `QueryFilter::excluded` span is scanned linearly per candidate (typical size 0–3 — the shooter + its weapon + its mount). The `predicate` hook is the escape valve for gameplay filters the layer mask can't express ("pet-safe objects" where pets are tagged via a dynamic flag). Predicate functions must be `noexcept`, stateless on the physics side, and may touch ECS reads — they run on the calling thread.

### 5. Memory + performance

- `raycast_closest` on 1 candidate ≤ 0.5 µs (null backend) / ≤ 1.5 µs (Jolt) — unit test.
- `raycast_batch` of 1024 rays against a static-heavy world ≤ 0.8 ms — Phase-12 perf gate §7 of the plan.
- `shape_overlap` against a 10 m³ volume with 64 candidates ≤ 80 µs — unit test.
- Zero heap allocations after the caller-supplied `out` span is allocated. The null backend uses a `thread_local` `std::vector` reused per call and cleared on entry.

## Consequences

- Gameplay code uses the same API in Unreal-ish `LineTraceByChannel` style.
- Phase 13 navmesh cache + BT perception queries route through `raycast_batch` so they share the job pool.
- Phase 17 VFX particle collision uses `raycast_batch` at chunk granularity (1 call per particle chunk, not per particle).

## Alternatives considered

- **Async future-returning queries.** Rejected — added bookkeeping; synchronous-parallel is sufficient for Phase 12 call sites.
- **Exposing Jolt's `NarrowPhaseQuery` directly.** Rejected — would leak Jolt types into public headers (CLAUDE.md non-negotiable #3).
- **Returning `std::vector<RaycastHit>` from `raycast_all`.** Rejected — forces an allocation per call; span-out keeps the hot path zero-alloc.

---

## ADR file: `adr/0036-physics-debug-pass.md`

## ADR 0036 — physics_debug_pass — frame-graph integration

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 12 Wave 12D)
- **Deciders:** Cold Coffee Labs engine + render group
- **Relates:** ADR-0031, ADR-0027 (ui_pass), Phase-5 frame graph, Phase-7 editor gizmos

## Context

Physics debugging without a visualization is blind. Jolt ships a `DebugRenderer` abstraction; we must adapt it to Greywater's frame-graph + render-HAL without creating a second line/triangle renderer.

## Decision

### 1. Public API

```cpp
namespace gw::physics {

enum class DebugDrawFlag : std::uint32_t {
    None         = 0,
    Bodies       = 1u << 0,     // AABBs + shape outlines
    Contacts     = 1u << 1,     // contact points + normals
    BroadPhase   = 1u << 2,     // broadphase tree cells
    Constraints  = 1u << 3,     // joint frames + limits
    Characters   = 1u << 4,     // capsule + ground normal
    ActiveIslands= 1u << 5,
    Velocities   = 1u << 6,
    Triggers     = 1u << 7,
};
using DebugDrawFlags = std::uint32_t;

struct DebugLine  { glm::vec3 a; glm::vec3 b; std::uint32_t rgba; };
struct DebugTri   { glm::vec3 a; glm::vec3 b; glm::vec3 c; std::uint32_t rgba; };

class DebugDrawSink {
public:
    virtual ~DebugDrawSink() = default;
    virtual void line(const DebugLine&) = 0;
    virtual void triangle(const DebugTri&) = 0;
    virtual void text(glm::vec3 pos_ws, std::string_view s, std::uint32_t rgba) = 0;
};

class PhysicsWorld {
    void set_debug_flags(DebugDrawFlags f);
    DebugDrawFlags debug_flags() const noexcept;
    // Emit debug geometry into the sink. Called from the render pass; safe
    // to call between substeps (reads only post-step state).
    void debug_draw(DebugDrawSink&) const;
};

} // namespace gw::physics
```

### 2. Frame-graph node

`engine/render/passes/physics_debug_pass.{hpp,cpp}` defines a `FrameGraphNode` that runs **after** `ui_pass` so physics overlays sit above HUD but below developer-console output. It:

1. Reads `PhysicsWorld::debug_flags()` — zero means the pass is skipped at record time.
2. Calls `PhysicsWorld::debug_draw(sink)` where `sink` is an adapter that writes into a transient GPU line buffer shared with the editor gizmos pipeline (Phase 7).
3. Binds the same line/triangle pipeline from `engine/render/debug_draw.{hpp,cpp}`.
4. Caps lines per frame at `physics.debug.max_lines` (default 32 768). Over-cap frames drop the tail and log a single throttled warning.

**The pass produces no Vulkan commands when flags == 0.** CLAUDE.md non-negotiable #1 (RX 580 baseline): no measurable frame-time when the developer toggle is off.

### 3. Release builds

In release builds the frame-graph node compiles to an empty class unless `dev.console.allow_release` is true. Retail players never see physics debug draw by accident.

### 4. CVar mapping

Each flag has a companion bool CVar that the settings binder can toggle:

| Flag | CVar |
|---|---|
| `Bodies`       | `physics.debug.bodies` |
| `Contacts`     | `physics.debug.contacts` |
| `BroadPhase`   | `physics.debug.broadphase` |
| `Constraints`  | `physics.debug.constraints` |
| `Characters`   | `physics.debug.character` |
| `ActiveIslands`| `physics.debug.islands` |
| `Velocities`   | `physics.debug.velocities` |
| `Triggers`     | `physics.debug.triggers` |

The master `physics.debug.enabled` CVar gates the pass end-to-end. A CVar change publishes `ConfigCVarChanged`; a small subscriber in `runtime::Engine` recomputes the flag mask and pushes it via `PhysicsWorld::set_debug_flags`.

### 5. Console

Console `physics.debug` command accepts tokens (whitespace-separated) and toggles the flags in one shot:

```
physics.debug bodies contacts character
physics.debug off
physics.debug all
physics.debug help
```

## Consequences

- One line/triangle pipeline for editor gizmos + physics debug + future VFX debug.
- Debug draw does not break the determinism contract — `debug_draw()` is `const`.
- The pass is free when off; perf gate `physics_debug_pass_off_overhead ≤ 0.05 ms`.

## Alternatives considered

- **ImGui draw lists.** Rejected — in-game surface; ImGui is editor-only (CLAUDE.md non-negotiable #12).
- **Separate pipeline per flag.** Rejected — redundant; one pipeline handles all line + triangle draws.
- **Draw commands recorded by physics on the job threads.** Rejected — the render thread owns Vulkan command recording; physics hands over geometry data, not draw calls.

---

## ADR file: `adr/0037-determinism-contract.md`

## ADR 0037 — Determinism contract + replay gate

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 12 Wave 12E)
- **Deciders:** Cold Coffee Labs engine + CI group
- **Relates:** ADR-0031, ADR-0032, ADR-0033, CLAUDE.md non-negotiables #5, #16

## Context

Determinism is the bedrock for:

1. Rewind-and-replay lag compensation (Phase 14).
2. Crash repro on `.gwreplay` files (Phase 15).
3. Procedural world determinism (Phase 19).
4. Multiplayer lockstep validation (Phase 14 *Two Client Night*).

Jolt 5.5 ships with `CROSS_PLATFORM_DETERMINISTIC ON`, but determinism is a **whole-system contract**, not a flag. This ADR captures what we own.

## Decision

### 1. `determinism_hash`

```cpp
namespace gw::physics {

struct DeterminismHash {
    std::uint64_t value{0};
};

DeterminismHash determinism_hash(const PhysicsWorld&, std::uint64_t frame_index);

} // namespace gw::physics
```

Implementation:

1. Gather all `BodyHandle`s and sort ascending by raw ID.
2. For each body, emit a canonicalized record:
   - `id` (u32)
   - `position_x/y/z` (f32, raw bit pattern, `-0.0 → 0.0`)
   - `orientation_x/y/z/w` (f32, canonical sign: if `w < 0` flip all four)
   - `linear_velocity_x/y/z` (f32, raw bits)
   - `angular_velocity_x/y/z` (f32, raw bits)
   - `motion_type` (u8)
   - `is_sleeping` (u8)
3. FNV-1a-64 over the record stream. Append `frame_index` at the end.

Budget: **≤ 80 µs for 1 024 bodies**. Enforced in `unit/physics/determinism_hash_test.cpp`.

### 2. Canonicalization rules (exhaustive)

| Field | Rule |
|---|---|
| Floats | Raw bit pattern except `-0.0 → 0.0`, NaN treated as error (asserts in debug) |
| Quaternion sign | `w ≥ 0` enforced by negating all four if `w < 0` |
| Body order | Ascending by `BodyID` |
| Sleeping bodies | Included, tagged with `is_sleeping` byte |
| Transient-only state (accumulated forces) | Excluded (reset every substep) |
| Frame index | Appended once at the end |

### 3. Replay format (`.gwreplay`)

Binary, little-endian, versioned:

```
magic   = "GWRP"
version = u16 (1)
flags   = u16 (1 = fixed-step, 2 = physics, 4 = character)
seed    = u64                 (RNG seed as of record)
fixed_dt= f32
frame_count = u32

// per-frame record, repeated frame_count times:
frame_index    : u32
wall_dt_us     : u32
input_blob_len : u16
input_blob     : bytes
hash           : u64          (determinism_hash at end-of-frame)
```

Inputs are a CRC-framed blob of `InputService::Snapshot` + `character_input` deltas. Replay format is forward-compatible: readers skip unknown `flags` bits.

### 4. `ReplayRecorder`

```cpp
class ReplayRecorder {
public:
    explicit ReplayRecorder(std::string output_path);
    void capture(std::uint32_t frame_index, std::uint32_t wall_dt_us,
                 std::span<const std::byte> input_blob,
                 DeterminismHash hash);
    ~ReplayRecorder();   // finalizes the file
};
```

Writes to a temp file and renames on success so a crash mid-record leaves no half-written capture.

### 5. `ReplayPlayer`

```cpp
class ReplayPlayer {
public:
    explicit ReplayPlayer(std::string input_path);
    std::size_t frame_count() const noexcept;
    void seek(std::uint32_t frame_index);
    const FrameRecord& current() const noexcept;
    // Advance to the next frame. Returns false at EOF.
    bool advance();
};
```

In deterministic replay mode, the engine:

1. Reads `FrameRecord::input_blob` and injects into the action system.
2. Runs a physics substep.
3. Calls `determinism_hash(world, frame_index)`.
4. Compares against `FrameRecord::hash`; on mismatch, asserts with a diff dump and a `physics.determinism.hash_miss` counter increment.

### 6. Upgrade-gate procedure

When a Jolt point-release is proposed:

1. Capture 60 s of the `sandbox_physics` scene on the current Jolt version (Windows + Linux).
2. Commit the capture under `tests/determinism/fixtures/<jolt-tag>.gwreplay`.
3. In the upgrade PR:
   - Bump `cmake/dependencies.cmake` Jolt pin.
   - Re-run the capture on **the new Jolt version**.
   - Re-play both captures cross-platform; each must replay to hash parity.
4. If hash parity is lost, attach a Jolt-upstream issue link and re-run the PR once the regression is fixed. **No mixed-Jolt PRs.**

### 7. CI jobs

| Job | Runs on | What it does |
|---|---|---|
| `physics_determinism_windows` | Win11 + clang-cl 22.1.x, ctest label `determinism` | Records a 60-frame replay of `sandbox_physics`; uploads as artifact |
| `physics_determinism_linux`   | Ubuntu 24.04 + clang 22.x, ctest label `determinism` | Downloads the Windows-recorded replay; replays it; asserts hash parity |

Both jobs are **required** before merge to `main`. Either failing fails the PR.

### 8. Hash frequency

Debug / deterministic headless mode → hash every frame. Release / retail → hash every `physics.determinism.hash_every_n` frames (default 30). A hash emit cost of 80 µs × 2 ≈ 0.16 ms per enforcement frame is budgeted.

### 9. What breaks determinism (hard errors)

- `std::chrono` drifting the fixed step.
- Any random-number generation inside `PhysicsWorld::step` that does not read from `rng.seed`.
- `std::unordered_map` / `std::unordered_set` iteration anywhere in the physics integration path.
- AVX / AVX2 / AVX-512 / FMA reordering — Jolt pin forbids.
- LTO / IPO — Jolt pin forbids.

## Consequences

- Phase 14 can implement client-side prediction + server rewind without "mostly deterministic" caveats.
- `.gwreplay` captures are safe to ship to QA as bug repros; any test box reproduces the hash.
- Jolt upgrades are a one-day procedure, not a surprise.

## Alternatives considered

- **Hash only positions.** Rejected — misses rotation + sleep-bit regressions that routinely cause multiplayer desyncs.
- **CRC-32 instead of FNV-1a-64.** Rejected — 32 bits is too narrow for a 60-frame, 1 024-body game; collision probability is non-negligible.
- **Hash in release only every 60 frames.** Considered; default 30 keeps the detection window tight enough to surface regressions within the Phase-13 `Living Scene` 60-second demo.

---

## ADR file: `adr/0038-phase12-cross-cutting.md`

## ADR 0038 — Phase 12 cross-cutting decisions (summary)

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 12 exit gate)
- **Deciders:** Cold Coffee Labs engine group
- **Summarises:** ADRs 0031–0037

This ADR is the cross-cutting summary for Phase 12. It pins dependencies, perf gates, and thread-ownership rules that span more than one wave.

## 1. Dependency pins

| Library | Pin | Gate | Default |
|---|---|---|---|
| Jolt Physics | v5.5.0 (2025-12-28) | `GW_ENABLE_JOLT` → `GW_PHYSICS_JOLT` | ON for `physics-*` / `playable-*`, OFF for `dev-*` |
| Jolt build flags | `CROSS_PLATFORM_DETERMINISTIC=ON`, `USE_SSE4_1=ON`, `USE_SSE4_2=ON`, `USE_AVX*=OFF`, `INTERPROCEDURAL_OPTIMIZATION=OFF`, `FLOATING_POINT_EXCEPTIONS_ENABLED=OFF`, `OBJECT_LAYER_BITS=16`, `USE_STD_VECTOR=OFF` | — | — |

Reason for AVX off: determinism parity across CI hosts and dev laptops. FMA reordering is an explicit determinism killer for Jolt ≥5.x.

Clean-checkout `dev-*` CI builds the **null physics backend** (stub world, all-zero hashes) so the repo ships with zero external physics deps by default.

## 2. Hot-path budgets (CI-enforced via `gw_perf_gate`)

| Scenario | Budget | ADR |
|---|---|---|
| `PhysicsWorld::step` — 500 sleeping bodies | ≤ 0.30 ms | 0031 |
| `PhysicsWorld::step` — 1 000 active free-falling boxes | ≤ 1.80 ms | 0031 |
| Character step (1 character) | ≤ 0.12 ms | 0033 |
| Character step (32-character crowd) | ≤ 2.10 ms | 0033 |
| 1 024 batched raycasts vs. static world | ≤ 0.80 ms | 0035 |
| `determinism_hash(1 024 bodies)` | ≤ 0.08 ms | 0037 |
| `physics_debug_pass` emit (1 000 bodies, all flags) | ≤ 0.60 ms | 0036 |
| Transform sync — ECS ↔ Jolt for 2 048 bodies | ≤ 0.25 ms | 0032 |
| `physics_debug_pass` overhead when flags == 0 | ≤ 0.05 ms | 0036 |

All gates ship with a ctest entry under label `phase12`. Any Jolt pin bump re-runs all nine.

## 3. Thread-ownership summary

| Thread | Owns | Calls made in Phase 12 |
|---|---|---|
| Main | `PhysicsWorld` lifetime + API; drains physics `InFrameQueue`s | `step`, `create/destroy_*`, `raycast*`, event drains |
| Jobs pool (engine/jobs) | Internal Jolt work via `JoltJobBridge`; `raycast_batch` chunks | — (all through `Scheduler::submit` / `parallel_for`) |
| Render | `physics_debug_pass` record | `debug_draw(sink)` (const, safe) |
| I/O (jobs pool) | `.kphys` decode | `kphys_loader::load` |
| Audio | Subscribes to `PhysicsContactAdded` for impact SFX | — |

No `std::thread` / `std::async` introduced. Jolt's `JobSystemThreadPool` is replaced by `JoltJobBridge`.

## 4. Thread-ownership table for Jolt internals

`JoltJobBridge` implements `JPH::JobSystem`:

| `JPH::JobSystem` method | Bridge target |
|---|---|
| `CreateBarrier` | Adapter over `gw::jobs::WaitGroup` |
| `DestroyBarrier` | deleter |
| `WaitForJobs` | `WaitGroup::wait` |
| `CreateJob` | `Scheduler::submit` at `JobPriority::Normal` |
| `QueueJob` | `Scheduler::submit` |
| `FreeJob` | ref-count-decrement (no-op post-submit) |

At runtime, `pstree` / `procexp` shows only `engine/jobs` worker threads — no Jolt thread names.

## 5. Determinism posture

- Cross-platform bit-exact across `dev-win` (clang-cl 22.1.3) and `dev-linux` (clang 22.x).
- CI gate `physics_determinism_windows` records; `physics_determinism_linux` replays.
- Hash is FNV-1a-64 over canonicalized state + frame index (ADR-0037 §1).

## 6. Cross-platform notes

- **Windows**: Jolt compiles with `/arch:SSE4.2` (implicit for x64); no `/arch:AVX`.
- **Linux**: clang `-msse4.2 -mno-avx -mno-avx2`. The compiler flags live in `engine/physics/CMakeLists.txt`, not in the Jolt fetch options, so any new compilation unit inherits them.
- **Steam Deck**: treated as Linux; SSE 4.2 is present; determinism replays parity-match.

## 7. Accessibility cross-walk

Physics touches accessibility only through debug draw. `physics.debug.enabled = false` by default; debug-draw colours honour `ui.contrast.boost` (when boost is on, outlines use the boosted palette). See `docs/a11y_phase12_selfcheck.md`.

## 8. What did not land in Phase 12 (deferred by design)

| Deferred | Target phase | Why |
|---|---|---|
| Soft-body / cloth | Phase 17 (VFX maturity) | Not gameplay-critical before then |
| Destruction / voxel fracture | Phase 22+ (*Martyrdom* / world) | Pre-reboot dailies used legacy labels; canonical milestone is *Martyrdom Online* — see `02_ROADMAP_AND_OPERATIONS.md` |
| Reverb probe baking | Phase 17 | Uses physics meshes but needs VFX maturity |
| GPU broadphase / cloth | Phase 20 (*Nine Circles Ground* / GPTM) | Scale-dependent |
| Navmesh physics-query coupling | Phase 13 | Lands with Recast |
| Vehicle audio impostor | Phase 17 | Vehicle ships; audio polish later |

## 9. Exit-gate checklist

- [x] Week rows 072–077 merged behind the `phase-12` tag
- [x] ADRs 0031–0037 land as Accepted
- [x] ADR 0038 lands as this summary
- [x] `gw_tests` green with ≥ 35 new physics cases
- [x] `ctest --preset dev-win` and `dev-linux` green
- [x] Performance gates §2 green
- [x] `docs/a11y_phase12_selfcheck.md` signed off
- [x] `docs/09_NETCODE_DETERMINISM_PERF.md (merged section **Phase 12 — Performance budgets**)` green on the CI box
- [x] `docs/09_NETCODE_DETERMINISM_PERF.md (merged section **Phase 12 — Replay protocol**)` accepted
- [x] `docs/05` marks Phase 12 *shipped*
- [x] Git tag `v0.12.0-physics` pushed

---

## ADR file: `adr/0039-engine-anim-facade.md`

## ADR 0039 — `engine/anim` facade + Ozz job pool + fixed-step clock

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 13 Wave 13A)
- **Deciders:** Cold Coffee Labs engine group
- **Scope:** weeks 078–083 (Phase 13 *Living Scene*)

## 1. Context

Phase 13 opens the *Living Scene* milestone (`docs/02_ROADMAP_AND_OPERATIONS.md` §473): a character walks a navmeshed scene driven by a Behavior Tree, animations blend deterministically, and physics collisions resolve under lockstep. This ADR pins the **animation facade contract** that every Phase 13+ subsystem (physics, BT, nav, net replication) depends on.

Binding non-negotiables (CLAUDE.md):
- #3 header quarantine — `<ozz/**>` must never appear in `engine/anim/*.hpp` outside `engine/anim/impl/`
- #5 RAII — every Ozz `Skeleton`, `Animation`, `SamplingContext` lives under `std::unique_ptr`
- #9 no STL containers on hot paths — Ozz's internal arrays are acceptable inside `impl/`
- #10 no `std::thread` / `std::async` — Ozz sample code uses `std::async`; we **do not**
- #11 OS-header quarantine — Phase 13 adds none

## 2. Decision

### 2.1 Public API — PIMPL facade

`engine/anim/public/animation_world.hpp` is the single entry point:

```cpp
class AnimationWorld {
public:
    AnimationWorld();
    ~AnimationWorld();
    AnimationWorld(const AnimationWorld&) = delete;
    AnimationWorld& operator=(const AnimationWorld&) = delete;

    [[nodiscard]] bool initialize(const AnimationConfig& cfg,
                                  events::CrossSubsystemBus* bus = nullptr,
                                  jobs::Scheduler* scheduler = nullptr);
    void shutdown();

    [[nodiscard]] SkeletonHandle   create_skeleton(const SkeletonDesc&);
    [[nodiscard]] ClipHandle       create_clip(const ClipDesc&);
    [[nodiscard]] AnimationGraphHandle create_graph(const AnimationGraphDesc&);

    void                           attach(EntityId, AnimationGraphHandle);
    void                           detach(EntityId);

    [[nodiscard]] std::uint32_t    step(float delta_time_s);
    void                           step_fixed();
    [[nodiscard]] float            interpolation_alpha() const noexcept;

    [[nodiscard]] RootMotionDelta  root_motion_delta(EntityId) const noexcept;
    [[nodiscard]] std::uint64_t    pose_hash(const PoseHashOptions& = {}) const;

    [[nodiscard]] BackendKind      backend() const noexcept;
    [[nodiscard]] std::string_view backend_name() const noexcept;
    // Buses, debug draw, CVars, reset, pause — mirrors `PhysicsWorld`.
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
```

**No `<ozz/**>` is reachable from any caller of this header.** The Impl lives in `animation_world.cpp`, which forwards to `engine/anim/impl/ozz_bridge.cpp` when `GW_ANIM_OZZ` is defined.

### 2.2 Backend gating

| Backend | Flag | Default | Determinism |
|---|---|---|---|
| Null (identity pose + canonical zero hashes) | — | always linked | trivially stable |
| Ozz 0.16.0 | `GW_ANIM_OZZ` via `GW_ENABLE_OZZ` | OFF for `dev-*`, ON for `living-*` | stable |
| ACL 2.1.0 decoder | `GW_ANIM_ACL` via `GW_ENABLE_ACL` (implies `GW_ANIM_OZZ`) | OFF / ON | stable (uniform decoder only) |

Clean-checkout `dev-*` CI builds the null backend and emits all-zero `pose_hash` values that still sort-order correctly — this keeps cross-OS determinism trivially provable on every commit.

### 2.3 Fixed-step clock

Animation samples at `anim.fixed_dt` (default 1/60 s). The accumulator is **the same clock** as physics (`physics.fixed_dt`) — they share a single fixed-step source in `runtime::Engine::tick()`. Changing either CVar re-caps determinism parity at runtime; the console command `anim.hash` snapshots the canonical pose hash for cross-OS diff.

### 2.4 Ozz job pool bridge

`engine/anim/impl/ozz_job_pool.{hpp,cpp}` is the **only** place that submits Ozz `SamplingJob` / `BlendingJob` / `LocalToModelJob` batches. It dispatches **per-character batches of size `anim.jobs.batch_size` (default 16)** through `jobs::Scheduler::parallel_for`. Zero `std::thread`, zero `std::async`, zero `std::future`.

### 2.5 Canonical pose hash

`pose_hash(opt)` = FNV-1a-64 over a sorted-by-entity-id sequence. Each entry:

```
[entity_id u64]
[clip_hash u64]
[time_quant u32]      // (playback_time_s * anim.hash.time_scale) as int
[blend_weights_quant u32 × N_active_clips]  // (weight * 1e6) as int
```

Quaternion canonicalization (`w >= 0`) applies to root-motion quats. Same trick as `engine/physics/determinism_hash.cpp`.

## 3. Consequences

- Netcode (Phase 14) serializes `(entity, clip_hash, time_quant, blend_weights_quant[])` tuples verbatim.
- Replay (`.gwreplay`) records root-motion deltas + graph state hashes; not raw poses.
- Ozz upgrade is ADR-worthy — any pin bump reruns the `living_determinism_{win,linux}` CI gate.
- Phase 17 material system consumes morph weights via the same `AnimationWorld` accessor.

## 4. Alternatives considered

- **GPU skinning inside animation step** — deferred to Phase 17; animation ships CPU poses that render skin in its own pass.
- **Ozz's own `std::async` sample path** — rejected by #10. We own the thread policy end-to-end.
- **A second VM for blend graphs** — rejected; graph is evaluated by a small interpreter tied to vscript IR (ADR-0043).

---

## ADR file: `adr/0040-acl-integration-kanim.md`

## ADR 0040 — ACL integration + `.kanim` compressed clip format

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 13 Wave 13A)
- **Deciders:** Cold Coffee Labs engine group
- **Related:** ADR-0039

## 1. Context

Ozz 0.16.0 ships with its own (lightweight) KeyframeReducer and non-uniformly-sampled track compression. It is **not** bit-identical across compiler upgrades when FMA reordering kicks in. ACL 2.1.0 (last tag 2023-12-07; `develop` tracked through 2025-11 commits) provides a deterministic, compiler-stable, uniformly-sampled decoder that is widely used in shipped titles (Unreal, Lumberyard).

Binding context:
- #3 header quarantine — `<acl/**>` lives only in `engine/anim/impl/acl_decoder.{hpp,cpp}`
- Phase-12 determinism ladder requires bit-identical clip bytes across Win + Linux
- Phase-15 persistence (ADR-0096 forward-ref) will content-hash `.kanim` blobs

## 2. Decision

### 2.1 Decoder family — **uniformly-sampled only**

ACL supports uniformly-sampled and non-uniformly-sampled track compression. We ship **only** uniformly-sampled decoders. Rationale:
- Uniform compression is bit-deterministic under clang 17+ with `-fno-fast-math`.
- Non-uniform decoding dispatches on sample density and introduces FMA-ordering drift.
- Memory savings of non-uniform over uniform are <5% for locomotion clips (Greywater's typical content).

`engine/anim/impl/acl_decoder.cpp` calls `acl::decompress_tracks` with `acl::decompression_settings_uniform` only. Non-uniform settings are `#error`'d at compile time.

### 2.2 `.kanim` on-disk layout

`.kanim` is content-addressed via xxHash3-128 (matches Phase-6 asset convention).

```
offset  size   field
0x00    4      'KANM'   magic
0x04    4      u32      version (current = 1)
0x08    8      u64      source_gltf_xxh3_lo
0x10    8      u64      source_gltf_xxh3_hi
0x18    4      u32      flags (bit0=has_root_motion, bit1=additive_layer)
0x1C    4      u32      ozz_skeleton_ref_hash (FNV-1a-32 of skeleton joint names)
0x20    4      u32      duration_us (microseconds)
0x24    4      u32      track_count
0x28    4      u32      acl_blob_bytes
0x2C    4      u32      reserved (zero)
0x30    *      bytes    ACL compressed track payload (aligned 16)
```

All multi-byte integers are little-endian. `acl::compressed_tracks` is embedded verbatim starting at `0x30`.

### 2.3 Cook pipeline

```
source.gltf
    → fastgltf::Parser           (Phase 6, already pinned)
    → ozz::animation::offline::RawAnimation (Ozz offline — tools path, not linked into engine)
    → acl::AnimationClip          (offline conversion; ozz-track → acl-track)
    → acl::compress_tracks        (uniform settings; deterministic)
    → .kanim on-disk blob
```

`tools/cook/kanim_cli` is the headless CLI; `engine/anim/assets/kanim_cooker.{hpp,cpp}` is the library. Both are guarded by `GW_ENABLE_ACL` — clean-checkout CI builds a stub cooker that writes a canonical-zero `.kanim` (all-zero track data, valid header).

### 2.4 Runtime load path

`engine/anim/assets/kanim_loader.cpp` maps the file, validates magic + version, asserts the skeleton ref-hash matches the attached skeleton, and exposes a raw span to the `acl_decoder` bridge. No heap allocations beyond Ozz's internal sampler buffers (pooled).

### 2.5 Content-hash determinism gate

CI job `kanim_cook_parity`:
1. Cook the same `.gltf` on Windows + Linux under `living-*` preset.
2. xxHash3-128 over `.kanim` bytes must match.
3. ACL pin bump reruns this gate.

## 3. Consequences

- Memory saving over raw Ozz compression: 10-15% typical locomotion; 20-25% on long-duration clips (graze loops, Phase 23).
- Decoder CPU: +5-8% vs Ozz native; acceptable for <2 ms animation-step budget.
- Upgrade procedure: ACL bump → PR with freshly-cooked corpus + `kanim_cook_parity` green across both OSes.

## 4. Alternatives considered

- **Ozz native compression only** — rejected: compiler-stability risk, Phase-12 cross-OS replay is already fragile under FMA reordering.
- **GLTF runtime format** — rejected: unacceptable memory + load-time cost for 100-character scenes.
- **Custom in-house codec** — rejected: out of scope; ACL is a known quantity.

---

## ADR file: `adr/0041-blend-trees-and-kanim-cook.md`

## ADR 0041 — Blend trees + glTF → kanim cook pipeline

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 13 Wave 13B)
- **Deciders:** Cold Coffee Labs engine group
- **Related:** ADR-0039, ADR-0040

## 1. Context

Ozz supplies `Blend2Job` / `BlendNJob` primitives. We wrap them in a graph structure so designers author locomotion + attack state machines without touching Ozz API. The editor graph (Phase 17) projects onto the same data — node editor is a view, not a second authoring path.

## 2. Decision

### 2.1 Graph vocabulary

```cpp
enum class BlendNodeKind : std::uint8_t {
    Clip,            // leaf: plays one clip at (time, weight)
    Blend1D,         // param → weights over N clips on a 1-D axis
    Blend2D,         // (x, y) → bilinear weights over 4 clips
    StateMachine,    // named states + timed transitions + conditions
    AdditiveLayer,   // base + delta clip overlay
};
```

Every node has:
- stable `NodeId` (u32, assigned at graph-build time, sorted by topological order)
- input ports (0..N child refs)
- output port (pose slot — Ozz `SoaTransform` span, sized by skeleton joint count)

### 2.2 Determinism contract

- Blend weights are **normalized then quantized to fixed-point** (`1e-6` precision) before entering `pose_hash`.
- `anim.blend.snap_threshold` (default 0.99) snaps a weight to 1.0 if its neighbours are below `1 - threshold` — prevents precision noise at the pure-animation boundary.
- State machine transitions use a fixed-point duration counter (integer microseconds) — no float time accumulation.

### 2.3 Authoring

Phase 13 v1 ships JSON-in-scene authoring:

```json
"animation_graph": {
  "skeleton": "player.skeleton",
  "root_motion": true,
  "state_machine": {
    "default_state": "Idle",
    "states": [
      { "name": "Idle", "clip": "player_idle.kanim" },
      { "name": "Walk", "clip": "player_walk.kanim" },
      { "name": "Run",  "clip": "player_run.kanim"  }
    ],
    "transitions": [
      { "from": "Idle", "to": "Walk", "duration_ms": 150, "condition": "speed > 0.1" },
      { "from": "Walk", "to": "Run",  "duration_ms": 200, "condition": "speed > 3.0" },
      { "from": "Run",  "to": "Walk", "duration_ms": 200, "condition": "speed < 3.0" },
      { "from": "Walk", "to": "Idle", "duration_ms": 200, "condition": "speed < 0.1" }
    ]
  }
}
```

The ImNodes projection (Phase 17) produces the exact same JSON — round-trip is a CI gate in Phase 17.

### 2.4 Cook pipeline — shape

```
asset.gltf ──(fastgltf)──► glTF AST
                              │
                              ├── skeleton joints ──► SkeletonDesc
                              ├── animations       ──► RawAnimation × N
                              └── meshes (skinned) ──► SkinnedMeshDesc
                                                        (Phase 6 mesh_cooker handles this part)

RawAnimation  ──(ozz offline)──► sampled keyframes
              ──(acl compress)──► compressed_tracks
              ──(kanim_cooker)──► .kanim blob
```

`kanim_cooker` is deterministic given identical source bytes + settings. The cooker settings struct is hashed into the content hash alongside the source gltf bytes.

### 2.5 Additive layer semantics

An additive layer computes `pose_base + (pose_ref_to_pose_delta)` in SoaTransform local space. Reference pose is always frame 0 of the delta clip. Weight scales the delta only. Interactions with IK are layered in the order: blend tree → additive → IK → morph → root motion strip (ADR-0042).

## 3. Consequences

- Graph evaluation cost is dominated by sampling, not blending — Ozz blend is ~0.02 ms / 50 joints / char.
- The JSON authoring path is stable enough to ship; a future binary `.kanimgraph` (Phase 17) can replace it without touching runtime.
- BLD can author graphs via `vscript.*` tools from ADR-0014 because the JSON is tokenizable by tree-sitter JSON grammar (RAG already pinned).

## 4. Alternatives considered

- **Hand-coded graph for each character** — rejected: not authorable by designers, not by BLD.
- **Binary graph format v1** — deferred; scope bloat for Phase 13.
- **State machine as a separate subsystem** — rejected: folds into the same graph; transitions are just timed blends.

---

## ADR file: `adr/0042-ik-morph-ragdoll.md`

## ADR 0042 — IK (two-bone / FABRIK / CCD) + morph targets + ragdoll bridge

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 13 Wave 13C)
- **Deciders:** Cold Coffee Labs engine group · Physics Lead co-owner for ragdoll
- **Related:** ADR-0039, ADR-0032 (physics ECS bridge), ADR-0033 (character controller)

## 1. Context

*Living Scene* (week 083) requires feet to plant on slopes, hand IK for weapon-holds, and a ragdoll fallback when the character dies. All three must run **after** physics for the frame so ground probes hit the authoritative collision world.

## 2. Decision

### 2.1 IK catalogue

Three solvers ship in Phase 13. Each runs per-character, per-frame, after blend-tree evaluation and before morph targets:

| Solver | Purpose | Source | Budget |
|---|---|---|---|
| TwoBoneIKJob | leg, arm, head-look (2-segment chain) | Ozz built-in | ≤ 0.03 ms / char |
| FABRIK | N-joint chain (up to 8 joints) | custom in `engine/anim/impl/fabrik_solver.cpp` | ≤ 0.12 ms / 5-joint char |
| CCD | fallback for non-convergent cases | custom in `engine/anim/impl/ccd_solver.cpp` | ≤ 0.25 ms / 5-joint char |

Common contract:

```cpp
struct IKChain {
    std::uint32_t joint_count{};
    std::uint16_t joint_indices[8]{};      // top → tip
    glm::vec3     target_ls{};             // local space
    glm::vec3     pole_ls{};               // optional pole hint (FABRIK)
    float         tolerance_m{0.002f};
    std::uint8_t  max_iterations{8};
    std::uint8_t  solver_kind{0};          // 0=TwoBone, 1=FABRIK, 2=CCD
    std::uint8_t  flags{0};                // bit0 = respect_joint_limits
};
```

Joint limits ship as per-joint min/max quaternion-swing bounds (two-cone), stored in `SkeletonDesc::joint_limits`. Solver honours bounds when `flags & 0x1`.

### 2.2 Determinism

- All three solvers are **fixed-iteration, deterministic**. No early-out on floating compare except via `tolerance_m`.
- Vectors are quantized to `1e-6 m` precision before entering `pose_hash`.
- CCD order iterates from tip to root (reverse index order) — stable.

### 2.3 Morph targets

Morph targets (blend shapes) are per-vertex delta positions + normals, applied in **model space after IK**:

```cpp
struct MorphTargetDesc {
    StringId         name{};
    std::uint32_t    vertex_count{};
    std::vector<glm::vec3> position_deltas;   // model-space
    std::vector<glm::vec3> normal_deltas;     // model-space (optional)
};

struct MorphTargetsRuntime {
    std::uint32_t    target_count{};
    float            weights[64]{};           // max 64 targets / mesh
};
```

Weights are driven by blend-tree leaf nodes (per-clip morph tracks, loaded from `.kanim` v2 — for Phase 13 we ship v1 without morph track compression; weights come from script events instead).

### 2.4 Ragdoll bridge

A `RagdollComponent` flag toggles a character between two authoritative sources:

```
animation_driven   → anim writes local-space pose → physics consumes character-controller frame
ragdoll_driven     → physics writes rigid-body state → anim derives model-space pose for render
```

Transition rule — **mutually exclusive per frame**, one-frame limbo:

1. Frame N: `RagdollActivated` event fires; component transitions to `limbo`.
2. Frame N: physics initializes rigid bodies from the current animation pose (hand-off).
3. Frame N+1: mode becomes `ragdoll_driven`; animation is read-only.

Recovery reverses the process through a `RagdollRecovered` event. Both states are deterministic because the hand-off pose is the output of the Phase-13 pose hash — byte-stable across OSes.

### 2.5 Interaction with physics (binding)

| Step | Owner | Notes |
|---|---|---|
| Physics `step()` | ADR-0031 | writes rigid-body state, character controller position |
| Animation `step()` | ADR-0039 | reads character-controller position (root motion consumer) |
| IK solve | this ADR | ground-probe raycast via `engine/physics/queries.hpp` — no IK geometry of its own |
| Morph apply | this ADR | in-place on extracted model-space pose |
| Render extract | Phase 5 | interpolated by `interpolation_alpha()` |

## 3. Consequences

- IK target-reach guaranteed within `tolerance_m` for TwoBone; FABRIK / CCD may miss on over-extended reach — `IKTargetMissed` event fires with residual.
- Morph weight array is capped at 64; hard-clamps at load time with a log warning.
- Ragdoll → recovery cycle costs 2 frames of latency; acceptable for the gameplay it targets.

## 4. Alternatives considered

- **Unreal-style Anim BP with IK nodes** — too much scope; our IK is 3 small solvers on top of Ozz.
- **Physics-authored IK** — rejected: IK runs in anim step, not physics step, so one-frame feedback is acceptable.
- **Morph in GPU compute** — deferred to Phase 17 VFX budget; Phase 13 keeps it CPU for determinism + memory clarity.

---

## ADR file: `adr/0043-bt-as-vscript-ir.md`

## ADR 0043 — Behavior Tree executor as a vscript IR evaluator

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 13 Wave 13D)
- **Deciders:** Gameplay Lead · Simulation Lead · BLD Lead
- **Related:** ADR-0008, ADR-0009, ADR-0014, ADR-0039

## 1. Context

**Product requirement:** BT authoring must go through two paths: (a) the editor graph (ImNodes), and (b) BLD chat via `vscript.*` tools (ADR-0014). Both must produce **bit-identical** runtime behavior. The cheapest way to guarantee that is to make the BT executor **the same VM** that runs ADR-0008/0009 vscript IR, with BT-shaped dispatch.

The alternative — a dedicated BT bytecode — is rejected by CLAUDE.md #14 ("Visual scripting: text IR is ground-truth") and would force BLD to emit two distinct serializations.

## 2. Decision

### 2.1 Node vocabulary

BT nodes map to vscript IR opcodes:

| BT kind | vscript IR form | Status model |
|---|---|---|
| Sequence | `(seq …children)` | short-circuits on first Failure |
| Selector | `(sel …children)` | short-circuits on first Success |
| Parallel (policy All) | `(par_all N …children)` | Success when N succeed |
| Parallel (policy Any) | `(par_any N …children)` | Success when N succeed |
| Inverter | `(inv child)` | flips Success ↔ Failure |
| Succeeder | `(succ child)` | always Success |
| Repeater | `(rep N child)` | repeat up to N times |
| Cooldown | `(cooldown secs child)` | Failure while on cooldown |
| Wait | `(wait secs)` | Running until elapsed |
| Action | `(action name …args)` | dispatches to registered action fn |
| Condition | `(cond name …args)` | dispatches to registered condition fn |

`.gwvs` files gain a `(tree …)` top-level form in addition to the Phase-8 `(script …)` form. One `.gwvs` can carry both a prelude script and multiple trees.

### 2.2 Blackboard

`BlackboardComponent` owns a `SmallMap<StringId, BlackboardValue>`:

```cpp
using BlackboardValue = std::variant<
    std::int32_t, float, double, bool,
    glm::vec3, glm::dvec3,
    EntityId, StringId
>;
```

- Typed keys. Writes are type-checked. Reads fall back to a typed default if the key is missing.
- Serialization order is **declaration order** (first write wins key slot), stable across runs.
- Floats hash through the pose-hash quantization (`1e-6` precision) to avoid FMA noise poisoning the BT state hash.

### 2.3 Node factory (engine/game split)

Engine ships a small set of **generic** actions/conditions; game code registers its own via:

```cpp
void register_action   (StringId name, BTActionFn  fn, void* ctx = nullptr);
void register_condition(StringId name, BTCondFn    fn, void* ctx = nullptr);
```

Engine-generic nodes (shipped in `engine/gameai`):

| Name | Kind | Purpose |
|---|---|---|
| `log` | Action | writes a line via `gw::core::Logger` |
| `emit_event` | Action | publishes an opaque event onto the cross-subsystem bus |
| `set_bb` | Action | writes a blackboard key |
| `get_bb` | Condition | reads a blackboard key and matches a value |
| `wait` | Action | built-in timed wait |
| `has_path` | Condition | PathRequest completed with Success |
| `at_target` | Condition | distance(current, target) ≤ tolerance |
| `move_to` | Action | issues a PathRequest + drives `LocomotionComponent.velocity` |

Game-side Greywater nodes (`patrol`, `flee`, `graze`, `engage`) live in `game/greywater/ecosystem/bt_nodes.cpp` (Phase 23), registered at game boot.

### 2.4 Tick budget

- `ai.bt.fixed_dt` (default 0.1 s) governs BT tick rate. BT sub-samples off the shared anim clock.
- `ai.bt.max_steps_per_tick` (default 256) caps node visits per entity per tick — guarantees bounded worst-case work under a bugged `(rep N)` with huge N.
- Batching: 16 entities per job submission into the `sim` lane.

### 2.5 Canonical BT state hash

```
[entity_id u64]
[tree_content_hash u64]              // hash of canonical-serialized IR bytes
[active_node_path_id u32]            // deterministic id for the currently-Running node
[blackboard_serialized_bytes]        // keys sorted, floats quantized
```

All four folded into FNV-1a-64, sorted by entity id. `behavior_tree::state_hash` is the accessor; it shares the FNV primitive from `engine/physics/determinism_hash.cpp`.

### 2.6 BLD-chat authoring parity

`bt_bld_parity` CI test: two BTs (one hand-written, one BLD-chat-produced via the `vscript.create_tree` tool) that express the same patrol policy produce **bit-identical** `state_hash` after 300 BT ticks on a canned blackboard. Canonicalization (attribute sort, deterministic whitespace) is enforced at save time.

## 3. Consequences

- No second VM; no second serializer; no second debugger.
- BT hot-reload rides on Phase-8 vscript hot-reload plumbing.
- Anti-pattern avoided: Unreal-style "BT Services" that tick every frame and mutate blackboard freely — our BT only ticks at `ai.bt.fixed_dt` and all mutations go through explicit nodes.

## 4. Alternatives considered

- **Dedicated BT bytecode** — rejected (see §1).
- **Utility AI instead of BT** — deferred to Phase 23 (faction AI); BT is the framework that ecosystem AI extends.
- **GOAP** — out of scope; cost-benefit does not clear the engineering bar for Phase 13.

---

## ADR file: `adr/0044-recast-detour-knav.md`

## ADR 0044 — Recast/Detour integration + `.knav` tile format

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 13 Wave 13E)
- **Deciders:** Gameplay Lead
- **Related:** ADR-0039, ADR-0032, Phase 19–23 per-planet navmesh bake (forward-ref)

## 1. Context

*Living Scene* ships a 32×32 m navmeshed patch. Phase 19+ scales to planet-scale (billions of tiles). Both regimes must share a single tile format and runtime.

Recast/Detour `v1.6.0` (2023-05-21) + `main` commit (2026-02) is the authoritative choice:
- Deterministic per-tile bake given identical input geometry + config.
- `DT_POLYREF64=ON` lifts the 2^22-tile cap (planet-scale requirement).
- PR #791 (landed 2026-02 on `main`) fixes `dtAlign` 8-byte alignment for 64-bit poly refs.

## 2. Decision

### 2.1 Tile geometry

Phase 13 v1 tile: 32 m × 32 m, `cell_size_m = 0.3 m`, `cell_height_m = 0.2 m`. Agent defaults:

| CVar | Default | Purpose |
|---|---|---|
| `nav.bake.agent_radius_m` | 0.4 | capsule radius |
| `nav.bake.agent_height_m` | 1.8 | capsule height |
| `nav.bake.agent_max_slope_deg` | 45 | walkable slope cap |

Tile ids are `(tile_x, tile_y, layer) : (i16, i16, u8)` packed into a `u64`. Planet-scale (Phase 19+) will re-pack to `(face, x, y, layer)`; the public `TileId` type is opaque.

### 2.2 Bake pipeline

```
triangle soup (glm::vec3 vertices + uint32 indices)
    → rcCreateHeightfield
    → rcRasterizeTriangles
    → rcFilterLowHangingWalkableObstacles
    → rcFilterLedgeSpans
    → rcFilterWalkableLowHeightSpans
    → rcBuildCompactHeightfield
    → rcErodeWalkableArea (agent_radius)
    → rcBuildDistanceField
    → rcBuildRegions
    → rcBuildContours
    → rcBuildPolyMesh
    → rcBuildPolyMeshDetail
    → dtCreateNavMeshData
    → .knav blob
```

Runs on `engine/jobs/` at `JobPriority::Background`. Input geometry is **snapshot-copied** before dispatch — writes to the source during bake silently invalidate the result (a new bake is queued automatically).

### 2.3 `.knav` on-disk layout

```
offset  size   field
0x00    4      'KNAV'
0x04    4      u32      version (current = 1)
0x08    8      u64      tile_id
0x10    4      u32      flags (bit0 = has_detail_mesh)
0x14    4      u32      poly_count
0x18    4      u32      vertex_count
0x1C    4      u32      detour_tile_bytes
0x20    *      bytes    Detour `dtMeshHeader`-prefixed tile payload
```

Content hash: FNV-1a-64 over `detour_tile_bytes` in tile-id order. CI gate `knav_cook_parity` compares bytes Win ↔ Linux.

### 2.4 Query surface

```cpp
struct PathRequest {
    EntityId    entity{kEntityNone};
    glm::dvec3  start_ws{};
    glm::dvec3  end_ws{};
    float       search_radius_m{10.0f};
    std::uint32_t max_polys{128};
};

struct PathResult {
    EntityId                entity{kEntityNone};
    std::vector<glm::dvec3> waypoints;   // straight-path polyline
    bool                    ok{false};
    std::uint32_t           error_code{0};
    std::uint64_t           path_hash{0};
};
```

- `find_path_closest` returns the closest reachable node when `end_ws` is unreachable.
- 64 batched queries go through one `parallel_for` job fan-out; result vector assembled under a single mutex.
- `dtNavMeshQuery` pool — one per worker thread, owned by `NavmeshWorld::Impl`.

### 2.5 Runtime tile management

`NavmeshWorld::add_tile(TileId, span<bytes>)` — loads a cooked `.knav`. `remove_tile(TileId)` unloads. `invalidate_tile(TileId)` triggers a bake on the background lane with current geometry.

Phase 23 `per_chunk_navmesh_bake` builds on top of this surface unchanged — chunk worker calls `add_tile` / `remove_tile` as chunks stream in.

### 2.6 DT_POLYREF64

`DT_POLYREF64=ON` is mandatory. Rationale: 32-bit poly refs cap total tiles at 2^22 ≈ 4 M — Phase 19 will exceed that on the first planet. The 2026 `dtAlign` fix (PR #791) makes 8-byte alignment correct; without it, tile load is unsafe on strict-alignment ARM targets.

## 3. Consequences

- Navmesh bake is a **background** operation; scenes can stream in before navmesh is available. Agents block path requests until `NavmeshBaked` fires for the enclosing tile.
- Runtime query is `O(log N)` in tile count; planet-scale is safe.
- Tile content hash is part of the `living_hash` cross-OS CI gate.

## 4. Alternatives considered

- **Unreal Navigation System** — not open source in a form we can take. Rejected.
- **Per-entity steering without navmesh** — rejected for scope of ecosystems (Phase 23).
- **Voxel-based nav (Overgrowth-style)** — memory cost is prohibitive at planet scale.

---

## ADR file: `adr/0045-living-scene-spec.md`

## ADR 0045 — *Living Scene* exit-demo scene spec + replay protocol

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 13 Wave 13F)
- **Deciders:** Simulation Lead · Gameplay Lead · CI Lead
- **Related:** ADR-0037 (physics replay), ADR-0039..0044

## 1. Context

*Living Scene* (week 083) is the Phase-13 gated demo. It must demonstrate the full stack (anim + BT + nav + physics) running deterministically under lockstep, with BT authored both via editor graph and BLD chat producing identical runtime behaviour. The demo is CI-gated and cross-OS-replayed — byte-identical `living_hash` is the gate.

## 2. Decision

### 2.1 Scene content (`apps/sandbox_living_scene/scene.gwscene`)

- 32 × 32 m rectangular patch with 1 m border walls.
- Two ramps (slopes 15°, 30°) dividing the patch into three regions.
- 4 patrol waypoints at `(8, 0, 8)`, `(24, 0, 8)`, `(24, 0, 24)`, `(8, 0, 24)`.
- 1 character (capsule: r=0.4 m, h=1.8 m) spawned at waypoint 0 with `Idle` → `Walk` locomotion graph.
- 3 clips baked: `idle.kanim`, `walk.kanim`, `run.kanim` (canonical placeholders — identity-pose animations that advance time; real content comes in Phase 20+ art pass).
- 1 BT (`patrol.gwvs`) — sequence of `move_to waypoint[i]` for i in 0..3, repeating.
- Physics: default Jolt if `GW_ENABLE_JOLT`, else null backend (still deterministic).

### 2.2 Exit-demo binary — `apps/sandbox_living_scene`

Prints, after 300 fixed-step frames:

```
[sandbox_living_scene] LIVING OK — frames=300 char_xy=<x,z> anim_hash=<hex> bt_hash=<hex> nav_hash=<hex> world_hash=<hex>
```

The `LIVING OK` token is the CTest regex gate. All four hashes appear alongside so cross-OS diffs surface the offending subsystem immediately.

### 2.3 Replay protocol (`.gwreplay` extension)

`.gwreplay` v2 (from Phase 12) gains three new frame-payload sections:

```
PhysicsFrame        — already defined (ADR-0037)
AnimationFrame      { clip_time_quant[] , blend_weights_quant[] , root_motion_delta }
BTFrame             { bt_step_seeds[] , active_node_path_id_by_entity[] , bb_writes[] }
NavFrame            { query_inputs[] , query_outputs_hash }
```

Each frame section is independently FNV-1a-64 hashed; `living_hash` combines the four with xor-chain fold.

### 2.4 Cross-OS CI gates

Two CI jobs:

- **`living_determinism_windows`** — records a `patrol.gwreplay` under the `living-win` preset. Uploads it as an artifact.
- **`living_determinism_linux`** — downloads the artifact, replays under `living-linux`, asserts byte-identical `living_hash` per frame.

Failure writes the diverging frame + section to the build log and fails the PR.

### 2.5 BLD-chat authoring parity gate

`bt_bld_parity` CTest:

1. Compile the hand-written `patrol.gwvs`.
2. Synthesize the same tree via `bld::tests::produce_patrol_bt()` (exercised through the `vscript.create_tree` tool path).
3. Run 300 BT ticks on both with the same blackboard seed.
4. Assert `state_hash_A == state_hash_B`.

This gate is **not** gated on BLD being online — it uses a recorded tool-response transcript checked into `tests/fixtures/bld/bt_patrol_transcript.json`.

### 2.6 Accessibility parity

`ui.motion.reduce` CVar (Phase 11) propagates into the demo:
- When ON, walk → idle transition collapses to immediate snap (no blend).
- Animation debug draw respects `ui.contrast.boost` for skeleton line colour — WCAG 2.2 AA palette.

## 3. Consequences

- Every Phase 13 commit must keep `sandbox_living_scene` green.
- Cross-OS CI jobs are the gating contract — PRs that diverge fail.
- Phase 14 netcode serializes the same four hashes to diagnose desyncs in flight.

## 4. Alternatives considered

- **Smaller demo (1 character, 1 clip, no BT)** — rejected; does not exercise the *Living Scene* claim.
- **Random-seed BT instead of canned patrol** — rejected; makes cross-OS replay non-trivial.
- **Separate recording/replay binaries** — rejected; one binary + `--record` / `--replay` flags.

---

## ADR file: `adr/0046-phase13-cross-cutting.md`

## ADR 0046 — Phase 13 cross-cutting decisions (summary)

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 13 exit gate)
- **Deciders:** Cold Coffee Labs engine group
- **Summarises:** ADRs 0039–0045

This ADR is the cross-cutting summary for Phase 13 — dependency pins, perf gates, thread ownership, upgrade procedure, and hand-off to Phase 14.

## 1. Dependency pins

| Library | Pin | Gate | Default |
|---|---|---|---|
| Ozz-animation | 0.16.0 (2025-01-19) | `GW_ENABLE_OZZ` → `GW_ANIM_OZZ` | OFF for `dev-*`, ON for `living-*` |
| ACL | v2.1.0 (2023-12-07) | `GW_ENABLE_ACL` → `GW_ANIM_ACL` (implies OZZ) | OFF / ON |
| Recast/Detour | v1.6.0 + `main` commit (PR #791) | `GW_ENABLE_RECAST` → `GW_GAMEAI_RECAST` | OFF / ON |
| Recast build flags | `RECASTNAVIGATION_DT_POLYREF64 ON`, demos / tests / examples OFF | — | — |
| Ozz build flags | `ozz_build_{tools,fbx,gltf,data,samples,howtos,tests} OFF` | — | — |
| ACL build flags | `ACL_IMPL_BUILD_{TESTS,TOOLS} OFF`, `ACL_IMPL_ENABLE_INSTALL OFF` | — | — |

Rationale for `ozz_build_{fbx,gltf} OFF`: Phase 6 already owns glTF import via fastgltf; FBX SDK is forbidden repo-wide.

Clean-checkout `dev-*` builds the null animation, null behavior-tree, and null navmesh backends (deterministic all-zero hashes) so no external Phase-13 dep is required for CI green.

## 2. Hot-path budgets (CI-enforced via `gw_perf_gate`)

| Scenario | Budget | ADR |
|---|---|---|
| `AnimationWorld::step` — 100 chars, 1 clip, no IK | ≤ 1.80 ms | 0039 |
| `AnimationWorld::step` — 100 chars, 3-clip SM blend + L2M | ≤ 2.50 ms | 0039 / 0041 |
| 1-char two-bone IK | ≤ 0.03 ms | 0042 |
| 1-char 5-joint FABRIK | ≤ 0.12 ms | 0042 |
| 1-char 5-joint CCD | ≤ 0.25 ms | 0042 |
| `BehaviorTreeWorld::tick` — 100 entities @ 10 Hz | ≤ 0.35 ms | 0043 |
| `BehaviorTreeWorld::tick` — 1 000 entities @ 10 Hz | ≤ 3.00 ms | 0043 |
| Navmesh tile bake (32×32 m, background) | ≤ 25.0 ms (off main) | 0044 |
| Path query 200 m | ≤ 0.25 ms | 0044 |
| 64 batched path queries | ≤ 2.00 ms | 0044 |
| `pose_hash` over 100 characters | ≤ 0.08 ms | 0039 |
| `bt_state_hash` over 100 entities | ≤ 0.05 ms | 0043 |
| `nav_content_hash` over 1 tile | ≤ 0.03 ms | 0044 |
| `living_hash` full-scene | ≤ 0.20 ms | 0045 |
| `animation_debug_pass` emit (100 skel) | ≤ 0.40 ms | 0039 |
| `navmesh_debug_pass` emit (4 tiles + 10 paths) | ≤ 0.30 ms | 0044 |

All gates ship under CTest label `phase13`. Any Ozz / ACL / Recast pin bump reruns all fifteen.

## 3. Thread-ownership summary

| Thread | Owns | Phase-13 duties |
|---|---|---|
| Main | `AnimationWorld`, `BehaviorTreeWorld`, `NavmeshWorld` lifetime + API; drains their `InFrameQueue` buses | `step`, `tick`, `drain_bake_results`, `create/destroy_*` |
| Jobs pool — `sim` lane | Ozz sampling / blending / L2M batches; BT subtree tick batches; IK solves | via `Scheduler::parallel_for` |
| Jobs pool — `query` lane | Path query batches | `NavmeshWorld::query_batch` |
| Jobs pool — `background` lane | Recast tile bakes; `.kanim` / `.knav` async load | `Scheduler::submit(Background, …)` |
| Render | `animation_debug_pass` + `navmesh_debug_pass` record; pose consumer | const-only reads |
| Audio | Subscribes to `AnimationKey` for footsteps / foley | never touches Ozz types |

Zero `std::thread` / `std::async` introduced. Ozz's sample `std::async` fanout is **not** used — we replace it with `ozz_job_pool.cpp`.

## 4. Tick ordering (enforced in `runtime::Engine::tick`)

```
physics.step()
 → anim.step()                   // consumes root-motion feedback from char controller
 → bt.tick()                     // 10 Hz sub-sampled off anim clock
 → nav.drain_bake_results()      // pulls NavmeshBaked events + corridor updates
 → ecs.transform_sync_system()
 → render.extract()
```

Hard invariant: a single BT step never crosses a fixed-step boundary. Batching is across entities, not across BT nodes of one entity.

## 5. Determinism ladder

1. Ozz runtime is deterministic under `-fno-fast-math` on Clang 17+.
2. ACL uniform decoder is deterministic — non-uniform is forbidden by ADR-0040.
3. Recast per-tile bake is deterministic given identical input + config; parallel tile bakes are independent because outputs don't cross-reference.
4. BT executor has no random branch; any `Random` node takes an explicit seed from `BlackboardComponent`.
5. `pose_hash` + `bt_state_hash` + `nav_content_hash` are canonical FNV-1a-64 of sorted entries with quantized floats.
6. `living_hash` = xor-chain fold of physics ⊕ anim ⊕ bt ⊕ nav hashes.
7. CI `living_determinism_{win,linux}` replays 30 s and asserts byte-identical hash frame-by-frame.

## 6. Upgrade-gate procedure (Ozz / ACL / Recast)

Any PR that bumps a Phase-13 dep pin **must**:

1. Re-cook the canonical test corpus (`tests/fixtures/anim/corpus.gltf`, `tests/fixtures/nav/corpus.gwscene`).
2. Run `kanim_cook_parity` + `knav_cook_parity` CI gates Win ↔ Linux — bytes must match.
3. Re-record `living_determinism_windows` artifact and replay on Linux — `living_hash` must match.
4. Replay the existing golden `.gwreplay` files from the previous pin; any divergence blocks the bump.
5. Annotate the new pin in `docs/10_APPENDIX_ADRS_AND_REFERENCES.md` §1 with the upgrade date.

No exceptions; Ozz-animation has a documented history of SoaTransform math-path drift between point releases.

## 7. Risks & retrospective (to be filled at phase exit)

| Risk | Outcome |
|---|---|
| Ozz 0.16 → 0.17 determinism drift | pin held at 0.16.0 for Phase 13; no drift observed under `-fno-fast-math` Clang 17. |
| ACL non-uniform decoder accidentally linked | compile-time `#error` added in `acl_decoder.cpp`; verified clean. |
| Recast tile bake non-determinism under parallel bake | per-tile seed from `(tile_x, tile_y, world_seed)` — serial inside tile, parallel between tiles; verified in `nav_bake_parity_test`. |
| BT hash drifts on blackboard floats | blackboard floats quantized on hash input; `bt_state_hash_float_stability_test` green. |
| IK fights physics | IK runs **after** physics in tick order; ground probe goes through `engine/physics/queries.hpp`. |
| Navmesh bake races ECS writes | bake takes a snapshot copy of triangle soup before dispatch; writes during bake silently re-queue. |
| BLD-authored BT bit-identical to hand-written | `.gwvs` canonicalized on save; `bt_bld_parity` green. |

## 8. Hand-off to Phase 14 (Networking)

Phase 14 inherits three locked contracts:

- **Anim replication:** serialize `(clip_hash, time_quant, blend_weights_quant[])` per character.
- **BT replication:** serialize `(tree_content_hash, active_node_path_id, bb_writes_delta[])`.
- **Nav corridor replication:** serialize `(path_hash, current_waypoint_index)` per agent.

Netcode ECS replication framework (ADR-0047, Phase 14 Wave 14A) builds on these.

The `living-*` preset is the base for `netcode-*` in Phase 14.

## 9. Exit-gate checklist

- [x] ADRs 0039–0046 landed with Status: Accepted.
- [x] `engine/anim/` + `engine/gameai/` module trees match ADR §4.
- [x] Headers contain **zero** `<ozz/**>`, `<acl/**>`, `<Recast*.h>`, `<Detour*.h>` in public API.
- [x] `pose_hash`, `bt_state_hash`, `nav_content_hash`, `living_hash` wired through console + CI.
- [x] 22 Phase-13 CVars registered; 11 console commands registered.
- [x] `sandbox_living_scene` prints `LIVING OK`; CTest `living_sandbox` green.
- [x] ≥ 55 new tests; CI target ≥ 383/383.
- [x] Roadmap Phase 13 → `completed` with closeout paragraph.
- [x] Risk-table retrospective filled in above §7.

---

## ADR file: `adr/0047-engine-net-facade.md`

## ADR 0047 — `engine/net/` facade + transport contract

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 14 Wave 14A)
- **Deciders:** Cold Coffee Labs engine group
- **Scope:** weeks 084–095 (Phase 14 *Two Client Night*)

## 1. Context

Phase 14 opens the *Two Client Night* milestone (`docs/02_ROADMAP_AND_OPERATIONS.md` §210): a dedicated server + two clients run at 60 Hz under a 120 ms injected RTT for ten minutes with zero desyncs and a voice round-trip ≤ 150 ms on LAN. Every Phase-14 deliverable attaches to one central PIMPL facade — `gw::net::NetworkWorld` — and every subsystem (transport, replication, prediction, voice, session) sits behind pure interfaces so the backend can be swapped without recompiling callers.

Binding non-negotiables (CLAUDE.md):

- **#3 header quarantine** — `<steam/**>`, `<opus.h>`, `<phonon.h>`, or any other third-party network header must never appear in `engine/net/*.hpp`; they are allowed only inside `engine/net/impl/*.cpp` behind compile-time gates.
- **#5 RAII** — every backend primitive (transport, codec, spatializer, session handle) lives under `std::unique_ptr` or `gw::Ref<T>`; no raw `new`/`delete`.
- **#6 RAII lifecycle** — `NetworkWorld::initialize(cfg, bus, sched)` is the single entry; a half-constructed `NetworkWorld` is structurally impossible.
- **#9 no STL containers in the replication hot path** — packet queues, delta scratch, ack rings use `engine/core/` containers + `engine/memory/` arenas.
- **#10 no `std::thread` / `std::async`** — every network job goes through `engine/jobs/Scheduler`.
- **#17 `float64` world-space positions** — transforms on the wire are quantized against the receiver's floating-origin anchor; never send a raw world `f64`.

## 2. Decision

### 2.1 Public API — PIMPL facade

`engine/net/network_world.hpp` is the single public entry. No third-party network header is reachable through it.

```cpp
class NetworkWorld {
public:
    NetworkWorld();
    ~NetworkWorld();
    NetworkWorld(const NetworkWorld&) = delete;
    NetworkWorld& operator=(const NetworkWorld&) = delete;

    [[nodiscard]] bool initialize(const NetworkConfig& cfg,
                                  events::CrossSubsystemBus* bus = nullptr,
                                  jobs::Scheduler* scheduler = nullptr);
    void shutdown();

    [[nodiscard]] bool           listen(std::uint16_t port);
    [[nodiscard]] PeerId         connect(std::string_view host, std::uint16_t port);
    void                         disconnect(PeerId peer) noexcept;

    [[nodiscard]] std::uint32_t  step(float delta_time_s);
    void                         step_fixed();

    [[nodiscard]] BackendKind    backend() const noexcept;
    // Replication, prediction, voice, session accessors — see §2.3.
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
```

### 2.2 Backend gating

| Backend | Flag | Default | Notes |
|---|---|---|---|
| **Null loopback** (in-proc ring buffer, deterministic) | — | always linked | 2c+server sandbox + every unit test; zero allocations on the hot path after warm-up |
| GameNetworkingSockets v1.4.1 | `GW_ENABLE_GNS` → `GW_NET_GNS` | OFF for `dev-*`, ON for `net-*` | Reliable + unreliable UDP, encryption, STUN-style NAT punch |
| Opus 1.5.x | `GW_ENABLE_OPUS` → `GW_NET_OPUS` | OFF for `dev-*`, ON for `net-*` | 20 ms frames, 24 kbps CBR default |
| Steam Audio spatializer | reuses Phase 10 `GW_ENABLE_PHONON` | inherits | Voice bus runs through the same listener |

Clean-checkout `dev-*` CI builds the null backend and reaches every Phase-14 code path deterministically — no third-party network dep is required for green.

### 2.3 Sub-facets

`NetworkWorld` owns (and hands back const references to) five subsystems:

| Accessor | Type | Owner |
|---|---|---|
| `transport()` | `ITransport&` | ADR-0047 |
| `replication()` | `ReplicationSystem&` | ADR-0048 / ADR-0049 |
| `prediction()` | `PredictionSystem&` | ADR-0050 / ADR-0054 |
| `voice()` | `VoiceSystem&` | ADR-0052 |
| `session()` | `ISessionProvider&` | ADR-0053 |
| `harness()` | `NetHarness&` | ADR-0051 |

All are constructed inside `initialize` and destroyed at `shutdown`; none exposes a two-phase `init`/`shutdown` below the facade.

### 2.4 Tick clock

`NetworkWorld::step_fixed()` advances one network tick at `net.tick_hz` (default 60). The accumulator is driven by the engine tick (`runtime::Engine::tick`) — the same clock that drives physics and anim (ADR-0037 §3, ADR-0039 §3). Changing `net.tick_hz` triggers a warning when physics and net clocks diverge; they are permitted to diverge only inside the rollback window (ADR-0050).

### 2.5 Thread ownership

| Thread | Owns |
|---|---|
| Main | `NetworkWorld` lifetime + public API; drains all `InFrameQueue<Net*>` buses |
| Jobs pool — `sim` lane | Replication build; delta encode/decode; voice encode/decode batches |
| Jobs pool — `net_io` lane (new, created when GNS is linked) | GNS poll; packet receive demux |
| Jobs pool — `background` lane | Session discovery file scan; replay record flush |

Zero `std::thread`, zero `std::async`. The net_io lane is a named lane inside `jobs::Scheduler`, not a private thread.

## 3. Consequences

- All Phase-14 subsystems land under one PIMPL — the pattern that already proved out for physics (0031) and anim (0039).
- Netcode ECS replication framework (ADR-0048) ties directly into this facade — no side-channel registrations.
- Upgrading GNS is ADR-worthy; any pin bump re-runs the Phase-14 desync + cross-platform replay gates (ADR-0055 §CI).
- Phase 16 session backends (Steam, EOS) land through ADR-0053 stubs already introduced here.

## 4. Alternatives considered

- **ENet direct** — rejected; no built-in encryption, lacks STUN-style NAT punch baseline, and Valve's GNS already carries that stack behind a permissive license.
- **yojimbo** — rejected; single-author LTS risk, DTLS-style crypto path we don't want to inherit.
- **A separate `engine/netcode/` split from `engine/net/`** — rejected; replication and transport are always used together, and the split doubles the header quarantine burden.
- **One mega `net` header** — rejected; split by concern (see §2.3) so hot-path translation units include only what they use.

---

## ADR file: `adr/0048-ecs-replication.md`

## ADR 0048 — ECS replication, delta compression, interest management

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 14 Wave 14C)
- **Deciders:** Cold Coffee Labs engine group
- **Scope:** Phase 14 (weeks 086+), extending ADR-0047

## 1. Context

Replication is the spine of *Two Client Night*: a dedicated server pushes a snapshot of the authoritative world to each peer every tick, deltas are computed against the peer's last acknowledged baseline, and an interest set gates what any one peer receives. The framework must deliver ≤ 32 KiB/s/client on a 2c+server sandbox and ≤ 96 KiB/s/client on a 16-peer hypothetical — both on the RX 580 baseline (CLAUDE.md #1) — while maintaining byte-identical replays cross-platform (ADR-0037).

## 2. Decision

### 2.1 Wire shape

```cpp
struct Snapshot {
    Tick                           tick{};
    std::uint32_t                  baseline_id{};    // peer's last ack, 0 = full
    std::uint64_t                  determinism_hash{}; // ADR-0037 oracle
    gw::Vector<EntityDelta>        deltas;           // core-container, arena-backed
};

struct EntityDelta {
    EntityReplicationId id{};       // (entity << 16) | revision
    std::uint32_t       field_bitmap{};  // set bit ⇒ field changed
    BitWriter           payload{};       // fields packed in declaration order
};
```

`BitWriter` lives in `engine/net/replication/bit_stream.hpp`; it writes to a bump-arena provided by the calling job and never touches the heap on the hot path.

### 2.2 Schema

Each replicated component declares fields via a macro that emits compile-time `FieldDesc` entries:

```cpp
GW_NET_REPLICATE(TransformComponent,
    GW_NET_FIELD_ANCHOR_POS(position, 24 /*bits*/, 0.01f /*m*/),
    GW_NET_FIELD_QUAT_SMALLEST3(orientation, 10 /*bits/axis*/),
    GW_NET_FIELD_FLOAT8(scale_uniform, 0.0f, 32.0f))
```

Field kinds:

| Kind | Bits | Notes |
|---|---|---|
| `ANCHOR_POS` | 24 × 3 | Quantized offset against the receiver's floating-origin anchor (CLAUDE.md #17); raw `f64` never on the wire |
| `QUAT_SMALLEST3` | 2 + 3×N | N-bit components of the three smallest quat axes |
| `FLOAT8` / `FLOAT16` | 8 / 16 | Quantized into a [min, max] range |
| `INT_VAR` | variable | Var-int 1–5 bytes, LEB-128 style |
| `BITFLAG` | 1 | Raw bit |

### 2.3 Baseline cache

Every peer gets a `BaselineCache` of the last `net.rep.baseline_ring` snapshots (default 16 = ~266 ms @ 60 Hz). Snapshots beyond the ring roll off deterministically; a client whose last ack is older than the ring is force-rebaselined (full snapshot). Baseline eviction is timer-free; it happens in the same tick that pushes a new snapshot.

### 2.4 Interest management

```cpp
struct InterestSet {
    glm::dvec3              anchor_ws{};         // peer's floating-origin anchor
    float                   radius_m{64.0f};     // per-component overrides via hint
    std::uint32_t           chunk_mask{};        // OR of relevance zones
};
```

Server builds per-peer `InterestSet` at the top of the replicate job; each entity's chunk coord (Phase-19 compatible grid) is tested against the mask, then each component's per-type override radius. Entities outside the set drop out of the snapshot cleanly — no explicit "remove" message; the peer reaps by timeout.

### 2.5 Job plumbing

`replicate_job` runs per-tick via `jobs::Scheduler::parallel_for` across peers. It reads an immutable ECS snapshot copy taken at the top of the tick so the simulation thread is free. Zero `std::thread`, zero `std::async`.

### 2.6 Budgets

| Scenario | Budget |
|---|---|
| Replication step (2c+server, 2 048 entities, 60 Hz) | ≤ 1.6 ms server main thread |
| Snapshot serialize (1 snap, 512 deltas) | ≤ 400 µs |
| Snapshot deserialize + apply | ≤ 450 µs |
| Delta bitmap scan (reflection-driven, 4 096 fields) | ≤ 120 µs |

Gates live in `docs/09_NETCODE_DETERMINISM_PERF.md (merged section **Phase 14 — Performance gates**)`.

## 3. Consequences

- Replays are purely byte-reproductions of the replicated snapshot stream: no local state is required to replay.
- Changing a field's bit budget bumps the schema content-hash; baseline rings are invalidated on mismatch.
- Any component can opt in with one macro; non-replicated components never walk the replicate_job.
- Anim, BT, and nav replication (ADR-0046 §8 hand-off) land as plain `GW_NET_REPLICATE` decls.

## 4. Alternatives considered

- **Quantize world positions as raw `f32`** — rejected; breaks determinism under floating-origin relocation.
- **Run replication off the main thread directly** — rejected; we take a snapshot copy so the main thread can publish state freely while replicate_job reads.
- **Use flatbuffers / protobuf for the snapshot** — rejected; these are fine for session metadata but the per-tick hot path demands fixed-width bit-packing.

---

## ADR file: `adr/0049-reliability-and-priority.md`

## ADR 0049 — Reliability tiers + priority scheduler

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 14 Wave 14D)
- **Deciders:** Cold Coffee Labs engine group
- **Scope:** Phase 14 (week 087), extends ADR-0047 + ADR-0048

## 1. Context

Not every replicated byte deserves the same delivery guarantee. Combat-authoritative fields must arrive, in order, or the hit rewind (ADR-0050) lies. Cosmetic fields are throw-away — an old one is worse than none. Animation state is "latest-wins." A single channel can't serve all three, and under a per-peer bandwidth ceiling the scheduler must pick *which* component gets the next byte when it can't all fit.

## 2. Decision

### 2.1 Reliability tiers (per-component enum)

| Tier | Semantics | Example components |
|---|---|---|
| **Reliable** | ACK ring + retransmit after `rtt × net.rel.rtt_retransmit_mult` (default 1.5), sequenced delivery | HealthComponent, InventoryOp, ChatMessage |
| **Unreliable** | fire-and-drop; newer sequence wins; older on arrival is dropped | TransformComponent, VelocityComponent |
| **Sequenced** | latest-wins; out-of-order arrivals discarded | AnimationStateComponent, VoiceFrame |

The tier is declared on the component via `GW_NET_RELIABILITY(kind)` alongside `GW_NET_REPLICATE(...)`. Absent declaration defaults to `Unreliable`.

### 2.2 Per-peer bandwidth ceiling

`net.send_budget_kbps` (default 96 KiB/s on 16c, enforced as ≤ 32 KiB/s on 2c+server) caps each peer's *down* stream. The scheduler uses **Deficit Round Robin (DRR)** across reliability tiers, with per-component `priority_hint` (0 = cosmetic, 255 = combat-authoritative) used as the DRR weight.

### 2.3 Starvation guard

Any component that has been deferred ≥ `net.prio.starvation_frames` (default 8) gets its priority boosted by +16 for the next tick and keeps boosting until delivered. Once delivered the hint resets. This bounds worst-case latency at `(starvation_frames × tick_hz_inv) × boost_ceiling` frames — 133 ms for 2c+server in default config.

### 2.4 Retransmit + ack windows

Reliable messages carry a 32-bit per-peer sequence and an ack bitmap in each inbound packet (standard 32-bit bitmap: bit i = ack for seq − i). Retransmit queues live per peer and are also arena-backed.

### 2.5 Channel IDs

```
ChannelId:
   0: System (connect/handshake/disconnect)
   1: Replication-Reliable
   2: Replication-Unreliable
   3: Replication-Sequenced
   4: Voice (always Unreliable-Sequenced)
   5: RPC-Reliable (engine + gameplay calls)
   6..31: reserved for game-specific tiers
```

The transport layer (ADR-0047) demuxes on channel id; the reliability layer is per-channel.

## 3. Consequences

- A malicious client can't starve combat; the hard boundary is per-peer bandwidth, not per-component.
- Game code opts into reliability by annotation, not by writing its own retransmit loop.
- Voice never blocks combat: voice is on its own channel with a lower priority tier, and voice packets drop first when the budget is tight.

## 4. Alternatives considered

- **FEC-only unreliability** — rejected for our reliable tier; FEC is voice-only (covers brief packet loss without a retransmit round trip).
- **Static per-component bandwidth caps** — rejected; DRR adapts to traffic automatically and starvation guard provides the backstop.
- **Leave priority to the gameplay layer** — rejected; networking owns latency ceilings so game code doesn't have to reimplement them per feature.

---

## ADR file: `adr/0050-lag-compensation.md`

## ADR 0050 — Server rewind + lag compensation

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 14 Wave 14E)
- **Deciders:** Cold Coffee Labs engine group
- **Scope:** Phase 14 (week 088), requires ADR-0037 + ADR-0048

## 1. Context

When a client fires at a target, the client's view is older than the server by `rtt/2 + client_interp_delay`. If the server runs the hit test at "now" it systematically misses. Valve-style lag compensation rewinds the authoritative world to the tick the shooter actually saw, runs the hit test, and restores. It requires physics that can checkpoint and replay (ADR-0037 §3).

## 2. Decision

### 2.1 API

```cpp
class ServerRewind {
public:
    void   archive_frame(Tick, const PhysicsWorld&, const TransformBuffer&);
    [[nodiscard]] RewindScope scoped_rewind(Tick target);
    [[nodiscard]] std::size_t history_frames() const;
    [[nodiscard]] std::uint64_t archive_hash(Tick) const;   // ADR-0037 oracle
};
```

`RewindScope` is RAII: the destructor snaps physics + transforms back by writing the archived state in reverse. Hit queries call:

```cpp
[[nodiscard]] RaycastHit hit_query_with_lag_comp(PeerId shooter,
                                                  const RaycastInput& ray,
                                                  Tick now,
                                                  const QueryFilter& filter);
```

which internally does `auto scope = rewind.scoped_rewind(now - peer_rtt/2)`, runs `PhysicsWorld::raycast_closest`, and returns the hit.

### 2.2 Window

`net.lagcomp.window_frames` = 12 (200 ms @ 60 Hz default). The archive is a ring buffer of snapshots — position, orientation, linear velocity, angular velocity per replicated body — not full physics state. The character controller capsule is archived separately because its collision volume is not a Jolt body in the null backend.

### 2.3 Hash restore

Every archived frame stores its ADR-0037 `determinism_hash`. After the `RewindScope` destructor restores, the hash is recomputed and compared against the archive entry; any mismatch fires `PhysicsHashMismatch` on the error bus and writes a `.rewind-mismatch.bin` diff dump. This is the oracle that proves rewind is bit-stable.

### 2.4 Serialization

Rewind never reaches for heap. The ring is a `gw::Vector<RewindFrame>` pre-sized to `window_frames × max_bodies` bytes at `NetworkWorld::initialize`.

### 2.5 Threading

All rewinds run on the server main thread inside `NetworkWorld::step_fixed()`. A rewind scope is *never* concurrent with a physics step (§4 tick order, ADR-0046 §4 pattern).

## 3. Consequences

- Hit tests are authoritative on the server, so clients only need to predict locally, not correct server truth.
- Rewind cost is O(bodies) per frame archive + O(bodies) per scoped_rewind; on a 2c+server sandbox with 2 048 bodies the archive is ≤ 1.1 ms per frame, within the budget.
- Changing `net.lagcomp.window_frames` invalidates any in-flight replay (forces rebaseline).

## 4. Alternatives considered

- **Client-authoritative hit tests** — rejected as a cheating vector.
- **Replay physics from fixed-step seed** — rejected; too expensive (re-stepping is ~10× archive).
- **Archive full physics state (contacts included)** — rejected for memory; velocities + transforms suffice because contact resolution is re-derived in the rewind.

---

## ADR file: `adr/0051-net-harness.md`

## ADR 0051 — Netcode harness: link-sim + desync detector

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 14 Wave 14F)
- **Deciders:** Cold Coffee Labs engine group
- **Scope:** Phase 14 (week 089)

## 1. Context

We cannot ship a netcode stack whose correctness we cannot reproduce. Every unit test, every integration run, every CI gate must exercise the replication + reliability + rollback stack under controllable network conditions with deterministic drop patterns. Desyncs must be detected within 200 ms of divergence and every detection must ship enough evidence to reproduce offline.

## 2. Decision

### 2.1 LinkSim — `ITransport` wrapper

```cpp
struct LinkProfile {
    float         latency_mean_ms{0.0f};
    float         latency_jitter_ms{0.0f};
    float         loss_pct{0.0f};       // 0..100
    float         duplicate_pct{0.0f};
    std::uint32_t reorder_window{0};
    std::uint64_t seed{0x9E3779B97F4A7C15ULL};
};

std::unique_ptr<ITransport> wrap_linksim(std::unique_ptr<ITransport> inner,
                                         LinkProfile profile);
```

LinkSim buffers each outbound packet with a scheduled delivery tick computed from `latency_mean_ms + rng_gauss(jitter_ms)`. Packets are dropped via seeded Bernoulli on `loss_pct`, duplicated via Bernoulli on `duplicate_pct`, and reordered within `reorder_window` via a priority queue keyed on delivery tick.

Deterministic property: same `LinkProfile::seed` + same send sequence + same tick clock = same drop/delay pattern across OSes.

### 2.2 Profile matrix

Every replication/reliability/rollback test runs under at least four profiles:

| Name | Latency / Jitter | Loss | Reorder |
|---|---|---|---|
| `clean` | 0 / 0 | 0 | 0 |
| `lan` | 60 / 5 | 0 | 0 |
| `wan` | 120 / 20 | 2 | 0 |
| `hostile` | 250 / 50 | 5 | 3 |

The sandbox exit-gate (`two_client_night_ctest`) uses `{120 ms, 2 %}` which is the *Two Client Night* contract.

### 2.3 Desync detector

```cpp
class DesyncDetector {
public:
    void push_hash(PeerId, Tick, std::uint64_t determinism_hash);
    [[nodiscard]] bool divergence_detected() const noexcept;
    void dump_last_window(std::filesystem::path) const;
    [[nodiscard]] std::uint32_t window_ticks() const noexcept;
};
```

The detector keeps a ring of the last `net.desync.window_ticks` (default 200 = ~3.3 s @ 60 Hz) hashes per peer + the server's own hash. On every push, peer hashes are compared against the server's hash for the same tick; the first mismatch sets `divergence_detected()` and (when `net.desync.auto_dump = true`) writes a `.desync.bin` diff dump containing:

- per-peer ring of `(tick, hash)` entries
- last 200 inbound `EntityDelta` payloads
- LinkProfile parameters
- `.gwreplay` v3 file reference (ADR-0055 §Replay)

The format matches ADR-0037's side channel so an engineer can replay the recorded trace offline.

### 2.4 Console commands

| Command | Effect |
|---|---|
| `net.linksim <rtt_ms> <loss_pct>` | Install LinkSim on the current transport |
| `net.linksim off` | Remove LinkSim |
| `net.desync.dump` | Force a diff dump even without divergence |

## 3. Consequences

- Cross-platform reproducibility is testable without third-party traffic-shaping tools.
- A failing CI run ships an artifact that an engineer can replay on their workstation under the same seed.
- The harness is cheap enough to leave on in development — `net.linksim off` is the default.

## 4. Alternatives considered

- **OS-level traffic control (tc, netem)** — rejected as CI-non-portable and non-deterministic.
- **Stochastic drops without seed** — rejected; irreproducibility kills the feedback loop.
- **Write desync dumps only on divergence** — rejected; `net.desync.dump` is useful for manual inspection of "looks fine but I'm suspicious" states.

---

## ADR file: `adr/0052-voice-pipeline.md`

## ADR 0052 — Voice pipeline: capture → Opus → transport → Steam Audio

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 14 Waves 14H + 14I)
- **Deciders:** Cold Coffee Labs engine group
- **Scope:** Phase 14 (weeks 091–092)

## 1. Context

Voice chat in *Two Client Night*: ≤ 150 ms round-trip on LAN, audibly correct spatialization, per-pair mute/block, opt-in push-to-talk default, FEC tolerant of 2 % packet loss. Opus at 20 ms frames, 48 kHz mono, 24 kbps CBR covers speech for RX 580 CPU headroom. Steam Audio already wires Phase 10's HRTF + atmosphere filter — voice reuses the same listener.

## 2. Decision

### 2.1 Pipeline

```
IVoiceCapture (mic @ 48 kHz mono, 20 ms frames)
  → VoiceCodec::encode (Opus, 24 kbps CBR, complexity 5, FEC on)
    → [seq:u16][len:u16][payload:N]
      → transport channel 4 (Unreliable-Sequenced, priority = Voice tier)
        → VoiceCodec::decode
          → VoiceSpatializer (Steam Audio HRTF + atmosphere filter)
            → audio mixer "voice" bus
              → output device
```

### 2.2 Public API

```cpp
class VoiceSystem {
public:
    [[nodiscard]] bool enable_push_to_talk() const noexcept;
    void set_push_to_talk(bool on) noexcept;
    void hold_transmit(bool hold) noexcept;

    void mute_peer(PeerId, bool muted) noexcept;
    [[nodiscard]] bool peer_muted(PeerId) const noexcept;

    [[nodiscard]] VoiceStats stats() const noexcept;
};
```

`VoiceStats` exposes `{encode_time_us, decode_time_us, packets_sent, packets_lost, fec_recovered}` for debug + perf gates.

### 2.3 Frame layout (wire)

| Field | Bits | Notes |
|---|---|---|
| seq | 16 | Rolls over; out-of-order arrival discarded |
| len | 16 | Opus payload length (bytes) |
| payload | 8 × len | Opus-encoded data |

Frame size at 24 kbps / 20 ms = 60 bytes + 4 header = 64 bytes. Three simultaneous speakers on a 2c+server sandbox = 9.6 KiB/s/client + 6-byte per-packet header × 150 pps ≈ 11.5 KiB/s — fits easily under `net.send_budget_kbps`.

### 2.4 Null backend

Without `GW_ENABLE_OPUS` the codec is a **null passthrough** that stores the raw 20 ms PCM frame and returns it on decode (exact round-trip for tests). The null capture is a seeded sine-wave source so round-trip SNR is testable against a reference.

### 2.5 Spatialization

`VoiceSpatializer` takes the decoded PCM + the speaker's world position (replicated via `TransformComponent`) and submits to `engine/audio`'s `IAudioService::voice_bus()`. When `GW_ENABLE_PHONON` is off, the spatializer falls back to constant-power panning by source azimuth — enough to keep the `voice_spatial_directionality_test` within 6 dB.

### 2.6 Budgets

| Stage | Budget on RX 580 CPU |
|---|---|
| Capture pull | ≤ 0.3 ms |
| Opus encode 20 ms CBR 24 kbps c5 | ≤ 1.1 ms |
| Transport + network | 10–60 ms |
| Opus decode | ≤ 0.4 ms |
| HRTF spatialize | ≤ 1.0 ms |
| Output device latency | ≤ 20 ms |
| **Total round-trip (LAN)** | **≤ 150 ms** |

## 3. Consequences

- Voice never competes with combat-authoritative replication for a reliable slot; it's Unreliable-Sequenced (ADR-0049) on its own channel.
- Per-peer mute/block is enforced at the mixer — muted peers never decode, saving CPU.
- STT caption plumbing (ADR-0055 §a11y) has a hook at the decoder so transcribers can subscribe without changes to the hot path.

## 4. Alternatives considered

- **libsamplerate in the path** — rejected; capture is already 48 kHz mono.
- **Multi-channel voice (stereo)** — rejected; doubles bandwidth, HRTF spatialization is mono-source by contract.
- **Speex** — rejected; Opus is the RFC-standard descendant and ships better quality per bit at speech band.

---

## ADR file: `adr/0053-session-provider.md`

## ADR 0053 — `ISessionProvider` + dev-local backend + Steam/EOS stubs

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 14 Waves 14J + 14K)
- **Deciders:** Cold Coffee Labs engine group
- **Scope:** Phase 14 (weeks 093–094), with backend fills deferred to Phase 16

## 1. Context

*Two Client Night* needs a discovery + rendezvous layer: a host advertises a session, clients find it and join. Phase 14 ships a dev-local backend (file-discovery) so the sandbox is self-sufficient; Phase 16 plugs Steam and EOS behind the same interface. The contract must be backend-stable from day one — Steam and EOS headers never leak through `session_provider.hpp`.

## 2. Decision

### 2.1 Public API

```cpp
enum class SessionError : std::uint8_t {
    Ok = 0,
    AlreadyHosting,
    NotFound,
    BackendDisabled,
    Forbidden,
    TransportError,
    InvalidConfig,
};

struct SessionDesc {
    std::string                  name{};           // human-facing
    std::uint32_t                max_peers{8};
    std::uint32_t                port{27015};
    std::uint64_t                seed{0};          // shared RNG seed; 0 = generate
    gw::Vector<std::string>      tags{};           // filters
};

struct SessionSummary {
    std::string       id{};
    std::string       name{};
    std::string       host{};       // IP literal or opaque backend token
    std::uint16_t     port{0};
    std::uint32_t     peer_count{0};
    std::uint32_t     max_peers{0};
    std::uint64_t     seed{0};
};

struct SessionHandle {
    std::uint64_t value{0};
    [[nodiscard]] constexpr bool valid() const noexcept { return value != 0; }
};

class ISessionProvider {
public:
    virtual ~ISessionProvider() = default;
    virtual std::expected<SessionHandle, SessionError> create(const SessionDesc&) = 0;
    virtual std::expected<void,         SessionError> join  (const SessionSummary&, SessionHandle& out) = 0;
    virtual std::vector<SessionSummary>                list  () = 0;
    virtual void                                       leave(SessionHandle) = 0;
    virtual std::string_view                           backend_name() const = 0;
};

std::unique_ptr<ISessionProvider> make_devlocal_session_provider(std::filesystem::path root);
std::unique_ptr<ISessionProvider> make_steam_session_provider_stub();   // always BackendDisabled
std::unique_ptr<ISessionProvider> make_eos_session_provider_stub();     // always BackendDisabled
```

The Steam / EOS stubs exist purely to prove the interface is backend-stable; they return `SessionError::BackendDisabled` on every operation and will be filled in Phase 16 behind `GW_ENABLE_STEAMWORKS` / `GW_ENABLE_EOS`.

### 2.2 dev-local backend

Writes `session_<id>.toml` under `root` (default `%APPDATA%/greywater/sessions/` on Windows, `$XDG_DATA_HOME/greywater/sessions/` on Linux) with shape:

```toml
id      = "a3f29-cafe-babe"
name    = "Two-Client Night"
host    = "127.0.0.1"
port    = 27015
seed    = "0x9E3779B97F4A7C15"
max_peers = 8
peer_count = 1
tags    = ["dev", "sandbox"]
created_at = "2026-04-21T12:00:00Z"
```

`list()` scans the directory; stale files (older than 60 s without a heartbeat) are filtered. Clients pick up `host:port` and `seed` directly.

### 2.3 Shared seed

The `seed` in the session descriptor becomes the shared RNG seed for the session: `physics.rng`, `gameai.blackboard.rng`, every deterministic subsystem folds it in at `NetworkWorld::initialize`. Same seed + same inputs = identical simulation on every peer from tick 0 — the precondition for ADR-0054 lockstep.

### 2.4 Security (dev-local only)

dev-local has **no authentication**. It is developer-mode only and the backend name emits `"devlocal (INSECURE)"` via `backend_name()`; the runtime refuses to select dev-local when `GW_RELEASE_BUILD` is set.

## 3. Consequences

- Phase 14 closes *Two Client Night* without Steam or EOS; Phase 16 swaps in the real backends without a single caller-site change.
- Seed-to-session binding means lockstep peers don't need an extra "sync" message — they agree from frame 0.
- Stale session files are garbage-collected on list(), so dev-local can't accumulate leaks across unclean exits.

## 4. Alternatives considered

- **Join-code pools (like GGPO)** — rejected for dev-local; the file discovery matches the developer workflow (launch two sandboxes in one terminal, both see the session).
- **Separate discovery and rendezvous services** — rejected; dev-local is a discovery + rendezvous singleton.
- **Require Steam/EOS from day one** — rejected; we need CI green on clean-checkout without SDK mounts.

---

## ADR file: `adr/0054-lockstep.md`

## ADR 0054 — Deterministic lockstep mode (GGPO-shaped)

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 14 Wave 14K)
- **Deciders:** Cold Coffee Labs engine group
- **Scope:** Phase 14 (week 094), requires ADR-0037 + ADR-0048

## 1. Context

Client-server replication is the default path; lockstep is the second. Lockstep peers all run the same simulation off the same inputs and the wire carries only input packets — ideal for competitive 2–4 player matches where every byte matters and the simulation is already deterministic. GGPO's model (frame delay + rollback + SyncTest) is the proven shape.

## 2. Decision

### 2.1 API — `LockstepSession`

```cpp
class LockstepSession {
public:
    explicit LockstepSession(LockstepConfig cfg);

    void add_local_input(PeerId local, std::span<const std::byte> input);
    [[nodiscard]] InputSet synchronize_input();
    void advance_frame();

    // State save/load — bound to ADR-0037 determinism-hash region.
    void save_state(gw::Vector<std::byte>& out);
    void load_state(std::span<const std::byte> in);

    [[nodiscard]] SyncTestReport run_sync_test(std::uint32_t frames);

    [[nodiscard]] std::uint64_t determinism_hash(Tick) const noexcept;
};

struct LockstepConfig {
    std::uint32_t frame_delay{1};        // default GGPO frame delay
    std::uint32_t rollback_window{8};    // 133 ms @ 60 Hz
    std::uint32_t peer_count{2};
};
```

### 2.2 Frame-delay + rollback

Every local input arrives `frame_delay` ticks later on the wire; if a remote input is missing at `synchronize_input()` time, the session predicts from the last received (same-as-GGPO). When the real input arrives, the session rolls back to the prediction tick, rewrites the input, and re-advances. `save_state` is called by the rollback harness; `load_state` is called to restore.

### 2.3 SyncTest

`run_sync_test(N)` is the GGPO pattern: simulate N frames, roll back 1 frame, resimulate, assert `determinism_hash` at frame N is byte-identical pre- and post-rollback. It's CI-mandatory on Debug and runs on `two_client_night_ctest` setup.

### 2.4 Rollback-pure enforcement

Any code reached from a `save_state` ↔ `load_state` region — including physics, anim, BT — must be **pure of the wall clock**: no `std::chrono`, no `std::random_device`, no telemetry, no file I/O, no RNG that is not seeded from the frame seed. The enforcement is:

- a namespace macro `GW_ROLLBACK_PURE` annotates rollback-reachable code;
- a clang-tidy check rejects `std::chrono::*`, `std::random_device`, `std::this_thread::*`, `std::printf`, `std::fopen` under `GW_ROLLBACK_PURE`;
- the rollback test (`lockstep_rollback_purity_test`) runs SyncTest with `net.debug.rollback_paranoia=1`, which enables compile-time checks + a runtime tripwire for wall-clock reads.

This is the structural shield for K5 (determinism purity, `docs/03_PHILOSOPHY_AND_ENGINEERING.md`).

## 3. Consequences

- 2–4 peer matches run at the frame-per-packet efficiency lockstep implies.
- Rollback determinism is proven every commit (SyncTest runs in Debug every build).
- Physics, anim, BT were already designed under ADR-0037 to satisfy this contract; Phase 14 simply audits them with the clang-tidy check.

## 4. Alternatives considered

- **Client-authoritative predict-and-reconcile only** — rejected; lockstep is the preferred path for small competitive matches because it never needs a snap-back.
- **GGPO library** — rejected; the library's thread model uses `std::thread`, violating CLAUDE.md #10. We reimplement its shape around `jobs::Scheduler`.
- **Longer rollback window (16 frames)** — rejected; 133 ms is enough for LAN + 100 ms WAN; doubling the window doubles worst-case CPU.

---

## ADR file: `adr/0055-phase14-cross-cutting.md`

## ADR 0055 — Phase 14 cross-cutting decisions (summary)

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 14 exit gate)
- **Deciders:** Cold Coffee Labs engine group
- **Summarises:** ADRs 0047–0054

This ADR is the cross-cutting summary for Phase 14 — dependency pins, perf gates, thread ownership, CI layout, replay format, a11y, and hand-off to Phase 15.

## 1. Dependency pins

| Library | Pin | Gate | Default |
|---|---|---|---|
| GameNetworkingSockets | v1.4.1 (2024-03-25) | `GW_ENABLE_GNS` → `GW_NET_GNS` | OFF for `dev-*`, ON for `net-*` |
| Opus | v1.5.2 (2024-05-15) | `GW_ENABLE_OPUS` → `GW_NET_OPUS` | OFF / ON |
| Steam Audio | reuses Phase 10 `GW_ENABLE_PHONON` | inherits | OFF / ON |

GNS transitively pulls OpenSSL + Protobuf; these are header-quarantined inside `engine/net/impl/` and never re-exported through public headers.

Clean-checkout `dev-*` builds the null transport + null Opus passthrough + constant-power panning (no Phonon); every Phase-14 code path is still reachable and every unit test is deterministic.

## 2. Hot-path budgets (CI-enforced via `gw_perf_gate`)

| Scenario | Budget | ADR |
|---|---|---|
| Replication step (2c+server, 2 048 entities, 60 Hz) | ≤ 1.6 ms server main thread | 0048 |
| Snapshot serialize (1 snap, 512 deltas) | ≤ 400 µs | 0048 |
| Snapshot deserialize + apply | ≤ 450 µs | 0048 |
| Delta bitmap scan (4 096 fields) | ≤ 120 µs | 0048 |
| Priority DRR schedule (2 048 comps, 32 KiB budget) | ≤ 80 µs | 0049 |
| Lag-comp archive (per frame) | ≤ 1.1 ms | 0050 |
| Lag-comp rewind (12-frame scope) | ≤ 2.2 ms | 0050 |
| Opus encode 20 ms CBR 24 kbps c5 | ≤ 1.1 ms | 0052 |
| Opus decode 20 ms | ≤ 0.4 ms | 0052 |
| Voice spatializer (HRTF, 48 kHz mono → stereo) | ≤ 1.0 ms | 0052 |
| `determinism_hash` (1 024 bodies) | ≤ 80 µs | ADR-0037 reused |
| Bandwidth down per peer @ 2c+server | ≤ 32 KiB/s | 0048 |
| Bandwidth down per peer @ 16c | ≤ 96 KiB/s | 0048 |
| Voice round-trip LAN | ≤ 150 ms | 0052 |
| LockstepSession SyncTest (1 rollback, 60 ticks) | ≤ 4 ms | 0054 |

All gates ship under CTest label `phase14`. Any GNS / Opus pin bump reruns every gate.

## 3. Thread-ownership summary

| Thread | Owns |
|---|---|
| Main | `NetworkWorld` lifetime + public API; drains `InFrameQueue<Net*>` buses |
| Jobs — `sim` lane | Replicate job, delta encode/decode, voice encode/decode batches |
| Jobs — `net_io` lane (linked with GNS) | GNS poll + packet demux |
| Jobs — `background` lane | Session dev-local file scan, `.gwreplay` flush |

Zero `std::thread` / `std::async`. The `net_io` lane is a named lane inside `jobs::Scheduler`, not a private thread.

## 4. Tick ordering (enforced in `runtime::Engine::tick`)

```
physics.step()
 → anim.step()
 → bt.tick()
 → nav.drain_bake_results()
 → net.step()                      // replicate_job, voice encode, linksim deliver
 → ecs.transform_sync_system()
 → render.extract()
```

Lag compensation's `scoped_rewind` is the only place where physics state briefly diverges from this pipeline; it always restores within the same tick.

## 5. `.gwreplay` v3 — `NETW` section

Extends `.gwreplay` v2 (ADR-0037) with a per-peer input + ack stream, making a full multiplayer session deterministically replayable.

```
v3 header adds:
    netw_version : u16    // = 3
    peer_count   : u32
per peer:
    peer_id      : u32
    seed         : u64
    inputs       : framed [ (tick u32, blob_len u16, blob bytes) ... ]
    acks         : framed [ (tick u32, baseline_id u32) ... ]
    hash_stream  : [ (tick u32, hash u64) ... ]
```

The server records both its local input stream (ADR-0037) and each connected peer's inputs + ack stream. `ReplayPlayer` re-drives the server against the null transport, replaying each peer's input at the recorded tick. Success criterion: per-frame `determinism_hash` identical to the recorded.

## 6. CI pipeline (additions)

```
.github/workflows/ci.yml:
  phase14-windows:
    needs: phase13-windows
    - cmake --preset dev-win && ctest --preset dev-win -L "phase14|smoke"
  phase14-linux:
    needs: phase13-linux
    - cmake --preset dev-linux && ctest --preset dev-linux -L "phase14|smoke"
  phase14-net-smoke-windows:
    needs: phase14-windows
    if: gns_enabled
    - cmake --preset net-win && ctest --preset net-win -L gns
  phase14-net-smoke-linux:
    needs: phase14-linux
    if: gns_enabled
    - cmake --preset net-linux && ctest --preset net-linux -L gns
  phase14-replay-cross-platform:
    needs: [phase14-windows, phase14-linux]
    - download .gwreplay artefact from Windows job
    - ./gw_tests --test-case="net replay cross-platform parity"
  phase14-two-client-night:
    needs: phase14-windows
    - ./sandbox_netplay --role=linkroom --rtt_ms=120 --loss_pct=2 --ticks=10000
    - grep -q "NETPLAY OK" output
    - grep -q "desyncs=0" output
```

`phase14-replay-cross-platform` and `phase14-two-client-night` are required before merge on any PR touching `engine/net/`, `engine/physics/`, or `engine/anim/`.

## 7. Accessibility self-check (`docs/a11y_phase14_selfcheck.md`)

- Voice chat defaults to opt-in push-to-talk; voice-activity mode is opt-in; mute-all-remote is one-click; per-peer mute list persists via `engine/core/config`.
- STT caption hook (`IVoiceTranscriber`, no-op default) — Phase-16 ships a real backend that feeds the subtitle system.
- Color-blind-safe debug palettes for `net.debug` overlays share the 8-entry palette used by `physics.debug` and `nav.debug`.
- Keyboard-first authoring — join-by-session-code UI is fully tab-reachable.
- Packet-loss icon pulse is capped at 2 Hz and respects `ui.reduce_motion`.

## 8. Determinism ladder

1. ADR-0037 `determinism_hash` is the oracle: every peer emits one per tick, every tick is compared.
2. The replication bit-packer is fixed-width and deterministic — no allocator-dependent ordering.
3. LinkSim uses a seeded RNG keyed on `LinkProfile::seed` + `tick` — same seed + same send sequence = same drops.
4. Lockstep SyncTest (`run_sync_test`) is CI-mandatory; any rollback-pure violation fails the gate.
5. `.gwreplay` v3 artifacts from Windows replay byte-identically on Linux.

## 9. Upgrade-gate procedure (GNS / Opus / Steam Audio)

Any PR that bumps a Phase-14 dep pin **must**:

1. Re-run `phase14-replay-cross-platform` against the existing golden `.gwreplay`s — bytes must match.
2. Re-run `phase14-two-client-night` at `{120 ms, 2 %}` — zero desyncs.
3. Re-run `phase14-net-smoke-*` on both OSes.
4. Annotate the new pin in `docs/10_APPENDIX_ADRS_AND_REFERENCES.md` §1 with the upgrade date.

## 10. Risks & retrospective

| Risk | Mitigation | Phase-14 outcome |
|---|---|---|
| GNS upstream changes break header quarantine | pin v1.4.1; wrap behind `ITransport`; `impl/` gate | null-backend CI green on clean checkout |
| Cross-platform FP non-determinism in quantizers | bit-quantize everything; never send `f64` | `phase14-replay-cross-platform` passes on null |
| Opus license clash | BSD-3, CI-safe | confirmed matches Jolt / ACL / Recast license surface |
| Steam Audio HRTF cost on RX 580 | Opus complexity 5; voice bus can fall back to pan | fallback exercised by `voice_spatial_fallback_test` |
| NAT traversal failure | GNS STUN first, session backend relay (Phase 16) | dev-local never needs NAT |
| Desync inside rollback window | SyncTest CI-mandatory; `GW_ROLLBACK_PURE` clang-tidy rule | `lockstep_two_client_determinism_test` green |
| Voice + replication bandwidth collision | per-peer DRR scheduler (ADR-0049); voice priority below combat | `priority_starvation_test` green |

## 11. Hand-off to Phase 15 (Persistence & Telemetry)

Phase 15 inherits two locked contracts:

- **`.gwreplay` v3** — Phase 15 telemetry events extend (not break) the v3 stream; a new `TELE` section is appended.
- **Per-peer session seed** (ADR-0053 §2.3) — Phase 15 save file includes the session seed; reloading a save into a new session rebinds the RNGs consistently.

The `net-*` preset is the base for `persist-*` in Phase 15.

## 12. Exit-gate checklist

- [x] ADRs 0047–0055 landed with Status: Accepted.
- [x] `engine/net/` module tree matches ADR §2 / §4.
- [x] Headers contain zero `<steam/**>`, `<opus.h>`, `<phonon.h>` in public API.
- [x] `determinism_hash` comparator wired through `DesyncDetector` + CI.
- [x] 26 Phase-14 CVars registered; 10 console commands registered.
- [x] `sandbox_netplay` prints `NETPLAY OK`; CTest `two_client_night_ctest` green.
- [x] ≥ 30 new Phase-14 tests; total CTest count ≥ 377 on `dev-win`.
- [x] Roadmap Phase 14 → `completed` with closeout paragraph.
- [x] Risk-table retrospective filled in §10.

---

## ADR file: `adr/0056-engine-persist-facade.md`

## ADR 0056 — Engine persist facade and `.gwsave` v1

- **Status:** Accepted
- **Date:** 2026-04-21
- **Deciders:** Cold Coffee Labs engine group

## Context

Phase 15 introduces durable gameplay state with chunk-scoped streaming, integrity guarantees, and a PIMPL facade mirroring `NetworkWorld` / `PhysicsWorld`.

## Decision

1. **`PersistWorld`** in `engine/persist/persist_world.hpp` is the only header callers include for persistence orchestration. Implementation owns container IO, optional `ILocalStore`, and optional `ICloudSave`.
2. **`.gwsave` v1** is a little-endian binary container: magic `GWSV`, versioned header (≤16 KiB readable without decompression), chunk TOC, compressed chunk bodies (zstd when `GW_ENABLE_ZSTD`), footer `FTR!` + BLAKE3-256 over all bytes preceding the digest.
3. **Header quarantine:** No `<sqlite3.h>`, `<zstd.h>`, `<blake3.h>` in any `engine/persist/*.hpp` public header. Vendor includes appear only in `engine/persist/impl/*.cpp` (and `integrity.cpp` for BLAKE3).
4. **Null / local FS backend** is the default for `dev-*` presets: atomic `tmp + rename` writes under an expandable `persist.save.dir`.
5. **`determinism_hash`** in the header fingerprints the ECS snapshot payload (canonical serialization bytes); it is advisory for debugging. **Integrity** is enforced only by the BLAKE3 footer.

## Consequences

- Save tooling and CI gates key off the frozen v1 layout in ADR-0064.
- Phase 19 `ChunkStreamingBridge` consumes chunk TOC entries without loading distant chunks.

## References

- ADR-0006 (ECS serialization), ADR-0037 (determinism), ADR-0053 (session seed), ADR-0055 §11 (hand-off).

---

## ADR file: `adr/0057-ecs-snapshot-and-migration.md`

## ADR 0057 — ECS snapshot wire format and save migration

- **Status:** Accepted
- **Date:** 2026-04-21

## Context

ECS state must round-trip across OSes and engine versions with an append-only migration chain.

## Decision

1. **Wire format** reuses ADR-0006 v3 primitives: `save_world(..., SnapshotMode::PieSnapshot)` bytes are the canonical chunk payload building block.
2. **`SchemaVersion`** (`schema_version` in `.gwsave` header) gates migrations. Current schema is published as a `constexpr` in `persist_types.hpp`.
3. **`SaveMigrationStep`**: `{ from_version, to_version, apply(SaveReader&, SaveWriter&) }`. Registry is `std::array`-backed append-only; steps are pure (no wall clock, no globals).
4. On load, if `on_disk_schema < current`, run the chain forward. If `engine_version` on disk exceeds the running engine and `persist.migration.strict=1`, refuse load (`ForwardIncompatible`).
5. **`determinism_hash`**: recomputed after deserialize; mismatch is `LoadError::DeterminismMismatch` unless best-effort mode (not used in strict default).

## Consequences

- Golden fixtures in `tests/fixtures/saves/v1/` are validated by `gw_tests` and future `gw_save_tool`.

---

## ADR file: `adr/0058-sqlite-local-store.md`

## ADR 0058 — SQLite local store (`ILocalStore`)

- **Status:** Accepted
- **Date:** 2026-04-21

## Decision

1. **`ILocalStore`** abstracts slots, settings KV, achievements mirror, telemetry queue, mods catalog, and `schema_version` rows.
2. **SQLiteCpp 3.3.1** (pinned in `cmake/dependencies.cmake`) is the only approved wrapper; raw `sqlite3` API is confined to `impl/sqlite_store.cpp`.
3. **Pragma stack** on open: `journal_mode=WAL`, `synchronous=NORMAL`, `foreign_keys=ON`. Tables are `STRICT` where SQLite ≥ 3.37.
4. **Core tables** match Phase 15 spec: `schema_version`, `slots`, `settings`, `achievements_mirror`, `telemetry_queue`, `mods`.
5. **Forward migrations** for DB schema are registered beside code; `PRAGMA wal_checkpoint(TRUNCATE)` runs on clean shutdown before cloud sync.

## Consequences

- Cook cache and runtime persistence share the same SQLite linkage discipline.

---

## ADR file: `adr/0059-icloudsave-and-conflict-policy.md`

## ADR 0059 — `ICloudSave` and conflict policy

- **Status:** Accepted
- **Date:** 2026-04-21

## Decision

1. **`ICloudSave`** exposes `put` / `get` / `list` / `remove` / `quota` / `backend_name`.
2. **`SlotStamp`**: `unix_ms`, `vector_clock`, `machine_salt` (FNV-1a of host + user id hash).
3. **`CloudConflictPolicy`**: `Prompt` (default), `PreferLocal`, `PreferCloud`, `PreferNewer`, `PreserveBoth`. Resolver table matches Phase 15 spec; Steam Cloud opacity implies `Prompt` is the safe default.
4. **Factories:** `local` dev FS backend always ships; `steam`, `eos`, `s3` compile only behind `GW_ENABLE_STEAMWORKS`, `GW_ENABLE_EOS`, `GW_ENABLE_CPR` respectively (stubs return null backend when OFF).
5. **`QuotaWarning`** event when `used_bytes / quota_bytes` crosses `persist.cloud.quota_warn_pct`.

## Consequences

- Phase 16 replaces stubs with production SDKs without interface churn.

---

## ADR file: `adr/0060-telemetry-sentry-backend.md`

## ADR 0060 — Telemetry crash backend (Sentry Native)

- **Status:** Accepted
- **Date:** 2026-04-21

## Decision

1. **`TelemetryWorld`** PIMPL facade; public headers never include `<sentry.h>`.
2. **`GW_ENABLE_SENTRY`** gates `impl/sentry_backend.cpp`. Default OFF on `dev-*`; ON on `persist-*` presets.
3. **DSN** resolves via OS keyring helper (Phase 9 pattern); never from env, CLI, or logs. Miss → null backend + one-shot warn + `CrashBufferedOffline` event.
4. **Crashpad** handler path and database path follow platform conventions (`%LOCALAPPDATA%\CCL\Greywater\sentry-db`, `$XDG_CACHE_HOME/ccl-greywater/sentry-db`).
5. **Attachments:** allow-list only (log head/tail); saves, queues, and secrets are never attached. `.gwreplay` only if `tele.attach_replay` and consent ≥ `CrashOnly+` as defined in ADR-0062.
6. **`before_send`** runs PII scrub chain (ADR-0061).

## Consequences

- Pin **Sentry Native 0.7.19**; bumps require ADR-0064 revision + full crash round-trip CI.

---

## ADR file: `adr/0061-telemetry-event-pipeline.md`

## ADR 0061 — Telemetry event pipeline

- **Status:** Accepted
- **Date:** 2026-04-21

## Decision

1. **`ITelemetrySink`** batches `EventEnvelope` records: `{ schema_version, ts, session_id_hash, event_name, props_json }`.
2. **Pipeline:** record → PII scrub → consent gate → in-memory ring → when `batch_size` or `flush_interval_s` or shutdown, enqueue SQLite row with BLAKE3-128 digest (`telemetry_queue`).
3. **Flush worker** uses `jobs::Scheduler` jobs (telemetry_io lane convention); HTTP POST to `tele.endpoint` when non-empty; exponential backoff on 5xx/network; 4xx drops row as permanent failure.
4. **`GW_TELEMETRY_COMPILED=OFF`** (release SKUs without TRC clearance) compiles stubs only; overhead target ≤60 µs cold path (ADR-0064).
5. **Session id** = SplitMix64 mix of `session_seed` xor `user_id_hash` (no raw PII).

## Consequences

- Offline queue survives restart; `tele.queue.max_age_days` enforces storage minimisation.

---

## ADR file: `adr/0062-privacy-contract.md`

## ADR 0062 — Privacy contract (GDPR, CCPA, COPPA 2026)

- **Status:** Accepted
- **Date:** 2026-04-21

## Decision

1. **Opt-in defaults:** `tele.enabled=0`, `tele.crash.enabled=0`, `tele.events.enabled=0` until first-launch consent UI completes.
2. **Consent tiers (disaggregated):** `None`, `CrashOnly`, `CoreTelemetry`, `AnalyticsAllowed`, `MarketingAllowed`. COPPA posture: under `tele.consent.age_gate_years` (default 13, locale-adjustable), choices clamp to `CrashOnly` maximum for third-party SDK exposure.
3. **State machine:** Initial → LegalNotice → AgeGate → ConsentDisaggregated → Persisted → Revocable. UI lives in **RmlUi** (`ui/privacy/`), not ImGui (CLAUDE.md #12).
4. **Persistence:** consent version string forces re-prompt on mismatch; SQLite `settings` namespace `tele.consent.*`.
5. **DSAR:** export manifest (JSON schema in implementation) + user-selected output dir; **deletion** nulls PII fields, purges telemetry queue rows for `user_id_hash`, removes cloud slots; gameplay blobs may remain if non-PII.
6. **BLD / agents:** consent-changing CVars are not user-writable from untrusted automation (treat as user Execute-tier; registry flags `kCVarReadOnly` for console except programmatic trusted paths).

## Schema block (DSAR manifest v1)

```json
{
  "schema_version": 1,
  "exported_unix_ms": 0,
  "user_id_hash": "hex64",
  "slots": [],
  "settings_namespaces": [],
  "telemetry_queue_rows": 0,
  "consent": { "tier": "None", "version": "", "granted_ms": 0 }
}
```

## Consequences

- Legal review updates this ADR when statutes or platform rules change.

---

## ADR file: `adr/0063-secure-local-storage.md`

## ADR 0063 — Secure local storage

- **Status:** Accepted
- **Date:** 2026-04-21

## Decision

1. **Keyring wrap** (Win DPAPI / Linux libsecret) holds DSN key names and OAuth refresh tokens; same helper family as Phase 9 BLD keyring.
2. **Save encryption:** optional XChaCha20-Poly1305 for SP saves with user-authored PII; MP saves default plaintext performance path. Wrong user binding fails round-trip at open.
3. **Redaction:** logs and attachments never contain secrets, session tickets, or full DSN.

## Consequences

- Phase 15 ships keyring integration points as stubs where OS helpers are not yet linked; production `persist-*` presets enable them.

---

## ADR file: `adr/0064-phase15-cross-cutting.md`

## ADR 0064 — Phase 15 cross-cutting (pins, CI, hand-off)

- **Status:** Accepted
- **Date:** 2026-04-21
- **Supersedes:** —
- **Companion ADRs:** 0056, 0057, 0058, 0059, 0060, 0061, 0062, 0063.

## 1. Dependency pins

| Library | Pin | CMake gate | Default `dev-*` | Default `persist-*` |
|---|---|---|---|---|
| SQLiteCpp | 3.3.1 | existing (always) | ON | ON |
| BLAKE3 (C) | 1.5.4 | always for `.gwsave` footer | ON | ON |
| zstd | 1.5.6 | `GW_ENABLE_ZSTD` | OFF | ON |
| Sentry Native SDK | 0.7.19 | `GW_ENABLE_SENTRY` | OFF | ON |
| cpr | 1.11.1 | `GW_ENABLE_CPR` | OFF | ON |
| Steamworks SDK | Phase 16 | `GW_ENABLE_STEAMWORKS` | OFF | OFF |
| EOS SDK | Phase 16 | `GW_ENABLE_EOS` | OFF | OFF |

Header-quarantine invariant (ADR-0056 §3): none of the above leak into any public header under `engine/persist/*.hpp` or `engine/telemetry/*.hpp`; all vendor types live in `impl/*.cpp`.

## 2. Performance budgets

See `docs/09_NETCODE_DETERMINISM_PERF.md (merged section **Phase 15 performance budgets**)`. The `gw_perf_gate_phase15` binary (tests/perf) enforces the numeric ceilings on every CI build; CTest label `phase15;perf`. Debug builds apply a generous slack so the gate stays green on `dev-*` and tightens under `persist-*` Release.

## 3. Thread ownership

| Owner | Work |
|---|---|
| Main | `PersistWorld` / `TelemetryWorld` lifetime; consent UI FSM |
| Jobs `persist_io` (priority Normal) | `.gwsave` IO, SQLite transactions, migration chain |
| Jobs `telemetry_io` (priority Low) | HTTP flush, Sentry upload, DSAR zip |
| Jobs `background` (priority Background) | cloud sync poll, quota check |

Named lanes are *conventions* on `jobs::Scheduler::submit` priorities — see `engine/jobs/lanes.hpp` (`submit_persist_io`, `submit_telemetry_io`, `submit_background`). No additional thread pools; crashpad's handler is a separate OS process managed by Sentry itself (not a Greywater thread).

## 4. Tick order (runtime)

After `render.extract()` (when present): `persist.step()` then `telemetry.step()`. Neither may block the main thread for IO — both dispatch work onto their named lanes and return within `O(µs)`.

## 5. CI

`.github/workflows/ci.yml` runs:

- `phase15-windows` / `phase15-linux` — `dev-*` preset, CTest `-L phase15` on every commit.
- `phase15-persist-smoke-windows` / `phase15-persist-smoke-linux` — `persist-*` preset with zstd/Sentry/cpr enabled; smoke gates label `phase15|perf`.
- `phase15-persistence-sandbox` — runs `sandbox_persistence`; requires the `PERSIST OK` marker in stdout.
- `phase15-save-compat-regression` — `gw_save_tool validate` + `gw_save_tool migrate --dry-run` over checked-in fixtures under `tests/fixtures/saves/v1/`.
- `phase15-save-cross-platform` — determinism_hash equality check across Windows + Linux; re-uses the parity test cases in `phase15_extras_test.cpp`.

## 6. Exit checklist (retrospective, 2026-04-21)

- [x] ADRs 0056 → 0064 Accepted.
- [x] Zero `<sentry.h>` / `<sqlite3.h>` / `<steam_api.h>` / `<eos_sdk.h>` / `<zstd.h>` in any public header.
- [x] `.gwsave` v1 round-trips ≥ 1024 entities deterministically; `phase15 — identical ECS inputs produce byte-identical gwsave bodies` locks the invariant.
- [x] `sandbox_persistence` prints `PERSIST OK` with 3+ saves, 1+ migration, 2+ cloud round-trips, 500+ events, 1 test-crash, DSAR export.
- [x] 25 `persist.*` / `tele.*` CVars registered; TOML round-trip covered by `phase15 — persist.* + tele.* survive TOML save → load`.
- [x] 12 Phase-15 console commands registered and discoverable; `phase15 — all 12 console commands are registered and discoverable` enforces the invariant.
- [x] ≥ 45 new Phase-15 tests landed across `phase15_sqlite_test.cpp`, `phase15_cloud_test.cpp`, `phase15_persist_world_test.cpp`, `phase15_extras_test.cpp`, `phase15_telemetry_test.cpp`, `persist_gwsave_test.cpp`; `dev-win` CTest count ≥ 445.
- [x] `persist-{win,linux}` presets build + smoke-run green on CI.
- [x] `phase15-persistence-sandbox` + `phase15-save-compat-regression` + cross-platform parity gates enforced.
- [x] `docs/09_NETCODE_DETERMINISM_PERF.md (merged section **Phase 15 performance budgets**)` ceilings enforced by `gw_perf_gate_phase15`.
- [x] `docs/a11y_phase15_selfcheck.md` ticked.
- [x] Roadmap Phase-15 row flipped to `completed` with closeout paragraph (`docs/02_ROADMAP_AND_OPERATIONS.md`).

## 7. `.gwsave` v1 format freeze

Layout is frozen as committed in ADR-0056 §4. Any change is a `v1 → v2` migration step in `engine/persist/migration.cpp`; the existing writer + reader continue to support v1 for the lifetime of the engine (determinism ladder §4).

## 8. CVar + console surface (frozen)

25 CVars, 12 commands — exhaustive list in `engine/persist/persist_cvars.cpp` and `engine/persist/console_commands.cpp` + `engine/telemetry/console_commands.cpp`. New entries in Phase 16 may append but must not rename or repurpose existing keys (TOML forward-compat).

## 9. Determinism ladder

1. `.gwsave` writer is bit-deterministic: `phase15 — identical ECS inputs produce byte-identical gwsave bodies`.
2. `determinism_hash` (ADR-0037) embedded in header; loader refuses mismatch in strict mode.
3. BLAKE3-256 footer is content-addressed; no clock, allocator, or RNG state influences it.
4. Migration chain is append-only, pure functions from `SaveReader&` → `SaveWriter&`.
5. SQLite WAL with `synchronous=NORMAL`; `PRAGMA wal_checkpoint(TRUNCATE)` at clean shutdown and before every cloud sync.
6. Telemetry session id = SplitMix64 of `(session_seed XOR user_id_hash)`, reproducible across restart.
7. DSAR zip sorted by key — byte-identical across runs for the same state.
8. Cross-platform parity gate enforces hash equality Windows ↔ Linux.

## 10. Risks retrospective (2026-04-21 closeout)

| Risk | Mitigation shipped | Residual |
|---|---|---|
| Sentry crashpad protocol drift | Pin 0.7.19; bumps gated by ADR; handler path baked in `impl/sentry_backend.cpp` | Phase 16 must rerun the round-trip gate on bump |
| SQLite WAL corruption after power-cut | `journal_mode=WAL`, `synchronous=NORMAL`, truncate-checkpoint at shutdown, `integrity_check` on recovery | Low; ship with telemetry event `persist.integrity_check_failed` |
| Save-format drift across OSes | zstd deterministic CDM, no byte-order runtime detection, `phase15-save-cross-platform` gate CI-mandatory | Nil (gated) |
| COPPA liability for third-party SDKs | Under-13 clamps to `CrashOnly`; `MarketingAllowed` disabled; consent UI disaggregated | Legal must re-verify quarterly per ADR-0062 |
| Steam Cloud conflict opacity | `PreferNewer` + vector clock = 95 % deterministic; `Prompt` fallback surfaces Steam-native UI | Phase 16 wires real Steam backend |
| EOS quota surprise | `QuotaWarning` at 80 % (CVar-configurable), hard block at 100 %, per-put poll | Phase 16 wires real EOS |
| Crashpad failure on sandboxed Linux distros | Fallback to in-process `sentry_capture_event`, local minidump retention | Phase 16 persist-linux preset smoke |
| Save bloat from uncompressed chunks | zstd-3 default behind `persist.compress.enabled`; per-chunk compression | None in gate |
| DSAR PII on disk post-export | User picks dir; README recommends BitLocker/LUKS; export is sorted + deterministic | User-education dependent |
| Telemetry endpoint outage | SQLite queue with exp-backoff; `tele.queue.max_age_days` drops per GDPR minimisation | Phase 16 plugs real cpr backend |
| BLD session hallucinating consent change | `tele.consent.*` CVars are `Execute`-tier (ADR-0015) — BLD cannot flip without user form-mode | Periodic audit |
| Compliance drift | ADR-0062 is the amendment log; quarterly BLD check-in required | Process discipline |

## 11. Hand-off to Phase 16 (*Ship Ready*, weeks 102–107)

Phase 16 inherits three frozen contracts:

1. **`ICloudSave` frozen** — Phase-16 Steamworks + EOS waves plug production impls into existing stubs without interface churn. `CloudConflictPolicy` enum is append-only.
2. **`ITelemetrySink` + consent tiers frozen** — Phase-16 WCAG 2.2 AA audit (week 107) validates RmlUi consent + DSAR UX without backend changes.
3. **`.gwsave` v1 frozen** — Phase-16 locale work adds display-string rows to slot metadata; any schema change is a `v1 → v2` migration, never a format break.

The `persist-*` preset becomes the base for the `ship-*` preset.

## 12. Exit gate closeout

Phase 15 exits Accepted on 2026-04-21. Hand-off block above is normative for Phase 16. For the roadmap status flip see `docs/02_ROADMAP_AND_OPERATIONS.md` §Phase-15 closeout.

---

## ADR file: `adr/0065-engine-platform-services-facade.md`

## ADR 0065 — `PlatformServicesWorld` facade + `IPlatformServices`

- **Status:** Accepted
- **Date:** 2026-04-21
- **Phase:** 16 (wave 16A)
- **Companions:** 0066 (Steamworks), 0067 (EOS), 0059 (cloud-save freeze), 0073 (phase-cross-cutting).

## 1. Context

Phase 15 froze `ICloudSave`. Phase 16 needs a generic store-facing façade that exposes:
- user identity (provider id, display name, stable user-hash),
- achievements + stats,
- leaderboards (read + submit),
- rich-presence strings (locale-bound — binds to Phase 16 i18n),
- friends presence (count only, never PII),
- user-generated content (Workshop / Mods feed),
- rate-limited events (per-backend throttle).

The **exact same interface** must admit the Phase-15 `ICloudSave` splice, the
Phase-16B Steamworks backend, and the Phase-16C EOS backend, without any
leakage of `<steam_api.h>` or `<eos_sdk.h>` into public headers.

## 2. Decision

Ship `engine/platform_services/platform_services_world.{hpp,cpp}` as a PIMPL
façade that owns an `IPlatformServices` pointer plus a separate `ICloudSave`
adapter (splice target for ADR-0059). Public headers never include any third-
party SDK header.

`IPlatformServices` is a pure virtual interface with these groups:

1. **Identity** — `current_user()`, `signed_in()`, `user_id_hash()`.
2. **Achievements** — `unlock_achievement(id)`, `is_unlocked(id)`, `clear(id)`.
3. **Stats** — `get_stat_i32/f32`, `set_stat_i32/f32`, `store_stats()`.
4. **Leaderboards** — `submit_score(board, value)`, `top_scores(board, k, out)`.
5. **Rich presence** — `set_rich_presence(key, value_loc_id)`.
6. **UGC / Workshop** — `list_subscribed_content(out)`, `download(content_id)`.
7. **Events** — `publish_event(name, payload_json)` (respects `tele.consent`).
8. **Lifecycle** — `backend_name()`, `initialize(cfg)`, `step(dt)`, `shutdown()`.

Three concrete backends ship this phase:

- **`make_dev_local_platform_services`** (16A) — in-memory, SQLite-backed,
  always available; used by `dev-*` + every unit test.
- **`make_steamworks_platform_services`** (16B) — compile-gated on
  `GW_ENABLE_STEAMWORKS`; `impl/steam_backend.cpp` is the only TU that
  includes `<steam_api.h>`.
- **`make_eos_platform_services`** (16C) — compile-gated on
  `GW_ENABLE_EOS`; `impl/eos_backend.cpp` is the only TU that includes
  `<eos_sdk.h>`.

A **factory** `make_platform_services_aggregated(backend, cfg)` returns the
dev-local backend unless the requested SDK is compiled in and available, in
which case it returns that backend. This mirrors the Phase-15 cloud-save
factory exactly.

## 3. Header-quarantine invariant

`grep -r '#include <steam_api' engine/platform_services/ | grep -v impl/`
produces zero hits. Same for `<eos_sdk`. Enforced in CI by
`tests/integration/phase16_header_quarantine_test.cpp`.

## 4. Interface (abbreviated — full in code)

```cpp
namespace gw::platform_services {

struct UserRef {
    std::string   id_hash_hex;
    std::string   display_name;
    std::string   locale_bcp47;
};

enum class PlatformError : std::uint8_t {
    Ok = 0, NotSignedIn, NotFound, RateLimited, BackendDisabled, IoError,
};

class IPlatformServices {
public:
    virtual ~IPlatformServices() = default;

    [[nodiscard]] virtual UserRef current_user() const = 0;
    [[nodiscard]] virtual bool signed_in() const noexcept = 0;

    virtual PlatformError unlock_achievement(std::string_view id) = 0;
    [[nodiscard]] virtual bool is_unlocked(std::string_view id) const = 0;

    virtual PlatformError set_stat_i32(std::string_view key, std::int32_t v) = 0;
    virtual PlatformError submit_score(std::string_view board, std::int64_t v) = 0;

    virtual PlatformError set_rich_presence(std::string_view key,
                                            std::string_view value_utf8) = 0;

    virtual PlatformError publish_event(std::string_view name,
                                        std::string_view payload_json) = 0;

    [[nodiscard]] virtual std::string_view backend_name() const noexcept = 0;

    virtual void step(double dt_seconds) = 0;
};

struct PlatformServicesConfig {
    std::string  backend{"local"};    // "local" | "steam" | "eos"
    std::string  storage_dir;         // expanded by the world (ADR-0056 `$user`)
    std::string  app_id;              // Steam AppID / EOS ProductId (ignored by local)
    bool         dry_run_sdk{true};   // in CI: short-circuit real SDK calls
};

[[nodiscard]] std::unique_ptr<IPlatformServices>
    make_dev_local_platform_services(const PlatformServicesConfig&);

[[nodiscard]] std::unique_ptr<IPlatformServices>
    make_platform_services_aggregated(std::string_view backend,
                                      const PlatformServicesConfig&);

} // namespace gw::platform_services
```

## 5. CVars

Seven new `plat.*` CVars:

| key | default | notes |
|---|---|---|
| `plat.backend` | `"local"` | `local` / `steam` / `eos` |
| `plat.app_id` | `""` | Steam AppID / EOS ProductId |
| `plat.achievements.enabled` | `true` | |
| `plat.leaderboards.enabled` | `true` | |
| `plat.rich_presence.enabled` | `true` | |
| `plat.rate_limit_per_minute` | `120` | per event name |
| `plat.dry_run_sdk` | `true` | CI shortcut; never hits real SDK network |

## 6. Consequences

- Steam/EOS backends stay purely additive — no interface churn in Phases
  17+. The `ICloudSave` freeze from ADR-0064 §11 binds; Phase 16B only
  provides a concrete `make_steamworks_cloud_save()` that slots into the
  existing factory.
- Rate-limiting is backend-agnostic; a ring-buffer in the façade catches
  abuse before any SDK call. `plat.rate_limit_per_minute` is the knob.
- The `sandbox_platform_services` exit gate uses the dev-local backend —
  real SDK runs stay behind `ship-*` preset + signed CI job.

---

## ADR file: `adr/0066-steamworks-backend.md`

## ADR 0066 — Steamworks backend (`GW_ENABLE_STEAMWORKS`)

- **Status:** Accepted
- **Date:** 2026-04-21
- **Phase:** 16 (wave 16B)
- **Pin:** Steamworks SDK **1.64** (shipped 2026-Q1).
- **Companions:** 0065 (façade), 0059 (`ICloudSave` freeze), 0073.

## 1. Decision

Integrate Steamworks SDK 1.64 behind the `GW_ENABLE_STEAMWORKS` CMake gate.
The only TUs that include `<steam_api.h>` are `impl/steam_backend.cpp` and
`impl/steam_cloud_save.cpp`; public headers remain SDK-free (verified by the
header-quarantine test).

## 2. Surface implemented

1. `make_steamworks_platform_services(cfg)` — returns `nullptr` if
   `SteamAPI_Init()` fails or if `dry_run_sdk` is true.
2. `make_steamworks_cloud_save(cfg)` — plugs the `ICloudSave` factory
   without ADR-0059 interface churn. Steam Cloud quota goes through
   `ISteamRemoteStorage::GetQuota`.
3. Workshop — the `list_subscribed_content()` and `download(id)` calls
   shim through `ISteamUGC`.
4. Achievements/stats — `ISteamUserStats`; `store_stats()` is called on
   `step()` when dirty.
5. Rich presence — `ISteamFriends::SetRichPresence(key, localized_value)`.

## 3. Lifecycle

- `initialize()` calls `SteamAPI_Init()` once; failure returns
  `PlatformError::BackendDisabled`; subsequent calls are no-ops.
- `step()` pumps `SteamAPI_RunCallbacks()` and drains the leaderboard
  submission queue (max 4 submissions per frame to stay inside the SDK's
  2/sec implicit cap).
- `shutdown()` calls `SteamAPI_Shutdown()`.

## 4. Rate-limiting + re-entry

The façade ring-buffer (`plat.rate_limit_per_minute`) gates every SDK
call so Steam's IPC queue never saturates. Re-entry is blocked by an
internal mutex — callbacks from Steam fire on a helper thread and go
through a single-producer `MPSCQueue<Event>` before reaching façade
consumers.

## 5. CI shape

- `ship-windows` / `ship-linux` presets define `GW_ENABLE_STEAMWORKS=ON`
  (Phase 16 closeout). The CI job runs a **signed** smoke that uses the
  real AppID 480 (Spacewar) sandbox with `dry_run_sdk=true`; production
  keys are never checked in.
- `dev-*` presets keep the gate OFF → all tests link against the
  dev-local backend only.

## 6. Header quarantine

```
engine/platform_services/impl/
  steam_backend.hpp       # forward decls only
  steam_backend.cpp       # includes <steam_api.h>
  steam_cloud_save.cpp    # includes <steam_api.h>
  steam_ugc.cpp           # includes <steam_api.h>
```

A CI grep for `#include <steam_api` outside `impl/` fails the build.

## 7. Risks

| Risk | Mitigation |
|---|---|
| SDK 1.64 header re-organisation (`steam_gameserver.h` split) | Only include `steam_api.h`; never pin internal headers |
| `SteamAPI_Init` order with GNS (Phase 14) | GNS' `SteamNetworkingSockets::Init()` is independent; wired before façade init |
| Workshop quota surprise | `UGCQueryHandle_t` is paged; default page 20 items |
| Steam overlay pause during `step()` | Façade `step()` is idempotent + wall-clock budget 0.5 ms |

---

## ADR file: `adr/0067-eos-backend.md`

## ADR 0067 — Epic Online Services backend (`GW_ENABLE_EOS`)

- **Status:** Accepted
- **Date:** 2026-04-21
- **Phase:** 16 (wave 16C)
- **Pin:** EOS SDK **1.17** (2026-Q1).
- **Companions:** 0065 (façade), 0066 (Steam companion), 0073.

## 1. Decision

Integrate EOS SDK 1.17 behind the `GW_ENABLE_EOS` CMake gate. Same shape as
ADR-0066: `impl/eos_backend.cpp`, `impl/eos_cloud_save.cpp`,
`impl/eos_achievements.cpp`. No public header pulls `<eos_sdk.h>`.

## 2. Surface implemented

1. `make_eos_platform_services(cfg)` — returns `nullptr` on
   `EOS_Platform_Create` failure or if `dry_run_sdk`.
2. `make_eos_cloud_save(cfg)` — plugs into the ADR-0059 factory.
3. Achievements via `EOS_Achievements` interface; mirrors stats through
   `EOS_Stats_IngestStat`.
4. Leaderboards via `EOS_Leaderboards_QueryLeaderboardRanks`.
5. Rich presence via `EOS_Presence_SetPresence`.
6. Mod/UGC feed via **Epic Mod SDK** (`EOS_ModSdk` 1.17).

## 3. Lifecycle

- `EOS_Initialize` + `EOS_Platform_Create` in `initialize`.
- `step()` pumps `EOS_Platform_Tick` and drains the submit queue.
- `shutdown()` calls `EOS_Platform_Release` then `EOS_Shutdown`.

## 4. Parity with Steam

Dual-unlock is achieved by emitting `platform.achievement.unlocked` on the
cross-subsystem bus; the Steamworks + EOS backends both listen and call
their respective SDK once. This is what the *Ship Ready* milestone gate
means by "Steam + EOS dual-unlock from a single gameplay event."

## 5. Risks

| Risk | Mitigation |
|---|---|
| EOS SDK dual-init with Steamworks | Both can run in the same process; the façade owns them in a deterministic vector |
| EOS Cloud conflict semantics differ from Steam | The ADR-0059 `CloudConflictPolicy` enum is append-only; EOS backend maps its ETag to `vector_clock` |
| Anti-cheat surface (Easy Anti-Cheat) | Out of scope for Phase 16; Phase 24 reopens |
| Epic Account login flow vs Steam offline | Façade `signed_in()` surfaces false when EOS auth has not completed; games degrade gracefully |

---

## ADR file: `adr/0068-gwstr-binary-tables.md`

## ADR 0068 — `.gwstr` binary localization tables

- **Status:** Accepted
- **Date:** 2026-04-21
- **Phase:** 16 (wave 16C)
- **Companions:** 0028 (`LocaleBridge`), 0069 (ICU runtime), 0070 (shaping).

## 1. Context

Phase 11 shipped `LocaleBridge` with in-memory FNV1a-hashed keys. Phase 16
promotes strings to an **on-disk binary format** so locales ship as
content-hashed, cook-time-validated, mmap-friendly blobs. This is the input
the engine reads at runtime; the source of truth remains XLIFF 2.1 (ADR-0069
§3) so translators never see a binary.

## 2. Format

```
.gwstr v1 layout (little-endian)

HeaderPrefix (32 bytes)
├── magic           u32  = 'GWSR'
├── version         u16  = 1
├── flags           u16  bit0 = bidi_hint_present
├── string_count    u32
├── blob_size       u32  (UTF-8 blob length)
├── locale_tag_len  u8   (BCP-47)
├── reserved        u8[3]
├── locale_tag      char[locale_tag_len]  // not inside header prefix

Index (string_count × 12 bytes)
├── key_hash        u32  (fnv1a32)
├── utf8_offset     u32  into blob
└── utf8_size       u32

Blob (UTF-8 bytes, no terminators)

Footer (36 bytes)
├── reserved        u32
└── blake3_256      u8[32]  content hash of [HeaderPrefix..Blob]
```

Index is sorted by `key_hash`; lookup is binary search. The blob is a
single concatenated UTF-8 pool; individual strings are `(utf8_offset,
utf8_size)`. Zero padding between strings; canonical byte-identical
output for deterministic cook.

## 3. Cook

`tools/cook/gw_cook` gains a `locale` stage: ingest `*.xliff` (ADR-0069
§3) → canonicalize → emit `content/locales/<tag>.gwstr`. The cook is
deterministic (same XLIFF → same bytes) and its BLAKE3 is embedded in
the VFS manifest.

## 4. Runtime load

`gw::i18n::load_gwstr(std::span<const std::byte>)` returns a
`StringTable` that the `LocaleBridge` can register as an override. The
loader is header-only read-path (no allocations after construction) and
returns `LocaleError::{Ok,BadMagic,VersionTooNew,Truncated,HashMismatch}`.

## 5. Compatibility & migration

- v1 is the floor. Any new field is a `v1 → v2` migration in
  `engine/i18n/gwstr_migration.cpp`.
- Strings loaded via `LocaleBridge::register_string_table(tag, table)`
  stack: the last table wins on collisions. This lets patches ship as
  small overlays without rebuilding the base locale.

## 6. Determinism

The cook stage hashes **after** NFC normalisation and **after** removing
trailing whitespace per translator convention. `bidi_hint_present` flips
when any string contains at least one RTL scalar (U+0590..U+06FF,
U+0700..U+08FF) — precomputed so the runtime doesn't re-scan.

---

## ADR file: `adr/0069-icu-runtime-and-xliff.md`

## ADR 0069 — ICU runtime + XLIFF 2.1 workflow

- **Status:** Accepted
- **Date:** 2026-04-21
- **Phase:** 16 (wave 16C)
- **Pin:** ICU4C **78.3** (Unicode 17 / CLDR 48, 2026-Q1).
- **Companions:** 0068 (`.gwstr`), 0070 (shaping + MessageFormat 2).

## 1. Decision

Link ICU4C 78.3 behind `GW_ENABLE_ICU` for number/date/plural/case
services. The `dev-*` preset builds a **null ICU shim** (`icu_null.cpp`)
that implements the same C++ interface using deterministic fall-backs
(English plurals, C-locale numbers, ISO-8601 dates). This keeps Phase
16 tests deterministic and the CI matrix CPU-light.

## 2. Public interface (sketch)

```cpp
namespace gw::i18n {

class NumberFormat {
public:
    static std::unique_ptr<NumberFormat> make(std::string_view bcp47);
    virtual ~NumberFormat() = default;

    virtual std::string format(std::int64_t v) const = 0;
    virtual std::string format(double v, int frac_digits) const = 0;
};

class DateTimeFormat {
public:
    static std::unique_ptr<DateTimeFormat> make(std::string_view bcp47,
                                                std::string_view skeleton);
    virtual ~DateTimeFormat() = default;

    virtual std::string format(std::int64_t unix_ms) const = 0;
};

enum class PluralCategory : std::uint8_t { Zero, One, Two, Few, Many, Other };
[[nodiscard]] PluralCategory plural_category(std::string_view bcp47,
                                              double quantity) noexcept;

[[nodiscard]] std::string to_lower_locale(std::string_view utf8, std::string_view bcp47);
[[nodiscard]] std::string to_upper_locale(std::string_view utf8, std::string_view bcp47);

} // namespace gw::i18n
```

## 3. XLIFF 2.1 workflow

Source of truth for translators: `content/locales/*.xliff` (XLIFF 2.1 with
ICU MessageFormat 2 placeholders). Cook pipeline:

1. `gw_cook locale scan` reads `.xliff`, validates MessageFormat 2
   placeholders, and flags missing translations against the English
   baseline.
2. Per-locale `*.gwstr` is emitted (ADR-0068 §3).
3. A translator-facing TSV export is available (`gw_cook locale export-tsv`)
   for spreadsheet round-trips; the import step reinvokes the MessageFormat 2
   validator.

Five locales ship at Phase 16 exit: `en-US`, `fr-FR`, `de-DE`, `ja-JP`,
`zh-CN`. Each locale must resolve ≥ 95% of the canonical key set or the
`phase16-locales` CI job fails.

## 4. Content validation

- Every placeholder declared in the English source must appear in every
  translation.
- Ordinal + plural forms use `{count, plural, ...}` — the cook validates
  a locale's plural categories match CLDR 48 for that BCP-47 tag.
- XLIFF notes flagged `critical=yes` block the cook until resolved.

## 5. Null-ICU behaviour

When `GW_ENABLE_ICU=OFF`:

- `NumberFormat::format(int)` = `std::to_string`.
- `NumberFormat::format(double, k)` = `%.*f`.
- `DateTimeFormat::format(ms)` = ISO-8601 `YYYY-MM-DDTHH:MM:SSZ`.
- `plural_category` = English rules (`1 → One`, else `Other`).
- `to_lower/upper_locale` = ASCII-safe `std::tolower/toupper`.

These fall-backs are **documented** in the a11y-selfcheck as "degraded
locale" — they keep `dev-*` deterministic without pretending to be
localised.

## 6. Header quarantine

`<unicode/*.h>` is included only in `impl/icu_backend.cpp`. Public
headers use opaque forward declarations + `std::unique_ptr`.

---

## ADR file: `adr/0070-harfbuzz-bidi-messageformat.md`

## ADR 0070 — HarfBuzz shaping, ICU BiDi, MessageFormat 2

- **Status:** Accepted
- **Date:** 2026-04-21
- **Phase:** 16 (wave 16D)
- **Pins:** HarfBuzz **14.0.0** (2026-Q1) · ICU4C 78.3 (shared with ADR-0069).
- **Companions:** 0028 (Phase 11 shaper), 0068, 0069, 0071 (a11y bundle).

## 1. Decision

Promote the Phase-11 `TextShaper` null mapper to a real HarfBuzz 14.0.0
shaper behind `GW_ENABLE_HARFBUZZ` (already wired for `playable-*`).
Phase 16 adds three new capabilities:

1. **Per-locale font-stack fallback.** `FontLibrary::fallback_chain(bcp47)`
   resolves a fallback order per locale (e.g. `ja-JP` → `Noto Sans CJK JP` →
   `Noto Sans` → emoji), so a single shape call can span scripts.
2. **ICU BiDi segmentation (UBA 15).** `gw::i18n::bidi_segments(utf8,
   base_dir)` returns runs of `TextDirection`; the shaper then calls HarfBuzz
   once per run with the right direction + script tag. Null builds return a
   single LTR run (stable for tests).
3. **MessageFormat 2** (ICU 78 final) — `gw::i18n::format_message(tag,
   pattern, args)` returns a localized string with plural / selectordinal /
   number / date placeholders. Null build implements a deterministic subset
   (`{name}` + `{count, plural, one {X} other {Y}}` + `{n, number}`).

## 2. Interface delta

`engine/i18n/bidi.hpp`:
```cpp
namespace gw::i18n {

enum class BaseDirection : std::uint8_t { LTR, RTL, Auto };

struct BidiRun {
    std::uint32_t start;  // byte offset
    std::uint32_t size;   // bytes
    ui::TextDirection dir;
    ui::TextScript    script;
};

std::size_t bidi_segments(std::string_view utf8, BaseDirection base,
                           std::vector<BidiRun>& out);

} // namespace gw::i18n
```

`engine/i18n/message_format.hpp`:
```cpp
namespace gw::i18n {

struct FmtArg {
    std::string_view key;
    enum class Kind { Str, I64, F64 } kind;
    std::string_view s;
    std::int64_t     i;
    double           f;
};

std::string format_message(std::string_view bcp47,
                           std::string_view pattern,
                           std::span<const FmtArg> args);

} // namespace gw::i18n
```

## 3. Shaper delta

`TextShaper::shape()` splits input into BiDi runs and shapes each run
with HarfBuzz when `GW_ENABLE_HARFBUZZ` is on. Null build returns the
Phase-11 contract unchanged (one glyph per codepoint, advance_x ≈
`pixel_size * 0.6`).

## 4. Performance budgets

- `bidi_segments(len ≤ 256 scalars)` < 2 µs on null; < 8 µs on ICU.
- `shape(256 scalars, Latin)` < 50 µs on HarfBuzz 14.
- `format_message` < 5 µs null; < 20 µs ICU.

Enforced by `gw_perf_gate_phase16` (ADR-0073 §6).

## 5. Header quarantine

`<hb.h>` only in `impl/hb_shaper.cpp`. `<unicode/ubidi.h>` only in
`impl/icu_bidi.cpp`. Public headers stay SDK-free.

---

## ADR file: `adr/0071-a11y-modes-and-subtitles.md`

## ADR 0071 — Accessibility modes + subtitles

- **Status:** Accepted
- **Date:** 2026-04-21
- **Phase:** 16 (wave 16E)
- **Companions:** 0072 (screen-reader bridge), 0073 (phase cross-cutting).

## 1. Decision

Ship `engine/a11y/` with four cross-cutting subsystems:

1. **Color-vision modes.** A 3×3 CAM16 LMS transform per mode applied in
   the tonemap pass. Modes: `None`, `Protanopia`, `Deuteranopia`,
   `Tritanopia`, `HighContrast`. The matrix is a `glm::mat3`; the UI
   pipeline reads `a11y::active_color_transform()` and the tonemap
   shader uses it as a uniform. The null/headless build still exposes
   the matrix so logic tests pass.

2. **Text scaling.** `ui.text.scale ∈ [0.5, 2.5]` drives every glyph's
   pixel size at render time. The settings binder writes it; layout
   reflow is automatic because glyph metrics are computed from the
   effective size. Minimum body size enforced ≥ 14pt per WCAG 2.2 AA
   §1.4.4.

3. **Motion reduction + photosensitivity.** `ui.reduce_motion` (bool) and
   `a11y.photosensitivity_safe` (bool). Both clamp flashing to ≤ 3 Hz
   per WCAG 2.2 AA §2.3.1 (three-flash rule) and compress large parallax
   translations > 32 px to ≤ 8 px in the UI shader.

4. **Subtitles + captions.** `engine/a11y/subtitles.{hpp,cpp}` queues
   caption events from audio + vscript + net, renders them via the UI
   service with `ui.caption.scale` and `ui.caption.speakercolor.enabled`,
   and exports a cue-list to telemetry so translators can audit
   per-locale timing.

## 2. CVars (new 16 — `a11y.*`)

| key | default | range |
|---|---|---|
| `a11y.color_mode` | `"none"` | one of {none, protan, deutan, tritan, hc} |
| `a11y.color_strength` | `1.0` | `[0, 1]` |
| `a11y.text.scale` | `1.0` | `[0.5, 2.5]` (mirrors `ui.text.scale`) |
| `a11y.contrast.boost` | `0.0` | `[0, 1]` |
| `a11y.reduce_motion` | `false` | |
| `a11y.photosensitivity_safe` | `false` | |
| `a11y.subtitle.enabled` | `true` | |
| `a11y.subtitle.scale` | `1.0` | `[0.5, 2.0]` |
| `a11y.subtitle.bg_opacity` | `0.7` | `[0, 1]` |
| `a11y.subtitle.speaker_color` | `true` | |
| `a11y.subtitle.max_lines` | `3` | `[1, 6]` |
| `a11y.screen_reader.enabled` | `false` | ADR-0072 |
| `a11y.screen_reader.verbosity` | `"terse"` | `terse`/`normal`/`verbose` |
| `a11y.focus.show_ring` | `true` | WCAG 2.2 AA §2.4.11 |
| `a11y.focus.ring_width_px` | `2.0` | `[1.0, 8.0]` |
| `a11y.wcag.version` | `"2.2"` | introspectable |

## 3. Color matrix freeze

```
Protanopia      Deuteranopia     Tritanopia       HighContrast
0.152  1.053 −0.205   0.367  0.861 −0.228   1.256 −0.077 −0.179   1.5  0   0
0.115  0.786  0.099   0.280  0.673  0.047   0.078  0.931 −0.009   0   1.5 0
−0.004 −0.048 1.052   −0.012 0.043  0.969  −0.007  0.018 0.990    0   0  1.5
```

Source: Machado, Oliveira, Fernandes (2009) CAM16 simulation, α=1.0. The
matrix is versioned; any future update is a new `a11y.color_mode` value
("protan2"), never a silent mutation. Canonical test fixture in
`tests/fixtures/a11y/color_matrices_v1.json`.

## 4. Subtitles pipeline

1. Audio world publishes `SubtitleCue{speaker, text_string_id,
   start_unix_ms, duration_ms, priority}` on the cross-subsystem bus.
2. `SubtitleQueue::push(cue)` enqueues; max concurrent lines = 3 by
   CVar. Overflow drops lowest priority.
3. `render()` walks active cues and emits a `ui::SubtitleLine` list that
   the UI service paints in a reserved bottom strip.
4. Telemetry records `a11y.subtitle.emitted` with speaker + duration
   (PII-scrubbed) when `tele.events.enabled`.

## 5. Exit gate

`a11y::selfcheck()` returns a `SelfCheckReport` with pass/fail booleans
per WCAG 2.2 AA criterion that's statically checkable (contrast, text
size, focus visibility, captions available). The *Ship Ready* demo
consumes this and prints `a11y_selfcheck=green` on all-pass.

---

## ADR file: `adr/0072-screen-reader-bridge.md`

## ADR 0072 — Screen-reader bridge (UIA / AT-SPI / NSAccessibility)

- **Status:** Accepted
- **Date:** 2026-04-21
- **Phase:** 16 (wave 16E)
- **Pins:** AccessKit-C **0.14.0** (2026-Q1).
- **Companions:** 0071 (a11y modes), 0073.

## 1. Decision

Ship a cross-platform screen-reader bridge via **AccessKit-C 0.14.0**, a
Rust-core C-ABI that speaks MS UI Automation (Windows), AT-SPI 2 (Linux),
and NSAccessibility (macOS, out of scope but kept on the table). The
bridge is behind `GW_ENABLE_ACCESSKIT`; the null build ships an
in-memory accessibility tree that passes the selfcheck but does not
talk to any OS API.

## 2. Interface

```cpp
namespace gw::a11y {

enum class Role : std::uint8_t {
    Unknown, Window, Button, CheckBox, Slider, Label, ListItem,
    Menu, MenuItem, ComboBox, Tab, TabList, ProgressBar, Image,
    Graphic, Application,
};

struct AccessibleNode {
    std::uint64_t node_id;         // stable across frames
    Role          role{Role::Unknown};
    std::string   name_utf8;       // already localized
    std::string   description_utf8;
    float         x{0}, y{0}, w{0}, h{0};  // virtual-pixel AABB
    bool          focused{false};
    bool          enabled{true};
    std::uint64_t parent_id{0};
    std::vector<std::uint64_t> children;
};

class IScreenReader {
public:
    virtual ~IScreenReader() = default;

    virtual void update_tree(std::span<const AccessibleNode> nodes) = 0;
    virtual void announce_focus(std::uint64_t node_id) = 0;
    virtual void announce_text(std::string_view utf8,
                               std::string_view politeness) = 0;

    [[nodiscard]] virtual std::string_view backend_name() const noexcept = 0;
};

std::unique_ptr<IScreenReader> make_null_screen_reader();
std::unique_ptr<IScreenReader> make_accesskit_screen_reader();

} // namespace gw::a11y
```

## 3. Bridging RmlUi

`engine/ui/ui_service` emits an `a11y::TreeDelta` after each layout when
`a11y.screen_reader.enabled`. `IScreenReader::update_tree` consumes it;
focus changes emit `announce_focus(id)` with the visible node name.

## 4. Politeness levels

Matches ARIA live regions: `off`, `polite`, `assertive`. Subtitles +
HUD updates default to `polite`; critical warnings (damage, low-O2)
use `assertive`. `a11y.screen_reader.verbosity` multiplexes: `terse`
skips `polite` announcements entirely.

## 5. Header quarantine

`<accesskit.h>` only in `engine/a11y/impl/accesskit_backend.cpp`. Public
headers stay platform-neutral.

## 6. Selfcheck tick items

- Every interactive element has a role + accessible name.
- Tab order == visual order.
- Focus ring is drawn.
- No `Role::Unknown` in a built scene.

Passing all four → `a11y::SelfCheckReport::screen_reader = Pass`.

---

## ADR file: `adr/0073-phase16-cross-cutting.md`

## ADR 0073 — Phase 16 cross-cutting (pins, CI, hand-off)

- **Status:** Accepted
- **Date:** 2026-04-21
- **Phase:** 16 exit
- **Companions:** 0065, 0066, 0067, 0068, 0069, 0070, 0071, 0072.

## 1. Dependency pins

| Library | Pin | CMake gate | `dev-*` | `ship-*` |
|---|---|---|---|---|
| Steamworks SDK | 1.64 | `GW_ENABLE_STEAMWORKS` | OFF | ON |
| EOS SDK | 1.17 | `GW_ENABLE_EOS` | OFF | ON |
| ICU4C | 78.3 (Unicode 17 / CLDR 48) | `GW_ENABLE_ICU` | OFF | ON |
| HarfBuzz | 14.0.0 | `GW_ENABLE_HARFBUZZ` | OFF (but `playable-*` ON) | ON |
| AccessKit-C | 0.14.0 | `GW_ENABLE_ACCESSKIT` | OFF | ON |

Header-quarantine invariant: none of the above leak into any public
header under `engine/platform_services/*.hpp`, `engine/i18n/*.hpp`, or
`engine/a11y/*.hpp`. `tests/integration/phase16_header_quarantine_test.cpp`
enforces.

## 2. Performance budgets

See `docs/09_NETCODE_DETERMINISM_PERF.md (merged section **Phase 16 — perf budgets**)`. `gw_perf_gate_phase16` enforces:

| metric | null `dev-*` | `ship-*` |
|---|---|---|
| `PlatformServicesWorld::step()` | ≤ 100 µs | ≤ 500 µs |
| `bidi_segments` (256 scalars) | ≤ 2 µs | ≤ 8 µs |
| `format_message` | ≤ 5 µs | ≤ 20 µs |
| `gwstr` load (10 k keys) | ≤ 300 µs | ≤ 1 ms |
| a11y `selfcheck` | ≤ 50 µs | ≤ 200 µs |

## 3. Thread ownership

| Owner | Work |
|---|---|
| Main | `PlatformServicesWorld` lifecycle; consent UI; a11y tree update |
| Jobs `platform_io` (Normal) | Workshop/UGC downloads, leaderboard submissions |
| Jobs `i18n_io` (Normal) | `.gwstr` load, XLIFF re-read on hot-reload |
| Jobs `background` (Background) | Presence refresh, friends count poll |

The names match ADR-0064 §3 lanes; `jobs::submit_platform_io`,
`jobs::submit_i18n_io` become available from `engine/jobs/lanes.hpp`.

## 4. Tick order

After Phase-15 `persist.step()` + `telemetry.step()`:
- `platform_services.step()` pumps SDK callbacks (dev-local: no-op),
- `i18n.step()` services XLIFF hot-reload (dev-local: no-op),
- `a11y.step()` rebuilds accessibility tree deltas,
- subtitles queue drain inside `ui.render()` (no additional tick step).

No subsystem may block main > 0.5 ms in `ship-*`.

## 5. CI

`.github/workflows/ci.yml`:

- `phase16-windows` / `phase16-linux` — `dev-*`, CTest `-L phase16`.
- `phase16-platform-smoke-windows` / `phase16-platform-smoke-linux` —
  `ship-*` preset with Steam + EOS ON in `dry_run_sdk=true` mode; smoke
  runs `sandbox_platform_services` and verifies `SHIP READY` marker.
- `phase16-locales` — validates every `.xliff` has ≥ 95% of canonical
  key set for each of {en-US, fr-FR, de-DE, ja-JP, zh-CN}.
- `phase16-a11y-selfcheck` — boots `sandbox_platform_services`, dumps
  `a11y_selfcheck.json`, fails on any `wcag22.*=Fail`.
- `phase16-header-quarantine` — greps for forbidden includes outside
  `impl/`; fails on any hit.

## 6. Exit checklist

- [x] ADRs 0065 → 0073 Accepted.
- [x] Zero `<steam_api.h>` / `<eos_sdk.h>` / `<unicode/*.h>` / `<hb.h>` /
      `<accesskit.h>` in any public header.
- [x] `PlatformServicesWorld` PIMPL ships with dev-local backend + Steam
      + EOS stubs compile-gated.
- [x] `.gwstr` v1 round-trips bit-deterministically across Windows +
      Linux.
- [x] `LocaleBridge` consumes `.gwstr`; ICU null + real shims both pass
      the plural + date tests.
- [x] BiDi segments are stable across Windows + Linux for the canonical
      `ar`, `he`, and mixed-Latin/Arabic fixtures.
- [x] MessageFormat 2 null shim resolves `{count, plural, one {.}
      other {.}}` + `{name}` correctly.
- [x] HarfBuzz `GW_ENABLE_HARFBUZZ=ON` smoke (part of `playable-*` +
      `ship-*`) shapes a Latin string ≤ 50 µs.
- [x] a11y modes + subtitles + screen-reader bridge ship with null
      backend; `accesskit` compiles gated.
- [x] ≥ 86 new Phase-16 tests landed; `dev-win` CTest count ≥ 559.
- [x] `ship-{win,linux}` presets build + smoke-run green on CI.
- [x] `sandbox_platform_services` prints `SHIP READY` with `steam=✓
      eos=✓ wcag22=AA i18n_locales≥5 a11y_selfcheck=green`.
- [x] `docs/09_NETCODE_DETERMINISM_PERF.md (merged section **Phase 16 — perf budgets**)` ceilings enforced.
- [x] `docs/a11y_phase16_selfcheck.md` ticked.
- [x] Roadmap Phase-16 row flipped to `completed`.

## 7. Frozen contracts handed to Phase 17

1. **`IPlatformServices` frozen** — Phase-17 shader/VFX work does not
   touch this surface; Phase-18 mods reuse `list_subscribed_content`.
2. **`.gwstr` v1 frozen** — cinematics (Phase 18) author localized
   subtitle cues against the same table.
3. **`a11y::SelfCheckReport` frozen** — release engineering (Phase 24)
   ratifies it as the WCAG 2.2 AA checklist.

## 8. Risks retrospective

| Risk | Mitigation shipped | Residual |
|---|---|---|
| Steamworks / EOS dual-init ordering | Façade owns both as deterministic vector; `SteamAPI_Init` before `EOS_Initialize` | Low; gated by `ship-*` CI |
| AccessKit-C crashes on headless Linux | Null backend is the default; AccessKit load is try/catch in `impl/` | AT-SPI bus absence surfaces warning only |
| ICU 78 plural rules drift from CLDR 47 | Lockfile pins CLDR 48 tables; null shim mirrors English | Locale coverage CI gate detects regressions |
| HarfBuzz 14 SDF baseline vs Phase-11 contract | Public `ShapedGlyph` POD unchanged; advance encoding stays 26.6 fixed | Null build's 1:1 glyph mapping remains the test oracle |
| WCAG 2.2 §2.4.11 focus obscuring | `a11y.focus.show_ring` defaults ON; selfcheck flips Fail when hidden | Manual audit still required per release |
| Translator burden of 5 locales | XLIFF round-trip + MessageFormat 2 lint | Procurement concern, not technical |
| Photosensitivity false-negatives | Three-flash rule clamps audio + UI; scene artists must annotate | Phase 18 VFX doc captures |
| Steam + EOS achievement drift | Dual-unlock via cross-subsystem bus; idempotent on both backends | Manual QA pass per milestone |
| Rich-presence PII leak | `rich_presence` values are `StringId`s (pre-localised); free text rejected | Enforced by `IPlatformServices::set_rich_presence` contract |
| `.gwstr` content-hash drift by OS line-endings | XLIFF cook normalises to LF + NFC before hashing | CI parity gate (Windows ↔ Linux) |
| WCAG 2.2 §1.4.4 text scaling vs layout break | Settings binder clamps `ui.text.scale` ∈ [0.5, 2.5]; UI reflow tested | Phase 18 cinematics must retest |
| BLD session drifting a11y CVars | `a11y.*` CVars registered as `Execute` tier (ADR-0015) — BLD cannot flip without form-mode elicitation | Periodic audit |

---

## ADR file: `adr/0074-shader-pipeline-maturity.md`

## ADR 0074 — Shader pipeline maturity (permutations, Slang, reflection, cache)

- **Status:** Accepted
- **Date:** 2026-04-21
- **Phase:** 17 (Studio Renderer)
- **Wave:** 17A
- **Companions:** 0075 (material façade), 0082 (cross-cutting).

## 1. Context

Phase 5 shipped `ShaderManager` with content-hashed DXC SPIR-V output + hot-reload. Phase 17 moves the shader system from a *single-entry-point* tool to a *permutation-aware pipeline* that knows how to:

1. combinatorially evolve shader permutations from orthogonal feature axes;
2. call a second compiler (Slang, SDK-gated) alongside DXC for advanced
   features (native custom derivatives, generics, modules);
3. source descriptor-set + push-constant layout **from the SPIR-V bytecode**
   (SPIRV-Reflect) rather than hand-rolled tables;
4. serve pre-cooked permutations from a content-hash LRU with ≥ 95 %
   warm-boot hit rate.

## 2. Decision

- Add four new compilation units to `engine/render/shader/`:
  `permutation.{hpp,cpp}` · `reflect.{hpp,cpp}` ·
  `slang_backend.{hpp,cpp}` · `permutation_cache.{hpp,cpp}`.
- Keep the Phase-5 `shader_manager.{hpp,cpp}` as the *transport* layer
  (file I/O, `VkShaderModule` creation, hot-reload file watch) — it is
  no longer the brain.
- **Permutation doctrine.** A template's permutation space is the cartesian
  product of its orthogonal *axes*:

  ```
  axis := enumeration of mutually-exclusive feature variants
        | boolean feature gate
  ```

  Axes are declared up-front with `PermutationAxis::Desc{}`; the engine
  rejects permutations whose index is not in the pre-cooked matrix.
  **The runtime never compiles on demand**; permutations are produced at
  build time (CI shader library) or on content reload (dev).
  Feature-flag pragmas in HLSL (`// gw_axis: NAME = a|b|c`) are the
  declarative source.
- **SM 6.9 collapses two axes.** Wave ops + 16-bit + 64-bit ints are
  mandatory at SM 6.9. The previous `GW_HAS_WAVES`, `GW_HAS_16BIT` axes
  are removed; a single `GW_FEATURE_SM69` runtime-probe flag drops back
  to the SM 6.6 path when the GPU lacks SM 6.9.
- **Dual compiler.** DXC (v1.9.2602) is the authoritative path. Slang
  (v2026.5.2) is an *additive* path behind `GW_ENABLE_SLANG`, OFF by
  default on `dev-*`, ON on the new `studio-*` presets. Slang output is
  validated by *round-trip SPIR-V equivalence* in CI (`studio-*` weekly).
- **SPIRV-Reflect.** `reflect.{hpp,cpp}` wraps SPIRV-Reflect (main@
  2026-02-25, vendored). It infers:
  - descriptor-set layouts (binding number, type, count, stage mask);
  - push-constant ranges;
  - vertex-input attributes (for vertex shaders);
  - workgroup size (for compute).

  Hand-written reflection structs in the Phase-5 path are deleted; the
  `MaterialTemplate` in ADR-0075 consumes the reflection output directly.
- **Permutation cache.** `permutation_cache.{hpp,cpp}` is a content-hash
  LRU keyed by `BLAKE3(source_bytes || defines || compiler_version)`,
  capped by `r.shader.cache_max_mb` (default 256). Cache hit = zero
  Slang/DXC invocation. Warm boot must reach ≥ 95 % hit rate or CI
  flags a regression.

## 3. Non-goals

- We do **not** introduce a runtime shader compiler on the gameplay
  thread. All compilation is off-thread via `jobs::submit_shader_compile`
  (ADR-0082).
- We do **not** ship Slang's module system in headers. Slang lives only
  in `.cpp`; `GW_ENABLE_SLANG=0` still compiles.
- We do **not** replace DXC with Slang. Slang is opt-in.

## 4. Header-quarantine invariant

No `<slang.h>`, `<dxc/*.h>`, `<spirv_reflect.h>`, `<spirv/*.h>` may appear
in any public header under `engine/render/**/*.hpp` or any other
`engine/**/*.hpp`. Each third-party SDK is `#include`d only from the
corresponding `*.cpp` behind its `GW_ENABLE_*` flag.
`tests/integration/phase17_header_quarantine_test.cpp` enforces.

## 5. CVars (§17A)

| Name | Type | Default | Flags | Notes |
|---|---|---|---|---|
| `r.shader.slang_enabled` | bool | false | kPersist\|kUserFacing | runtime echo of build gate |
| `r.shader.permutation_budget` | i32 | 64 | kPersist | per-template ceiling |
| `r.shader.cache_max_mb` | i32 | 256 | kPersist | LRU cap |
| `r.shader.hot_reload` | bool | true | kPersist | file-watch on/off |
| `r.shader.sm69_emulate` | bool | false | kDevOnly | force the SM 6.6 path for CI |

## 6. Console commands

`r.shader.rebuild_all`, `r.shader.list_perms`, `r.shader.stats`,
`r.shader.reflect <name>`.

## 7. Perf

See `docs/09_NETCODE_DETERMINISM_PERF.md (merged section **Phase 17 performance budgets — Studio Renderer**)`. Shader-cache warm-boot hit rate ≥ 95 %;
permutation count per template ≤ 64 (≤ 128 fail band). Enforced by
`gw_perf_gate_phase17`.

## 8. Test plan

`tests/unit/render/shader/phase17_permutation_test.cpp` (12 cases) +
`phase17_slang_smoke_test.cpp` (6 cases). Slang cases auto-`SKIP` under
`GW_ENABLE_SLANG=0`.

## 9. Consequences

- **Positive.** Deterministic permutation space, SDK-agnostic descriptor
  layout inference, zero-cost Slang opt-in, sub-millisecond permutation
  selection at draw time.
- **Negative.** Authors now declare axes up front; ad-hoc `#ifdef`s in
  HLSL are deprecated (migration table in wave 17B).
- **Risk (H): permutation explosion.** Mitigated by the per-template
  ceiling + `gw_perf_gate_phase17`. ADR-0074 is the source of truth for
  axis orthogonality.

## 10. References

- SIGGRAPH 2025 course — *Permutation-aware shader graphs*.
- Slang Release v2026.5.2 (2026-03-31).
- DXC v1.9.2602 (2026-02).
- SPIRV-Reflect main@2026-02-25.
- Vulkan 1.4.336 (2025-12-12).

---

## ADR file: `adr/0075-material-system-facade.md`

## ADR 0075 — Material system façade (`MaterialWorld`, templates, instances)

- **Status:** Accepted
- **Date:** 2026-04-21
- **Phase:** 17 (Studio Renderer)
- **Waves:** 17B (core), 17C (authoring)
- **Companions:** 0074 (shader pipeline), 0076 (`.gwmat` v1), 0082.

## 1. Context

Greywater's renderer is PBR metal-roughness by default and is moving to
realistic low-poly (surface-driven appearance, high-quality lighting).
Wave 17B introduces a *first-class* material subsystem separated into:

- **`MaterialTemplate`** — pipeline permutation, reflection-derived
  descriptor layout, default parameter block.
- **`MaterialInstance`** — a **256-byte** parameter block + up to 16
  texture-slot handles bound at draw time.
- **`MaterialWorld`** — the PIMPL façade (mirrors
  `PlatformServicesWorld` / `A11yWorld`); single RAII owner of every
  template and every instance, reports stats, hosts the reload chain.

## 2. Decision

### 2.1 Façade shape

`MaterialWorld` is a **PIMPL** class. Public surface (matches the Phase-
16 pattern):

```cpp
class MaterialWorld {
 public:
  MaterialWorld();
  ~MaterialWorld();
  [[nodiscard]] bool initialize(MaterialConfig,
                                gw::config::CVarRegistry*,
                                gw::events::CrossSubsystemBus* = nullptr,
                                gw::jobs::Scheduler* = nullptr);
  void shutdown();
  [[nodiscard]] bool initialized() const noexcept;

  void step(double dt_seconds);  // drains reload chain

  [[nodiscard]] MaterialTemplateId  create_template(const MaterialTemplateDesc&);
  [[nodiscard]] MaterialInstanceId  create_instance(MaterialTemplateId);
  void                              destroy_instance(MaterialInstanceId);
  void                              set_param(MaterialInstanceId, std::string_view key, float v);
  void                              set_param(MaterialInstanceId, std::string_view key, glm::vec4 v);
  void                              set_texture(MaterialInstanceId, std::uint32_t slot, TextureHandle);
  [[nodiscard]] MaterialStats       stats() const noexcept;
  [[nodiscard]] bool                reload(MaterialInstanceId);  // hot-reload one
  [[nodiscard]] std::size_t         reload_all();
  // ... see engine/render/material/material_world.hpp
};
```

### 2.2 Template → Instance

- A `MaterialTemplate` owns a `gw::render::shader::PermutationTable`
  slice (ADR-0074). Permutation axes match glTF PBR extensions:
  `clearcoat`, `specular`, `sheen`, `transmission` (each a binary axis)
  + the base `metal-rough` / `dielectric` quality tier.
- A `MaterialInstance` is a 256-byte POD parameter block + a
  16-slot texture table. The block layout is **inferred** from the
  permutation's SPIR-V reflection (ADR-0074 §2 reflect).

### 2.3 glTF PBR extension matrix

The renderer ships with five opaque-PBR permutations:

| template | KHR extensions |
|---|---|
| `pbr_opaque/metal_rough` | — |
| `pbr_opaque/dielectric` | `KHR_materials_specular` |
| `pbr_opaque/clearcoat` | `KHR_materials_clearcoat` |
| `pbr_opaque/sheen` | `KHR_materials_sheen` |
| `pbr_opaque/transmission` | `KHR_materials_transmission` |

All five share one `MaterialTemplate` with the permutation axes
orthogonal. Combined permutations (e.g. clearcoat + sheen) are legal
but unused in the shipping art pack today — pre-cooked in
`studio-*` presets only.

### 2.4 Hot-reload chain

Wave 17C wires the reload chain:

```
.hlsl changed → shader_manager file-watch → recompile + reflect
               → MaterialTemplate pipeline swap (atomic)
               → MaterialInstance parameter re-layout
               → frame_graph pass rebind next frame
```

Hot-reload work goes through the new `jobs::material_reload` lane
(ADR-0082 §3). `mat.auto_reload` CVar throttles the chain to ≤ 5 Hz.

### 2.5 Authoring (Wave 17C)

The façade exposes:

- `list_templates()` / `list_instances()` — read-only iteration.
- `describe_instance(MaterialInstanceId)` — serialized JSON blob the
  editor panel renders.
- `preview_sphere(MaterialTemplateId)` — returns a lightweight scene
  description (a sphere with a chosen template) for the in-editor
  material preview. Engine-pure; the editor app consumes this as an
  ImGui panel input in Phase 18+.

## 3. Non-goals

- No node-graph material editor. Graph authoring is intentionally
  deferred to Phase 22 tooling.
- No procedural substance-style materials. Shipping pack is glTF-PBR.

## 4. Header-quarantine invariant

Public `engine/render/material/*.hpp` may **not** include any HLSL/DXC/
Slang/Vulkan-specific SDK header. Vulkan pipeline handles live behind
opaque IDs (`TextureHandle`, `PipelineHandle`).

## 5. CVars (§17B/C, ≥ 8)

| Name | Default | Notes |
|---|---|---|
| `mat.default_template` | `pbr_opaque/metal_rough` | initial template for new instances |
| `mat.instance_budget` | 4096 | hard cap |
| `mat.texture_cache_mb` | 512 | texture-slot cache |
| `mat.auto_reload` | true | Wave 17C |
| `mat.preview_sphere` | true | editor convenience |
| `mat.quality_tier` | `high` | low / med / high |
| `mat.sheen_enabled` | true | gates the sheen permutation |
| `mat.transmission_enabled` | true | gates transmission permutation |

## 6. Console commands

`mat.list`, `mat.reload <id>`, `mat.set <id> <key> <value>`,
`mat.dump <id>`, `mat.preview <template>`.

## 7. Perf

See `docs/09_NETCODE_DETERMINISM_PERF.md (merged section **Phase 17 performance budgets — Studio Renderer**)`. 1024 instances upload ≤ 1.5 ms GPU;
per-frame CPU upload ≤ 0.3 ms.

## 8. Test plan

- `phase17_material_template_test.cpp` — 14 cases.
- `phase17_material_instance_test.cpp` — 12 cases.
- `phase17_gwmat_test.cpp` — 10 cases (ADR-0076).

## 9. Consequences

- **Positive.** Clear ownership, deterministic permutation paths,
  scriptable material authoring via console, hot-reload survives the
  entire running session.
- **Negative.** Fixed 256-byte parameter block excludes exotic BRDFs
  (deferred until Phase 22 *procedural materials*).

## 10. Hand-off (frozen for Phase 20 planetary rendering)

`MaterialTemplate` ABI is frozen at the end of Phase 17. Planetary
rendering (Phase 20) inherits it as a first-class input.

---

## ADR file: `adr/0076-gwmat-v1-binary-format.md`

## ADR 0076 — `.gwmat` v1 binary material file format

- **Status:** Accepted
- **Date:** 2026-04-21
- **Phase:** 17 (Studio Renderer)
- **Wave:** 17B
- **Companions:** 0075 (MaterialWorld), 0068 (`.gwstr` binary tables).

## 1. Context

Material instances must round-trip deterministically across machines
(server authoritative → client), git (content-addressed assets) and
hot-reload. `.gwmat` v1 is the on-disk representation.

## 2. Decision — layout

```
+0   u32  magic        = 'GWMT' (0x474D5754 little-endian)
+4   u16  major        = 1
+6   u16  minor        = 0
+8   u32  flags        (bit0 = compressed)
+12  u32  payload_size (bytes from +32 to footer)
+16  u64  content_hash (FNV-1a-64 of [+0..+12] || parameter-block || slot-table)
+24  u32  param_block_size (≤ 256)
+28  u16  slot_count       (≤ 16)
+30  u16  template_name_len
+32  ... template_name (ASCII, no NUL, length prefix above)
     ... u32[64] parameter_block_layout_table  (key_hash, offset, type)
     ... u8[param_block_size] parameter_block
     ... slot_count × { u32 texture_fingerprint; u16 slot_idx; u16 reserved }
+end u32  footer_magic = 'EOGW' (0x45 4F 47 57)
```

- **Total header size: 32 bytes.**
- `parameter_block_layout_table` has up to 64 entries × 12 bytes = 768 B
  worst case. Packed with `SoA` to keep `memcpy` straightforward.
- `texture_fingerprint` is the 32-bit upper half of the texture's
  BLAKE3 id (ADR-0058 style).

## 3. Content hashing

- Algorithm: **FNV-1a-64** with a 64-bit Fibonacci diffusion finalizer.
  Identity hash, not security hash.
- Rationale (mirrors ADR-0068 §4): keeps `greywater::material` independent
  from `greywater::persist` + BLAKE3 to avoid the circular-dependency
  hazard that would arise if material I/O required `greywater::persist`.
- Determinism: identical input bytes produce identical hashes across
  Windows + Linux + clang-cl. Enforced by
  `phase17_gwmat_test.cpp::gwmat_roundtrip_is_bit_deterministic`.

## 4. Endianness, alignment, limits

- Little-endian only (all shipping targets; big-endian emission is a
  deliberate non-goal).
- 4-byte alignment on all integers; the 32-byte header + 256-byte
  payload keeps the whole record 4-byte aligned.
- Hard limits: ≤ 256 B parameter block, ≤ 16 texture slots, template
  name ≤ 128 B.

## 5. Forward / backward compatibility

- `major = 1` is frozen.
- Minor bump adds optional fields **after** the texture-slot table but
  **before** the footer. Unknown minor fields are skipped during load.

## 6. I/O API

```cpp
namespace gw::material {
  [[nodiscard]] GwMatBlob  encode_gwmat(const MaterialInstance&, const MaterialTemplate&);
  [[nodiscard]] std::expected<MaterialInstance, GwMatError>
                           decode_gwmat(std::span<const std::byte>, MaterialWorld&);
  [[nodiscard]] std::uint64_t content_hash(std::span<const std::byte>);
}
```

Errors: `GwMatError::MagicMismatch`, `VersionUnsupported`,
`HashMismatch`, `Truncated`, `TemplateUnknown`, `ParamLayoutMismatch`.

## 7. Test plan

`tests/unit/render/material/phase17_gwmat_test.cpp` — 10 cases:

1. encode/decode round-trip preserves every parameter.
2. `content_hash` is bit-deterministic across two builds.
3. Flipping one byte in the payload changes the hash.
4. Corrupted magic rejected.
5. Corrupted footer rejected.
6. Unknown template name rejected with `TemplateUnknown`.
7. Oversize parameter block (257 B) rejected.
8. Oversize slot count (17) rejected.
9. Minor-version forward-compat: v1.1 loaded by v1.0 code with a warning.
10. Hash stable when parameter block contains NaN/Inf (normalized to
    canonical bit-pattern before hashing).

## 8. Consequences

- **Positive.** Deterministic material content-hash, git-friendly,
  streaming-ready (compressed variant reserved for Phase 20).
- **Negative.** Version bumps require a minor bump + migration table —
  registered in `MaterialWorld::register_migration()`.

---

## ADR file: `adr/0077-gpu-particle-system.md`

## ADR 0077 — GPU particle system (`engine/vfx/particles/`)

- **Status:** Accepted
- **Date:** 2026-04-21
- **Phase:** 17 (Studio Renderer)
- **Wave:** 17D
- **Companions:** 0078 (ribbons + decals), 0082 (cross-cutting).

## 1. Context

The engine needs a 1M-particle-per-frame-at-60 fps budget for combat
VFX, atmospheric smoke, and ember/sparks. CPU particle systems are ruled
out (ADR-12 §3 RX 580 baseline). Wave 17D ships a GPU compute-driven
system.

## 2. Decision — pipeline

Four compute dispatches per frame per emitter, bracketed by an indirect
draw:

```
emit.comp    → writes new particles to live ping-pong buffer
sim.comp     → integrates forces, curl-noise, aging
compact.comp → streamed compaction of dead particles
(draw indirect) — sorted optional (vfx.particles.sort_mode=bitonic|none)
```

- **Ping-pong buffers.** Two SSBOs of size
  `vfx.particles.budget × sizeof(GpuParticle)`. `sim.comp` reads from
  A, writes to B. Next frame swap.
- **Workgroup size = 64.** Covers GCN 64-lane waves (RX 580) and splits
  into two 32-lane wavefronts on NVIDIA/Intel cleanly.
- **Curl-noise motion.** 3D noise texture (256³ R8) driven by Perlin +
  domain-warp; `vfx.particles.curl_noise_octaves` controls cost. CPU
  reference implementation mirrored in the test harness for
  determinism.
- **Lifecycle.** `alive = age < lifetime_s`. The compactor sorts dead
  indices to the tail using a parallel prefix sum; draw count comes
  from the compactor's live-count output.
- **Sort.** Optional bitonic sort on `(depth, id)` for alpha-blended
  particles. Tests use the CPU reference (deterministic).

## 3. CPU data model

```cpp
struct GpuParticle {       // 48 bytes
  glm::vec3 position;  float age;
  glm::vec3 velocity;  float lifetime;
  glm::vec2 size;      float rotation;  std::uint32_t color_rgba;
};
static_assert(sizeof(GpuParticle) == 48);

class IParticleEmitter {
 public:
  virtual ~IParticleEmitter() = default;
  virtual void emit(std::uint32_t n, std::uint64_t seed) = 0;
  virtual std::uint64_t live_count() const noexcept = 0;
  // ... see engine/vfx/particles/particle_emitter.hpp
};
```

## 4. Determinism

GPU particle simulation is **not** bit-deterministic across vendors.
Tests run the CPU reference pipeline (`particle_simulation.cpp`'s
`simulate_cpu_reference`) which *is* bit-deterministic. GPU sort is an
optimisation, not a correctness requirement.

## 5. CVars (§17D, ≥ 4)

| Name | Default | Notes |
|---|---|---|
| `vfx.particles.budget` | 1048576 | hard cap |
| `vfx.particles.sort_mode` | `none` | none \| bitonic |
| `vfx.particles.curl_noise_octaves` | 3 | 1..6 |
| `vfx.gpu_async` | true | particle simulate on async compute queue |

## 6. Console commands

`vfx.particles.spawn <emitter>`, `vfx.particles.clear`,
`vfx.particles.stats`.

## 7. Perf budget

See `docs/09_NETCODE_DETERMINISM_PERF.md (merged section **Phase 17 performance budgets — Studio Renderer**)`.
- Simulate @ 1M: ≤ 2.5 ms GPU.
- Render @ 1M: ≤ 2.0 ms GPU.
- Hard fail band at 4.0 / 3.5 ms respectively.

## 8. Test plan

`tests/unit/vfx/phase17_particles_test.cpp` — 16 cases:
emission counts, lifecycle, stream compaction, curl-noise determinism,
ping-pong invariants, budget enforcement, sort-stability under fixed
seed.

## 9. Hand-off (frozen for Phase 20)

`IParticleSystem` contract is frozen; planetary rendering (Phase 20) and
combat VFX (Phase 21) inherit it.

---

## ADR file: `adr/0078-ribbons-and-decals.md`

## ADR 0078 — Ribbon trails + screen-space deferred decals

- **Status:** Accepted
- **Date:** 2026-04-21
- **Phase:** 17 (Studio Renderer)
- **Wave:** 17E
- **Companions:** 0077 (particles), 0082.

## 1. Context

Ribbon trails (tracer rounds, dash effects, contrails) and deferred
decals (bullet holes, scorch marks, blood) round out the VFX surface
between particles (17D) and the post chain (17F).

## 2. Decision — ribbons

- `RibbonState` is a per-ribbon state machine: `Inactive → Emitting →
  Fading → Inactive`. Each ribbon owns a vertex ring buffer capped by
  `vfx.ribbons.max_length`.
- GPU tessellation rebuilds triangle strips from the vertex ring each
  frame. No CPU triangle emission.
- Per-segment UV is stored on the ring (width, age) so shader code
  ramps width/alpha without CPU touching the buffer.

## 3. Decision — decals

- Decals are **screen-space deferred** (Wronski 2015 method).
- Projection volume: an OBB stored as `{ mat4 inv_world, vec3 half_extents, u32 flags }`.
- For every screen pixel covered by the OBB, the decal shader
  reconstructs world position from the depth buffer, tests if it is
  inside the OBB, and if so blends G-buffer albedo + normal from a
  bindless texture pair.
- **Edge fix.** Wronski 2015's well-known edge artefact (when a decal's
  OBB crosses a steep depth discontinuity) is mitigated via:
  1. Depth-read-only pipeline + `VK_COMPARE_OP_GREATER` (prevents
     self-overlap);
  2. Per-tile clip-plane filter (reuse the Phase-5
     `forward_plus/cluster.*` infrastructure for spatial binning);
  3. Bindless albedo+normal texture tables per MJP 2021 guidance.
- **Cluster binning.** Decals are assigned to clusters during the
  existing light-culling compute pass (Phase 5), amortising the spatial
  index.

## 4. Non-goals

- No volumetric decals. (Deferred to Phase 20 atmospherics.)
- No animated decals (frame-sheet). (Deferred to Phase 21 combat VFX.)

## 5. CVars (§17E, ≥ 4)

| Name | Default | Notes |
|---|---|---|
| `vfx.ribbons.budget` | 256 | simultaneous ribbons |
| `vfx.ribbons.max_length` | 128 | vertex-ring size per ribbon |
| `vfx.decals.budget` | 1024 | simultaneous decals |
| `vfx.decals.cluster_bin` | true | reuse forward_plus clusters |

## 6. Console commands

`vfx.ribbons.spawn`, `vfx.decals.project <x,y,z>`.

## 7. Test plan

- `phase17_ribbons_test.cpp` — 6 cases.
- `phase17_decals_test.cpp` — 8 cases.

## 8. Perf

See `docs/09_NETCODE_DETERMINISM_PERF.md (merged section **Phase 17 performance budgets — Studio Renderer**)`; decals + ribbons share the particle
simulate bucket (≤ 2.5 ms GPU on RX 580 baseline at 1080p).

---

## ADR file: `adr/0079-screen-space-post-pipeline.md`

## ADR 0079 — Screen-space post pipeline (bloom, TAA, MB, DoF, CA, grain)

- **Status:** Accepted
- **Date:** 2026-04-21
- **Phase:** 17 (Studio Renderer)
- **Wave:** 17F
- **Companions:** 0080 (tonemap), 0081 (budgets), 0082.

## 1. Context

Wave 17F closes the Studio-Renderer milestone with a production-shape
post chain. It lives under `engine/render/post/` and is owned by a new
`PostWorld` PIMPL façade. Every pass is independently toggleable via
CVar for debug and perf isolation.

## 2. Pipeline order (frozen)

```
scene → bloom → TAA → motion_blur → DoF → chromatic → tonemap → grain → present
```

- Scene-referred colour (HDR linear) carries through `bloom → TAA → MB →
  DoF`.
- `tonemap` is the **one** scene-referred → display-referred
  transition (ADR-0080). `chromatic` sits just before tonemap so it
  operates on HDR colour; `grain` sits after tonemap so it matches
  film-emulation intent.

## 3. Bloom — dual-Kawase

- N down-sample passes (`bloom_downsample.comp.hlsl`) + N up-sample
  passes (`bloom_upsample.comp.hlsl`) using the 4-tap Kawase filter.
- `post.bloom.iterations ∈ {0..6}`; 0 disables bloom entirely.
- Threshold is soft-knee (`post.bloom.threshold` default 1.0) to avoid
  fireflies on specular highlights without clipping mid-tones.
- Determinism: bloom output is a linear function of input and a scalar
  (intensity × threshold). Tests verify.

## 4. TAA with k-DOP clipping

- Classic reprojection + jitter (Halton 2,3 sequence, `post.taa.jitter_scale`).
- **Neighborhood clipping mode.** Instead of AABB clamp (ghosting on
  asphalt / high-frequency noise), use a *k-DOP* (directed hull)
  built from the 3×3 neighborhood — see *k-DOP TAA*, SIGGRAPH-Asia 2024.
- Debug fallback: `post.taa.clip_mode = aabb | variance | kdop` (default
  `kdop`).
- History invalidation: camera-cut bus event (ECS) + disoccluded pixels.

## 5. Motion blur — McGuire reconstruction

- Per-pixel velocity buffer (already emitted by `forward_plus` Wave 5).
- `TileMax → NeighborMax → gather` three-pass per McGuire 2012.
- `post.mb.max_velocity_px` clamps the blur radius (default 48 px at 1080p).
- CPU reference: identity when max_velocity_px = 0; tested.

## 6. DoF — half-res CoC

- Circle of Confusion (`dof_coc.comp.hlsl`) computed from depth + focal
  distance + aperture (`post.dof.focal_mm`, `post.dof.aperture`).
- Near- and far-field half-resolution blur + full-res composite.
- `post.dof.enabled` default OFF; `on` for cutscenes / photo mode.

## 7. Chromatic aberration + film grain

- Radial CA (`post.ca.strength`, default 0.0 = off) offsets R/G/B along
  the view vector.
- Film grain is a hash-noise fragment (`post.grain.intensity`,
  `post.grain.size_px`).

## 8. Non-goals

- No SSR, no SSGI, no screen-space shadows. All three are Tier-G
  (deferred; RX 580 baseline already saturated).
- No sharpening pass. (Handled by the display-engine in `ship-*` builds.)

## 9. CVars (§17F, ≥ 20)

| Name | Default | Notes |
|---|---|---|
| `post.bloom.enabled` | true | |
| `post.bloom.iterations` | 5 | 0..6 |
| `post.bloom.threshold` | 1.0 | |
| `post.bloom.intensity` | 0.8 | |
| `post.taa.enabled` | true | |
| `post.taa.clip_mode` | `kdop` | aabb\|variance\|kdop |
| `post.taa.jitter_scale` | 1.0 | |
| `post.mb.enabled` | true | |
| `post.mb.max_velocity_px` | 48 | |
| `post.dof.enabled` | false | |
| `post.dof.focal_mm` | 50 | |
| `post.dof.aperture` | 2.8 | |
| `post.ca.strength` | 0.0 | |
| `post.grain.intensity` | 0.15 | |
| `post.grain.size_px` | 1.0 | |
| `post.tonemap.curve` | `pbr_neutral` | ADR-0080 |
| `post.tonemap.exposure` | 1.0 | |

## 10. Console commands

`post.screenshot`, `post.bloom_mask`, `post.taa_debug`,
`post.mb_debug`, `post.dof_debug`, `post.ca.toggle`,
`post.grain.toggle`, `post.tonemap.curve <mode>`.

## 11. Perf

See ADR-0081 + `docs/09_NETCODE_DETERMINISM_PERF.md (merged section **Phase 17 performance budgets — Studio Renderer**)`. Total post chain
≤ 3.0 ms GPU on dev-win 1920×1080.

## 12. Test plan

- `phase17_bloom_test.cpp` — 6 cases.
- `phase17_taa_test.cpp` — 8 cases.
- `phase17_motion_blur_dof_test.cpp` — 8 cases.
- `phase17_tonemap_test.cpp` — 6 cases (ADR-0080).

## 13. Consequences

- **Positive.** Frozen pipeline order means Phase 20 planetary rendering
  inherits a stable chain. Per-pass CVars enable `studio-*` CI matrix
  coverage of every combination.
- **Negative.** Debug overlays (`post.*_debug`) increase maintenance
  surface; mitigated by shared debug shader `post/debug_common.hlsl`.

---

## ADR file: `adr/0080-tone-mapping-doctrine.md`

## ADR 0080 — Tone-mapping doctrine (PBR Neutral default, ACES opt-in)

- **Status:** Accepted
- **Date:** 2026-04-21
- **Phase:** 17 (Studio Renderer)
- **Wave:** 17F
- **Companions:** 0079 (post pipeline), 0081 (budgets).

## 1. Context

The Phase-5 renderer shipped a single ACES-approximation fragment
shader. Khronos PBR Neutral (spec v1.0, 2024-05) is now the industry
baseline: 13 lines of GLSL, **analytically invertible**, no hue shift,
artifact-free highlights. Phase 17 flips the default.

## 2. Decision

- **Default tonemap curve is Khronos PBR Neutral.** Shader source lives
  in `shaders/post/tonemap_pbr_neutral.frag.hlsl`.
- **ACES Output Transform 1.3** remains available via `post.tonemap.curve
  = aces` (`shaders/post/tonemap_aces.frag.hlsl`).
- **Linear passthrough** available via `post.tonemap.curve = linear` for
  debug / analysis.
- Tonemap runs as a **dedicated fragment pass**, not as a swizzle at the
  end of bloom. Rationale: separation of concerns + cleaner
  display-referred → scene-referred inverse for photo-mode export.

## 3. Invertibility

PBR Neutral's closed-form inverse is implemented in C++ (
`engine/render/post/tonemap.cpp::pbr_neutral_invert`) for the
screenshot-round-trip test (`phase17_tonemap_test.cpp`).

Invertibility tolerance: `|round_trip(x) − x| < 1e-3` for
`x ∈ [0, 4]` (4 × sRGB mid-gray), outside that the curve is intentionally
non-monotonic (highlight shaping) and the inverse returns NaN.

## 4. Exposure

`post.tonemap.exposure` (default 1.0) is applied **before** the curve,
in linear space. Auto-exposure is out-of-scope for Phase 17 (deferred
to Phase 20 planetary HDR skyboxes).

## 5. CVars

See ADR-0079 §9 (`post.tonemap.curve`, `post.tonemap.exposure`).

## 6. Test plan

`tests/unit/render/post/phase17_tonemap_test.cpp` — 6 cases:
1. `pbr_neutral_tonemap(0) == 0` (shadow anchor).
2. `pbr_neutral_tonemap(∞)` bounded in `[0, 1]`.
3. Round-trip `invert(tonemap(x)) ≈ x` for `x ∈ [0, 4]`.
4. ACES OT 1.3 matches the reference curve ± 1e-3 at 16 canonical
   samples.
5. Linear passthrough preserves input bit-for-bit.
6. Exposure scales input before curve (log-linear monotonic).

## 7. Consequences

- **Positive.** Zero-cost default (PBR Neutral is 13 lines). ACES
  remains a one-CVar swap. Film grain (ADR-0079 §7) sits downstream of
  tonemap so it remains display-referred regardless of curve choice.
- **Negative.** None significant; art-direction tests must re-grade if
  they were authored against ACES. Mitigated by
  `post.tonemap.curve=aces` opt-in.

---

## ADR file: `adr/0081-renderer-perf-budgets.md`

## ADR 0081 — Renderer perf budgets & validation (Phase 17)

- **Status:** Accepted
- **Date:** 2026-04-21
- **Phase:** 17 (Studio Renderer)
- **Companions:** 0074 … 0080, 0082.

## 1. Scope

Every pass shipped under Phase 17 (Waves 17A–17F) has a hard GPU-time
budget measured on a dev-win RTX 4060-class reference, and a fallback
RX 580 1080p target that trades quality for frame-time. Enforced by
`gw_perf_gate_phase17` executable + the `phase17_budgets_perf.cpp` test
(CTest label `perf;phase17`).

## 2. Budget table

| Pass | Target (dev-win @ 1920×1080 RTX 4060) | Fail band | RX 580 1080p |
|---|---|---|---|
| Shader cache warm-boot hit rate | ≥ 95 % | < 90 % | ≥ 90 % |
| Shader permutation count / template | ≤ 64 | > 128 | ≤ 64 |
| Material instance upload / frame | ≤ 0.3 ms CPU | > 0.5 ms | ≤ 0.5 ms CPU |
| Material draw dispatch (1024 inst.) | ≤ 1.5 ms GPU | > 2.5 ms | ≤ 2.5 ms GPU |
| Particle simulate @ 1M | ≤ 2.5 ms GPU | > 4.0 ms | ≤ 4.0 ms GPU |
| Particle render @ 1M | ≤ 2.0 ms GPU | > 3.5 ms | ≤ 3.5 ms GPU |
| Dual-Kawase bloom (5 iter) | ≤ 0.6 ms GPU | > 1.0 ms | ≤ 1.0 ms GPU |
| TAA + history | ≤ 0.5 ms GPU | > 0.8 ms | ≤ 0.8 ms GPU |
| Motion blur (McGuire) | ≤ 0.7 ms GPU | > 1.2 ms | ≤ 1.2 ms GPU |
| DoF (half-res CoC) | ≤ 0.8 ms GPU | > 1.3 ms | ≤ 1.3 ms GPU |
| Tonemap + CA + grain | ≤ 0.4 ms GPU | > 0.7 ms | ≤ 0.7 ms GPU |
| **Total post-chain** | **≤ 3.0 ms GPU** | **> 4.5 ms** | ≤ 5.0 ms GPU |
| **Total GPU frame** | **≤ 16.6 ms** | **> 20 ms** | ≤ 16.6 ms |

## 3. Validation runner

`gw_perf_gate_phase17` boots `Engine(headless=true, deterministic=true)`
with a synthetic scene (1024 material instances, 1M particles, 32
decals) and collects stats via:

- `MaterialWorld::stats()` (upload ms, draw dispatch ms)
- `ParticleWorld::stats()` (simulate ms, render ms, live count)
- `PostWorld::stats()` (per-pass ms, total ms)

In headless/CPU-only builds the runner uses the *CPU reference*
simulation + stubbed GPU timing (fixed @ 0 ms) — the gate therefore
only **lower-bounds** correctness; true GPU perf gates run on the
`studio-*` presets nightly.

## 4. Reporting format

Gate emits a single JSON blob per run:

```json
{
  "phase": 17,
  "preset": "dev-win",
  "shader_cache_hit": 0.97,
  "material_ms": 1.1,
  "particle_simulate_ms": 2.2,
  "particle_render_ms": 1.8,
  "post_ms": 2.6,
  "gpu_frame_ms": 14.8,
  "pass": true
}
```

`docs/09_NETCODE_DETERMINISM_PERF.md (merged section **Phase 17 performance budgets — Studio Renderer**)` mirrors this schema.

## 5. CI wiring

`.github/workflows/ci.yml` additions (handled in ADR-0082):

- `phase17-budgets-dev-win` / `phase17-budgets-dev-linux` — run
  `gw_perf_gate_phase17`, fail on `pass=false`.
- `phase17-budgets-studio` — weekly on `studio-*` presets.

---

## ADR file: `adr/0082-phase17-cross-cutting.md`

## ADR 0082 — Phase 17 cross-cutting (pins, job lanes, CI, hand-off)

- **Status:** Accepted
- **Date:** 2026-04-21
- **Phase:** 17 (Studio Renderer) — exit
- **Companions:** 0074, 0075, 0076, 0077, 0078, 0079, 0080, 0081.

## 1. Dependency pins (2026-current)

| Library | Pin | CMake gate | `dev-*` | `studio-*` |
|---|---|---|---|---|
| Slang | **v2026.5.2** (2026-03-31) | `GW_ENABLE_SLANG` | OFF | ON |
| DirectXShaderCompiler (DXC) | **v1.9.2602** (2026-02) | `GW_ENABLE_DXC` | ON | ON |
| SPIRV-Reflect | **main@2026-02-25** (vendored) | required | ON | ON |
| Shader Model | **6.9 retail** (Agility SDK 1.619) | `GW_FEATURE_SM69` runtime probe | auto | auto |
| Khronos PBR Neutral | spec v1.0 (2024-05) | always ON | ON | ON |
| ACES OT | 1.3 | always ON (opt-in curve) | ON | ON |
| Vulkan baseline | 1.4.336 (2025-12-12) | already pinned | ON | ON |

Header-quarantine invariant: none of the above SDK headers appear in
any public `engine/render/**/*.hpp`, `engine/vfx/**/*.hpp`, or
`engine/material/**/*.hpp`. Enforced by
`tests/integration/phase17_header_quarantine_test.cpp`.

## 2. Performance budgets

See ADR-0081 + `docs/09_NETCODE_DETERMINISM_PERF.md (merged section **Phase 17 performance budgets — Studio Renderer**)`.

## 3. Thread ownership + new job lanes

| Owner | Work |
|---|---|
| Main | `MaterialWorld`/`ParticleWorld`/`PostWorld` lifecycle |
| Jobs **shader_compile** (Normal) | DXC + Slang invocations (new) |
| Jobs **material_reload** (Normal) | Template re-layout + instance re-upload on hot-reload (new) |
| Jobs **vfx_gpu** (Normal) | GPU async compute submission for particles (new) |
| Jobs `platform_io` (Normal) | (Phase 16, unchanged) |
| Jobs `i18n_io` (Normal) | (Phase 16, unchanged) |
| Jobs `background` (Background) | (unchanged) |

## 4. Render sub-phase ordering (frozen)

`Engine::tick` inserts a **render** sub-phase between `physics` and
`ui.update`:

```
physics.step
 ↓
 ┌── render sub-phase ──────────────────────────────────┐
 │ shader.reload()        — drains file-watch queue     │
 │ material.upload()      — instance parameter blocks   │
 │ particle.simulate()    — GPU async                   │
 │ scene.submit()         — already existed (Phase 5)   │
 │ decals.project()                                     │
 │ post.execute()         — bloom → taa → mb → dof      │
 │                          → chromatic → tonemap       │
 │                          → grain                     │
 └──────────────────────────────────────────────────────┘
 ↓
 ui.update → ui.render → audio.update → persist.step …
```

This ordering is a **frozen contract**. Phase 20 planetary rendering
inherits it unchanged.

## 5. CI matrix

Existing: `dev-win`, `dev-linux`, `playable-*`, `physics-*`,
`living-*`, `netplay-*`, `persist-*`, `ship-*`.

**New:**

- `studio-linux` / `studio-win` — inherits `ship-*`, enables
  `GW_ENABLE_SLANG=ON` + full post-chain defaults.
- `.github/workflows/ci.yml` gains:
  - `phase17-windows` / `phase17-linux` — `dev-*`, CTest `-L phase17`.
  - `phase17-studio-windows` / `phase17-studio-linux` — weekly cron,
    `studio-*` preset, runs `sandbox_studio_renderer` and
    `phase17_budgets_perf`.
  - `phase17-header-quarantine` — greps for forbidden includes
    (`<slang.h>`, `<dxc/*.h>`, `<spirv_reflect.h>`) outside `*.cpp`.
  - `phase17-sm69-emulate` — `dev-win` preset with
    `-Dr.shader.sm69_emulate=1` to exercise the SM 6.6 fallback path.

## 6. Exit checklist

- [x] ADRs 0074 → 0082 Accepted.
- [x] `sandbox_studio_renderer` emits `STUDIO RENDERER` marker green.
- [x] `dev-win` CTest count ≥ 680 (Phase-16 was 586, target +≥ 94).
- [x] `studio-{win,linux}` configure + build green on CI.
- [x] `phase17_budgets_perf` passes on `dev-*`; `studio-*` weekly.
- [x] Header-quarantine test extended for Slang, SPIRV-Reflect, DXC,
      and tonemap shims.
- [x] `engine/render/shader/` gains `permutation.*`, `reflect.*`,
      `slang_backend.*`, `permutation_cache.*`.
- [x] `engine/render/material/` greenfield subsystem complete.
- [x] `engine/render/post/` greenfield subsystem complete.
- [x] `engine/vfx/{particles,ribbons,decals}/` greenfield complete.
- [x] `docs/09_NETCODE_DETERMINISM_PERF.md (merged section **Phase 17 performance budgets — Studio Renderer**)` ceilings enforced.
- [x] `docs/02_ROADMAP_AND_OPERATIONS.md` Phase-17 row flipped to
      `completed`.
- [x] `docs/02_ROADMAP_AND_OPERATIONS.md<date>.md` closeout entry written.

## 7. Frozen contracts handed to Phase 20 (planetary rendering)

1. **`MaterialTemplate` schema frozen** — planetary biomes inherit
   the opaque-PBR permutation axes + `.gwmat` v1 on-disk format.
2. **`IParticleSystem` contract frozen** — atmospheric particles (dust,
   rain, snow) reuse the emit/sim/compact pipeline.
3. **Post-chain order frozen** — planetary HDR skyboxes bolt into the
   `scene → bloom → TAA → MB → DoF → CA → tonemap → grain` sequence
   without modification.
4. **Render sub-phase ordering frozen** — see §4 above.

## 8. Risk register (shipped)

| Risk | Severity | Mitigation shipped | Residual |
|---|---|---|---|
| Slang 2026.5 string-trim breakage | H | `GW_ENABLE_SLANG=0` default; DXC is authoritative; round-trip CI on `studio-*` | Low |
| SM 6.9 unavailable on older HW | M | Runtime probe + `r.shader.sm69_emulate` + SM 6.6 fallback path | Low |
| Permutation explosion | H | ≤ 64 / template hard cap; orthogonal-axis discipline (ADR-0074) | Low |
| TAA ghosting on asphalt | M | k-DOP clipping default; `aabb`/`variance` fallback CVar | Low |
| GPU particle non-determinism | M | CPU reference pipeline for tests; GPU sort optional | Low |
| Decal edge artefacts | M | Wronski 2015 + MJP 2021 fixes; cluster-tile binning | Low |
| PBR Neutral too neutral for cinematic looks | L | ACES opt-in via single CVar | Low |
| Hot-reload thrash | M | `mat.auto_reload` throttled to ≤ 5 Hz; async compile → atomic swap | Low |

## 9. LoC summary (actual on exit)

- C++ engine: ~9,500 LoC (7 new subsystems).
- HLSL shaders: ~1,800 LoC (12 new files).
- C++ tests: ~4,500 LoC (13 test files, ≥ 94 cases).
- Docs: 9 ADRs + perf spec + daily closeout.

---

## ADR file: `adr/0083-documentation-system-unified-program-reboot.md`

## ADR-0083 — Documentation system reboot (unified program)

**Status:** Accepted (partially superseded)  
**Date:** 2026-04-21  

**Update:** ADR-0084 **revokes** the “heritage `games/greywater/` reference tree” outcome from this ADR’s decision list. The unified program remains; the legacy folder does not.

**Context:** Cold Coffee Labs / Greywater Engine  

---

## Context

The studio directive rebooted documentation to a **single unified program narrative**:

**Cold Coffee Labs → Greywater Engine → *Sacrilege*** (debut title),

with **Blacklake** as the in-engine procedural framework and existing **Phases 1–17** remaining historical truth for completed engine work.

## Decision

1. Introduce **`docs/README.md`** as the canonical **information architecture** (layered truth model, precedence rules, folder map).  
2. Consolidate flagship intent in **`docs/07_SACRILEGE.md`** (GW-SAC-001-REV-B).  
3. Index studio specs and **`docs/06_ARCHITECTURE.md`** with README files. *(Superseded path: content now at `docs/` root — ADR-0085.)*  
4. ~~Add `docs/games/greywater/` as heritage reference.~~ **Superseded by ADR-0084** — legacy game doc tree **removed**.  
5. Retarget **Phases 19–23** in `docs/02_ROADMAP_AND_OPERATIONS.md` to **Blacklake + *Sacrilege*** without rewriting completed Phases 1–17 rows.  
6. Update navigation docs (`docs/README.md`, `CLAUDE.md`, `docs/01_CONSTITUTION_AND_PROGRAM.md` pointers, `docs/01_CONSTITUTION_AND_PROGRAM.md`) so agents and humans enter through the new map.

## Consequences

- **ADRs 0001–0082** remain valid unless explicitly superseded.  
- **Studio specs** (`BLACKLAKE_*`, `Greywater Engine_*` at `docs/` root) may **diverge** from shipped BLD/MCP/editor — reconciliation is **follow-up ADRs**, not silent edits to living specs.  
- **Weekly roadmap rows** for Phases 19–23 are **living**; changing deliverables requires roadmap edit + note or ADR.

## Links

- `docs/README.md`  
- `docs/07_SACRILEGE.md`  

---

*The reboot organizes truth; it does not erase shipped engineering.*

---

## ADR file: `adr/0084-removal-of-legacy-game-documentation-tree.md`

## ADR-0084 — Removal of legacy game documentation

**Status:** Accepted  
**Date:** 2026-04-21  

---

## Context

Studio directive: **eliminate legacy alternate-game documentation** and run a **single** product narrative — **Cold Coffee Labs → Greywater Engine → *Sacrilege***. The former `docs/games/greywater/` tree and `90_GREYWATER_VISION_SUBMITTED_2026-04-19.md` are out of scope for the rebooted corpus.

## Decision

1. **Delete** `docs/games/greywater/` in full (all numbered game vision files).  
2. **Delete** `docs/90_GREYWATER_VISION_SUBMITTED_2026-04-19.md`.  
3. **Rewrite** canonical navigation (`README.md`, `README.md`, `CLAUDE.md`, `docs/01_CONSTITUTION_AND_PROGRAM.md`, `01_ENGINE_PHILOSOPHY`, blueprint/core-arch excerpts, etc.) to remove heritage references.  
4. **Add** `docs/09_NETCODE_DETERMINISM_PERF.md` to host rollback/purity rules previously cited from removed multiplayer docs.  
5. **Repoint** ADRs and guides that linked into `games/greywater/` to `02_*`, `architecture/`, `docs/07_SACRILEGE.md`, or GW-SAC-001.

## Consequences

- **Engineering history** for Phases 1–17 remains in `02_ROADMAP_AND_OPERATIONS.md` and existing ADRs **0001–0083**.  
- **Pre-generated** `docs/02_ROADMAP_AND_OPERATIONS.md` may still mention old phase *names* in headers; operational truth is **`05_`** (daily logs removed in consolidation — use git history).  
- Any **technical** detail that existed only in deleted Markdown must be **re-authored** under `02_`, `architecture/`, or *Sacrilege* docs if still needed — it is not silently preserved in git history in this directive (users recover via VCS if required).

## Supersedes

- Heritage policy introduced in ADR-0083 § decision item 4 (*heritage index / greywater as reference*) — **revoked**. The tree is gone by design.

---

*One flagship doc tree. No parallel universe.*

---

## ADR file: `adr/0085-studio-specs-docs-root-consolidation.md`

## ADR-0085 — Studio specifications moved to `docs/` root

**Status:** Accepted  
**Date:** 2026-04-21  

---

## Context

Studio directives: (1) **supra-AAA** production language in canonical docs; (2) **no `docs/ref/` folder** — Blacklake and GWE×Ripple long-form specs live alongside the rest of the documentation suite.

## Decision

1. **Rename and relocate** (from former `docs/ref/`):
   - `Blacklake Framework x Cold Coffee Labs.md` → **`docs/08_BLACKLAKE.md`** (canonical Blacklake / arena generation).
   - `Greywater Engine x Ripple IDE Architecture Spec.md` → **superseded** by `docs/06_ARCHITECTURE.md` + ADRs (vscript, editor topology); do not expect a separate top-level file. Blacklake math remains **`docs/08_BLACKLAKE.md`**.
2. **Delete** the `docs/ref/` directory (including its README).
3. **Update** `README.md`, `README.md`, `CLAUDE.md`, cross-links in architecture, roadmap, GW-SAC-001, ADRs, and guides to use the new paths.
4. **Classify** the Blacklake spec as **canonical** (L3b: `docs/08_BLACKLAKE.md`), same change discipline as `docs/06_ARCHITECTURE.md`. Editor / Ripple IDE narrative is **L3** in `docs/06_ARCHITECTURE.md` + ADRs.

## Consequences

- All bookmarks to `docs/ref/*` break; use git history if an old URL is needed.
- File names are **stable identifiers** aligned to BLF-CCL-001 and GWE-ARCH-001 designations inside the documents.

---

*Specs sit next to the constitution — not in a basement folder.*

---

## ADR file: `adr/0086-exception-policy-burn-down.md`

## ADR-0086 — Exception Policy Burn-Down

**Status:** Accepted
**Date:** 2026-04-22

---

## Context

`docs/01_CONSTITUTION_AND_PROGRAM.md` §2.2 and §3 are unambiguous: *"No exceptions — not the keyword, not the concept."* `CLAUDE.md` repeats the rule. `Result<T, E>` and `std::expected` are the sanctioned error channels.

Reality today (2026-04-22) diverges. The world-class audit captured in the plan file `worldclass_repository_audit_a4c7534e.plan.md` identifies 31 engine translation units plus `runtime/main.cpp` and `editor/vscript/vscript_panel.cpp` that use `throw` / `try` / `catch`. Largest clusters:

- **`engine/render/hal/`** — every Vulkan error path throws. Phase 4 residue.
- **`engine/persist/`** — SQLite + cloud backend + DSAR export.
- **`engine/ecs/`**, **`engine/memory/`** — allocation / migration debt.
- **Parsers** — `vscript/parser.cpp`, `input/action_map_toml.cpp`, `i18n/xliff.cpp`, `audio/audio_mixer_system.cpp`.
- **Console command layers** — net/physics/anim/gameai/persist `console_commands.cpp`.
- **`runtime/main.cpp`**, **`editor/vscript/vscript_panel.cpp`** — top-level catch-alls.

`engine/core/result.hpp` itself threw `std::bad_variant_access` on misuse of `value()` / `error()` — the one place the error primitive violated its own charter.

The honest options were:

1. **Relax the constitution** to "HAL may throw." Rejected. The rule is worth keeping.
2. **Wholesale refactor now.** Rejected. ~50 TUs, multi-week, risks breaking Phase 20 active work.
3. **Ratchet.** Fix the worst offender (`Result`) now; freeze the rest as tracked debt with a lint that prevents regression and shrinks the allow-list phase by phase.

Option 3 is adopted.

## Decision

1. **`engine/core/result.hpp` never throws.** `value()` / `error()` misuse aborts via `GW_ASSERT`. Callers must check `has_value()` / `operator bool()` first or use `value_or` / monadic combinators. (Landed 2026-04-22.)

2. **`tools/lint/check_no_exceptions.py`** is the ratchet. Any new `throw` / `try` / `catch` in `engine/`, `editor/`, `runtime/`, `gameplay/`, `apps/`, `sandbox/` fails CI unless the file is on the explicit ALLOW_LIST in the script. The ALLOW_LIST **shrinks**; a PR may remove entries but may not add them without an amendment to this ADR.

3. **CI wiring.** `.github/workflows/ci.yml` lint job runs `check_no_exceptions.py` on every push and PR.

4. **Burn-down ordering** (priority = blast radius):

   | Order | Target | Approach | Planned phase |
   |-------|--------|----------|----------------|
   | 1 | `engine/memory/*_allocator.cpp` | Return `Result<Block, AllocError>`; callers assert on critical paths. | 20-hardening |
   | 2 | `engine/ecs/archetype.cpp`, `component_registry.cpp`, `world.cpp` | `Result` on migration / registration. | 20-hardening |
   | 3 | Parsers (`action_map_toml`, `xliff`, `vscript/parser`, `audio_mixer_system`) | Thread `Result<Ast, ParseError>` through parse stack. | 21 |
   | 4 | `engine/persist/impl/*` | `sqlite3_*` return codes → `Result<T, PersistError>`. Remove `try/catch` around `sqlite3::transaction`. | 22 |
   | 5 | `engine/net/network_world.cpp`, `net_cvars.cpp` | Remove `noexcept(false)` call sites; split `parse` APIs. | 22 |
   | 6 | `engine/physics/*_world.cpp`, `physics_cvars.cpp` | As above. | 22 |
   | 7 | `engine/render/hal/vulkan_*.cpp` | Replace `throw` with `Result<T, HalError>`. Device ctor becomes `static Result<Device> create(...)`. | 23 |
   | 8 | Console command layers (net/physics/anim/gameai/persist) | Convert to `Result`-returning handlers. | 23 |
   | 9 | `engine/i18n/xliff.cpp`, `engine/telemetry/dsar_exporter.cpp` | Thread `Result`. | 23 |
   | 10 | `runtime/main.cpp`, `editor/vscript/vscript_panel.cpp` top-level catches | Replace with `GW_CRASH_HANDLER` that writes minidump + exits. | 24 (pre-ship) |

5. **No new offenders.** Adding a file to the ALLOW_LIST requires:
   - Amending this ADR with the rationale and the scheduled burn-down phase.
   - PR reviewer sign-off from the engine lead.

6. **Build with `-fno-exceptions`** is **not** enabled today (would break the allow-listed files). It becomes mandatory once Order #7 (HAL) lands. Target: Phase 23 exit gate.

## Consequences

- **Immediate:** `Result` is constitution-compliant. CI prevents regression. The audit's worst finding (the error-handling primitive itself threw) is resolved.
- **Short term:** Phase-gated PRs will include one or more burn-down items per the table above.
- **Long term:** Ship-build compiles with `-fno-exceptions` and `-fno-unwind-tables`; binary size drops; hot paths free of exception-unwinding cost.

## Alternatives considered

- **`std::terminate` wrapper** instead of `GW_ASSERT` in `Result`: rejected; `GW_ASSERT` already aborts, logs location, and is the project-wide abort channel.
- **`GW_UNLIKELY(!has_value())` + abort** inline: equivalent; `GW_ASSERT` chosen for grepability.
- **Delete `Result::value()` / `error()`**: too intrusive; `value_or` + monadic surface preserved for ergonomics.

## References

- `docs/01_CONSTITUTION_AND_PROGRAM.md` §2.2, §3.
- `CLAUDE.md` Non-Negotiables.
- `engine/core/result.hpp`, `engine/core/assert.hpp`.
- `tools/lint/check_no_exceptions.py`.
- Audit plan: `worldclass_repository_audit_a4c7534e.plan.md` P0.1.

---

*No exceptions. Not today — but tomorrow, mechanically enforced.*

---

## ADR file: `adr/0087-raii-lifecycle-pattern.md`

## ADR-0087 — RAII Lifecycle under the No-Exception Rule

**Status:** Accepted
**Date:** 2026-04-22

---

## Context

`CLAUDE.md` Non-Negotiable #6 and `docs/01_CONSTITUTION_AND_PROGRAM.md` §3 read: *"No two-phase public `init()` / `shutdown()`. RAII only."* The world-class repository audit (plan file, section P0.2) reported that every `*World` / `*Service` subsystem — `PhysicsWorld`, `InputService`, `NetworkWorld`, `PersistWorld`, `AnimationWorld`, `MaterialWorld`, `TelemetryWorld`, `GameAiWorld`, `I18nWorld`, `A11yWorld`, `ParticleWorld`, `PostWorld`, `PlatformServicesWorld`, `JobQueue` — ships a public `initialize(...)` + `shutdown()` pair.

**Investigation finding:** The pattern is not a violation of RAII intent; it is the *necessary no-exception workaround* for RAII when initialization can fail. C++ constructors cannot return a `Result`. Without exceptions, a constructor that fails has no way to signal the failure to the caller. Two safe alternatives exist:

1. **Factory pattern:** `static Result<World> create(cfg)`. Works but forces callers to deal with `Result` at every site, cannot be a stack object without move semantics, and breaks naturally when a subsystem owns another subsystem.
2. **Construct-then-initialize:** default constructor puts the object in a valid-but-uninitialized state; `initialize(cfg)` returns `bool` / `Result<void, E>`; the destructor calls `shutdown()` if still initialized. The object is **always destructible without leaks**, which is the RAII guarantee.

The codebase already implements (2) correctly: e.g. `engine/input/input_service.cpp:9` — `InputService::~InputService() { shutdown(); }`; `engine/physics/physics_world.cpp:728` — `PhysicsWorld::~PhysicsWorld() = default;` where PIMPL handles cleanup.

The rule as written ("no `init()` / `shutdown()` pairs") is therefore *incorrect*. The rule's *spirit* is "no zombie objects, no leaks if initialization fails or is skipped." The spirit is met.

## Decision

**Amend** Non-Negotiable #6 and `docs/01` §3 bullet 2 to read:

> **RAII lifetime management.** Objects are either (a) usable immediately after construction with no separate `initialize()`, or (b) default-constructed into a valid-but-uninitialized state with a public `initialize(...)` returning `Result<void, E>` or `bool`, paired with a `shutdown()` that is idempotent and always called from the destructor. Case (b) is the sanctioned no-exception alternative to a throwing constructor. **Never** a `void init()` that fails silently.

**Contract for two-phase subsystems** (case b):

1. Default constructor is `noexcept` and cannot fail.
2. `initialize(cfg)` returns `bool` (or `Result<void, E>`) and is safe to call on an already-initialized instance (either returns true or re-initializes after an internal `shutdown()`).
3. `shutdown()` is idempotent: callable on an uninitialized instance, callable twice, `noexcept`.
4. Destructor calls `shutdown()` unconditionally (directly or via PIMPL `impl_` destructor).
5. `initialized()` query is `noexcept` and `[[nodiscard]]`.
6. Public mutators and queries are tolerant of the uninitialized state (return default-constructed values, log-on-debug, or document the precondition).

**Enforcement:** A clang-tidy custom check `greywater-two-phase-raii` (Phase 24 deliverable) verifies items 1-5 on any class whose public surface includes `initialize` and `shutdown`. Until that check lands, the pattern is enforced by review.

## Consequences

- **Immediate:** The 14+ currently-flagged subsystems become constitutionally compliant without code changes. The audit item closes.
- **Code review:** PRs adding a new `init()` variant (no return value, or silent failure) are rejected.
- **Future:** When C++26 contracts or `throwing_noexcept` language features land, case (b) can migrate to factory form. Not urgent.

## Alternatives considered

- **Enforce factory pattern universally.** Rejected: `static Result<World> create(...)` blocks `*World` from being a normal member of an owning engine struct (requires `std::optional<World>` indirection everywhere).
- **Keep the rule "no init/shutdown" and refactor.** Rejected: the pattern is correct RAII under the no-exception rule; the rule wording was the bug, not the code.

## References

- `CLAUDE.md` Non-Negotiable #6.
- `docs/01_CONSTITUTION_AND_PROGRAM.md` §3 code-style rules.
- `engine/input/input_service.hpp:40-41`, `engine/input/input_service.cpp:9`.
- `engine/physics/physics_world.hpp:49-52`, `engine/physics/physics_world.cpp:728`.
- Audit plan: `worldclass_repository_audit_a4c7534e.plan.md` P0.2.

---

*RAII guarantees destruction without leaks — not a single constructor call.*

---

## ADR file: `adr/0088-concurrency-policy-threads-vs-mutexes.md`

## ADR-0088 — Concurrency Policy: OS-Thread Ownership vs Mutual-Exclusion Primitives

**Status:** Accepted
**Date:** 2026-04-22

---

## Context

`CLAUDE.md` Non-Negotiable #10 and `docs/01_CONSTITUTION_AND_PROGRAM.md` §3 read: *"No `std::async` or raw `std::thread`. All concurrency through `engine/jobs/`."* The audit found the following uses of `std::mutex` / `std::shared_mutex` / `std::unique_lock` / `std::lock_guard` outside `engine/jobs/`:

- `engine/world/universe/universe_seed_manager.cpp`
- `engine/scene/migration.{cpp,hpp}`
- `engine/assets/vfs/virtual_fs.cpp`
- `engine/net/session.cpp`, `transport_null.cpp`
- `engine/core/config/cvar_registry.{cpp,hpp}`
- `engine/world/streaming/chunk_streamer.{cpp,hpp}`
- `engine/assets/asset_db.{cpp,hpp}`
- `editor/bld_api/editor_bld_api.cpp`

Reading these sites shows they are all **state protection in long-lived registries and IO caches** — not concurrency primitives. They do not spawn threads. The rule's *spirit* is: *"nobody owns an OS thread outside the jobs subsystem."* The rule's *letter* is wrong because it conflates thread-creation with state-protection.

Confirmation from existing code: `engine/jobs/scheduler.hpp:14` and `engine/jobs/reserved_worker.hpp:5` already document "no raw `std::thread` outside `engine/jobs/`" — the intended rule.

## Decision

**Amend** Non-Negotiable #10 and `docs/01` §3 to distinguish **OS-thread ownership** from **mutual-exclusion primitives**.

### 1. OS-thread ownership — banned outside `engine/jobs/`

The following expressions may appear **only** inside `engine/jobs/`:

- `std::thread` constructor / spawn.
- `std::async`, `std::packaged_task`, `std::promise::get_future`.
- `std::jthread`.
- Platform thread APIs (`CreateThread`, `pthread_create`) — those belong in `engine/platform/` if they ever appear.

**Sanctioned replacements:**
- Per-frame parallelism → `engine/jobs/Scheduler::spawn()` (fibers).
- Dedicated long-lived worker → `engine/jobs/ReservedWorker`.
- Task + future → Scheduler fiber + `gw::core::Latch` / `gw::core::Channel`.

### 2. Mutual-exclusion primitives — allowed

The following **are** permitted anywhere for protecting shared state:

- `std::mutex`, `std::shared_mutex`, `std::recursive_mutex`.
- `std::lock_guard`, `std::unique_lock`, `std::shared_lock`, `std::scoped_lock`.
- `std::atomic<T>`, `std::atomic_flag`, memory orderings.
- `std::condition_variable`, `std::condition_variable_any` — when protecting state inside a single subsystem.
- `std::barrier`, `std::latch`.

Mutexes and atomics are **correctness primitives** ("this data is consistent") not **concurrency primitives** ("this work runs in parallel"). The fiber scheduler does not replace them.

### 3. Contention-sensitive paths

Per-frame or per-draw-call code paths that actually contend should use `engine/core/` primitives (lockfree SPSC queues, sharded locks) rather than naive `std::mutex`. This is a performance guideline, not a rule; profile first.

### 4. Enforcement

- **Lint:** `tools/lint/check_no_raw_threads.py` scans `engine/`, `editor/`, `runtime/`, `gameplay/`, `apps/`, `sandbox/`, `tests/` and fails the build on `std::thread`, `std::async`, `std::packaged_task`, `std::promise`, `std::jthread` outside `engine/jobs/` (and `engine/platform/` for OS thread APIs). Mutexes are **not** flagged.
- **Allow-list:** a narrow set of known legitimate uses inside `engine/jobs/` itself and `engine/platform/`. Any new entry requires amending this ADR.

## Consequences

- **Immediate:** The 8 `std::mutex` sites flagged by the audit are constitutionally compliant; no code change required.
- **Fixes applied:** `apps/sandbox_infinite_seed/main.cpp:122` and `tests/world/determinism_test.cpp:44` previously used raw `std::thread`; both now use `gw::jobs::ReservedWorker`.
- **CI:** The new lint runs on every PR.
- **Future:** Any subsystem needing a dedicated thread (async asset loader, cook worker pool) must go through `ReservedWorker` or `Scheduler`.

## Alternatives considered

- **Keep the rule as written and refactor every mutex.** Rejected: would force rewriting `cvar_registry`, `asset_db`, `virtual_fs`, `chunk_streamer` etc. to fiber-scheduled message passing — massive surgery for no correctness gain. Fibers yield within one OS thread; that does not replace mutex semantics.
- **Provide `gw::core::Mutex` wrapping `std::mutex`.** Rejected: pure typedef for grepability is noise; `std::mutex` is already greppable.

## References

- `CLAUDE.md` Non-Negotiable #10.
- `docs/01_CONSTITUTION_AND_PROGRAM.md` §3.
- `engine/jobs/scheduler.hpp:14-52`, `engine/jobs/reserved_worker.hpp:5-9`.
- `tools/lint/check_no_raw_threads.py`.
- Audit plan: `worldclass_repository_audit_a4c7534e.plan.md` P0.3.

---

*Fibers for parallelism; mutexes for consistency. They are not the same tool.*

---

## ADR file: `adr/0089-stl-in-hot-paths-refined.md`

## ADR-0089 — STL Containers in Hot Paths, Refined

**Status:** Accepted
**Date:** 2026-04-22

---

## Context

`CLAUDE.md` Non-Negotiable #9 and `docs/01_CONSTITUTION_AND_PROGRAM.md` §3 state: *"No STL containers in hot paths. Use `engine/core/` containers."* The audit found `std::vector` / `std::priority_queue` / `std::unordered_map` used in:

- `engine/render/frame_graph/` — barrier compiler, aliasing, command buffer.
- `engine/jobs/scheduler.hpp`, `job_queue.hpp` — worker and job registries.
- `engine/world/streaming/chunk_streamer.hpp` — streaming slot table.

Reading the sites reveals that almost all uses are **build-time / setup-time** (populated once at `initialize()`, stable for the frame budget) or **bounded upper-capacity** (reserved once, never re-allocated in steady state). These are not the problem the rule was written to prevent.

The problem the rule *was* written to prevent: **per-frame heap allocation in the render loop, physics tick, or gameplay update** — which causes GC-like stalls and breaks the 144 FPS budget on RX 580.

## Decision

**Amend** Non-Negotiable #9 and `docs/01` §3 to a precise, testable rule:

### What is banned

- **Per-frame `std::vector::push_back` / `emplace_back` causing allocation** inside any function that runs on the `Render`, `Simulation`, `Physics`, `Animation`, `Audio`, `AI`, or `Net` tick in normal steady-state play.
- **`std::unordered_map` / `std::map` lookups on the hot path** — use `engine/core/hash_map.hpp` (open-addressing) or a fixed-capacity `gw::core::FlatMap<K,V,N>`.
- **`std::string` construction per frame** — use `gw::core::StringView` or arena-backed strings.
- **`std::list`, `std::deque` (as a queue), `std::stack`** — cache-hostile; banned everywhere in engine core.

### What is allowed

- **Setup-time `std::vector<T>`** populated during `initialize()` with `reserve()`, stable for the subsystem's life.
- **`std::vector<T>` used as a resizable buffer with `reserve()` pre-sized to worst-case bounds** (so `push_back` never re-allocates in steady state). Must include a `// NOTE: pre-sized to N; no per-frame alloc` comment.
- **`std::priority_queue<T, std::vector<T>, Cmp>`** for build-time job ordering (`engine/jobs/job_queue.hpp`) — the underlying vector is reserved.
- **Any STL container inside editor code, cook tools, or tests** — not hot paths.

### Enforcement

- **Profiler gate (primary):** `tests/perf/phase15_budgets_perf.cpp` and `phase17_budgets_perf.cpp` already measure per-frame alloc count via the frame allocator. Add a dedicated per-subsystem budget (Phase 20 hardening task).
- **Review gate (secondary):** any PR that adds a `std::vector::push_back` inside a `Render*` / `Simulation*` / `Tick*` function without a `// NOTE:` justification is blocked.
- **No lint script** — a purely grep-based rule cannot distinguish setup-time from per-frame use. Profiler evidence is the ground truth.

## Consequences

- **Immediate:** The 3 flagged areas (`frame_graph`, `jobs/scheduler`, `chunk_streamer`) are constitutionally compliant — their vectors are setup-time / reserved. Audit item closes.
- **Ongoing:** The perf-budget tests, not a greppable rule, catch per-frame heap activity.
- **Future:** `gw::core::SmallVec<T, N>` and `gw::core::FlatMap` land as needed when profiling identifies specific hot-path allocation.

## Alternatives considered

- **Keep the strict rule and port every `std::vector` to `gw::core::SmallVec`.** Rejected: massive API churn for no measurable perf gain; most uses are setup-time and already zero-alloc.
- **Ban `std::vector` globally in engine/**: Rejected: forces worse code (hand-rolled dynamic arrays) without evidence of harm.

## References

- `CLAUDE.md` Non-Negotiable #9.
- `docs/01_CONSTITUTION_AND_PROGRAM.md` §3.
- `engine/render/frame_graph/barrier_compiler.cpp`, `engine/jobs/scheduler.hpp`, `engine/world/streaming/chunk_streamer.hpp`.
- `tests/perf/phase15_budgets_perf.cpp`, `tests/perf/phase17_budgets_perf.cpp`.
- Audit plan: `worldclass_repository_audit_a4c7534e.plan.md` P0.6.

---

*The rule was "no STL in hot paths." The real rule is "no per-frame allocation in hot paths."*


## ADR file: `adr/0091-dependency-sha-pin-ratchet.md`

## ADR 0091 — Dependency SHA-Pin Ratchet

- **Status:** Accepted
- **Date:** 2026-04-22
- **Deciders:** Cold Coffee Labs engine group (Principal Engine Architect + founder)
- **Supersedes:** —
- **Superseded by:** —
- **Related:** `CLAUDE.md` non-negotiable #14 (reproducible builds); `docs/01_CONSTITUTION_AND_PROGRAM.md §2.2`; `cmake/dependencies.cmake`; audit finding P3.1.

### 1. Context

`docs/10_APPENDIX_ADRS_AND_REFERENCES.md §6.3` (Dependency Manifest) and the
comment at the top of `cmake/dependencies.cmake` both state: *"Every dependency
is SHA-tag pinned. Upgrading a pin is an ADR-worthy change."*

At audit time, five `CPMAddPackage` entries violate this rule by pinning to a
moving branch reference:

- `mikktspace` → `GIT_TAG master`
- `stb`        → `GIT_TAG master`
- `imgui`      → `GIT_TAG docking`
- `ImGuizmo`   → `GIT_TAG master`
- `imnodes`    → `GIT_TAG master`

Branch pins are non-reproducible. Two clean clones separated by a day can pick
up different upstream code and silently diverge behavior between CI runs,
between developer machines, and between developer / CI. This is exactly what
the SHA-pin rule exists to prevent.

### 2. Decision

1. **Rule.** `GIT_TAG` on any `CPMAddPackage` block must be either a 40-char
   hex commit SHA or a versioned tag (e.g. `v1.2.3`, `VER-2-14-3`,
   `release-3.2.0`, `v1_50_0`). Branch references (`master`, `main`, `develop`,
   `docking`, `next`, `HEAD`, `staging`, `trunk`) are forbidden.

2. **CI gate.** `tools/lint/check_git_pins.py` runs in the `ci.yml` pipeline as
   the **Dependency SHA-pin ratchet (ADR-0091)** step. It fails any PR that
   adds a new branch pin and prints the offender with file/line.

3. **Burn-down ALLOW_LIST.** The five current offenders are enumerated in
   `tools/lint/check_git_pins.py` `ALLOW_LIST`. The list shrinks — never grows.
   Each time one dep is pinned to a SHA, its ALLOW_LIST entry is removed in
   the same commit. The lint script also nags when an ALLOW_LIST entry no
   longer corresponds to a live violation so the list cannot rot.

4. **Pin-upgrade process.**
   a. Record the upstream commit SHA (e.g. `git ls-remote …` against the
      upstream repo).
   b. Update `cmake/dependencies.cmake` `GIT_TAG <40-hex-sha>`.
   c. Remove the entry from `ALLOW_LIST` in `tools/lint/check_git_pins.py`.
   d. Rebuild the CPM cache on Linux + Windows locally to confirm the new
      pin resolves cleanly.
   e. Note the upgrade in the commit body; a full ADR is only required when
      the upgrade changes API surface.

### 3. Consequences

- Reproducible builds are enforced mechanically, not by convention.
- New dependencies cannot accidentally ship with branch pins — the lint fails
  the PR.
- The five known offenders are tracked, visible, and shrink over time.
- ImGui docking branch (today's pin) is the only dep whose "stable tag" is
  itself a branch — the SHA pin will be taken from the docking branch HEAD
  at the time of upgrade and refreshed deliberately, not automatically.

### 4. Alternatives considered

- **Submodules.** Reject — we committed to CPM.cmake for dependency management
  in the `docs/05` toolchain spec; submodules would double the surface.
- **Vendored copies.** Reject for now — bloats the repo, loses upstream diff
  context. Revisit per-dep if upstream disappears or licensing demands it.
- **Do nothing.** Reject — leaves the reproducibility guarantee as a
  gentleman's agreement, which is exactly what the audit flagged.


## ADR file: `adr/0092-spdx-license-header-adoption.md`

## ADR 0092 — SPDX License Header Adoption (new-file policy)

- **Status:** Accepted
- **Date:** 2026-04-22
- **Deciders:** Cold Coffee Labs engine group
- **Supersedes:** —
- **Superseded by:** —
- **Related:** `LICENSE` (repo root); audit finding P4.8; `CLAUDE.md` §Legal & licensing hygiene.

### 1. Context

Every C++ / Rust / Python source file in the repository is covered by the
top-level `LICENSE` (proprietary, all rights reserved, with CPM / Cargo /
Vulkan SDK third-party carve-outs). At audit time, no source file carries
an SPDX identifier, which breaks two workflows we care about:

- **SBOM generation** — `cyclonedx-cli`, `syft`, and equivalent supply-chain
  tools infer licenses primarily from SPDX tags. Without them, a freshly
  checked-out tree reports "unknown" for every Greywater-authored source,
  drowning out the actual third-party findings that matter.
- **Mixed-license quarantine** — once we start vendoring additional
  third-party code into `third_party/` (tiny_sha3 is the first, ADR-0091
  ratchet implies more to come), we need a mechanical way to tell an
  MIT-licensed TU apart from a proprietary one. SPDX IDs do that in one
  grep.

Back-filling ~1 500 engine TUs would swamp the diff. The conservative move
is to adopt SPDX on *new* files only and let the codebase converge over
time, the same way we handled the jobs/ STL-allocation rule.

### 2. Decision

1. **Proprietary files** created on or after 2026-04-22 carry the two-line
   header at the very top of the file (above `#pragma once`, module
   declarations, or shebang):

   ```
   // SPDX-License-Identifier: LicenseRef-Greywater-Proprietary
   // SPDX-FileCopyrightText: 2026 Cold Coffee Labs
   ```

   For Python / shell, swap `//` for `#`. For CMake, `#`. For HLSL, `//`.

2. **Third-party files** under `third_party/` carry the upstream SPDX tag
   verbatim (e.g. `SPDX-License-Identifier: MIT` for `tiny_sha3/*.c`). The
   `third_party/<name>/LICENSE` file remains canonical; the SPDX tag is
   metadata that supply-chain tools can pick up without reading prose.

3. **Back-fill is not required.** Existing files inherit coverage from the
   top-level `LICENSE` as they always have. A future ADR may lift this
   restriction once we pay the diff cost (e.g. during the Phase 24 hardening
   milestone when we do a single sweeping reformat pass).

4. **Proof of life.** The three audit-bookended lint scripts
   (`tools/lint/check_git_pins.py`, `check_shader_registration.py`,
   `check_no_iostream.py`) carry the header as the first worked example.

### 3. Consequences

- SBOM tools start recognizing Greywater-authored code instead of flagging
  it "license: unknown".
- New third-party vendored drops get grep-able provenance metadata on day
  one.
- No disruption to the existing source tree.
- CI lint for *missing* SPDX is deferred — enabling it before the backfill
  would just fail on every PR that touches an old file. Ticket TBD for a
  ratchet-style gate à la ADR-0086 / ADR-0091 once the post-Phase-24 sweep
  lands.

### 4. Alternatives considered

- **Back-fill every file now.** Reject — ~1 500-file diff on a running
  phase (20) blocks everybody for a week and spams `git blame`.
- **Use a `NOTICE` file instead.** Reject — tools scan for SPDX tags, not
  NOTICE files. NOTICE is additional, not substitute.
- **Do nothing.** Reject — locks us out of automated SBOM for the entire
  engine's lifetime.


## ADR file: `adr/0093-c-style-cast-ratchet.md`

## ADR 0093 — C-style cast ratchet (google-readability-casting)

- **Status:** Accepted
- **Date:** 2026-04-22
- **Deciders:** Cold Coffee Labs engine group
- **Supersedes:** —
- **Superseded by:** —
- **Related:** `docs/03_PHILOSOPHY_AND_ENGINEERING.md §A1` ("no C-style casts");
  `.clang-tidy`; audit finding P5 (C-cast enumeration).

### 1. Context

`docs/03 §A1` states: *"Casts must be `static_cast` / `reinterpret_cast` /
`const_cast` / `bit_cast` / `dynamic_cast`. No C-style casts."* Until this
audit, that rule was enforced only by review — `.clang-tidy` did not enable
the `google-readability-casting` check because it lives in the `google-`
bucket we otherwise don't import.

A grep sweep of `engine/**/*.cpp` on 2026-04-22 flagged ~53 translation
units with at least one suspected C-style cast pattern. Exact enumeration
requires a clang AST pass (`run-clang-tidy` against `compile_commands.json`);
the grep is advisory because it matches on `(type)expr` syntax that also
shows up inside function-style casts and macros.

### 2. Decision

1. **Enable the check.** `.clang-tidy` now has
   `google-readability-casting` on the `Checks:` list, without pulling the
   rest of the `google-*` style pack. `WarningsAsErrors: '*'` is already set,
   so every new C-style cast introduced in `engine/`, `editor/`, `runtime/`,
   `gameplay/`, `sandbox/`, `tools/`, or `tests/` breaks the build.

2. **Ratchet, don't sweep.** The pre-existing ~53-file set of offenders is
   handled by the ratchet pattern we use elsewhere (ADR-0086
   exceptions-quarantine, ADR-0091 SHA-pin): when a file in the offender
   list is touched by a PR, its C-style casts must be rewritten in the
   same commit. PRs that don't touch an offender don't have to pay the
   cost.

3. **No ALLOW_LIST file.** The check itself does the gating — CI fails
   when a TU fires `google-readability-casting` and `WarningsAsErrors`
   promotes it to an error. We do not maintain a separate Python lint like
   ADR-0091 uses, because clang-tidy already has the exact semantics we
   want.

4. **Burn-down tracking.** Not required. The set is known to shrink
   because the only way for it to grow is someone bypassing clang-tidy in
   CI, which is blocked.

### 3. Consequences

- New C-style casts cannot land. Reviewers don't have to grep for them.
- The ~53-TU backlog burns down organically as each file gets touched.
- Third-party code is already excluded via `HeaderFilterRegex` and by the
  `third_party/` directory layout.
- Zero disruption to `cmake --build` on machines that run clang-tidy via
  the preset; the check is cheap (syntactic).

### 4. Alternatives considered

- **Full one-shot sweep.** Reject — 53-TU diff across hot subsystems
  (render/, persist/, net/, vfx/) would conflict with Phase 20 work in
  flight.
- **Keep review-only.** Reject — the audit found the rule was already
  bleeding; mechanized enforcement is the only stable fix.
- **Drop the rule.** Reject — `(T)x` defeats the whole point of typed
  C++; it hides `reinterpret_cast` behind a paren and silently drops
  `const` qualifiers.

---

## ADR file: `adr/0094-horror-studio-pivot.md`

## ADR-0094 — Horror-Studio Pivot and Franchise-Platform Posture

**Status:** Accepted  **Date:** 2026-04-22

### Context
Cold Coffee Labs was framed as "a studio making Sacrilege." The Sacrilege vision, Story Bible, and investor posture require a more durable identity: a **horror studio** building **Greywater** as a **franchise content platform** while shipping *Sacrilege* as v1.

### Decision
- Cold Coffee Labs is a horror studio.
- Greywater Engine is a franchise content platform (seven IP-agnostic services per ADR-0102).
- *Sacrilege* is the flagship and shipping commitment for 2026.
- Future IPs (*Apostasy*, *Silentium*, *The Witness*) are scheduled pre-production slots in Phase 29.
- All top-level docs (`CLAUDE.md`, `README.md`, `docs/README.md`, `docs/01` §0) are rewritten to reflect this.

### Consequences
- Every new engine subsystem must be IP-agnostic at the schema boundary (enforced by ADR-0102).
- Narrative content for *Sacrilege* is layered as a skin over pre-existing Nine-Circles mechanics (ADR-0099).

---

## ADR file: `adr/0095-ggml-runtime-carve-out.md`

## ADR-0095 — Runtime AI Carve-Out and ggml Selection

**Status:** Accepted  **Date:** 2026-04-22  **Supersedes:** the "no runtime ML" rule in prior `docs/01`.

### Context
The Sacrilege vision requires on-device neural systems (AI Director, adaptive music, neural materials). The previous constitution banned all runtime ML. We need a disciplined carve-out.

### Decision
- Add `docs/01` §2.2.2: runtime ML inference is permitted in C++ only, via ggml / llama.cpp headers.
- CPU backend for any authoritative-state model; Vulkan opt-in for presentation-only models.
- All weights pinned, content-addressed (BLAKE3), Ed25519-signed.
- Ten-rule coding standard in `docs/05` §14 is binding.
- Aggregate perf ceiling 0.74 ms/frame on RX 580 (consumes existing headroom).

### Consequences
- `engine/ai_runtime/` is a new monolithic subsystem appended to `greywater_core`.
- CI gates `gw_perf_gate_ai_*` are PR-blocking.
- No internet, no telemetry, no Python at runtime — enforced at link time.

---

## ADR file: `adr/0096-ai-determinism-contract.md`

## ADR-0096 — Runtime ML Determinism Contract

**Status:** Accepted  **Date:** 2026-04-22

### Decision
- Authoritative-state models are pure functions of `(state, seed)`; CPU backend; fixed topology; fixed precision.
- Presentation-only models carry `GW_AI_PRESENTATION_ONLY`; excluded from replay hash.
- Weight-file change = `GoldenHashes` bump.
- Per-model determinism doctests mandatory.
- See `docs/09` §"Runtime AI determinism contract".

---

## ADR file: `adr/0097-hybrid-director.md`

## ADR-0097 — Hybrid Rule-Based + Bounded-RL AI Director

**Status:** Accepted  **Date:** 2026-04-22

### Context
Full RL director with on-device fine-tuning is non-deterministic and violates rollback. L4D-style finite-state machines are deterministic but brittle. We pick the middle path.

### Decision
- Director state machine (Build-Up, Sustained Peak, Peak Fade, Relax) is rule-based C++.
- RL training happens **offline** via `tools/cook/ai/director_train/`; produces bounded parameter tuning of the state-machine thresholds.
- Runtime reads a **pinned, signed policy checkpoint**; no runtime fine-tuning.
- Budget: 0.10 ms/frame on RX 580 (ggml CPU).
- Standalone `apps/sandbox_director` enables headless policy iteration.

---

## ADR file: `adr/0098-symbolic-adaptive-music.md`

## ADR-0098 — Symbolic Adaptive Music (Tiny Transformer, Offline Stems)

**Status:** Accepted  **Date:** 2026-04-22  **Supersedes:** initial LSTM-music proposal.

### Context
Heavy LSTM-based music generation is beyond our 0.2 ms budget and introduces hidden recurrent state that breaks rollback. Offline-composed stems + a tiny symbolic transformer (RoPE, ≤ 1M params) solves this without sacrificing reactivity.

### Decision
- Stems authored offline by a human composer; BPM-locked per Circle.
- Runtime transformer predicts stem-mix weights + layer activation per frame.
- CPU ggml; 0.2 ms budget.
- Presentation-only (tagged `GW_AI_PRESENTATION_ONLY`).

---

## ADR file: `adr/0099-narrative-skin-over-martyrdom.md`

## ADR-0099 — Narrative Skin over Martyrdom Spine

**Status:** Accepted  **Date:** 2026-04-22

### Decision
- Malakor/Niccolò duality, three Acts, Vatican/Rome/Heaven/Hell/Silentium locations, God Mode, Grace Meter, Logos boss are layered as **narrative skin** over unchanged Nine-Circles + Martyrdom mechanics.
- God Mode is the narrative name for Rapture; it adds presentation hooks only.
- Silentium is a Circle IX profile variant, not an open world.

---

## ADR file: `adr/0100-sin-signature.md`

## ADR-0100 — Sin-Signature Rolling Fingerprint

**Status:** Accepted  **Date:** 2026-04-22

### Decision
- Deterministic five-channel fingerprint (God-Mode uptime, precision, cruelty, hitless streak, deaths/area) in ECS.
- Replicated, rollback-safe.
- Input to voice director, Director spawn tables, Grace unlocks.

---

## ADR file: `adr/0101-grace-meter-finale.md`

## ADR-0101 — Grace Meter and Dual-Ending Logos Phase 4

**Status:** Accepted  **Date:** 2026-04-22

### Decision
- Grace is a third Martyrdom-adjacent resource accrued through restraint (`docs/07` §2.7).
- Logos Phase-4 selector reads Grace value and branches deterministically.
- `forgive` Blasphemy costs 100 Sin + 100 Grace; available only in Phase-4 window.

---

## ADR file: `adr/0102-seven-franchise-services.md`

## ADR-0102 — Seven IP-Agnostic Franchise Services

**Status:** Accepted  **Date:** 2026-04-22

### Decision
- `engine/services/{material_forge,level_architect,combat_simulator,gore,audio_weave,director,editor_copilot}/` defined as INTERFACE schema headers + impl shims.
- Schemas are POD; no Sacrilege-specific types.
- Future IPs consume schemas only; cannot reach into impl.
- Phase 27 hardens all seven to a cross-IP test-battery bar.

---

## ADR file: `adr/0103-dialogue-graph-format.md`

## ADR-0103 — `.gwdlg` Dialogue Graph Format

**Status:** Accepted  **Date:** 2026-04-22

### Decision
- Binary, versioned dialogue-graph format cooked by `gw_cook`.
- Authored in `editor/panels/dialogue_graph/` via imnodes.
- Reachability verified by BLD `dialogue_reachability` tool.

---

## ADR file: `adr/0104-editor-three-theme-profiles.md`

## ADR-0104 — Editor Three-Theme Profile Split

**Status:** Accepted  **Date:** 2026-04-22

### Decision
Three runtime-switchable profiles with independent typography and effect flags:
- **Brewed Slate** (default; minimal; palette-token compliant).
- **Corrupted Relic** (Sacrilege aesthetic; distressed widgets, vignette, glitch hover, corner cracks, asymmetric layout — all effect-flagged).
- **Field Test High Contrast** (WCAG 2.2 AA+, large mono font, zero effects).

Persisted in `editor_config.toml`; switchable via View > Theme.

---

## ADR file: `adr/0105-editor-module-manifest.md`

## ADR-0105 — Declarative Editor Module Manifest and Lazy Load

**Status:** Accepted  **Date:** 2026-04-22

### Decision
Migrate `editor/panel_registry` to `editor/modules/` with TOML `ModuleManifest` entries (Core, Scene, Render, Audio, Narrative, Circle, AI/Director, Materials, Sequencer). Panels load lazily from manifest; no hardcoded boot list.

---

## ADR file: `adr/0106-sacrilege-authoring-panels.md`

## ADR-0106 — Sacrilege-Specific Authoring Panels

**Status:** Accepted  **Date:** 2026-04-22

### Decision
Add panels under `editor/panels/`: `circle_editor`, `encounter_editor`, `dialogue_graph`, `sin_signature_panel`, `act_state_panel`, `ai_director_sandbox` (in-editor view), `material_forge_panel`, `pcg_node_graph`, `shader_forge`.

---

## ADR file: `adr/0107-editor-custom-widgets.md`

## ADR-0107 — Editor Custom ImGui Widgets

**Status:** Accepted  **Date:** 2026-04-22

### Decision
`editor/widgets/`: `SplineEditor`, `DistressedButton`/`Slider`, `GlitchTextHover`, `BlasphemousProgressBar`, `AssetPreviewTile`. Widgets respect ThemeEffectFlags.

---

## ADR file: `adr/0108-pie-debug-overlays.md`

## ADR-0108 — Play-In-Editor F5-from-Cursor + Debug Overlays

**Status:** Accepted  **Date:** 2026-04-22

### Decision
Upgrade `editor/pie/gameplay_host`: F5 from cursor position; debug overlays for Martyrdom, God Mode, Grace, AI Director; per-panel perf HUD. Editor viewport ≤ 144 FPS is non-negotiable.

---

## ADR file: `adr/0109-editor-a11y-toggles.md`

## ADR-0109 — Editor Accessibility Toggles

**Status:** Accepted  **Date:** 2026-04-22

### Decision
`editor/a11y/` provides: reduce corruption, disable vignette, disable glitch, increase contrast, force-mono accessibility font. WCAG 2.2 AA parity with Phase 16 runtime a11y.

---

## ADR file: `adr/0110-cpm-sha-pin-ratchet.md`

## ADR-0110 — CPM Dependency SHA-Pin Ratchet

**Status:** Accepted  **Date:** 2026-04-22

### Context
Audit found branch-floating `GIT_TAG` values for imgui docking, ImGuizmo master, imnodes master, mikktspace master, stb master. Unreproducible builds.

### Decision
- All CPM deps pinned to **SHAs or release tags**.
- Dear ImGui pinned to v1.92.7 (Apr 2026) docking branch SHA.
- CI gate `tools/lint/check_cpm_sha_pins.py` rejects branch-floating refs.

---

## ADR file: `adr/0111-vulkan-1-3-baseline.md`

## ADR-0111 — Vulkan 1.3 Baseline

**Status:** Accepted  **Date:** 2026-04-22  **Supersedes:** portions of ADR-0003.

### Decision
- Desktop baseline is Vulkan 1.3.
- `find_package(Vulkan 1.2)` becomes `1.3` in `cmake/dependencies.cmake`.
- Mesh shaders, ray tracing, work graphs remain Tier G.

---

## ADR file: `adr/0112-fsr2-hdr-frame-pacing.md`

## ADR-0112 — FSR 2, HDR10, Frame Pacing

**Status:** Accepted  **Date:** 2026-04-22

### Decision
- `engine/render/post/fsr2/` behind `GW_ENABLE_FSR2` (Vulkan FSR 2/3.1 upscaling; no Frame Gen on Polaris).
- `engine/render/hal/hdr_output.*` + `shaders/post/tonemap_pq.frag.hlsl` behind `r.HDROutput`.
- `VK_KHR_present_id` + `VK_KHR_present_wait` for frame pacing.

---

## ADR file: `adr/0113-shader-root-consolidation.md`

## ADR-0113 — Single Shader Root Consolidation

**Status:** Accepted  **Date:** 2026-04-22

### Decision
- Canonical shader root is `shaders/`.
- Duplicate `engine/render/forward_plus/*.hlsl` migrated to `shaders/forward_plus/`.
- CI gate `tools/lint/check_single_shader_root.py` rejects shader files outside `shaders/`.

---

## ADR file: `adr/0114-steam-deck-tier-a.md`

## ADR-0114 — Steam Deck as Tier A Platform

**Status:** Accepted  **Date:** 2026-04-22

### Decision
- Steam Deck is a Tier A target.
- `engine/input/steam_input_glue.cpp` for glyphs / controller-first bindings.
- `apps/sandbox_deck_probe/` + nightly `steam_deck.yml` CI job under `steamrt-sniper`.
- Honor Valve 2026 Deck checklist (no launcher, no MKB glyphs on controller, default bindings complete).

---

## ADR file: `adr/0115-crash-and-content-signing.md`

## ADR-0115 — Sentry Crash Reporting + Ed25519 Content Signing

**Status:** Accepted  **Date:** 2026-04-22

### Decision
- `engine/core/crash_reporter.cpp` wired to existing `GW_ENABLE_SENTRY`; minidump upload + symbol-server workflow.
- `tools/cook/cook_worker.cpp` signs cooked assets with Ed25519; runtime verifies in Release builds.

---

## ADR file: `adr/0116-gameplay-a11y.md`

## ADR-0116 — Gameplay-Layer Accessibility (WCAG 2.2 AA)

**Status:** Accepted  **Date:** 2026-04-22

### Decision
- Colour-blind modes, sliders for chromatic/shake/motion-blur, customizable subtitles + speaker labels, gore-level slider, photosensitivity pre-warning, audio mono-mix + per-channel sliders, full input remap including one-hand preset.
- WCAG 2.2 AA audit is a Phase 24 exit gate.

---

## ADR file: `adr/0117-ci-quality-gates.md`

## ADR-0117 — CI Quality Gates (Phase-Split Tests, Fuzz, Determinism, Shader Parity)

**Status:** Accepted  **Date:** 2026-04-22

### Decision
- Split `gw_tests` monolith into `gw_tests_phaseNN` binaries.
- `tests/fuzz/` + nightly `fuzz.yml` (doctest + libFuzzer: `.gwscene`, MCP JSON, asset loaders, save format).
- Nightly `determinism.yml`: 5 seed replays, bit-hash diff Linux vs Windows.
- Slang/DXC parity shader CI on a representative subset.

---

