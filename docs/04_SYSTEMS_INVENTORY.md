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

*Sacrilege* is a **3D arena FPS** layered with a theological-horror Story Bible. Cold Coffee Labs is a **horror studio** and Greywater is a **franchise content platform**. The systems below reflect both scopes:

- **In:** arena-scale procedural generation, fast FPS combat, persistent gore, Martyrdom loop, Nine Circles (with Location Skins), runtime AI (bounded, deterministic, §2.2.2), narrative reactivity (dialogue graph, Sin-signature, Acts I–III, Grace finale), seven IP-agnostic franchise services
- **Out:** planetary simulation, orbital mechanics, atmospheric layers (flight), galaxy generation, open-world survival, base building, territory systems, faction utility AI, unbounded runtime ML, internet-connected AI

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
| Vulkan 1.3 backend | `engine/render/` | volk + VMA (ADR-0111 baseline) | 4 | A |
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
| Horror post stack | `engine/render/post/` | CA, film grain, screen shake, Sin tendrils, Ruin desaturation, Rapture whiteout, **God Mode reality warp** (mirror-step, gravity invert) — all in one pass | 21/22 | A |
| FSR 2 upscaling (Vulkan) | `engine/render/post/fsr2/` | AMD FSR 2/3.1 (no Frame Gen on Polaris); `GW_ENABLE_FSR2` | 24 | B |
| HDR10 swapchain output | `engine/render/hal/` | `VK_EXT_hdr_metadata` + PQ tonemap `shaders/post/tonemap_pq.frag.hlsl`; `r.HDROutput` | 24 | B |
| Frame pacing / present timing | `engine/render/hal/swapchain.cpp` | `VK_KHR_present_id` + `VK_KHR_present_wait` | 24 | A |
| GPU-driven occlusion culling | `engine/render/forward_plus/` | Hi-Z + `VK_KHR_draw_indirect_count`; `r.GPUCulling` | 24 | B |
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
| **God Mode / Blasphemy State** onset + VFX hooks | `gameplay/martyrdom/god_mode.hpp` | 22 | A |
| **Blasphemy State** reality-warp transitions | `gameplay/martyrdom/blasphemy_state.hpp` | 22 | A |
| **Grace meter** (Act III terminal Blasphemy `forgive`) | `gameplay/martyrdom/grace_meter.hpp` | 23 | A |
| **Malakor / Niccolò** dual-consciousness voice director | `gameplay/characters/malakor_niccolo/` | 22 | A |
| **Acts I / II / III** encounter gates | `gameplay/acts/` | 22–23 | A |
| **Logos alternate Phase 4** (Silent God, Grace finale) | `gameplay/boss/logos/` | 23 | A |

---

## 2c. Runtime AI (Tier A — seed-fed, deterministic, §2.2.2 carve-out)

On-device C++ inference only. Bounded per-system budgets on RX 580; total ≤ 0.74 ms/frame. All weights are pinned, versioned, Ed25519-signed cooked assets under `assets/ai/`.

| System | Module | Library / Decision | Phase | Tier | Budget |
|--------|--------|--------------------|-------|------|--------|
| Inference runtime (CPU) | `engine/ai_runtime/inference_runtime.*` | ggml / llama.cpp C++ headers, CPU backend (ADR-0095) | 26 | A | n/a |
| Inference runtime (GPU, opt-in read-only) | `engine/ai_runtime/inference_runtime.*` | ggml Vulkan backend behind `GW_AI_VULKAN=OFF` | 26 | B | n/a |
| Pinned model registry | `engine/ai_runtime/model_registry.hpp` | BLAKE3 + Ed25519 verify; semantic versioning | 26 | A | n/a |
| **Hybrid AI Director** — rule state machine + bounded RL params | `engine/ai_runtime/director_policy.hpp` | L4D-style Build-Up/Sustained/Fade/Relax + PPO-tuned parameter clamps (ADR-0097) | 26 | A | ≤ 0.1 ms |
| **Symbolic adaptive music** — layered stems + tiny transformer | `engine/ai_runtime/music_symbolic.hpp` | RoPE ≤ 1M params, CPU (ADR-0098) | 26 | A | ≤ 0.2 ms |
| **Neural material evaluator** — runtime eval of cooked weights | `engine/ai_runtime/material_eval.hpp` | ggml Vulkan compute; non-authoritative | 26 | B | ≤ 0.4 ms |
| AI CVars / budgets / toggles | `engine/ai_runtime/ai_cvars.hpp` | Perf gate `gw_perf_gate_ai_runtime` | 26 | A | n/a |
| AI Director Sandbox (standalone app) | `apps/sandbox_director/` | Headless-CI + interactive ImGui modes; `gw_perf_gate_director` (ADR-0107) | 26 | A | ≤ 0.1 ms |

