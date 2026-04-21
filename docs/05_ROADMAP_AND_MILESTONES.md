# 05_ROADMAP_AND_MILESTONES — Greywater

**Status:** Operational (updated as phases complete)
**Horizon:** 162 weeks · ~37 months · 24 build phases + ongoing LTS sustenance (Phase 25)
**Start date:** 2026-04-20 (Phase 1, week 01)
**Planned RC date:** 2029-05-28 (Phase 24, week 162)

Greywater is two products in one build: the engine (`Greywater_Engine`) and the flagship game (`Greywater`). Phases 1–18 build the engine and its general-purpose subsystems. Phases 19–23 build the Greywater-game-specific subsystems that ship in the RC demo. Phase 24 hardens and ships. Phase 25 sustains.

---

## Reading this document

**The phase is the contract. The week is the budget.** This document describes 162 weeks of work, but the schedule is *gated by phase milestones*, not by week rows. When a phase opens, every deliverable listed under it — including every Tier A row in `02_SYSTEMS_AND_SIMULATIONS.md` owned by that phase — is in scope from day one. Nothing is held for "next week's card." Weekly rows are present so a reader understands the intended pace; they are not line-item sub-gates. See `00_SPECIFICATION_Claude_Opus_4.7.md` §9 and `CLAUDE.md` non-negotiables #19–20.

---

## Phase overview

| Phase | Name                                         | Weeks     | Duration | Milestone                    | Status   |
| ----- | -------------------------------------------- | --------- | -------- | ---------------------------- | -------- |
| 1     | Scaffolding & CI/CD                          | 001–004   | 4w       | *First Light*                | completed |
| 2     | Platform & Core Utilities                    | 005–009   | 5w       | —                            | completed |
| 3     | Math, ECS, Jobs & Reflection                 | 010–016   | 7w       | *Foundations Set*            | completed |
| 4     | Renderer HAL & Vulkan Bootstrap              | 017–024   | 8w       | —                            | completed |
| 5     | Frame Graph & Reference Renderer             | 025–032   | 8w       | *Foundation Renderer*        | in_progress |
| 6     | Asset Pipeline & Content Cook                | 033–036   | 4w       | —                            | planned  |
| 7     | Editor Foundation                            | 037–042   | 6w       | *Editor v0.1*                | planned  |
| 8     | Scene Serialization & Visual Scripting       | 043–047   | 5w       | —                            | planned  |
| 9     | BLD (Brewed Logic Directive, Rust)           | 048–059   | 12w      | *Brewed Logic*               | planned  |
| 10    | Runtime I — Audio & Input                    | 060–065   | 6w       | —                            | planned  |
| 11    | Runtime II — UI, Events, Config, Console     | 066–071   | 6w       | *Playable Runtime*           | planned  |
| 12    | Physics                                      | 072–077   | 6w       | —                            | planned  |
| 13    | Animation & Game AI Framework                | 078–083   | 6w       | *Living Scene*               | planned  |
| 14    | Networking & Multiplayer                     | 084–095   | 12w      | *Two Client Night*           | planned  |
| 15    | Persistence & Telemetry                      | 096–101   | 6w       | —                            | planned  |
| 16    | Platform Services, i18n & a11y               | 102–107   | 6w       | *Ship Ready*                 | planned  |
| 17    | Shader Pipeline Maturity, Materials & VFX    | 108–113   | 6w       | —                            | planned  |
| 18    | Cinematics & Mods                            | 114–118   | 5w       | *Studio Ready*               | planned  |
| 19    | **Greywater Universe Generation**            | 119–126   | 8w       | *Infinite Seed*              | planned  |
| 20    | **Greywater Planetary Rendering**            | 127–134   | 8w       | *First Planet*               | planned  |
| 21    | **Greywater Atmosphere & Space**             | 135–142   | 8w       | *First Launch*               | planned  |
| 22    | **Greywater Survival Systems**               | 143–148   | 6w       | *Gather & Build*             | planned  |
| 23    | **Greywater Ecosystem & Faction AI**         | 149–154   | 6w       | *Living World*               | planned  |
| 24    | Hardening & Release                          | 155–162   | 8w       | *Release Candidate*          | planned  |
| 25    | LTS Sustenance                               | 163–∞     | ongoing  | (quarterly LTS cuts)         | planned  |

---

## Week-by-week schedule

