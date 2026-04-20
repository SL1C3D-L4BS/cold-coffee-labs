# 08_GLOSSARY — Greywater_Engine

**Status:** Reference
**Convention:** Alphabetical. Each term: name, one-to-three-sentence definition, citation or context.

---

## A

**ACL (Animation Compression Library).** A permissively-licensed C++ library for skeletal animation compression. Used alongside Ozz-animation to shrink shipped animation assets.

**ADR (Architecture Decision Record).** A short dated document capturing a single architectural decision, its context, the options considered, and the chosen trade-off. Lives under `docs/adr/`. Required for any change that crosses a non-negotiable in `00_SPECIFICATION`.

**AoSoA.** Array-of-Structs-of-Arrays. Hybrid memory layout that preserves locality within cache lines while enabling SIMD across them. Used selectively in the ECS for hot components.

**Archetype.** In the ECS, the unique set of component types an entity has. Entities with the same archetype live in the same contiguous chunk of memory. The basis for cache-friendly iteration.

**Arena allocator.** An allocator that owns a large contiguous block and hands out smaller regions from it. Freeing the arena frees everything in it at once — O(1) teardown.

**ASan (AddressSanitizer).** A Clang instrumentation that detects memory errors (use-after-free, buffer overflow, leaks). Runs nightly in CI.

## B

**Bindless.** A descriptor-management style where shaders index into large descriptor arrays rather than binding descriptors per-draw. Vulkan 1.3 supports bindless via `VK_EXT_descriptor_indexing`. Baseline in Greywater's renderer.

**Binding (Rust/C++).** Generated code that allows one language to call the other. `cxx` generates C++ bindings to Rust; `cbindgen` generates C headers for Rust.

**BLD (Brewed Logic Directive).** Greywater_Engine's native AI copilot subsystem. Implemented as a Rust crate linked into the editor via a narrow C-ABI. MCP-based, with proc-macro-generated tool surfaces.

**`bld-ffi`.** The Rust crate that defines the C-ABI boundary between BLD and the C++ editor. Covered by Miri to validate UB freedom.

**`bld-tools`.** The Rust crate hosting the `#[bld_tool]` proc macro and the tool registry. Tools annotated with `#[bld_tool]` become MCP-callable at compile time.

**Graph-as-ground-truth visual scripting.** A visual-scripting pattern where the node graph is the canonical serialized form. Greywater **deliberately does not use this pattern** — text IR is canonical, graph is a projection.

**Brew Doctrine.** Greywater's philosophy statement. See `01_ENGINE_PHILOSOPHY.md`.

## C

**Candle.** A pure-Rust ML runtime. Primary local-inference backend for BLD's RAG and offline mode.

**`cbindgen`.** A tool that generates C and C++ headers from Rust types. Used to expose BLD's public API to C++.

**Clang.** The LLVM C/C++ frontend. **The only C++ compiler supported by Greywater.** On Windows, `clang-cl` targets the MSVC ABI.

**CMake.** The build generator. Greywater uses 3.28+ with file sets, target-based design, and `CMakePresets.json`. Ninja is the only generator.

**Command bus / `ICommand`.** The undo/redo primitive in `engine/core/command/`. Every mutating action is an `ICommand` pushed on the stack. Ctrl+Z reverses. BLD tools submit commands via C-ABI callback.

**Commitments (non-negotiable).** Load-bearing decisions that cannot be changed without stakeholder sign-off. Enumerated in `00_SPECIFICATION` §2.

**Corrosion.** A CMake module that integrates Cargo builds into CMake. The bridge between C++ and Rust in Greywater's build graph.

**CPM.cmake.** A CMake dependency manager that pins C++ dependencies by commit SHA. Source-code integration, no system packages.

**CRTP (Curiously Recurring Template Pattern).** A C++ idiom for compile-time polymorphism — a class inherits from a template parameterized by itself. Used where virtual dispatch would cost in hot paths.

**`cxx`.** A Rust library by dtolnay for safe Rust/C++ interop. Used to bridge BLD into the editor.

## D

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

**`gw::` (namespace).** Greywater's root C++ namespace. `gw::render`, `gw::ecs`, `gw::jobs`, `gw::audio`, etc.

**`GW_ASSERT`.** Greywater's assertion macro. Captures stack trace, optionally opens debugger, logs via `GW_LOG`.

**`.gwscene`.** The binary scene format. Versioned, migration-hooked.

**`.gwseq`.** The binary sequencer format for cinematics. Shares serialization machinery with `.gwscene`.

**GameNetworkingSockets (GNS).** An open-source networking library. Greywater's transport layer. Battle-tested at large-scale multiplayer.

**GBNF (GGML Backus-Naur Form).** A grammar format used by llama.cpp to constrain model output. Greywater derives GBNF grammars from tool JSON schemas so small local models emit structurally valid tool calls.

**glTF.** The 3D asset interchange format. Greywater's primary mesh/scene import.

## H

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

**MCP (Model Context Protocol).** An open protocol for exposing tool surfaces to AI agents. Version `2025-11-25` is the one we implement. BLD is an MCP server; the editor panel and any compliant external client are MCP clients.

**miniaudio.** A single-header C audio library. Greywater's mixer/device backend.

**Miri.** A Rust interpreter that detects undefined behavior. Run nightly on `bld-ffi` to validate the FFI boundary.

**Module (Phase-1 context).** A sub-directory under `engine/` representing a cohesive subsystem. Not to be confused with C++20 modules.

## N

**Ninja.** A small, fast build system used by CMake. The only generator supported by Greywater.