**Explicit rejections on RX 580 hardware floor:** full neural audio synthesis at runtime, on-device LLM dialog generation, Internet-sourced model updates, on-device fine-tuning outside replicated save-state.

---

## 2d. Narrative Layer (Tier A — Story Bible runtime)

Story-Bible-aware systems that skin the Martyrdom spine with Malakor / Niccolò, Acts I–III, and the Grace finale. All dialogue and Act data is cooked data (`.gwdlg`, `.gwact`); runtime is deterministic and rollback-safe.

| System | Module | Library / Decision | Phase | Tier |
|--------|--------|--------------------|-------|------|
| Dialogue graph runtime | `engine/narrative/dialogue_graph.hpp` | `.gwdlg` reader; typed IR (speaker / line / trigger / consequence) | 21 | A |
| Act / Phase state machine | `engine/narrative/act_state.hpp` | Acts I / II / III with phase gates (ADR-0099) | 21 | A |
| Sin-signature fingerprint | `engine/narrative/sin_signature.hpp` | Rolling table (God-Mode-uptime, Precision, Cruelty, Hitless, Deaths/area) (ADR-0100) | 22 | A |
| Malakor / Niccolò voice director | `engine/narrative/voice_director.hpp` | Speaker selection + line pick (ADR-0099) | 22 | A |
| Grace meter | `engine/narrative/grace_meter.hpp` | Act III-only; terminal `forgive` Blasphemy (ADR-0101) | 23 | A |
| Dialogue reachability analysis | BLD tool `dialogue_reachability` | Agent-side graph analysis of `.gwdlg` (HITL) | 23 | B |

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
| **In-game UI runtime** | `engine/ui/` | RmlUi service + Vulkan HAL backend + text shaping | 11 | A |
| **Shipped RML/HTML/CSS assets** | `assets/ui/` | — (cooked UI content) | 11+ | A |
| **Privacy / DSAR UI fragments** | `ui/privacy/` | RmlUi (GDPR consent, data-export dialogs) | 16 | A |

**Rule:** ImGui is for the editor. RmlUi is for in-game. These are permanent, non-negotiable choices.

### 8.1 UI root ownership

Three directories participate in the in-game UI stack and each has a distinct role:

- **`engine/ui/`** — C++23 runtime only. Owns the RmlUi service, the Vulkan HAL backend adapter, text shaping, and the event plumbing. No art, no markup. ADR-0026..0028.
- **`assets/ui/`** — Cooked, version-stable RML/HTML/CSS/images referenced by the shipping game. Built through the `gw_cook` pipeline like any other asset. Human-authored sources live under `content/ui/`.
- **`ui/privacy/`** — Phase 16 DSAR / consent UI fragments (GDPR, COPPA). Kept as a separate root because the privacy surface has stricter review and ships in every build including demo / free trials. ADR-0062.

All three are **additive**, not layered. Nothing under `assets/ui/` or `ui/privacy/` is compiled C++; they are data consumed by `engine/ui/`.

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
| Gameplay a11y toggles (Tier A, ADR-0116) | `engine/a11y/` + `gameplay/hud/` | Colour-blind palettes, chromatic/shake/motion-blur sliders, subtitles + speaker labels, gore slider, photosensitivity pre-warning, audio mono-mix + per-channel sliders, one-hand input preset | 24 | A |
| Editor a11y toggles | `editor/a11y/` | Reduce corruption, disable vignette/glitch, force HC, force mono font | 21–23 | A |
| Narrative localization (`.gwdlg` strings) | `engine/narrative/` + `engine/i18n/` | CSV round-trip via `editor/panels/localization_panel` | 21–23 | B |

---

## 11. Persistence & Telemetry