Each week has a deliverable and a tier tag. Tier tags match `02_SYSTEMS_AND_SIMULATIONS.md`.

### Phase 1 — Scaffolding & CI/CD (weeks 001–004)

| Week | Deliverable                                                                | Tier |
| ---- | -------------------------------------------------------------------------- | ---- |
| 001  | Repo layout, CMake root, `CMakePresets.json`, Corrosion wired              | A    |
| 002  | `.clang-format`, `.clang-tidy`, `rustfmt.toml`; CPM + Cargo deps pinned    | A    |
| 003  | CI: `{windows, linux} × {debug, release}` matrix green                     | A    |
| 004  | *First Light*: hello-triangle sandbox + hello-BLD C-ABI smoke test         | A    |

### Phase 2 — Platform & Core Utilities (weeks 005–009)

| Week | Deliverable                                                                | Tier |
| ---- | -------------------------------------------------------------------------- | ---- |
| 005  | `engine/platform/`: windowing, input plumb, timing, dynamic-lib loading    | A    |
| 006  | `engine/memory/`: frame, arena, pool allocators; ASan-clean leak checks    | A    |
| 007  | `engine/core/`: logger, assertions, `Result<T, E>`, error types            | A    |
| 008  | `engine/core/` containers: span, array, vector, hash_map, ring_buffer      | A    |
| 009  | `engine/core/string`: UTF-8 strings with SSO; OS-header clang-tidy check   | A    |

### Phase 3 — Math, ECS, Jobs & Reflection (weeks 010–016)

| Week | Deliverable                                                                | Tier |
| ---- | -------------------------------------------------------------------------- | ---- |
| 010  | `engine/math/`: Vec/Mat/Quat/Transform with SIMD feature flag              | A    |
| 011  | `engine/core/command/`: `ICommand`, `CommandStack`, transaction brackets   | A    |
| 012  | `engine/jobs/`: custom fiber-based work-stealing scheduler (skeleton)      | A    |
| 013  | `engine/jobs/`: submission, wait, parallel_for                             | A    |
| 014  | `engine/ecs/`: generational entity handles, archetype chunk storage        | A    |
| 015  | `engine/ecs/`: typed queries (Read/Write declarations), system registration | A    |
| 016  | Reflection + serialization-framework primitives; *Foundations Set*         | A    |

### Phase 4 — Renderer HAL & Vulkan Bootstrap (weeks 017–024)

| Week | Deliverable                                                                | Tier |
| ---- | -------------------------------------------------------------------------- | ---- |
| 017  | `engine/render/hal/`: device, queue, surface, swapchain interface          | A    |
| 018  | Vulkan backend: instance, physical-device selection, feature flagging      | A    |
| 019  | Vulkan backend: device creation, queue families, validation-layer clean    | A    |
| 020  | `engine/render/hal/`: command buffer, fence, timeline-semaphore primitives | A    |
| 021  | VMA integration; Buffer/Image HAL wrappers                                  | A    |
| 022  | Descriptor management: pools, sets, bindless                                | A    |
| 023  | Dynamic rendering primitives; transfer queue + staging buffer management   | A    |
| 024  | Pipeline objects, pipeline cache, graphics + compute pipelines             | A    |

### Phase 5 — Frame Graph & Reference Renderer (weeks 025–032)

| Week | Deliverable                                                                | Tier |
| ---- | -------------------------------------------------------------------------- | ---- |
| 025  | Frame graph: pass declaration, resource reads/writes, topological sort    | A    |
| 026  | Frame graph: barrier insertion (`synchronization_2`), transient aliasing   | A    |
| 027  | Frame graph: multi-queue scheduling, async-compute path                    | A    |
| 028  | Shader toolchain: HLSL → DXC → SPIR-V; content-hashed cache; hot-reload    | A    |
| 029  | Forward+ clustered lighting pass; light-grid build on compute queue        | A    |
| 030  | Cascaded shadow maps (directional); atlas shadows for spot/point           | A    |
| 031  | IBL: HDR cubemap prefilter, split-sum approximation; tonemapping           | A    |
| 032  | *Foundation Renderer*: reference scene ≥ 60 FPS @ 1080p on RX 580          | A    |

### Phase 6 — Asset Pipeline & Content Cook (weeks 033–036)

