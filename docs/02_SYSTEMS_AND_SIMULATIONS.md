# 02_SYSTEMS_AND_SIMULATIONS — Greywater_Engine System Inventory

**Status:** Reference
**Last revised:** 2026-04-19

This document is the complete enumeration of every subsystem in Greywater_Engine, classified by **Tier**. Tiering is the core defense against scope creep — every architectural conversation should end with the question *"is this Tier A?"*

---

## Tier legend

| Tier | Commitment                                                                  |
| ---- | --------------------------------------------------------------------------- |
| **A**| **Ships in v1.0.** Cannot be cut without stakeholder-level scope decision.  |
| **B**| Ships if schedule holds. Can be cut to Post-v1 without board-level escalation. |
| **C**| Stretch. Post-v1. Referenced here so we know we chose not to build it.      |
| **G**| Hardware-gated. Ships only where `RHICapabilities` reports support.         |

Every subsystem below has a **tier**, an **owning phase** (see `05_ROADMAP_AND_MILESTONES.md` — phases 1–25; Phase 25 is LTS), an **owning module** (directory in the repo), and a **concrete library decision** or a *custom* label.

---

## 1. Core C++ Foundation (Tier A)

| System              | Module                     | Library / Decision                            | Phase |
| ------------------- | -------------------------- | --------------------------------------------- | ----- |
| Platform abstraction| `engine/platform/`         | Custom; GLFW under the hood for windowing     | 2     |
| Memory allocators   | `engine/memory/`           | Custom (frame, arena, pool, free-list)        | 2     |
| Logger              | `engine/core/`             | Custom; spdlog as evaluated fallback          | 2     |
| Assertions          | `engine/core/`             | Custom `GW_ASSERT` with stack trace           | 2     |
| Error types         | `engine/core/`             | Custom `Result<T, E>`                         | 2     |
| Containers          | `engine/core/`             | Custom (span, array, vector, hash_map, ring)  | 2     |
| Strings             | `engine/core/`             | Custom UTF-8 with SSO                         | 2     |
| Command bus         | `engine/core/command/`     | Custom `ICommand` + `CommandStack`            | 3     |
| Event system        | `engine/core/events/`      | Custom type-safe pub/sub                      | 11    |
| Config / CVars      | `engine/core/config/`      | Custom registry; TOML persistence             | 11    |
| Math library        | `engine/math/`             | Custom (candidate retire glm)                 | 3     |
| Job system          | `engine/jobs/`             | Custom fiber-based work-stealing              | 3     |
| Reflection          | `engine/core/`             | Macro-based; tagged-header codegen later      | 3     |
| Serialization core  | `engine/core/`             | Versioned binary primitives                   | 3     |
| ECS                 | `engine/ecs/`              | Custom archetype-based                        | 3     |
| Dynamic library load| `engine/scripting/`        | Custom hot-reload via platform layer          | 2     |

## 2. Rendering (Tier A unless noted) — RX 580 baseline

| System              | Module                           | Library / Decision                            | Phase  | Tier |
| ------------------- | -------------------------------- | --------------------------------------------- | ------ | ---- |
| Rendering HAL       | `engine/render/hal/`             | Custom; ~30 types abstraction                 | 4      | A    |
| Vulkan 1.2 backend (1.3 opportunistic) | `engine/render/`    | `volk` + `VulkanMemoryAllocator`              | 4      | A    |
| Descriptor system   | `engine/render/`                 | Bindless via `VK_EXT_descriptor_indexing`     | 4      | A    |
| Pipeline cache      | `engine/render/`                 | Vulkan pipeline cache + content hashes        | 4      | A    |
| Frame graph         | `engine/render/`                 | Custom explicit-resource-lifetime + barriers  | 5      | A    |
| Async compute path  | `engine/render/`                 | Timeline semaphores, multi-queue scheduling   | 5      | A    |
| Shader pipeline     | `engine/render/shader/`          | HLSL + DXC → SPIR-V                           | 5, 17  | A    |
| Material system     | `engine/render/material/`        | Data-driven instances; graph editor Tier C    | 5, 17  | A    |
| Forward+ lighting   | `engine/render/`                 | Custom clustered                              | 5      | A    |
| Cascaded shadows    | `engine/render/`                 | Custom                                        | 5      | A    |
| IBL                 | `engine/render/`                 | Split-sum approximation                       | 5      | A    |
| Tonemapping         | `engine/render/`                 | Custom filmic + exposure                      | 5      | A    |
| Profiler integration | `engine/render/`                | Tracy GPU zones + CPU correlation             | 5      | A    |
| WebGPU backend      | `engine/render/webgpu/`          | Dawn (eventual)                               | Post   | C    |
| DX12 backend        | —                                | Not planned                                   | —      | C    |
| Ray tracing         | `engine/render/rt/`              | `VK_KHR_ray_tracing_pipeline`                 | Post   | G    |
| Mesh shaders        | `engine/render/`                 | `VK_EXT_mesh_shader`                          | Post   | G    |
| GPU work graphs     | `engine/render/`                 | Vulkan 1.4 extension                          | Post   | G    |
| GPU particles       | `engine/vfx/`                    | Custom compute-based                          | 17     | B    |
| Screen-space post   | `engine/vfx/`                    | Bloom, DoF, motion blur, CA, grain             | 17     | B    |
| Screen-space GI     | `engine/render/`                 | SSGI placeholder; ReSTIR or baked Post        | Post   | C    |
| Virtual texturing   | `engine/render/`                 | Not planned v1                                | Post   | C    |

