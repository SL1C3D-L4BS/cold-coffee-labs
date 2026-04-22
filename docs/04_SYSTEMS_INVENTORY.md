# 04 — Systems Inventory

**Status:** Reference · Last revised 2026-04-22  
**Precedence:** L5 — authoritative subsystem list; narrowed by ADRs

> Tiering is the core defense against scope creep. Every architectural question ends: *"Is this Tier A?"*

## Tier Legend

| Tier | Commitment |
|------|------------|
| **A** | **Ships in v1.0.** Cannot be cut without stakeholder scope decision. |
| **B** | Ships if schedule holds. Can slip to Post-v1. |
| **C** | Stretch / Post-v1. Listed so we know we chose not to build it yet. |
| **G** | Hardware-gated. Ships only where `RHICapabilities` reports support. |

---

## Scope Note

*Sacrilege* is a **3D arena FPS**. The systems below reflect that scope:

- **In:** arena-scale procedural generation, fast FPS combat, persistent gore, Martyrdom loop, Nine Circles
- **Out:** planetary simulation, orbital mechanics, atmospheric layers (flight), galaxy generation, open-world survival, base building, territory systems, faction utility AI — none of these are *Sacrilege*

---

## 1. Core C++ Foundation (Tier A)

| System | Module | Library / Decision | Phase |
|--------|--------|--------------------|-------|
| Platform abstraction | `engine/platform/` | Custom; GLFW for windowing | 2 |
| Memory allocators | `engine/memory/` | Custom (frame, arena, pool, free-list) | 2 |
| Logger | `engine/core/` | Custom spdlog-style | 2 |
| Assertions | `engine/core/` | Custom `GW_ASSERT` with stack trace | 2 |
| Error types | `engine/core/` | Custom `Result<T, E>` | 2 |
| Containers | `engine/core/` | Custom (span, array, vector, hash_map, ring) | 2 |
| Strings | `engine/core/` | Custom UTF-8 with SSO | 2 |
| Command bus | `engine/core/command/` | Custom `ICommand` + `CommandStack` | 3 |
| Event system | `engine/core/events/` | Custom type-safe pub/sub | 11 |
| Config / CVars | `engine/core/config/` | Custom registry; TOML persistence | 11 |
| Math library | `engine/math/` | Custom Vec/Mat/Quat/Transform + SIMD | 3 |
| Job system | `engine/jobs/` | Custom fiber-based work-stealing | 3 |
| Reflection | `engine/core/` | Macro-based; P2996 migration later | 3 |
| Serialization core | `engine/core/` | Versioned binary primitives | 3 |
| ECS | `engine/ecs/` | Custom archetype-based | 3 |
| Dynamic library load | `engine/scripting/` | Custom hot-reload via platform layer | 2 |

---

## 2. Rendering (Tier A — RX 580 baseline @ 144 FPS for Sacrilege)

| System | Module | Library / Decision | Phase | Tier |
|--------|--------|--------------------|-------|------|
| Rendering HAL | `engine/render/hal/` | Custom ~30-type abstraction | 4 | A |
| Vulkan 1.2 backend (1.3 opportunistic) | `engine/render/` | volk + VMA | 4 | A |
| Descriptor system (bindless) | `engine/render/` | `VK_EXT_descriptor_indexing` | 4 | A |
| Pipeline cache | `engine/render/` | Vulkan pipeline cache + content hashes | 4 | A |
| Frame graph | `engine/render/` | Custom DAG; auto-barriers, transient aliasing | 5 | A |
| Async compute path | `engine/render/` | Timeline semaphores, multi-queue | 5 | A |
| HLSL shader pipeline | `engine/render/shader/` | HLSL + DXC → SPIR-V; permutations | 5, 17 | A |
| Material system | `engine/render/material/` | `MaterialTemplate` / `MaterialInstance`, `.gwmat` | 5, 17 | A |
| Forward+ lighting | `engine/render/` | Custom clustered | 5 | A |
| Cascaded shadows | `engine/render/` | Custom | 5 | A |
| IBL | `engine/render/` | Split-sum approximation | 5 | A |
| Tonemapping | `engine/render/post/` | PBR Neutral (default) + ACES | 5, 17 | A |
| Horror post stack | `engine/render/post/` | CA, film grain, screen shake, Sin tendrils, Ruin desaturation, Rapture whiteout — all in one pass | 21 | A |
| GPU blood decal system | `engine/render/vfx/` | Compute stamp + drip; 512 ring buffer | 21 | A |
| Circle-specific atmosphere | `engine/render/` | Per-circle fog/ambient parameters; no full raymarched scattering | 21 | A |
| GPU particles | `engine/vfx/particles/` | Compute emit/sim; muzzle flash, blood spray, explosions | 17 | B |
| Post-processing (bloom, TAA, DoF, grain) | `engine/render/post/` | Dual-Kawase bloom, k-DOP TAA, McGuire MB, CoC DoF | 17 | B |
| Ray tracing | `engine/render/rt/` | `VK_KHR_ray_tracing_pipeline` | Post | G |
| Mesh shaders | `engine/render/` | `VK_EXT_mesh_shader` | Post | G |
| WebGPU backend | `engine/render/webgpu/` | Dawn (eventual) | Post | C |