| System | Module | Library | Phase | Tier |
|--------|--------|---------|-------|------|
| Save/load (`.gwsave`) | `engine/persistence/` | Custom + SQLite | 15 | A |
| Hell Seed save/restore | `gameplay/` | Custom via persistence | 22 | A |
| Speedrun timer persistence | `gameplay/ui/` | Custom | 24 | B |
| Crash reporting | `engine/core/crash_reporter.cpp` | Sentry Native SDK via `GW_ENABLE_SENTRY`; minidump upload + symbol server (ADR-0114) | 15/24 | B |
| Content signing (cooked assets) | `tools/cook/cook_worker.cpp` | Ed25519 signature + BLAKE3 manifest; runtime verify in Release (ADR-0115) | 24 | A |
| Opt-in telemetry (GDPR) | `engine/telemetry/` | Aggregated crash + seed + completion-rate; first-run opt-in | 24 | B |
| Gameplay replay capture | `bld/bld-replay/` (extended) | `gameplay_replay` schema: input stream + seed + version; < 1 MB per run; bitwise-deterministic reproduce | 24 | A |
| Steam Input integration (Tier A) | `engine/input/steam_input_glue.cpp` | Correct Deck glyphs; no M+KB on controller; default bindings complete (ADR-0113) | 24 | A |
| Mod SDK (formalized) | `gameplay/mod_example/` + docs | Manifest TOML + filesystem jail + IPC channel for BLD tools | 24 | B |

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
| `bld-bridge` | Editor ↔ BLD in-process bridge |
| `bld-tools-macros` | `#[bld_tool]` proc-macro implementation crate |
| `bld-replay` | Deterministic replay (audit + gameplay_replay extension) |

**BLD tools relevant to Sacrilege authoring:**
- `encounter.place_enemy(circle, pos, type, patrol_path)`
- `encounter.generate_room_layout(circle, enemy_density, resource_density)`
- `circle.set_sdr_params(circle_id, params_json)`
- `circle.preview_biome(circle_id, seed)` — renders a preview frame
- `asset.place_mesh(path, pos, rot, scale)`
- `behavior_tree.create_node(bt_asset, node_type, parent_id)`
- `debug.spawn_enemy(type, pos)` — in play mode
- `concept_to_material(prompt | concept_image) -> .gwmat` (cook-time; HITL)
- `encounter_suggest(circle, difficulty)` → placement set
- `exploit_detect(level)` → list of stuck spots, soft-locks, infinite loops
- `scene_heatmap(replay_trace)` → heat visualization
- `voice_line_generate(character, context)` → line draft (HITL mandatory)
- `dialogue_reachability(.gwdlg)` → unreachable/dead branches
- `director_tune(scenario_yaml, reward_weights)` → checkpoint search with early stopping

---

## 13. Franchise Services (Tier A — IP-agnostic content platform, Phase 27)

The engine is factored into **seven services** that define the reusable surface for every Cold Coffee Labs IP. Sacrilege consumes all seven; *Apostasy* / *Silentium* / *The Witness* prove the platform in Phase 29. Data schemas are IP-agnostic; no Sacrilege-specific types leak.

| Service | Module | What it owns | Schema root | Phase | Tier |
|---------|--------|--------------|-------------|-------|------|
| **Material Forge** | `engine/services/material_forge/` | `.gwmat` authoring + neural PBR eval hook + node graph IR | `schema/material.hpp` | 27 | A |
| **Level Architect** | `engine/services/level_architect/` | Blacklake arena layouts + cook-time WFC/RL scorer + deterministic parameter bundles | `schema/layout.hpp` | 27 | A |
| **Combat Simulator** | `engine/services/combat_simulator/` | Enemy DNA graph + encounter scoring + BT registration index | `schema/encounter.hpp` | 27 | A |
| **Gore System** | `engine/services/gore/` | Dismemberment rules, decal propagation, limb entities as nav obstacles | `schema/gore.hpp` | 27 | A |
| **Audio Weave** | `engine/services/audio_weave/` | Layered stems + symbolic transformer music runtime + BPM-sync crossfade | `schema/music.hpp` | 27 | A |
| **AI Director** | `engine/services/director/` | Hybrid rule state machine + bounded RL params + Sandbox | `schema/director.hpp` | 27 | A |
| **Editor Co-Pilot** | `engine/services/editor_copilot/` | BLD tool surface + HITL flows + transaction brackets | `schema/copilot.hpp` | 27 | A |

**Contract test:** every service ships a doctest + an "imaginary second IP" integration test (a trivial prototype linking only the service's schema + impl headers) that is required to stay green.

---

*Every row is a decision. Every tier is a commitment. Every "Out-of-scope" is a feature we deliberately chose not to build.*