## 2a. Greywater Game Rendering (Tier A — game-specific)

These systems are built for the Greywater game specifically, but use the engine's public API. They live in `engine/world/` as engine capabilities that Greywater consumes.

| System                       | Module                       | Library / Decision                                 | Phase  | Tier |
| ---------------------------- | ---------------------------- | -------------------------------------------------- | ------ | ---- |
| Universe seed management     | `engine/world/universe/`     | 256-bit seed (SHA-256 of phrase); coord-derived    | 19     | A    |
| Hierarchical procedural gen  | `engine/world/universe/`     | Cluster → galaxy → system → planet → chunk         | 19     | A    |
| Spherical planet rendering   | `engine/world/planet/`       | Quad-tree cube-sphere; GPU compute mesh gen        | 20     | A    |
| Hardware tessellation        | `engine/world/planet/`       | Adaptive LOD terrain tessellation                  | 20     | A    |
| Atmospheric scattering       | `engine/world/atmosphere/`   | Physically-based, raymarched single-pass, no LUT   | 21     | A    |
| Volumetric clouds            | `engine/world/atmosphere/`   | Raymarched, Perlin-Worley 3D density               | 21     | A    |
| Floating origin              | `engine/world/origin/`       | Custom; world recenters at configurable threshold  | 20     | A    |
| Chunk streaming              | `engine/world/streaming/`    | 32m chunks, 5 LOD, LRU cache, time-sliced gen      | 19     | A    |
| Biome blending               | `engine/world/biome/`        | Continuous gradients, temperature + humidity + soil | 19    | A    |
| Vegetation instancing        | `engine/world/vegetation/`   | GPU instancing, indirect draw, 4 LOD levels        | 20     | A    |
| Ocean rendering              | `engine/world/ocean/`        | FFT ocean surface waves, fog, caustics              | 20     | B    |
| Starfield                    | `engine/world/space/`        | Deterministic star catalog from galaxy seed        | 21     | A    |
| Re-entry plasma VFX          | `engine/world/atmosphere/`   | Shader effect keyed on heating rate                | 21     | A    |

## 2b. Greywater Simulation (Tier A — game-specific)

| System                       | Module                       | Library / Decision                                 | Phase  | Tier |
| ---------------------------- | ---------------------------- | -------------------------------------------------- | ------ | ---- |
| Orbital mechanics            | `engine/world/orbit/`        | Newtonian gravity, `float64` positions             | 21     | A    |
| Atmospheric layers           | `engine/world/atmosphere/`   | 5 layers with drag/lift/heating interpolation      | 21     | A    |
| Re-entry physics             | `engine/world/atmosphere/`   | Heat + drag + structural stress model              | 21     | A    |
| Structural integrity graph   | `engine/world/buildings/`    | Graph-based support propagation; collapse          | 22     | A    |
| Ecosystem simulation         | `engine/world/ecosystem/`    | Predator/prey dynamics, spawn maps, migration      | 23     | B    |
| Faction utility AI           | `engine/world/factions/`     | Score-based action selection                       | 23     | B    |
| Resource distribution        | `engine/world/resources/`    | Geological-rule-driven procedural placement        | 19     | A    |
| Crafting & recipes           | `engine/world/crafting/`     | ECS-backed recipe database, workbench tiers        | 22     | A    |
| Territory claim system       | `engine/world/territory/`    | Claim flags, ownership, contested zones            | 22     | B    |
| Deterministic universe RNG   | `engine/world/universe/`     | Seeded `std::mt19937_64` per coordinate            | 19     | A    |