---

## 2a. Blacklake Arena Generation (Tier A — Sacrilege-facing)

These systems generate the Nine Inverted Circles. They are **arena-scale** — not planet-scale, not universe-scale.

| System | Module | Library / Decision | Phase | Tier |
|--------|--------|--------------------|-------|------|
| Hell Seed management (256-bit, HEC) | `engine/world/universe/` | Custom; SHA3-256 domain separation | 19 | A |
| HEC seeding protocol | `engine/world/universe/` | Hierarchical entropy cascade | 19 | A |
| SDR Noise primitive | `engine/world/sdr/` | Gabor wavelet superposition; replaces Perlin | 19 | A |
| PCG64-DXSM RNG | `engine/world/universe/` | Per-chunk deterministic RNG | 19 | A |
| Chunk streaming | `engine/world/streaming/` | 32m chunks, LRU cache, time-sliced generation | 19 | A |
| Biome classification | `engine/world/biome/` | Temperature+humidity+chaos → BiomeId per Circle | 19 | A |
| Resource distribution | `engine/world/resources/` | Poisson-disc placement from chunk seed | 19 | A |
| Determinism validator | `engine/world/universe/` | MurmurHash3 cross-platform verification | 19 | A |
| GPTM topology model | `engine/world/gptm/` | Hybrid heightmap / SVO; LOD mesh builder | 20 | A |
| LOD selector | `engine/world/gptm/` | Screen-space error metric; 5 LOD levels | 20 | A |
| Terrain tile renderer | `engine/world/gptm/` | GPU indirect draw; 5 draw calls for all terrain | 20 | A |
| Circle SDR profiles | `engine/world/biome/` | Constexpr per-Circle SDR params (Limbo→Treachery) | 20 | A |
| Floating origin | `engine/world/origin/` | Recentre at 2048m; fires OriginShiftEvent | 20 | A |
| Vegetation instancing | `engine/world/vegetation/` | GPU indirect draw, 4 LOD, 256 instances/chunk | 20 | A |
| Circle-specific fog | `engine/world/biome/` | Per-biome fog parameters; no volumetric scattering | 21 | A |

**Not in scope (explicitly excluded):**
- ~~Spherical planet rendering~~ — *Sacrilege* is arena-scale
- ~~Orbital mechanics~~ — no space flight
- ~~Galaxy/system/planet hierarchy~~ — the "universe" is Hell, nine arenas
- ~~Atmospheric layers for flight~~ — no ground-to-orbit
- ~~Starfield~~ — Circle IX (Treachery) has a black void sky, not a star catalog
- ~~Ecosystem simulation~~ — enemies are hand-designed, not simulated ecosystems
- ~~Faction utility AI~~ — enemies have BTs, not strategic faction AI
- ~~Territory claim system~~ — not a survival game

---

## 2b. Sacrilege Gameplay Systems (Tier A — all in `gameplay/`)