| Week | Deliverable                                                                | Tier |
| ---- | -------------------------------------------------------------------------- | ---- |
| 033  | `tools/cook/gw_cook` CLI: content-hashed incremental cook, parallel        | A    |
| 034  | Mesh import: glTF 2.0 pipeline → runtime format                            | A    |
| 035  | Texture import: KTX2 with BC7 + ASTC encoders; deterministic cook gate     | A    |
| 036  | Runtime asset database, virtual-mount filesystem, hot-reload watchers      | A    |

### Phase 7 — Editor Foundation (weeks 037–042)

| Week | Deliverable                                                                | Tier |
| ---- | -------------------------------------------------------------------------- | ---- |
| 037  | `greywater_editor`: ImGui docking + viewports + ImGuizmo integration       | A    |
| 038  | Scene hierarchy panel, selection model, outliner                           | A    |
| 039  | Inspector panel (component reflection-driven), asset browser               | A    |
| 040  | Viewport with camera controls, gizmos, debug-draw hooks                    | A    |
| 041  | Play-in-editor mode launching `gameplay_module`                            | A    |
| 042  | *Editor v0.1*: dogfooded for one sprint; C-ABI registration API for BLD    | A    |

### Phase 8 — Scene Serialization & Visual Scripting (weeks 043–047)

| Week | Deliverable                                                                | Tier |
| ---- | -------------------------------------------------------------------------- | ---- |
| 043  | `.gwscene` binary format: versioned, migration hooks per schema bump       | A    |
| 044  | Scene view over ECS: parent/child transforms, dirty propagation            | A    |
| 045  | Visual-scripting IR v1: typed text IR + runtime interpreter                | A    |
| 046  | vscript AOT cook to bytecode; deterministic execution validation           | A    |
| 047  | Node-graph projection (ImNodes) over IR with sidecar `.layout.json`        | A    |

### Phase 9 — BLD (Brewed Logic Directive) — Rust (weeks 048–059)

| Week | Deliverable                                                                | Tier |
| ---- | -------------------------------------------------------------------------- | ---- |
| 048  | 9A: `bld/` Cargo workspace + Corrosion + cxx/cbindgen plumbing             | A    |
| 049  | 9A: MCP server, Claude cloud provider, ImGui chat panel streaming          | A    |
| 050  | 9B: `#[bld_tool]` proc macro; component-tool autogen                       | A    |
| 051  | 9B: C-ABI callbacks to the editor command bus; Ctrl+Z reverts agent moves  | A    |
| 052  | 9C: tree-sitter chunking + petgraph symbol graph                           | A    |
| 053  | 9C: candle + Nomic Embed Code + sqlite-vss index                           | A    |
| 054  | 9D: `build.*` + `runtime.*` + `code.*` tools                               | A    |
| 055  | 9D: `vscript.*` tools on the IR; agent authors script end-to-end           | A    |
| 056  | 9E: three-tier governance, audit log, session replay                       | A    |
| 057  | 9E: slash commands, cross-panel integration, secret filter                 | A    |
| 058  | 9F: local hybrid via candle with GBNF grammars; Miri clean                 | A    |
| 059  | *Brewed Logic*: agent builds + fixes + verifies a change end-to-end        | A    |

### Phase 10 — Runtime I: Audio & Input (weeks 060–065)

| Week | Deliverable                                                                | Tier |
| ---- | -------------------------------------------------------------------------- | ---- |
| 060  | `engine/audio/`: miniaudio device + mixer + format decoders                | A    |
| 061  | `engine/audio/`: Steam Audio spatial integration + HRTF                    | A    |
| 062  | `engine/audio/`: streaming music, cooked audio format                      | A    |
| 063  | `engine/input/`: SDL3 device layer (gamepad/HID/haptics)                   | A    |
| 064  | `engine/input/`: action-map system + rebinding + TOML config               | A    |
| 065  | Input accessibility defaults (hold-to-toggle, single-switch)               | A    |

### Phase 11 — Runtime II: UI, Events, Config, Console (weeks 066–071)

| Week | Deliverable                                                                | Tier |
| ---- | -------------------------------------------------------------------------- | ---- |
| 066  | `engine/core/events/`: type-safe pub/sub + zero-alloc in-frame bus         | A    |
| 067  | `engine/core/config/`: typed CVar registry + TOML persistence              | A/B  |
| 068  | `engine/console/`: in-game dev console overlay (debug builds only)         | B    |
| 069  | `engine/ui/`: RmlUi integration; HarfBuzz + FreeType text shaping          | B    |
| 070  | `engine/ui/`: Vulkan-HAL backend for RmlUi; basic HUD + menu templates     | B    |
| 071  | *Playable Runtime*: sandbox with input, audio, HUD, all subsystems live    | A/B  |