## 3. Physics & Simulation (Tier A)

| System              | Module                     | Library                                       | Phase | Tier |
| ------------------- | -------------------------- | --------------------------------------------- | ----- | ---- |
| Rigid body dynamics | `engine/physics/`          | Jolt Physics                                  | 12    | A    |
| Character controller| `engine/physics/`          | Jolt                                          | 12    | A    |
| Constraints         | `engine/physics/`          | Jolt                                          | 12    | A    |
| Triggers / queries  | `engine/physics/`          | Jolt raycasts + shape queries                 | 12    | A    |
| Vehicles            | `engine/physics/`          | Jolt vehicle constraint                       | 12    | B    |
| Deterministic mode  | `engine/physics/`          | Jolt lockstep configuration                   | 12    | A    |
| Cloth               | —                          | Not planned v1                                | Post  | C    |
| Destruction         | —                          | Not planned v1                                | Post  | C    |
| SPH fluids          | —                          | Not planned v1                                | Post  | C    |
| FFT oceans          | —                          | Not planned v1                                | Post  | C    |

## 4. Animation (Tier A)

| System              | Module                     | Library                                       | Phase | Tier |
| ------------------- | -------------------------- | --------------------------------------------- | ----- | ---- |
| Skeletal runtime    | `engine/anim/`             | Ozz-animation                                 | 13    | A    |
| Compression         | `engine/anim/`             | ACL                                           | 13    | A    |
| Blend trees         | `engine/anim/`             | Custom over ECS                               | 13    | A    |
| IK (FABRIK, CCD)    | `engine/anim/`             | Ozz + custom                                  | 13    | A    |
| Morph targets       | `engine/anim/`             | glTF pipeline                                 | 13    | A    |

## 5. Audio (Tier A)

| System              | Module                     | Library                                       | Phase | Tier |
| ------------------- | -------------------------- | --------------------------------------------- | ----- | ---- |
| Device / mixer      | `engine/audio/`            | miniaudio                                     | 10    | A    |
| Spatial audio       | `engine/audio/`            | Steam Audio                                   | 10    | A    |
| Decoders            | `engine/audio/`            | Opus, Ogg Vorbis, WAV                         | 10    | A    |
| DSP (reverb/EQ/comp)| `engine/audio/`            | Steam Audio + custom                          | 10    | A    |
| Streaming music     | `engine/audio/`            | Custom, disk-backed                           | 10    | A    |

## 6. Networking (Tier B)

| System              | Module                     | Library                                       | Phase | Tier |
| ------------------- | -------------------------- | --------------------------------------------- | ----- | ---- |
| Transport           | `engine/net/`              | GameNetworkingSockets                         | 14    | B    |
| ECS replication     | `engine/net/`              | Custom delta + interest management            | 14    | B    |
| Lag compensation    | `engine/net/`              | Custom rewind-and-replay                      | 14    | B    |
| Voice chat          | `engine/net/`              | Opus + GNS + Steam Audio spatial              | 14    | B    |
| Matchmaking         | `engine/net/`              | `ISessionProvider` abstract                   | 14    | B    |
| Deterministic lockstep| `engine/net/`            | Custom on top of Jolt deterministic mode      | 14    | B    |

## 7. Input (Tier A)

| System              | Module                     | Library                                       | Phase | Tier |
| ------------------- | -------------------------- | --------------------------------------------- | ----- | ---- |
| Device layer        | `engine/input/`            | SDL3 (gamepad/HID/haptics)                    | 10    | A    |
| Keyboard/mouse      | `engine/input/`            | Platform layer direct                         | 10    | A    |
| Action-map system   | `engine/input/`            | Custom                                        | 10    | A    |
| Accessibility       | `engine/input/`            | Custom (hold-to-toggle, single-switch)        | 10    | A    |

## 8. User Interface

### Editor UI (Tier A — locked)
| System                | Module                   | Library                                       | Phase | Tier |
| --------------------- | ------------------------ | --------------------------------------------- | ----- | ---- |
| Editor framework      | `editor/`                | Dear ImGui (docking + viewports)              | 7     | A    |
| Gizmos                | `editor/`                | ImGuizmo                                      | 7     | A    |
| Node graph (vscript)  | `editor/vscript_panel/`  | ImNodes                                       | 8     | A    |
| Markdown (BLD chat)   | `editor/agent_panel/`    | imgui_md                                      | 9     | A    |