| System | Module | Phase | Tier |
|--------|--------|-------|------|
| Martyrdom Engine — Sin accrual | `gameplay/martyrdom/` | 22 | A |
| Martyrdom Engine — Mantra accrual | `gameplay/martyrdom/` | 22 | A |
| Blasphemy system (5 abilities) | `gameplay/martyrdom/` | 22 | A |
| Rapture/Ruin state machine | `gameplay/martyrdom/` | 22 | A |
| StatCompositionSystem | `gameplay/martyrdom/` | 22 | A |
| Player controller — swept-AABB | `gameplay/player/` | 22 | A |
| Bunny hop + air control | `gameplay/player/` | 22 | A |
| Slide mechanic | `gameplay/player/` | 22 | A |
| Wall kick | `gameplay/player/` | 22 | A |
| Grapple hook | `gameplay/player/` | 22 | A |
| Blood surf | `gameplay/player/` | 22 | A |
| Weapon system (6 weapons + alt-fires) | `gameplay/weapons/` | 22 | A |
| Hitscan hit detection | `gameplay/weapons/` | 22 | A |
| Projectile system | `gameplay/weapons/` | 22 | A |
| Melee hit detection | `gameplay/weapons/` | 22 | A |
| Gore & dismemberment | `gameplay/gore/` | 23 | A |
| Limb entities (nav obstacles) | `gameplay/gore/` | 23 | A |
| Blood decal placement | `gameplay/gore/` | 23 | A |
| Diegetic HUD (RmlUi) | `gameplay/hud/` | 21 | A |
| Enemy archetypes — all 8 types | `gameplay/enemies/` | 23 | A |
| BT node registrations | `gameplay/enemies/` | 23 | A |
| Glory Kill system | `gameplay/enemies/` | 23 | A |
| God Machine boss — 4 phases | `gameplay/boss/` | 23 | A |
| Arena collapse system | `gameplay/boss/` | 23 | A |
| Circle transition system | `gameplay/circles/` | 23 | A |
| Speedrun timer + seed display | `gameplay/ui/` | 24 | B |

---

## 3. Physics & Simulation

| System | Module | Library | Phase | Tier |
|--------|--------|---------|-------|------|
| Rigid body dynamics | `engine/physics/` | Jolt Physics | 12 | A |
| Character controller (custom AABB) | `gameplay/player/` | Custom over Jolt | 22 | A |
| Ragdoll / limb physics | `engine/physics/` | Jolt | 23 | A |
| Constraints | `engine/physics/` | Jolt | 12 | A |
| Triggers / raycasts / shape queries | `engine/physics/` | Jolt | 12 | A |
| Deterministic mode | `engine/physics/` | Jolt lockstep | 12 | A |
| Vehicles | `engine/physics/` | Jolt | Post | C |

---

## 4. Animation

| System | Module | Library | Phase | Tier |
|--------|--------|---------|-------|------|
| Skeletal runtime | `engine/anim/` | Ozz-animation | 13 | A |
| Compression | `engine/anim/` | ACL | 13 | A |
| Blend trees | `engine/anim/` | Custom over ECS | 13 | A |
| IK (FABRIK, CCD) | `engine/anim/` | Ozz + custom | 13 | A |
| Morph targets | `engine/anim/` | glTF pipeline | 13 | A |
| BPM-sync melee events | `gameplay/weapons/` | Custom frame-event system | 21 | A |
| Dismemberment attach-point events | `gameplay/gore/` | Custom | 23 | A |

---

## 5. Audio

| System | Module | Library | Phase | Tier |
|--------|--------|---------|-------|------|
| Device / mixer | `engine/audio/` | miniaudio | 10 | A |
| Spatial audio (HRTF) | `engine/audio/` | Steam Audio | 10 | A |
| Decoders | `engine/audio/` | Opus, Ogg Vorbis, WAV | 10 | A |
| DSP (reverb/EQ/compression) | `engine/audio/` | Steam Audio + custom | 10 | A |
| Streaming music | `engine/audio/` | Custom disk-backed | 10 | A |
| Martyrdom audio controller | `gameplay/martyrdom/` | Custom over engine audio | 21 | A |
| BPM-sync layer crossfade | `engine/audio/` | Custom layer fade API | 21 | A |

---

## 6. Networking (Tier B — *Sacrilege* has optional co-op)

| System | Module | Library | Phase | Tier |
|--------|--------|---------|-------|------|
| Transport | `engine/net/` | GameNetworkingSockets | 14 | B |
| ECS replication | `engine/net/` | Custom delta + interest management | 14 | B |
| Rollback (Martyrdom loop deterministic) | `engine/net/` | Custom rewind-replay | 14 | B |
| Voice chat | `engine/net/` | Opus + GNS + Steam Audio spatial | 14 | B |
| Seed sharing / co-op hell | `gameplay/net/` | Custom lobby + seed broadcast | 22 | B |