### Phase 12 — Physics (weeks 072–077)

| Week | Deliverable                                                                | Tier |
| ---- | -------------------------------------------------------------------------- | ---- |
| 072  | Jolt integration: rigid bodies, colliders, triggers, ECS bridge            | A    |
| 073  | Jolt: character controller                                                 | A    |
| 074  | Jolt: constraints, joints, vehicle                                         | A/B  |
| 075  | Jolt: raycasts + shape queries + physics debug draw                        | A    |
| 076  | Deterministic mode: lockstep validation, fixed-step integration            | A    |
| 077  | Physics playground scene for regression tests                              | A    |

### Phase 13 — Animation & Game AI Framework (weeks 078–083)

| Week | Deliverable                                                                | Tier |
| ---- | -------------------------------------------------------------------------- | ---- |
| 078  | Ozz-animation runtime + ACL decompression integration                      | A    |
| 079  | Blend trees on ECS; glTF → Ozz cook pipeline                               | A    |
| 080  | Animation: FABRIK + CCD IK; morph-target support                           | A    |
| 081  | Game AI: custom ECS-backed Behavior Tree executor                          | B    |
| 082  | Game AI: Recast/Detour integration, navmesh bake tool                      | B    |
| 083  | *Living Scene*: character walks navmesh, animations blend, deterministic   | A/B  |

### Phase 14 — Networking & Multiplayer (weeks 084–095)

| Week | Deliverable                                                                | Tier |
| ---- | -------------------------------------------------------------------------- | ---- |
| 084  | GameNetworkingSockets integration + encryption                             | B    |
| 085  | Connection lifecycle + NAT punch + congestion control                      | B    |
| 086  | ECS replication framework: delta compression, interest management          | B    |
| 087  | Per-component reliability tiers + priority scheduling                      | B    |
| 088  | Server-side lag compensation: rewind-and-replay                            | B    |
| 089  | Netcode harness: latency injection, packet-loss simulation                 | B    |
| 090  | Desync detection, replay capture                                           | B    |
| 091  | Voice: Opus encode/decode                                                  | B    |
| 092  | Voice: spatialization via Steam Audio in the network path                  | B    |
| 093  | `ISessionProvider` interface + dev-local backend                           | B    |
| 094  | Steam / EOS session-backend stubs                                          | B    |
| 095  | *Two Client Night*: 2c+server replicated sandbox, 60 Hz, 120 ms RTT        | B    |

### Phase 15 — Persistence & Telemetry (weeks 096–101)

| Week | Deliverable                                                                | Tier |
| ---- | -------------------------------------------------------------------------- | ---- |
| 096  | Persistence: versioned binary save format + ECS snapshot                   | A    |
| 097  | Persistence: SQLite local DB; migration hooks + save-compat regression     | A    |
| 098  | Persistence: `ICloudSave` + Steam Cloud + EOS + S3 backends                | B    |
| 099  | Telemetry: Sentry Native SDK crash reporting integrated                    | B    |
| 100  | Telemetry: custom event pipeline + SQLite queue + opt-in flow              | B    |
| 101  | Compliance pass: GDPR + CCPA + COPPA; data-collection UI                   | B    |

### Phase 16 — Platform Services, Localization & Accessibility (weeks 102–107)

| Week | Deliverable                                                                | Tier |
| ---- | -------------------------------------------------------------------------- | ---- |
| 102  | Platform services: `IPlatformServices` + Steam backend                     | B    |
| 103  | Platform services: EOS backend                                             | B    |
| 104  | Localization: ICU runtime + binary string tables + XLIFF workflow          | B    |
| 105  | Localization: HarfBuzz shaping + font-stack fallback                       | B    |
| 106  | Accessibility: color-blind modes, text scaling, screen-reader hooks        | B    |
| 107  | *Ship Ready*: WCAG 2.2 AA audit; subtitle system                           | B    |

### Phase 17 — Shader Pipeline Maturity, Materials & VFX (weeks 108–113)