### In-game UI (Tier B — **deliberately not ImGui**)
| System              | Module                     | Library                                       | Phase | Tier |
| ------------------- | -------------------------- | --------------------------------------------- | ----- | ---- |
| In-game UI          | `engine/ui/`               | RmlUi (HTML/CSS-inspired)                     | 11    | B    |
| Text shaping        | `engine/ui/`               | HarfBuzz + FreeType                           | 11    | B    |
| Vector rendering    | `engine/ui/`               | RmlUi backend via Vulkan HAL                  | 11    | B    |

## 9. Localization & Accessibility (Tier B)

| System              | Module                     | Library                                       | Phase | Tier |
| ------------------- | -------------------------- | --------------------------------------------- | ----- | ---- |
| String tables       | `engine/i18n/`             | Custom binary                                 | 16    | B    |
| Locale runtime      | `engine/i18n/`             | ICU                                           | 16    | B    |
| Font fallback       | `engine/i18n/`             | Custom stack with auto-fallback               | 16    | B    |
| XLIFF workflow      | `tools/`                   | Custom                                        | 16    | B    |
| Color-blind modes   | `engine/a11y/`             | Custom (lighting-integrated)                  | 16    | B    |
| Text scaling        | `engine/a11y/`             | RmlUi + custom                                | 16    | B    |
| Subtitles           | `engine/a11y/`             | Custom with speaker attribution               | 16    | B    |
| Screen-reader hooks | `engine/a11y/`             | AccessKit-C (UIA / AT-SPI / NSAccessibility)  | 16    | B    |
| Motion reduction    | `engine/a11y/`             | Custom (respects `a11y.motion_reduce` CVar)   | 16    | B    |
| Photosensitivity safe mode | `engine/a11y/`      | Custom (clamps flash rate, disables strobes)  | 16    | B    |

## 10. Persistence (Tier A–B)

| System              | Module                     | Library                                       | Phase | Tier |
| ------------------- | -------------------------- | --------------------------------------------- | ----- | ---- |
| Save system         | `engine/persist/`          | Custom versioned binary                       | 15    | A    |
| Local database      | `engine/persist/`          | SQLite                                        | 15    | A    |
| Cloud save          | `engine/persist/`          | `ICloudSave` + Steam/EOS/S3 backends          | 15    | B    |
| User config         | `engine/core/config/`      | TOML                                          | 11    | A    |

## 11. Telemetry & Observability (Tier B)

| System              | Module                     | Library                                       | Phase | Tier |
| ------------------- | -------------------------- | --------------------------------------------- | ----- | ---- |
| Crash reporting     | `engine/telemetry/`        | Sentry Native SDK                             | 15    | B    |
| Event pipeline      | `engine/telemetry/`        | Custom, SQLite-queued, opt-in                 | 15    | B    |
| Profiler            | `engine/` (integration)    | Tracy                                         | 5     | A    |

## 12. Scripting

| System              | Module                     | Library                                       | Phase | Tier |
| ------------------- | -------------------------- | --------------------------------------------- | ----- | ---- |
| Gameplay hot-reload | `engine/scripting/`        | Custom dylib loader + file-watch              | 2     | A    |
| Visual scripting IR | `engine/vscript/`          | Custom typed text IR                          | 8     | A    |
| vscript runtime     | `engine/vscript/`          | Custom interpreter + AOT cook                 | 8     | A    |
| Node-graph view     | `editor/vscript_panel/`    | ImNodes over IR                               | 8     | A    |

## 13. BLD — Brewed Logic Directive (Tier A, Rust)

Revised 2026-04-21 for Phase 9 opening. Earlier rows (`candle` primary, `llama-cpp-rs` fallback, `sqlite-vss`, `Nomic Embed Code` runtime) are superseded by the three ADRs cited below. Rationale captured inline.