---

## 7. Input

| System | Module | Library | Phase | Tier |
|--------|--------|---------|-------|------|
| Device layer | `engine/input/` | SDL3 | 10 | A |
| Keyboard/mouse | `engine/input/` | Platform layer direct | 10 | A |
| Action-map system | `engine/input/` | Custom | 10 | A |
| Accessibility (hold-to-toggle, single-switch) | `engine/input/` | Custom | 10 | A |

---

## 8. User Interface

| System | Module | Library | Phase | Tier |
|--------|--------|---------|-------|------|
| **Editor UI** | `editor/` | Dear ImGui + docking + viewports + ImNodes + ImGuizmo | 7 | A |
| **In-game HUD** | `gameplay/hud/` | RmlUi + HarfBuzz + FreeType | 21 | A |

**Rule:** ImGui is for the editor. RmlUi is for in-game. These are permanent, non-negotiable choices.

---

## 9. The Greywater Editor — Sacrilege Toolchain

| Tool | Module | Phase | Tier |
|------|--------|-------|------|
| 3D viewport with real-time preview | `editor/viewport/` | 7 | A |
| Asset browser (mesh, texture, audio) | `editor/asset_browser/` | 7 | A |
| Entity hierarchy panel | `editor/hierarchy/` | 7 | A |
| Circle editor (SDR param tuning, real-time preview) | `editor/circle_editor/` | 20 | A |
| Encounter editor (enemy placement, patrol paths) | `editor/encounter_editor/` | 23 | A |
| Behavior tree wiring panel | `editor/bt_panel/` | 13 | A |
| BLD AI panel (chat, tool invocation) | `editor/agent_panel/` | 9 | A |
| Sequencer panel (.gwseq timeline) | `editor/seq_panel/` | 18 | B |
| Play mode (F5 launch from cursor) | `editor/play_mode/` | 7 | A |
| Shader hot-reload | `editor/` | 5 | A |

---

## 10. Localization & Accessibility

| System | Module | Library | Phase | Tier |
|--------|--------|---------|-------|------|
| String tables | `engine/i18n/` | Custom binary | 16 | B |
| Locale runtime | `engine/i18n/` | ICU | 16 | B |
| WCAG 2.2 AA accessibility | `engine/a11y/` | AccessKit-C | 16 | B |
| Post-effect accessibility toggles | `gameplay/hud/` | CVar-driven | 21 | A |

---

## 11. Persistence & Telemetry

| System | Module | Library | Phase | Tier |
|--------|--------|---------|-------|------|
| Save/load (`.gwsave`) | `engine/persistence/` | Custom + SQLite | 15 | A |
| Hell Seed save/restore | `gameplay/` | Custom via persistence | 22 | A |
| Speedrun timer persistence | `gameplay/ui/` | Custom | 24 | B |
| Crash reporting | `engine/telemetry/` | Sentry Native SDK | 15 | B |

---

## 12. BLD — Brewed Logic Directive

BLD is the AI copilot for the editor. It is a Rust crate linked into the editor via narrow C-ABI. It serves *Sacrilege* authoring.

| Crate | Role |
|-------|------|
| `bld-ffi` | C-ABI boundary; Miri-validated |
| `bld-mcp` | MCP 2025-11-25 wire protocol |
| `bld-tools` | `#[bld_tool]` proc macro; ~60 tools for scene, encounter, circle, asset, debug |
| `bld-provider` | Anthropic cloud + local model backends |
| `bld-rag` | RAG pipeline — tree-sitter chunking, sqlite-vec index |
| `bld-agent` | Main agent loop |
| `bld-governance` | HITL approval; every BLD action is undoable via CommandStack |

**BLD tools relevant to Sacrilege authoring:**
- `encounter.place_enemy(circle, pos, type, patrol_path)`
- `encounter.generate_room_layout(circle, enemy_density, resource_density)`
- `circle.set_sdr_params(circle_id, params_json)`
- `circle.preview_biome(circle_id, seed)` — renders a preview frame
- `asset.place_mesh(path, pos, rot, scale)`
- `behavior_tree.create_node(bt_asset, node_type, parent_id)`
- `debug.spawn_enemy(type, pos)` — in play mode

---

*Every row is a decision. Every tier is a commitment. Every "Out-of-scope" is a feature we deliberately chose not to build.*