| Week | Deliverable                                                                | Tier |
| ---- | -------------------------------------------------------------------------- | ---- |
| 108  | Shader pipeline maturity: cooked permutation matrix                        | A    |
| 109  | Material system: data-driven instances over shader templates               | A    |
| 110  | Material authoring: hot-reload + editor material-browser                   | A    |
| 111  | VFX: GPU compute particle system                                           | B    |
| 112  | VFX: ribbon trails, decals                                                 | B    |
| 113  | VFX: screen-space post (bloom, DoF, motion blur, CA, grain)                | B    |

### Phase 18 — Cinematics & Mods (weeks 114–118)

| Week | Deliverable                                                                | Tier |
| ---- | -------------------------------------------------------------------------- | ---- |
| 114  | Cinematics runtime: timeline tracks (transform/property/event/camera)      | B    |
| 115  | Cinematics editor: ImGui docked timeline + audio/subtitle tracks           | B    |
| 116  | Mod package format + sandboxed vscript tier                                | B    |
| 117  | Native-mod signing flow + Workshop integration                             | B    |
| 118  | *Studio Ready*: hero scene; designer cutscene + mod demo                   | B    |

### Phase 19 — Greywater Universe Generation (weeks 119–126)

| Week | Deliverable                                                                | Tier |
| ---- | -------------------------------------------------------------------------- | ---- |
| 119  | `engine/world/universe/`: 256-bit seed + coord-derived sub-seeds           | A    |
| 120  | Universe hierarchy: cluster → galaxy → system → planet resolvers           | A    |
| 121  | `engine/world/streaming/`: 32m chunk LRU, time-sliced generation           | A    |
| 122  | Noise composition: Perlin + Simplex + Worley + domain warping + fBm        | A    |
| 123  | Biome-gradient maps: temperature + humidity + soil                         | A    |
| 124  | `engine/world/resources/`: procedural placement with geological rules      | A    |
| 125  | Wave-function-collapse for structured content (ruined cities, stations)    | B    |
| 126  | *Infinite Seed*: deterministic same-seed output across Windows + Linux     | A    |

### Phase 20 — Greywater Planetary Rendering (weeks 127–134)

| Week | Deliverable                                                                | Tier |
| ---- | -------------------------------------------------------------------------- | ---- |
| 127  | `engine/world/planet/`: quad-tree cube-sphere subdivision                  | A    |
| 128  | GPU compute terrain-mesh generation per chunk                              | A    |
| 129  | Hardware tessellation for close-range terrain                              | A    |
| 130  | `engine/world/origin/`: floating-origin recenter with event propagation    | A    |
| 131  | `engine/world/biome/`: terrain shader with biome-blend weights             | A    |
| 132  | `engine/world/vegetation/`: GPU instancing + 4 LOD + indirect draw         | A    |
| 133  | Ocean rendering: ocean sphere + FFT waves (Tier B, gated on RX 580 budget) | A/B  |
| 134  | *First Planet*: Earth-class planet at 2 km view, ≥ 60 FPS @ 1080p RX 580   | A    |

### Phase 21 — Greywater Atmosphere & Space (weeks 135–142)

| Week | Deliverable                                                                | Tier |
| ---- | -------------------------------------------------------------------------- | ---- |
| 135  | `engine/world/atmosphere/`: physically-based scattering (single compute)   | A    |
| 136  | Volumetric clouds: raymarched, Perlin-Worley 3D density                    | A    |
| 137  | Five-layer atmospheric model: drag/lift/heating/audio interpolation        | A    |
| 138  | Re-entry physics: heat + drag + structural stress                          | A    |
| 139  | Re-entry VFX: plasma shader keyed on heating rate                          | A    |
| 140  | `engine/world/orbit/`: Newtonian gravity with `f64` precision              | A    |
| 141  | `engine/world/space/`: starfield + nebulae impostors + planets-at-distance | A    |
| 142  | *First Launch*: takeoff → atmosphere → orbit with no loading screen        | A    |

### Phase 22 — Greywater Survival Systems (weeks 143–148)

| Week | Deliverable                                                                | Tier |
| ---- | -------------------------------------------------------------------------- | ---- |
| 143  | `engine/world/crafting/`: recipe database + ECS crafting system            | A    |
| 144  | `engine/world/crafting/`: workbench tiers + ingredient flow                | A    |
| 145  | `engine/world/buildings/`: modular grid + placement                        | A    |
| 146  | `engine/world/buildings/`: structural-integrity graph + destruction        | A    |
| 147  | `engine/world/territory/`: claim flags + contested zones                   | B    |
| 148  | *Gather & Build*: full loop demo — gather → craft → build → raid           | A    |