| System              | Module                         | Library                                                      | Phase | Tier | ADR |
| ------------------- | ------------------------------ | ------------------------------------------------------------ | ----- | ---- | --- |
| MCP transport       | `bld/bld-mcp/`                 | `rmcp` 1.x (stdio default; Streamable HTTP in 9E)            | 9     | A    | 0011 |
| Provider abstraction| `bld/bld-provider/`            | Own `ILLMProvider` trait; `rig-core` as carrier              | 9     | A    | 0010 |
| Primary cloud       | `bld/bld-provider/`            | Anthropic Claude Opus 4.7 via `rig-core`; `reqwest` + SSE    | 9     | A    | 0010 |
| Primary local       | `bld/bld-provider/`            | `mistral.rs` v0.8 + Qwen 2.5 Coder 7B Q4K + structured output| 9     | A    | 0016 |
| Fallback local      | `bld/bld-provider/`            | `llama-cpp-2` + GBNF from JSON Schema                        | 9     | A    | 0016 |
| Tool registry       | `bld/bld-tools/`, `bld-tools-macros/` | `#[bld_tool]` proc-macro + `schemars` + component autogen | 9     | A    | 0012 |
| RAG — chunking      | `bld/bld-rag/`                 | `tree-sitter-cpp`, `-rust`, `-glsl`, `-json`, `-md` + custom `.gwvs`, `.gwscene` | 9 | A | 0013 |
| RAG — symbol graph  | `bld/bld-rag/`                 | `petgraph` + PageRank (α=0.85, 64 iters)                     | 9     | A    | 0013 |
| RAG — vector store  | `bld/bld-rag/`                 | `rusqlite` + **`sqlite-vec`** (sqlite-vss deprecated upstream)| 9    | A    | 0013 |
| RAG — embeddings (prebuilt) | `bld/bld-rag/`         | Nomic Embed Code via `candle` — **dev-box build only**, ships as blob | 9 | A | 0013 |
| RAG — embeddings (runtime)  | `bld/bld-rag/`         | `fastembed-rs` + `nomic-embed-text-v2-moe` (fits RX 580 8 GB)| 9     | A    | 0013 |
| RAG — watcher       | `bld/bld-rag/`                 | `notify` + `notify-debouncer-mini` (2 s debounce, 30 s quiet)| 9     | A    | 0013 |
| Agent loop          | `bld/bld-agent/`               | Plan-Act-Verify with ReAct fallback; 30-step cap             | 9     | A    | 0014 |
| Governance          | `bld/bld-governance/`          | Three-tier (Read/Mutate/Execute) + form/url elicitation      | 9     | A    | 0011, 0015 |
| Audit log + replay  | `bld/bld-governance/`, `tools/bld_replay/` | JSONL per session; BLAKE3 digests; `gw_agent_replay` binary | 9 | A | 0015 |
| Secret filter       | `bld/bld-governance/`          | Compile-time deny-list + runtime `.agent_ignore` + pre-commit | 9    | A    | 0015 |
| Keyring secrets     | `bld/bld-provider/`            | `keyring` 4.0.x + platform-native features                   | 9     | A    | 0010 |
| C-ABI bridge        | `bld/bld-bridge/`, `bld-ffi/`  | `cxx` + `cbindgen`; Surface H v1, Surface P v1→v2→v3         | 9     | A    | 0007 |
| Hybrid retrieval    | `bld/bld-rag/`                 | Lexical (`grep`) + vector (`sqlite-vec`) + symbol graph       | 9     | A    | 0013 |
| Offline mode        | `bld/bld-provider/`            | Explicit non-dismissable banner; Read-tier only with no cloud | 9    | A    | 0016 |

## 14. Game AI (Tier B — distinct from BLD)

| System              | Module                     | Library                                       | Phase | Tier |
| ------------------- | -------------------------- | --------------------------------------------- | ----- | ---- |
| Behavior Trees      | `engine/gameai/`           | Custom ECS-backed                             | 13    | B    |
| Pathfinding         | `engine/gameai/`           | Recast/Detour                                 | 13    | B    |
| Utility AI / GOAP   | `engine/gameai/`           | Custom (optional overlays)                    | 13    | C    |

## 15. Cinematics & Content Tools (Tier B)

| System              | Module                     | Library                                       | Phase  | Tier |
| ------------------- | -------------------------- | --------------------------------------------- | ------ | ---- |
| Sequencer runtime   | `engine/sequencer/`        | Custom timeline                               | 18     | B    |
| Timeline editor     | `editor/sequencer_panel/`  | ImGui docked                                  | 18     | B    |
| Content cooker      | `tools/cook/`              | `gw_cook` custom                              | 6, 17  | A    |
| glTF import         | `tools/cook/`              | cgltf or fastgltf                             | 6      | A    |
| KTX2 import         | `tools/cook/`              | libktx + BC7/ASTC encoders                    | 6      | A    |
| Animation cook      | `tools/cook/`              | Ozz offline library                           | 13     | A    |
| Scene serialization | `engine/scene/`            | `.gwscene` versioned binary                   | 8      | A    |
| Sequencer format    | `engine/sequencer/`        | `.gwseq` versioned binary                     | 18     | B    |

