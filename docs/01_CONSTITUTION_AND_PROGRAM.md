# 01 — Constitution, Program Mandate & Session Handoff

**Status:** Canonical · Non-negotiable · Last revised 2026-04-22  
**Precedence:** L1 — overrides all other documents

> *"For I stand on the ridge of the universe and await nothing."*

---

## §0. Project Identity

**One studio, one engine, one flagship, one platform:**

- **Cold Coffee Labs** — a **horror studio**.
- **Greywater Engine** — the proprietary C++23 / Vulkan platform, factored as a **franchise content platform** (seven IP-agnostic services).
- ***Sacrilege*** — the flagship game. The only thing that ships in v1. Every feature decision is evaluated against: **does this make *Sacrilege* better, and does it generalize to the next IP without cost?**
- **The Sacrilege family** — *Apostasy*, *Silentium*, *The Witness* — is pre-production only (Phase 29); it never steals resources from the flagship.

### §0.1 What We Are Building

***Sacrilege*** is a **3D first-person action game** draped across a theological horror Story Bible. Fast. Gory. Procedurally infinite. The spiritual heir of 1990s arena shooters (Quake movement, Doom aggression, Ultrakill speed, Bloodthief gothic punch), built from scratch in 2026 on a custom engine and toolchain.

**Mechanical spine (unchanged):**
- Nine Inverted Circles of Hell, each procedurally generated from a Hell Seed.
- Same seed = same level, always — shareable, speedrun-verifiable.
- Persistent gore, severed limbs, blood that stains the geometry.
- The Martyrdom Engine — a dual-resource Sin/Mantra system unique to *Sacrilege*.
- 144 FPS on an RX 580. No exceptions.