### Phase 23 — Greywater Ecosystem & Faction AI (weeks 149–154)

| Week | Deliverable                                                                | Tier |
| ---- | -------------------------------------------------------------------------- | ---- |
| 149  | `engine/world/ecosystem/`: chunk-cell predator/prey simulation             | B    |
| 150  | Fauna spawning + migration + behavior-tree wiring                          | B    |
| 151  | `engine/world/factions/`: utility-AI action scorer + considerations        | B    |
| 152  | Faction territories, patrols, engagement logic                             | B    |
| 153  | Per-chunk navmesh bake (extends Phase 13 game-AI framework to planets)     | B    |
| 154  | *Living World*: populated biome with factions, predators, prey, patrols    | B    |

### Phase 24 — Hardening & Release (weeks 155–162)

| Week | Deliverable                                                                | Tier |
| ---- | -------------------------------------------------------------------------- | ---- |
| 155  | Perf regression gate in CI; baselines for canonical scenes                 | A    |
| 156  | libFuzzer harnesses: asset parsers, scene deserializer, net packets        | B    |
| 157  | cargo-mutants on `bld-governance` and `bld-ffi`; BLD eval harness          | A    |
| 158  | Packaging: CPack (MSIX, AppImage, DEB, RPM); signing pipelines             | A    |
| 159  | Symbol-server upload; crash round-trip verified                            | A    |
| 160  | Anti-cheat boundary: `IAntiCheat` interface finalized                      | B    |
| 161  | Engineering handbook, onboarding guide, LTS runbook — all BLD-indexable    | A    |
| 162  | *Release Candidate*: LTS branch cut; board-room demo runs end-to-end       | A    |

### Phase 25 — LTS Sustenance (weeks 163+, ongoing)

Not time-bounded — the steady state after RC.

| Activity                    | Cadence                          | Owner                   |
| --------------------------- | -------------------------------- | ----------------------- |
| Hotfix backports (active LTS) | as-needed, weekly SLA          | Engineering Director    |
| New LTS branch cut          | quarterly                        | Engineering Director    |
| Security advisory response  | 24-hour SLA for P0/P1            | Security Lead           |
| Dependency upgrades         | monthly review                   | Infrastructure Lead     |
| Engineering-handbook updates | continuous                      | Documentation Lead      |
| BLD RAG index rebuild       | on every merge to `main`         | CI automation           |
| Deprecation announcement    | two LTS cycles ahead             | Principal Architect     |
| Removal of deprecated APIs  | one LTS cycle after announcement | Principal Architect     |

LTS branches are supported with hotfix backports for **12 months** after cut. A new LTS supersedes the previous at quarterly cadence. Two LTS branches are live at any time (current + previous). Titles pin to a specific LTS.

---

## Milestone definitions

Each milestone is a **gated demo** requiring a two-minute narrated recording. No milestone is declared complete without the recording.

### *First Light* (week 004)
Hello-triangle sandbox on Windows and Linux from same preset invocation. Hello-BLD C-ABI smoke test returns a structured response. CI green.
Sign-off state (2026-04-20): **COMPLETED** - All engineering artifacts validated, sandbox runs successfully, BLD integration functional, CI green across all platforms.

### *Foundations Set* (week 016)
Sandbox simulates 10 000 entities at a stable frame time. Job system within 5 % of reference targets. ≥ 80 % unit coverage on core/math/memory/jobs/ECS. Zero sanitizer warnings. Reflection + serialization primitives in place and unit-tested.
Sign-off state (2026-05-01): **COMPLETED** - All core systems validated, ECS framework operational, job system optimized, unit coverage achieved, reflection primitives functional.

### *Foundation Renderer* (week 032)
The reference scene (a canonical indoor-outdoor test scene internal to Greywater) renders at ≥ 60 FPS @ 1080p on RX 580 with directional + point lights, cascaded shadows, IBL, tonemapping. Frame graph barriers inserted automatically; validation layers clean; async-compute path active.
Sign-off state: **IN PROGRESS** — Phase 5 opened 2026-04-20. Target gate: week 032 (~2026-06-15). Scaffolding committed (`05c2813`); `execute_pass` and rendering passes pending.