## 16. Mod Support (Tier B)

| System              | Module                     | Library                                       | Phase | Tier |
| ------------------- | -------------------------- | --------------------------------------------- | ----- | ---- |
| Mod package format  | `engine/mod/`              | Custom (IR + assets + optional dylib)         | 18    | B    |
| Sandbox tier        | `engine/mod/` + `bld/`     | Reduced vscript tool tier                     | 18    | B    |
| Native mod signing  | `engine/mod/`              | Custom                                        | 18    | B    |

## 17. Platform Services (Tier B)

| System              | Module                         | Library                                       | Phase | Tier |
| ------------------- | ------------------------------ | --------------------------------------------- | ----- | ---- |
| Services interface  | `engine/platform_services/`    | Custom `IPlatformServices`                    | 16    | B    |
| Steam backend       | `engine/platform_services/`    | Steamworks SDK                                | 16    | B    |
| EOS backend         | `engine/platform_services/`    | Epic Online Services SDK                      | 16    | B    |
| Galaxy backend      | `engine/platform_services/`    | GOG Galaxy SDK                                | Post  | C    |
| Console backends    | —                              | When agreements exist                         | Post  | C    |

## 18. Security (Tier B)

| System              | Module                     | Library                                       | Phase | Tier |
| ------------------- | -------------------------- | --------------------------------------------- | ----- | ---- |
| Anti-cheat boundary | `engine/net/anticheat/`    | `IAntiCheat` interface                        | 24    | B    |
| Content integrity   | `engine/assets/`           | SHA-256 hashes on cooked assets               | 24    | A    |
| Server authority    | `engine/net/`              | Built-in to replication (§6)                  | 14    | B    |

## 19. Testing & QA (Tier A)

| System                   | Module          | Library                                  | Phase   | Tier |
| ------------------------ | --------------- | ---------------------------------------- | ------- | ---- |
| Unit tests (C++)         | `tests/`        | doctest                                  | 1+      | A    |
| Unit tests (Rust)        | `bld/*/tests/`  | cargo test                               | 9+      | A    |
| Integration (scenes)     | `tests/`        | Custom harness                           | 13+     | A    |
| Performance regression   | `tests/perf/`   | Tracy traces vs. baseline                | 5+      | A    |
| Smoke (sample scenes)    | CI workflow     | Scripted 60-second runs                  | 7+      | A    |
| Fuzzing                  | `tests/fuzz/`   | libFuzzer                                | 24      | B    |
| Mutation (Rust)          | `bld/` target   | cargo-mutants                            | 24      | B    |
| BLD eval harness         | `tests/bld/`    | Custom canned-task scorer                | 9+      | A    |
| UB / sanitizer nightly   | CI workflow     | ASan, UBSan, TSan, Miri                  | 1+      | A    |

## 20. Packaging & Release (Tier A)

| System              | Module                     | Library                                       | Phase | Tier |
| ------------------- | -------------------------- | --------------------------------------------- | ----- | ---- |
| Installer           | `cmake/packaging/`         | CMake + CPack                                 | 24    | A    |
| Windows installer   | `cmake/packaging/`         | MSIX                                          | 24    | A    |
| Linux packages      | `cmake/packaging/`         | AppImage + DEB + RPM                          | 24    | A    |
| Signing             | CI workflow                | Authenticode (Win), GPG (Linux)               | 24    | A    |
| Symbol servers      | CI workflow                | PDB (Win), split DWARF (Linux)                | 24    | A    |
| LTS branch cadence  | CI workflow                | Quarterly cuts, 12-month backport SLA         | 25    | A    |

---

## Scope summary

| Bucket                | Count |
| --------------------- | ----- |
| Tier A (must ship)    | ~72   |
| Tier B (likely ship)  | ~35   |
| Tier C (post-v1)      | ~15   |
| Hardware-gated (G)    | ~5    |
| **Total catalogued**  | **~127** |

**Tiering principle:** if the team is falling behind, Tier B is the first to cut. Tier A is not cut without stakeholder-level sign-off. Tier C is never promoted without board-level review.

This list is the **complete** set. Every system someone might ask about — from render to cinematics to anti-cheat to mod support — is either listed here or is explicitly not being built. There is no hidden subsystem, no "we'll figure it out later," no undocumented commitment.

---

*Inventoried deliberately. Tiered defensively. Shipped by Cold Coffee Labs.*