**`Nomic Embed Code`.** A code-specialized embedding model. Greywater's default for RAG embeddings (offline via candle).

## O

**Obsidian.** Greywater's primary UI background color (`#0B1215`). See `01_ENGINE_PHILOSOPHY` §3.

**Ozz-animation.** An open-source minimal-overhead skeletal animation runtime. Greywater's animation backend.

## P

**Symbol-graph ranking.** An eigenvector-based ranking (PageRank-style) applied over a symbol graph. Greywater uses it to boost structurally important definitions during RAG retrieval.

**`petgraph`.** A Rust graph library. Used by BLD's RAG to build and query symbol graphs.

**Phase (1–11).** The eleven-stage buildout in `05_ROADMAP_AND_MILESTONES`.

**Pool allocator.** An allocator for fixed-size objects backed by a free-list. Used for hot component types and particle systems.

**Principal Engine Architect.** The role Claude Opus 4.7 is authorized to play on this project. See `00_SPECIFICATION` §1.

## R

**RAG (Retrieval-Augmented Generation).** A technique for grounding LLM responses in retrieved context. Greywater's BLD indexes both engine source and user projects.

**RmlUi.** An open-source HTML/CSS-inspired in-game UI library. Greywater's choice for HUDs and menus. **Deliberately not ImGui.**

**`rmcp`.** The official Rust MCP SDK. BLD's MCP server is built on it.

**RTTI (Run-Time Type Information).** C++ runtime type introspection. **Disallowed in Greywater hot paths.**

**Rust.** The language used for BLD only. Everywhere else is C++23.

## S

**Sandbox (executable).** The engine's own feature test bed. Links `greywater_core` directly, runs experiments without editor or game weight.

**Sentry.** A crash-reporting service. Greywater uses the Sentry Native SDK for minidump upload.

**SemVer (Semantic Versioning).** Greywater's engine versioning scheme.

**SDL3.** Simple DirectMedia Layer version 3. Used for gamepad/HID/haptics input. Keyboard/mouse go through the platform layer directly.

**SPIR-V.** The bytecode format for Vulkan shaders. HLSL/GLSL compile to SPIR-V via DXC/glslang.

**sqlite-vss.** A SQLite extension for vector similarity search. Greywater's vector index for BLD's RAG.

**Steam Audio.** A spatial-audio SDK. Greywater's choice for HRTF + occlusion + reflection.

## T

**Tier (A/B/C/G).** Scope classification. A must ship, B likely ships, C is post-v1, G is hardware-gated. See `02_SYSTEMS_AND_SIMULATIONS`.

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

**RX 580 / Polaris.** AMD Radeon RX 580 8GB (GCN 4.0, 2016). Greywater's baseline development hardware and target player platform. Performance budget: 1080p / 60 FPS. No ray-tracing cores, no mesh-shader support.

**Atmospheric scattering.** Greywater's physically-based atmospheric scattering model — single compute-pass, no high-dimensional lookup tables, renders accurately from ground to space. Implementation in `engine/world/atmosphere/`.

**Floating origin.** A technique for maintaining `float32` render precision at planetary or interstellar scale: the world's origin is periodically recentered on the player, with all entities translated. Required for Earth-class planets.

**Quad-tree cube-sphere.** A planet-subdivision scheme with 6 root quad faces (one per cube face) that recursively subdivide based on LOD. Greywater's planet mesh topology.

**`float64` / `f64`.** Double precision. Required for all world-space positions in Greywater due to planetary scale. Converted to `float` only for local-to-chunk math and GPU vertex attributes (via subtraction from a floating camera origin).

**Deterministic procedural universe.** The principle that given the same universe seed and spatial coordinates, any two machines will produce byte-identical content. Enables infinite-without-storage worlds and rollback netcode.

**Chunk streaming.** On-demand generation and caching of 32 m³ world chunks, with LRU eviction and time-sliced generation across multiple frames. Never blocks the main thread.

**Marching Cubes.** An algorithm for extracting an isosurface mesh from a 3D scalar field. Greywater uses it for caves and voxel-based structures.

**Rollback netcode.** A networking technique that predicts remote inputs, simulates locally with zero input lag, and rewinds/resimulates when predictions are wrong. Requires a deterministic simulation. Greywater uses it for PvP combat.

**Deterministic lockstep.** A networking technique where every client simulates identically from shared inputs. Requires a fully deterministic simulation. Greywater uses it for 4-client large-scale simulation scenarios.

**Structural integrity graph.** A connectivity graph over building pieces used to propagate support values from ground-connected nodes outward. Unsupported structures collapse. Greywater's base-building destruction model.

**Wave Function Collapse (WFC).** A procedural content generation algorithm using adjacency constraints. Greywater uses it for ruined cities and space stations.

**Utility AI.** A decision-making technique where actions are scored by weighted considerations and the highest-scoring action is selected. Greywater uses it for faction-level strategic decisions.

<!-- second Steam Audio entry consolidated with the earlier one above -->

**Realistic low-poly.** Greywater's art direction: reduced polygon counts with physically-based realistic lighting, restrained materials, strong atmospheric scattering as the principal mood lever. See `docs/games/greywater/20_ART_DIRECTION.md` for the seven properties this style embodies.

## W

**WCAG 2.2.** Web Content Accessibility Guidelines version 2.2. Greywater targets AA compliance on the reference HUD.

**WebGPU.** A cross-platform GPU API. Greywater plans a WebGPU backend behind the HAL for web and sandboxed targets; not v1.

---

*Terminology deliberately. If a term is used in the codebase or docs and is not here, add it.*