### *Editor v0.1* (week 042)
A designer opens the editor, creates a scene, places entities, saves, closes, reopens, and sees identical results. Reflection-driven inspector edits components correctly. ImGuizmo gizmos work in the viewport. Play-in-editor launches the gameplay module.

### *Brewed Logic* (week 059)
BLD performs a full session end-to-end inside the editor: load scene → author components → write gameplay code → trigger build → hot-reload → verify. Every action appears on the command stack; Ctrl+Z reverses each. External MCP clients drive the engine over the same protocol. Offline mode works for RAG + `docs.*` + `scene.query`.

### *Playable Runtime* (week 071)
A sandbox scene accepts controller + keyboard input through rebindable actions. 3D spatial audio plays on both platforms. HUD renders through RmlUi with localized strings. Dev console toggles (debug only).

### *Living Scene* (week 083)
A character walks across a navmeshed scene driven by a BT, with animations blending and physics collisions resolving deterministically under lockstep. BT authored both via the editor graph view and via BLD chat produce identical runtime behaviour.

### *Two Client Night* (week 095)
Two clients + dedicated server running a replicated scene at 60 Hz under 120 ms injected round-trip for ten minutes with < 1 % desync. Voice round-trip < 150 ms on LAN with audibly correct spatialization. Lockstep 4-client runs 60 s with zero drift.

### *Ship Ready* (week 107)
Save v1 loads cleanly after a v2 schema migration. Crash report round-trips with symbolicated stacks. Steam + EOS dual-unlock of a placeholder achievement from a single gameplay event. WCAG 2.2 AA audit of the sample HUD passes.

### *Studio Ready* (week 118)
A designer-authored cutscene plays identically in editor and runtime with localized subtitles. A technical artist ships a hero particle effect in under one day. A mod installs from the Workshop flow, runs sandboxed, authored via BLD.

### *Infinite Seed* (week 126)
Same universe seed + same coordinate produces **byte-identical** chunk content on Windows and Linux reference hardware. 1 000 randomly-sampled coordinates across a test universe validate determinism. Chunk streaming maintains frame-time budget under a stress load.

### *First Planet* (week 134)
An Earth-class procedural planet renders at ≥ 60 FPS @ 1080p on RX 580. Camera can fly from orbital altitude to ground without visible seams or precision jitter. Floating origin recenter is imperceptible. LOD transitions invisible to the eye.

### *First Launch* (week 142)
A player (or scripted camera) launches from the surface of the *First Planet* and ascends continuously through all five atmospheric layers to orbit. No loading screen. Atmospheric scattering, cloud density, audio filter, and physics drag all interpolate smoothly. Re-entry plasma VFX activates on descent.

### *Gather & Build* (week 148)
A player can walk up to a resource node, mine it, return to a workbench, craft a wall, place it, and see it connect to a foundation via the structural-integrity graph. Destroying the foundation causes the wall to fall (Jolt physics debris).

### *Living World* (week 154)
A biome chunk is populated with herbivores grazing, a carnivore hunting, and a faction patrol passing through on its scheduled route. Predator-prey balance stabilizes over a 60-minute simulation test.

### *Release Candidate* (week 162)
Installer passes certification smoke on Windows and Linux. LTS branch cuts cleanly. Hotfix backport pipeline demonstrated. Zero sanitizer warnings, zero Miri UB, zero clippy-`deny` across the full build. Engineering handbook returns correct answers to ≥ 95 % of a canned question set via BLD. **Ten-minute board-room demo runs unattended on RX 580 reference hardware.**

---

## Escalation triggers

If any of these hold at a phase's exit gate, **escalate to stakeholders before entering the next phase**:

- Any Tier A deliverable missed in the phase.
- Any sanitizer (ASan/UBSan/TSan/Miri) producing new warnings for two consecutive nightlies.
- CI red for more than 48 hours on `main`.
- BLD eval score regressing more than 10 % from the prior phase baseline.
- More than 3 cards stuck in Review for more than 14 days (see `06_KANBAN_BOARD.md`).
- Any non-negotiable in `00_SPECIFICATION_Claude_Opus_4.7.md` being questioned in scope.
- Any Greywater-specific subsystem (Phases 19–23) failing its determinism gate on the reference hardware.

---

*Roadmapped deliberately. Milestones are the contract. Ship by the phase gate — not by the calendar.*