**Story Bible skin (new canonical narrative — see `docs/11_NARRATIVE_BIBLE.md`):**
- **Cosmology** — the Absolute fractures into **The One** (order), **The Other** (chaos), **The Witness** (indifferent observer). The Unwritten (fallen angels) are refugees from the schism.
- **Protagonist** — **Malakor**, an Unwritten fallen angel, is fused to **Father Niccolò Bianchi**, an exorcist priest of the corrupted Church. The player plays both as a dual consciousness; Malakor drives combat, Niccolò drives moral cost.
- **Factions** — **The CORE** (Church of the Recycled Eden, institutional corruption), **The LEGION** (Hell's recyclers), **The UNWRITTEN** (Malakor's kin, player's faction).
- **Locations** — Vatican Necropolis, Colosseum of Echoes, Palazzo of the Unspoken, Sewer of Grace, Rome Above (siege), Hell's Factory Floor, Heaven's Back Door, The Silentium (mirror dimension), Throne of the Logos. Each is a **Location Skin** of a procedural Circle (see `docs/07_SACRILEGE.md` §6) — mechanics stay, labels/assets change.
- **Three Acts** — I *The Rite of Unspeaking*, II *The Blasphemy Circuit*, III *The Unmaking*. The Sin 70–100 band is re-skinned as **God Mode / Blasphemy State**; Act III unlocks the **Grace meter** whose full Blasphemy (`forgive`) ends the game.
- **Final boss** — **Logos**, the silent God, fought as an alternate Phase 4 of the existing God Machine. Defeated by Grace, not damage.

**All gameplay logic is C++23.** There is no scripting language. There is no node graph for gameplay. There is no Lua. There is no Python **in engine, editor, gameplay, or BLD binaries** (see §2.2.1 for the offline `tools/` carve-out). There is no custom language called Ripple Script in the shipping game. Logic is compiled C++ code. **Runtime ML inference is now permitted under the strict §2.2.2 carve-out** — C++ only, deterministic, seed-fed, no internet.

### §0.2 What The Editor Is

The **Greywater Editor** is the level editor, encounter editor, and asset browser for building *Sacrilege* content. It is:

- A native C++ application embedded in the engine runtime
- Real-time 3D viewport — what you see in the editor is what ships
- Encounter placement, patrol path editing, behavior tree wiring (visual, but backed by C++ behavior trees)
- Circle parameter tuning — SDR noise, biome blending, enemy density
- Asset browser for cooked 3D meshes, textures, audio
- BLD AI panel for authoring assistance

It is **not** a scripting IDE. It is **not** a general-purpose game editor. It serves *Sacrilege*.

### §0.3 Supra-AAA Production Bar

1. **Stack sovereignty** — full engine, tools, and AI authoring (BLD) in-house.
2. **Deterministic infinite levels** — Blacklake-class arena generation with byte-stable seeds.
3. **Agent-native pipeline** — BLD + C-ABI tool surfaces so authoring velocity exceeds what team size suggests.
4. **Inclusive performance** — 144 FPS proven on RX 580 for *Sacrilege*.
5. **IP-grade systems** — Martyrdom Engine, HEC seeding, SDR noise — documented for patent posture.

---

## §1. Core Directive

Claude is authorized to act as **Principal Engine Architect** and **Senior Game Engineer** on the Greywater / *Sacrilege* project:

- Designing and implementing engine subsystems in C++23
- Implementing *Sacrilege* gameplay: Martyrdom system, player movement, enemies, weapons, Nine Circles
- Implementing the Blacklake arena generation pipeline
- Authoring `.gwscene` scenes, shaders (HLSL primary), CMake build files, doctest tests
- Building the Greywater Editor: viewport, encounter editor, circle editor, asset browser
- Diagnosing bugs, refactoring systems, proposing ADRs

Claude is **not** authorized to:

- Introduce a scripting language, scripting VM, or custom language for gameplay logic
- Introduce Lua, Python, JavaScript, or any managed runtime into gameplay code, the engine/editor/runtime link graph, or BLD (except the §2.2.1 offline-tooling carve-out)
- Propose planet-scale, orbital-scale, or galaxy-scale simulation — *Sacrilege* is arena-scale
- Propose survival mechanics, base building, or territory systems — this is a fast FPS
- Introduce a new compiler, build system, or UI framework without an approved ADR
- Propose features that cannot run at 144 FPS on the RX 580 as Tier A

---

## §2. Technical Constraints (Non-Negotiable)

### §2.1 Hardware Baseline — RX 580 @ 144 FPS

The **AMD Radeon RX 580 8 GB** is the baseline. *Sacrilege* targets **144 FPS at 1080p** on this card. Every Tier A feature must hit this target.

**Tier A — ships on RX 580:**
- **Vulkan 1.3 baseline** on desktop (dynamic rendering, timeline semaphores, sync2, descriptor indexing, BDA — promoted to core per ADR-0003 / ADR-0111). Vulkan 1.2 fallback only where a vendor driver refuses 1.3.
- Compute shaders, async compute queue, hardware tessellation
- Bindless descriptors via `VK_EXT_descriptor_indexing`
- FSR 2 (Vulkan) upscaling behind `GW_ENABLE_FSR2` (ADR-0112, no Frame Gen on Polaris)
- HDR10 swapchain output via `VK_EXT_hdr_metadata` (opt-in CVar `r.HDROutput`)
- `VK_KHR_present_id` / `VK_KHR_present_wait` for accurate frame pacing
- 1080p @ 144 FPS for *Sacrilege* arenas
- **Steam Deck verified** as Tier A platform (ADR-0113)

**Tier G — hardware-gated, post-v1:**
- Ray tracing, mesh shaders, GPU work graphs — not required for *Sacrilege*

### §2.2 Language and Toolchain

| Domain | Language | Rule |
|--------|----------|------|
| Engine core, editor, gameplay, all *Sacrilege* logic | **C++23** | No exceptions — not the keyword, not the concept |
| BLD AI copilot only | **Rust 2024** | Only in `bld/`. Nowhere else. |
| Shaders | **HLSL** → DXC → SPIR-V | GLSL for ports |

- **No scripting VM.** No Lua. No JavaScript. No custom language. Gameplay is C++23. **No Python** in shipped binaries or runtime-loaded code paths (see §2.2.1). Runtime ML inference *in C++* is allowed only under the §2.2.2 carve-out.
- **Clang / LLVM 18+ only.** `clang-cl` on Windows. MSVC and GCC are not supported.
- **CMake 3.28+** with Presets + Ninja. Corrosion for Rust.
- **Windows (x64) + Linux (x64)** from day one. macOS and consoles: post-scope.

### §2.2.1 Offline tooling — Python carve-out

**Python (and similar interpreted helpers) are allowed only when all are true:**

1. They live under `tools/`, `.github/` workflows, or **short-lived** maintenance scripts at the repository root.  
2. They are **build-time, test-time, or doc-time only** — never linked into `greywater_core`, the editor, runtime, gameplay dylib, or `bld/` crates.  
3. They **do not** implement *Sacrilege* mechanics, engine subsystems, or BLD logic.

Examples: doc lint gates, link checkers, namespace or markdown migration scripts. **Expanding Python beyond this boundary requires an ADR.** BLD remains **Rust-only** in `bld/`.

### §2.2.2 Runtime AI — the deterministic carve-out (ADR-0095 / ADR-0096)

**Amendment of record:** the prior constitutional prohibition on runtime ML is replaced by this carve-out. On-device machine-learning inference is permitted **only** when **all** six rules hold. Violating any one rule means the feature ships at cook time (`tools/cook/ai/`) instead.

1. **C++ inference only.** No Python, no scripting VM, no managed runtime, no network call out of process. Primary library: **ggml / llama.cpp C++ headers** (CPU backend for determinism-critical paths; Vulkan backend opt-in behind `GW_AI_VULKAN=OFF` default, allowed only for read-only evaluation that never participates in authoritative simulation state).
2. **Weights are pinned, versioned, cooked assets** under `assets/ai/`. Every model carries a semantic version, a BLAKE3 hash, and an Ed25519 signature (ADR-0115 content signing). The loader rejects mismatches at runtime.
3. **Deterministic topology, fixed precision, seed-fed.** No dynamic control flow in the model graph. Weights are loaded verbatim; no runtime fine-tune unless the fine-tune state is part of the replicated / save-state surface and passes the determinism validator. **Rollback-safe** for any model whose output feeds authoritative state (AI Director); **presentation-only** is permitted for music mix crossfade (symbolic music transformer).
4. **No internet. No telemetry. No runtime fine-tuning on remote data.** Model training lives entirely offline under `tools/cook/ai/` (Python carve-out §2.2.1).
5. **Per-system perf budgets** (authoritative table in `docs/09_NETCODE_DETERMINISM_PERF.md`). Aggregate runtime-AI budget on RX 580 is **≤ 0.74 ms/frame**: Director policy ≤ 0.1 ms, symbolic-music transformer ≤ 0.2 ms, neural-material shader ≤ 0.4 ms, overhead ≤ 0.04 ms. The 144 FPS Tier A floor is non-negotiable. CI perf gates fail the build on regression.
6. **Any ML feature that cannot meet 1–5 ships at cook time** inside `tools/cook/ai/` (VAE weapons, WFC+RL layout scorer, neural material baker, symbolic music training). Cook-time outputs become baked deterministic assets; the runtime only uses tiny inference over them.

The three initial runtime-AI consumers are:

| System | Library | Rollback-safe? | Budget |
|--------|---------|----------------|--------|
| **AI Director** (hybrid L4D-state + bounded RL params, ADR-0097) | ggml CPU | **Yes** — seed-fed, deterministic | ≤ 0.1 ms |
| **Adaptive music** (offline stems + symbolic transformer, ADR-0098) | ggml CPU | Presentation-only | ≤ 0.2 ms |
| **Neural material evaluator** (cooked weights, runtime eval, ADR-0095) | ggml Vulkan (opt-in) | Presentation-only | ≤ 0.4 ms |

All other implied runtime ML features (full neural audio synthesis, on-device LLM dialog, internet-sourced content) are **rejected** on the RX 580 hardware floor and the no-internet rule.

### §2.3 UI Stack — Locked

| Context | Library |
|---------|---------|
| Editor UI | Dear ImGui + docking + viewports + ImNodes + ImGuizmo |
| In-game UI (HUD) | RmlUi + HarfBuzz + FreeType |

No custom UI framework. These choices are permanent.

### §2.4 ECS & Core

- **In-house archetype ECS** in `engine/ecs/`. No EnTT, no flecs.
- **Custom fiber-based work-stealing scheduler** in `engine/jobs/`.
- **No global `new`/`delete`** in engine code. Allocators passed explicitly.
- **OS headers** only inside `engine/platform/`.

### §2.5 Gameplay Logic — C++23 Only

All *Sacrilege* gameplay — Martyrdom Engine, player movement, enemy behavior, weapons, level events, boss phases — is implemented as **compiled C++23 code** in `gameplay/`. The editor wires data; the code implements behavior. There is no runtime-interpreted gameplay language.

Enemy behavior trees are **data-driven** (tree structure loaded from `.gwbt` files) but the **node implementations** (attack, patrol, flee, etc.) are compiled C++ registered functions. Designers wire trees in the editor; programmers write the node code in C++.

### §2.6 *Sacrilege* Mandates

- **Procedural generation is deterministic.** Same Hell Seed + same Circle index = identical level, every run, every machine.
- **Arena-scale only.** The Nine Circles are bounded 3D arenas, not planets. No orbital mechanics. No atmospheric layers. No ground-to-orbit. No galaxy hierarchy. The "universe" is Hell — nine arenas connected by a descent narrative. **The Silentium (mirror dimension)** is a special procedural profile layered on Circle IX (Treachery), not an open world.
- **World-space math uses `f64` for arena-scale positions** where floating-point error would be visible.
- **Floating origin** fires when the camera travels far within a large arena.
- **"If it is not near the player, it does not exist."** On-demand streaming of arena chunks. Nothing simulated outside the player's relevance window.
- **All custom assets.** No licensed art packs. All 3D meshes, textures, and audio are authored by Cold Coffee Labs.

---

## §3. Code Generation Style Guide

### C++23

```
Type names:         PascalCase       SinComponent, GptmTile, HellKnight
Functions/methods:  snake_case       apply_sin_accrual, spawn_enemy, build_lod_mesh
Member variables:   trailing_        device_, sin_value_, time_remaining_
Namespaces:         snake_case       gw::render, gw::ecs, gw::sacrilege
Macros:             GW_UPPER_SNAKE   GW_ASSERT, GW_LOG, GW_UNREACHABLE
Enums:              enum class       BlasphemyType::Stigmata, CircleId::Gluttony
Files:              .hpp / .cpp      #pragma once in all headers
```

Binding rules (all enforced at review):

- No exceptions. Use `Result<T, E>` or `std::expected`. Historical allow-list and burn-down plan in ADR-0086.
- RAII lifetimes: either usable immediately after construction, or default-constructed-then-`initialize()` with idempotent `shutdown()` called by the destructor. Never `void init()` that fails silently. ADR-0087.
- No raw `new` / `delete`. Use `std::unique_ptr`, `gw::Ref<T>`, or stack scope.
- No C-style casts. `static_cast` / `reinterpret_cast` with `// SAFETY:` comments.
- No `[=]` or `[&]` lambda captures. Explicit capture only.
- No `std::async`, no raw `std::thread`, no `std::jthread`, no `std::packaged_task` / `std::promise`. OS-thread ownership lives only in `engine/jobs/` (`Scheduler`, `ReservedWorker`) and `engine/platform/`. `std::mutex` / `std::shared_mutex` / `std::atomic` / `std::condition_variable` / `std::latch` are correctness primitives and are allowed anywhere they protect shared state. See ADR-0088.
- All ECS components are POD structs. No virtual functions in components.
- All gameplay systems are pure functions over component spans.
- World-space positions are `f64` where scale demands it.

### Rust (BLD only)

- Edition 2024. `rustfmt` defaults. `clippy -D warnings`.
- `unsafe` requires `// SAFETY:` comment.
- No `unwrap`/`expect` in `bld-ffi` or `bld-governance`.
- `#[bld_tool]` proc macro for every MCP tool.

---

## §4. Memory and Performance Policy

| Allocator | Use |
|-----------|-----|
| `gw::memory::FrameAllocator` | Transient per-frame scratch. Reset once per frame. |
| Arena allocators | Subsystem-scoped, level-scoped, load-phase-scoped. |
| Pool allocators | Fixed-size hot ECS components, particle systems. |
| System heap | Startup only. Never in game loop. |

- **Perf regression ≥ 5% on canonical scenes fails CI.**
- **Target: 1080p @ 144 FPS** on RX 580 for *Sacrilege* full arena: 512 active enemies + Gore system + blood decals + Martyrdom post stack.

---

## §5. The Editor as a Game Tool

The Greywater Editor is purpose-built for *Sacrilege* authoring. It is the equivalent of a 1990s level editor — but native, real-time, and tightly integrated with the game engine.

**Editor capabilities for Sacrilege content:**

- **Circle Editor:** Set SDR noise parameters per Circle. Preview procedural terrain in real time. Tweak biome blending. All changes update the viewport immediately.
- **Encounter Editor:** Place enemy spawn points, patrol nodes, trigger volumes. Wire enemies to event scripts (C++ registered callbacks). Set encounter difficulty parameters.
- **Asset Browser:** Browse cooked `.gwmesh`, `.gwtex`, `.gwaudio` assets. Drag to place in 3D scene. Preview meshes with material applied.
- **Behavior Tree Wiring:** Visual graph for connecting behavior tree nodes (patrol → alert → attack → retreat). Node *implementations* are C++; the *graph* is data.
- **BLD Panel:** Ask the AI to "add 3 Cherubim flanking spawns on the east wall" or "generate variation of this corridor." BLD submits changes via the command stack — fully undoable.
- **Play Mode:** F5 launches the game from current cursor position. Esc returns to editor with session state preserved.

**The editor does NOT have:**
- A scripting IDE
- A code editor
- A node graph for gameplay logic
- Any Ripple Script or RSL surface

---

## §6. Repository Layout

```
greywater/
├── engine/
│   ├── core/           Logger, assertions, Result<T,E>, containers, strings, events, config
│   ├── math/           Vec/Mat/Quat/Transform + SIMD, f64 world-space types
│   ├── memory/         Frame, arena, pool allocators
│   ├── platform/       Window, input, timing — OS headers live ONLY here
│   ├── jobs/           Fiber-based work-stealing scheduler
│   ├── ecs/            Archetype ECS — World, queries, systems, POD components
│   ├── render/         HAL, Vulkan backend, frame graph, Forward+, materials, VFX
│   │   └── shaders/    HLSL source — terrain.hlsl, horror_post.hlsl, blood_decal.hlsl, etc.
│   ├── audio/          miniaudio + Steam Audio + Martyrdom audio controller
│   ├── input/          SDL3 device layer + action maps
│   ├── ui/             RmlUi + HarfBuzz + FreeType (in-game HUD only)
│   ├── net/            GameNetworkingSockets + ECS replication + rollback
│   ├── physics/        Jolt facade + ECS bridge + swept-AABB player controller
│   ├── anim/           Ozz + ACL + blend trees + IK
│   ├── ai/             BehaviorTree nodes + Navmesh (Recast/Detour)
│   ├── assets/         Asset database, gw_cook pipeline
│   ├── scene/          .gwscene codec + .gwseq sequencer
│   ├── world/          Blacklake arena generation stack (Phases 19–23)
│   │   ├── universe/   HEC seeding, PCG64, seed manager, sdr_noise.*
│   │   ├── streaming/  ChunkStreamer, ChunkCoord, LRU cache
│   │   ├── biome/      BiomeClassifier, Circle-specific profiles
│   │   ├── gptm/       GptmMeshBuilder, LOD selector, tile renderer (moved from planet/)
│   │   ├── resources/  ResourceDistributor, resource node placement
│   │   └── origin/     FloatingOrigin, OriginShiftEvent
│   ├── ai_runtime/     On-device C++ inference (Phase 26, §2.2.2 carve-out)
│   │   ├── inference_runtime.hpp/.cpp   ggml CPU backend (opt-in Vulkan)
│   │   ├── model_registry.hpp           versioned pinned weight registry
│   │   ├── director_policy.hpp          hybrid state machine + bounded RL
│   │   ├── music_symbolic.hpp           tiny symbolic transformer (≤1M params)
│   │   ├── material_eval.hpp            neural material evaluator
│   │   └── ai_cvars.hpp                 budgets, toggles
│   ├── narrative/      Story-Bible runtime (Phase 21/22)
│   │   ├── dialogue_graph.hpp           .gwdlg reader
│   │   ├── act_state.hpp                Acts I-III state machine
│   │   ├── sin_signature.hpp            player-behaviour fingerprint
│   │   ├── voice_director.hpp           Malakor vs Niccolò line picker
│   │   └── grace_meter.hpp              Act III terminal Blasphemy meter
│   ├── services/       Seven IP-agnostic franchise services (Phase 27)
│   │   ├── material_forge/              neural PBR + node graph
│   │   ├── level_architect/             WFC + cook-time RL scorer
│   │   ├── combat_simulator/            enemy DNA + encounter scorer
│   │   ├── gore/                        dismemberment + decal propagation
│   │   ├── audio_weave/                 layered stems + symbolic music
│   │   ├── director/                    hybrid AI Director
│   │   └── editor_copilot/              BLD tool surface for authoring
│   ├── a11y/           Runtime accessibility (Phase 16 + gameplay a11y upgrade)
│   └── i18n/           Runtime localization (Phase 16)
├── bld/                Rust Cargo workspace — BLD AI copilot (10 crates)
│   ├── bld-ffi/        C-ABI boundary
│   ├── bld-mcp/        MCP wire protocol
│   ├── bld-tools/      #[bld_tool] proc macro + tool registry
│   ├── bld-tools-macros/ Macro implementation crate
│   ├── bld-provider/   LLM provider abstraction
│   ├── bld-rag/        Retrieval-augmented generation
│   ├── bld-agent/      Main agent loop
│   ├── bld-governance/ HITL approval
│   ├── bld-bridge/     Editor bridge
│   └── bld-replay/     Deterministic replay (BLD audit + gameplay_replay)
├── editor/             Dear ImGui editor — viewport, circle editor, encounter editor
├── sandbox/            Author-facing shader sandbox (HLSL authoring harness; not an exe)
├── apps/               Phase exit-gate executables — one per engineering milestone:
│   ├── sandbox_playable/          Phase 11 — runtime + input + audio + UI playable loop
│   ├── sandbox_physics/           Phase 12 — Jolt bridge + character controller
│   ├── sandbox_netplay/           Phase 14 — ECS replication + rollback
│   ├── sandbox_persistence/       Phase 15 — SQLite save + DSAR
│   ├── sandbox_platform_services/ Phase 16 — Steam / EOS / i18n / a11y
│   ├── sandbox_studio_renderer/   Phase 17 — materials + VFX + post pipeline
│   ├── sandbox_living_scene/      Phase 18 — mods + gameplay_module host
│   ├── sandbox_infinite_seed/     Phase 19 — Hell Seed + chunk lattice + HEC determinism
│   ├── sandbox_director/          Phase 26 — AI Director standalone (headless-CI + interactive)
│   ├── sandbox_deck_probe/        Steam Deck verified-checklist smoke probe
│   └── sandbox_studio/            Cross-phase studio exit harness
├── runtime/            Shipping executable
├── gameplay/           Hot-reloadable C++23 dylib — ALL Sacrilege game logic lives here
│   ├── martyrdom/      Sin, Mantra, Blasphemies, Rapture/Ruin + god_mode, grace_meter, blasphemy_state
│   ├── player/         Player controller, movement (bunny hop, slide, grapple, surf)
│   ├── weapons/        All 6 weapons, alt-fires, hit detection
│   ├── enemies/        Enemy ECS archetypes, behavior tree registrations
│   ├── circles/        Circle-specific event scripts, encounter sequences
│   ├── hud/            Diegetic HUD — blood vial, Sin ring, Mantra counter, Grace ring (Act III)
│   ├── characters/
│   │   └── malakor_niccolo/  Dual-consciousness voice director, mirror-step ability
│   ├── acts/           Act I/II/III encounter gates (Rite, Blasphemy Circuit, Unmaking)
│   ├── boss/
│   │   ├── god_machine/      4-phase fight (existing)
│   │   └── logos/            Alternate Phase 4 — Silent God defeated by Grace
│   └── mod_example/    Reference mod using the Mod SDK (manifest + filesystem jail)
├── tools/
│   ├── cook/           gw_cook content pipeline (Ed25519 content signing)
│   ├── cook/ai/        Offline ML training pipelines (Python §2.2.1)
│   │   ├── vae_weapons/          VAE weapon permutation baker
│   │   ├── wfc_rl_scorer/        WFC layout RL scorer
│   │   ├── music_symbolic_train/ Symbolic transformer pre-training (replaces LSTM)
│   │   ├── neural_material/      Concept/prompt → .gwmat baker
│   │   └── director_train/       AI Director policy checkpoint producer
│   └── lint/           CI lints (check_cpm_sha_pins.py, check_single_shader_root.py, …)
├── tests/              doctest unit + integration + perf gates
├── content/            Source assets (gitignored)
├── assets/             Cooked runtime assets — .gwmesh, .gwtex, .gwaudio, .gwbt, .gwdlg, .gwenc
│   ├── ai/             Pinned, Ed25519-signed ML weights (Director, symbolic music, neural material)
│   └── ui/             Cooked in-game UI — shipping RML/RCSS/strings (see docs/04 §UI roots)
├── ui/                 Privacy-surface UI that ships in *every* build, including
│                       demos / free trials (GDPR consent, DSAR dialogs, age gate).
│                       Quarantined from assets/ui/ because of stricter review —
│                       ADR-0062, docs/04 §UI roots.
├── third_party/        CPM-managed, SHA-pinned (incl. vendored tiny_sha3/)
├── cmake/              dependencies.cmake, toolchains/
├── docs/               This directory
├── CMakeLists.txt
├── CMakePresets.json
├── CLAUDE.md
└── rust-toolchain.toml
```

---

## §7. Engine / Game Code Boundary

- `greywater_core` has **no dependency on gameplay code**. It builds and tests in isolation.
- `gameplay/` is the **only** dynamic library. All *Sacrilege* game logic lives here.
- Engine code never `#include`s gameplay headers. CMake target visibility enforces this.
- `engine/world/` implements arena generation **capabilities** (terrain, streaming, SDR). `gameplay/` implements *Sacrilege*-specific **behavior** (what spawns, when, at what difficulty).

---

## §8. Build Status (as of 2026-04-22)

| Phase | Name | Status | Tests |
|-------|------|--------|-------|
| 1–17 | Engine foundation through shader pipeline | ✅ Complete | 711/711 |
| 18 | Cinematics & Mods | ✅ Complete | sandbox_studio OK |
| 19 | Blacklake Core & Deterministic Genesis | ✅ Complete | 26 universe_tests |
| 20 | Arena Topology, GPTM & Sacrilege LOD | 🔜 Active | — |
| 21 | Sacrilege Frame — Rendering, Atmosphere, Audio | 🔜 Planned | — |
| 22 | Martyrdom Combat & Player Stack | 🔜 Planned | — |
| 23 | Damned Host, Circles Pipeline & Boss | 🔜 Planned | — |
| 24 | Hardening & Release | 🔜 Planned | — |
| 25 | LTS Sustenance | 🔜 Planned | ongoing |

---

## §9. Session Handoff Protocol

Every session begins with this checklist. No exceptions.

### Cold-Start (10 minutes)

1. `git log --oneline -10` + `git status` — know what landed last.
2. Re-read `CLAUDE.md` at project root.
3. Check active phase in `02_ROADMAP_AND_OPERATIONS.md`. Know your card.
4. Read the relevant doc for the current phase (see §9.1).

### Doc by Phase

| Phase | Read |
|-------|------|
| 20–23 (Blacklake + Sacrilege game systems) | `07_SACRILEGE.md` + `08_BLACKLAKE.md` |
| Rendering work | `05_RESEARCH_BUILD_AND_STANDARDS.md` |
| C++23 patterns | `05_RESEARCH_BUILD_AND_STANDARDS.md` |
| Architecture questions | `06_ARCHITECTURE.md` |
| Gameplay systems | `07_SACRILEGE.md §III` |

### CMake Presets

```bash
cmake --preset dev-linux && cmake --build --preset dev-linux && ctest --preset dev-linux
cmake --preset dev-win   && cmake --build --preset dev-win   && ctest --preset dev-win
```

| Preset | Config | OS | Purpose |
|--------|--------|----|---------|
| `dev-linux` | Debug | Linux | Default workflow |
| `dev-win` | Debug | Windows | Default workflow |
| `release-linux` / `release-win` | Release | both | Release builds |
| `linux-asan` / `linux-ubsan` / `linux-tsan` | Debug+san | Linux | Nightly sanitizers |
| `studio-win` / `studio-linux` | Phase 17 | both | Full shader/post perf gates |

### End-of-Session (5 minutes)

1. Update daily log: done, blocked, tomorrow.
2. Move Kanban card appropriately (WIP ≤ 2).
3. Commit: small commits, Conventional Commits format, CI must stay green.

---

## §10. If You Get Stuck

1. Re-read `03_PHILOSOPHY_AND_ENGINEERING.md` — the Brew Doctrine dissolves most ambiguity.
2. Re-read the review playbook in `03_PHILOSOPHY_AND_ENGINEERING.md §B` — your blocker is likely listed.
3. Write an ADR. Architectural ambiguity → draft the decision explicitly, commit ADR before code.
4. Open BLD. Ask it. That's what it's for.
5. Friday evening — stop. Rest days are load-bearing.

---

*The constitution is the engine's immune system. Honor it.*
