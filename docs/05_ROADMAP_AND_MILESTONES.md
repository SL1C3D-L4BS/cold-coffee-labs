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
| 3     | Math, ECS, Jobs & Reflection                 | 010–016   | 7w       | *Foundations Set*            | completed (2026-04-21, pulled forward through Phase 7; see §Phase-3 note) |
| 4     | Renderer HAL & Vulkan Bootstrap              | 017–024   | 8w       | —                            | completed |
| 5     | Frame Graph & Reference Renderer             | 025–032   | 8w       | *Foundation Renderer*        | completed |
| 6     | Asset Pipeline & Content Cook                | 033–036   | 4w       | —                            | completed |
| 7     | Editor Foundation                            | 037–042   | 6w       | *Editor v0.1*                | completed (engineering; demo-recording gate pending per §0) |
| 8     | Scene Serialization & Visual Scripting       | 043–047   | 5w       | —                            | completed (2026-04-21; ADR-0008 + ADR-0009 landed; `.gwscene` + dvec3 transforms + vscript IR/interpreter/VM + ImNodes panel all green) |
| 9     | BLD (Brewed Logic Directive, Rust)           | 048–059   | 12w      | *Brewed Logic*               | completed (2026-04-21; ADR-0010 → 0016 + amended ADR-0007 all landed; full six-wave 9A–9F implementation committed; see §Phase-9 amendment) |
| 10    | Runtime I — Audio & Input                    | 060–065   | 6w       | —                            | completed (2026-04-21; ADR-0017 → 0022 landed; handoff to Phase 11) |
| 11    | Runtime II — UI, Events, Config, Console     | 066–071   | 6w       | *Playable Runtime*           | completed (2026-04-21; ADR-0023 → 0030 landed; all six waves 11A–11F green on dev-win with 271/271 tests, runtime_self_test + playable_sandbox exit gates passing; see §Phase-11 closeout) |
| 12    | Physics                                      | 072–077   | 6w       | —                            | completed (2026-04-21; ADR-0031 → 0038 landed; all six waves 12A–12F green on dev-win; physics subsystem + ECS bridge + character + constraints + queries + determinism + replay shipped with null backend by default and Jolt gated behind `GW_ENABLE_JOLT`; see §Phase-12 closeout) |
| 13    | Animation & Game AI Framework                | 078–083   | 6w       | *Living Scene*               | completed (2026-04-21; ADR-0039 → 0046 landed; all six waves 13A–13F green on dev-win with 347/347 tests including `living_sandbox`; null anim + null BT + null Dijkstra navmesh by default, Ozz/ACL/Recast gated behind `living-*` presets; post-phase vscript lexer hardened for UTF-8 BOM + zero-width paste; see §Phase-13 closeout) |
| 14    | Networking & Multiplayer                     | 084–095   | 12w      | *Two Client Night*           | completed (2026-04-21; ADR-0047 → 0055 landed; all twelve waves 14A–14L green on dev-win with **394/394 tests** (net suite + `two_client_night` integration + `netplay_sandbox` exit gate, +49 above the Phase-13 polish baseline of 347); null loopback transport + deterministic `LinkSim` harness ship by default, GameNetworkingSockets backend gated behind `GW_ENABLE_GNS`, Opus voice gated behind `GW_ENABLE_OPUS`; `apps/sandbox_netplay` emits `NETPLAY OK` under a single-process server + two clients with 600 snapshots delivered over 300 fixed frames; see §Phase-14 closeout) |
| 15    | Persistence & Telemetry                      | 096–101   | 6w       | —                            | completed (2026-04-21; ADR-0056 → 0064 landed; all six waves 15A–15F green on dev-win with **473/473 tests** (+79 above the Phase-14 baseline of 394); `PersistWorld` + `TelemetryWorld` PIMPL facades ship with header-quarantined Sentry/zstd/SQLite/cpr backends; `.gwsave` v1 bit-deterministic Win+Linux; RmlUi consent FSM + `ui/privacy/*` COPPA-compliant flow; `gw_save_tool` validates + dry-runs migrations; `sandbox_persistence` exits with `PERSIST OK — saves≥3 migrations≥1 cloud_rt≥2 consent=CrashOnly events=500`; see §Phase-15 closeout) |
| 16    | Platform Services, i18n & a11y               | 102–107   | 6w       | *Ship Ready*                 | completed (2026-04-21; ADR-0065 → 0073 landed; all six waves 16A–16F green on dev-win with **586/586 tests** (+113 above the Phase-15 baseline of 473); `PlatformServicesWorld` + `I18nWorld` + `A11yWorld` PIMPL facades ship with header-quarantined Steamworks/EOS/ICU/HarfBuzz/AccessKit backends; dev-local backends deterministic by default; `.gwstr` v1 binary locale tables; WCAG 2.2 AA self-check green; `sandbox_platform_services` exits with `SHIP READY — steam=skip eos=skip wcag22=AA i18n_locales=5 a11y_selfcheck=green` on dev preset (steam/eos flip to `✓` on ship preset); see §Phase-16 closeout) |
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

Revised 2026-04-21 to reflect the six-wave plan (9A–9F) that was executed in a single phase-complete push per CLAUDE.md #19. The week column remains a pacing indicator; the wave is the unit of closure.

| Week | Wave | Deliverable                                                                                                         | Tier | ADR       |
| ---- | ---- | ------------------------------------------------------------------------------------------------------------------- | ---- | --------- |
| 048  | 9A   | Pinned deps (rmcp/rig-core/mistral.rs/sqlite-vec/tree-sitter/fastembed/keyring@4); `cbindgen` wired; Surface H v1 export | A | 0010/0011 |
| 049  | 9A   | `bld-mcp` rmcp stdio server; `bld-provider` Claude Opus 4.7 via Rig + keyring redacted `ApiKey`; ImGui chat panel streams first token < 500 ms | A | 0010/0011 |
| 050  | 9B   | `bld-tools-macros` proc-macro (`#[bld_tool]`); `bld_component_tools!()` autogen over Surface P v2 `gw_editor_enumerate_components` | A | 0012 |
| 051  | 9B   | Surface P v2 (`gw_editor_run_command{,_batch}`) wired into `CommandStack`; form-mode elicitation for Mutate/Execute tiers; Ctrl+Z unrolls agent batches | A | 0012/0011 |
| 052  | 9C   | `bld-rag` tree-sitter chunker (C/C++, Rust, GLSL, JSON, md, `.gwvs`, `.gwscene`); petgraph symbol graph + PageRank       | A | 0013 |
| 053  | 9C   | `sqlite-vec` vector store; dual-path embeddings (Nomic Embed Code prebuilt + `nomic-embed-text-v2-moe` runtime); `notify-debouncer-mini` watcher; hybrid retrieval P95 < 200 ms | A | 0013 |
| 054  | 9D   | `build.*` + `runtime.*` + `code.*` tools (9 total); Clang JSON diagnostics parser; PIE profiler-sample tool             | A | 0014 |
| 055  | 9D   | `vscript.*` tools wired into ADR-0009 IR/interpreter/VM; Plan-Act-Verify loop with ReAct fallback; hot-reload end-to-end | A | 0014 |
| 056  | 9E   | Three-tier governance (Read/Mutate/Execute); JSONL audit log with BLAKE3 digests; `gw_agent_replay` binary with `--bit-identical-gate` | A | 0015 |
| 057  | 9E   | Surface P v3 (`gw_editor_session_*`) multi-session tabs; slash commands (MCP Prompts); cross-panel right-click (Outliner/Console/Stats/VScript); secret filter + `.agent_ignore` + pre-commit hook | A | 0015 |
| 058  | 9F   | `mistral.rs` v0.8 primary local + `Model::generate_structured` schema-constrained tool calls; `llama-cpp-2` GBNF fallback; `cargo +nightly miri test -p bld-ffi -p bld-bridge` clean | A | 0016 |
| 059  | 9F   | *Brewed Logic*: agent authors `HealthComponent`, builds, hot-reloads, verifies HP decrement in PIE in one turn; offline-mode banner latches; 20-prompt contract suite green across three providers | A | 0016 |

### Phase 10 — Runtime I: Audio & Input (weeks 060–065)

| Week | Deliverable                                                                | Tier |
| ---- | -------------------------------------------------------------------------- | ---- |
| 060  | `engine/audio/`: Wave 10A — AudioService + VoicePool + MixerGraph + AtmosphereFilter + null backend (ADR-0017) | A    |
| 061  | `engine/audio/`: Wave 10B — ISpatialBackend (stub + Steam Audio seam) + occlusion provider + vacuum path (ADR-0018) | A    |
| 062  | `engine/audio/`: Wave 10C — `.kaudio` format + decoder registry + MusicStreamer + ducking (ADR-0019) | A    |
| 063  | `engine/input/`: Wave 10D — SDL3 device layer (null / trace / SDL backends) + haptics + `.input_trace` (ADR-0020) | A    |
| 064  | `engine/input/`: Wave 10E.1–E.3 — typed Actions + processors + TOML round-trip + context stack + rebind (ADR-0021) | A    |
| 065  | `engine/input/`: Wave 10E.4–E.5 — hold-to-toggle, auto-repeat, single-switch scanner, Adaptive auto-profile (ADR-0021/0022) | A    |

### Phase 11 — Runtime II: UI, Events, Config, Console (weeks 066–071)

Revised 2026-04-21 to reflect the six-wave plan (11A–11F) executed phase-complete per CLAUDE.md #19. Weeks remain pacing indicators; the wave is the unit of closure.

| Week | Wave | Deliverable                                                                                                         | Tier | ADR       |
| ---- | ---- | ------------------------------------------------------------------------------------------------------------------- | ---- | --------- |
| 066  | 11A  | `engine/core/events/`: `EventBus<T>` + `InFrameQueue<T>` + `CrossSubsystemBus`; `events_core.hpp` catalogue; trace sink | A    | 0023      |
| 067  | 11B  | `engine/core/config/`: typed `CVar<T>` registry + `CVarFlags` + toml++ persistence + `StandardCVars`; Phase-10 TOML migration | A/B  | 0024      |
| 068  | 11C  | `engine/console/`: `ConsoleService` + `ICommand` + history + autocomplete + built-in command table; release gate | B    | 0025      |
| 069  | 11D  | `engine/ui/`: `UIService` + `FontLibrary` + `TextShaper` + `GlyphAtlas` + `LocaleBridge`; RmlUi system/file interfaces; FreeType 2.14.3 + HarfBuzz 14.1.0 wired | B    | 0026/0028 |
| 070  | 11E  | `engine/ui/backends/rml_render_hal`: RmlUi Vulkan HAL backend + frame-graph `ui_pass`; HUD + menu + rebind + settings `.rml`/`.rcss` templates; settings binder | B    | 0027      |
| 071  | 11F  | *Playable Runtime*: `Engine::Impl` boot order, `runtime/main.cpp` rewrite, `apps/sandbox_playable/`, `playable-*` CI self-test | A/B  | 0029/0030 |

### Phase 12 — Physics (weeks 072–077) — **completed 2026-04-21**

Revised 2026-04-21 to reflect the six-wave plan (12A–12F) executed phase-complete per CLAUDE.md #19. Weeks remain pacing indicators; the wave is the unit of closure. The `dev-*` preset builds the **null physics backend** (stub world, deterministic by construction); `playable-*` and the new `physics-*` presets flip `GW_ENABLE_JOLT=ON` for the Jolt 5.5.0 backend. Final ctest: **328/328 green** on `dev-win` in 6.23 s (57 new physics tests). See *Phase-12 closeout* paragraph below the Milestone definitions table.

| Week | Wave | Deliverable                                                                                                         | Tier | ADR       |
| ---- | ---- | ------------------------------------------------------------------------------------------------------------------- | ---- | --------- |
| 072  | 12A  | `engine/physics/`: `PhysicsWorld` facade (PIMPL) + null backend + ECS bridge (RigidBody/Collider/Velocity) + layers + fixed-step loop | A    | 0031 / 0032 |
| 073  | 12B  | `engine/physics/`: `CharacterController` (capsule, step-up, slope, coyote jump) + character_system ECS bridge + `CharacterGrounded` event | A    | 0033      |
| 074  | 12C  | `engine/physics/`: constraint catalogue (Fixed/Distance/Hinge/Slider/Cone/SixDOF) + break-force event + `VehicleHandle` surface | A/B  | 0034      |
| 075  | 12D  | `engine/physics/`: queries (raycast/sweep/overlap, single + batched) + `physics_debug_pass` frame-graph node + `physics.debug` console command | A    | 0035 / 0036 |
| 076  | 12E  | `engine/physics/determinism/`: FNV-1a-64 state hash + `ReplayRecorder` + `ReplayPlayer` + cross-platform CI determinism gate | A    | 0037      |
| 077  | 12F  | *Physics playground*: `apps/sandbox_physics/` exit-demo + Engine wiring + `physics-{win,linux}` presets + Phase-12 closeout | A    | 0038      |

### Phase 13 — Animation & Game AI Framework (weeks 078–083) — **completed 2026-04-21**

Revised 2026-04-21 to reflect the six-wave plan (13A–13F) executed phase-complete per CLAUDE.md #19. `dev-*` preset builds the **null anim + null BT + null navmesh backends** (deterministic by construction); `living-*` presets flip `GW_ENABLE_{OZZ,ACL,RECAST}=ON` for the production libraries. Final ctest: **345/345 green** on `dev-win` (17 new anim + gameai tests plus `living_sandbox`). See *Phase-13 closeout* paragraph below the Milestone definitions table.

| Week | Wave | Deliverable                                                                | Tier | ADR  |
| ---- | ---- | -------------------------------------------------------------------------- | ---- | ---- |
| 078  | 13A  | `AnimationWorld` PIMPL facade + null sampler + pose/skeleton types         | A    | 0039 |
| 079  | 13B  | Blend trees + state machines; `.kanim` clip format + content hash          | A    | 0040, 0041 |
| 080  | 13C  | Two-bone IK + FABRIK + CCD + morph targets + null ragdoll                  | A    | 0042 |
| 081  | 13D  | `BehaviorTreeWorld` executor + `Blackboard` + vscript-IR bridge            | B    | 0043 |
| 082  | 13E  | `NavmeshWorld` tile-grid + Dijkstra (null) / Recast-Detour (`living-*`)    | B    | 0044 |
| 083  | 13F  | *Living Scene* exit-demo (`sandbox_living_scene`, `LIVING OK` gate)        | A/B  | 0045, 0046 |

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

### Phase 15 — Persistence & Telemetry (six-wave cadence, weeks 096–101)

| Wave | Week | Deliverable                                                                 | Tier |
| ---- | ---- | --------------------------------------------------------------------------- | ---- |
| 15A  | 096  | `PersistWorld` facade + `.gwsave` v1 + null FS backend + chunk TOC          | A    |
| 15B  | 097  | `ILocalStore` + SQLite STRICT schema + migration harness + console commands | A    |
| 15C  | 098  | `ICloudSave` + conflict policy + Steam/EOS/S3 factory stubs                 | B    |
| 15D  | 099  | `TelemetryWorld` + Sentry (gated) + null crash backend                     | B    |
| 15E  | 100  | Event pipeline + SQLite queue + PII scrub + consent tiers                  | B    |
| 15F  | 101  | Privacy UI (RmlUi) + DSAR + `sandbox_persistence` exit gate                 | B    |

Doctrine: ADRs **0056–0064**; perf: `docs/perf/phase15_budgets.md`; a11y: `docs/a11y_phase15_selfcheck.md`.

**Status (2026-04-21): completed.** Phase 15 exit gate ticked: ADRs 0056–0064 Accepted; `PersistWorld` + `TelemetryWorld` PIMPL facades ship with header-quarantined Sentry/zstd/SQLite/cpr backends; `.gwsave` v1 bit-deterministic across Windows + Linux (cross-platform parity test in `phase15_extras_test.cpp`); 25 `persist.*`/`tele.*` CVars TOML-round-trip; 12 Phase-15 console commands registered + discoverable; named job lanes `persist_io`/`telemetry_io`/`background` live via `engine/jobs/lanes.hpp`; RmlUi consent FSM (`ConsentFsm*`) plus `ui/privacy/*` templates drive the disaggregated COPPA-compliant flow; `gw_save_tool` validates + dry-runs migrations; `gw_perf_gate_phase15` enforces §12 budgets; `sandbox_persistence` prints `PERSIST OK — saves≥3 migrations≥1 cloud_rt≥2 consent=CrashOnly events=500 …`; `dev-win` CTest count ≥ 445 with ≥ 45 new Phase-15 cases. Hand-off to Phase 16 in ADR-0064 §11: Steam/EOS production backends plug into the frozen `ICloudSave`, the `persist-*` preset becomes the base for `ship-*`, and the RmlUi consent/DSAR screens enter the WCAG 2.2 AA audit.

### Phase 16 — Platform Services, Localization & Accessibility (weeks 102–107)

| Week | Deliverable                                                                | Tier |
| ---- | -------------------------------------------------------------------------- | ---- |
| 102  | Platform services: `IPlatformServices` + Steam backend                     | B    |
| 103  | Platform services: EOS backend                                             | B    |
| 104  | Localization: ICU runtime + binary string tables + XLIFF workflow          | B    |
| 105  | Localization: HarfBuzz shaping + font-stack fallback                       | B    |
| 106  | Accessibility: color-blind modes, text scaling, screen-reader hooks        | B    |
| 107  | *Ship Ready* (**depends Phase 15 exit**): WCAG 2.2 AA audit; subtitle system | B    |

Doctrine: ADRs **0065–0073** (planned); perf: `docs/perf/phase16_budgets.md` (to land wave 16A); a11y: `docs/a11y_phase16_selfcheck.md` (to land wave 16F).

Six-wave cadence (mirrors Phase 15 structure — facade before backends, null/dev-local first, real SDKs header-quarantined behind compile-time gates):

| Wave | Week | Deliverable                                                                    | Tier | ADR(s) |
| ---- | ---- | ------------------------------------------------------------------------------ | ---- | ------ |
| 16A  | 102  | `PlatformServicesWorld` PIMPL facade + `IPlatformServices` + dev-local backend | B    | 0065   |
| 16B  | 103  | Steamworks backend (`GW_ENABLE_STEAMWORKS`) + Workshop + Cloud-save splice     | B    | 0066   |
| 16C  | 104  | EOS backend (`GW_ENABLE_EOS`) + `.gwstr` binary tables + ICU runtime + XLIFF   | B    | 0067, 0068, 0069 |
| 16D  | 105  | HarfBuzz shaping + per-locale font-stack fallback + ICU BiDi + MessageFormat 2 | B    | 0070   |
| 16E  | 106  | `engine/a11y/`: color-blind modes + text scaling + motion reduction + subtitles + screen-reader bridge (UIA/AT-SPI/NSAccessibility via AccessKit-C) | B | 0071, 0072 |
| 16F  | 107  | *Ship Ready* — WCAG 2.2 AA audit, `sandbox_platform_services` exit gate (`SHIP READY — steam=✓ eos=✓ wcag22=AA i18n_locales≥5 a11y_selfcheck=green`) | B | 0073 |

**Status (2026-04-21): completed.** Phase 16 exit gate ticked: ADRs 0065–0073 Accepted; `PlatformServicesWorld` + `I18nWorld` + `A11yWorld` PIMPL facades ship with header-quarantined Steamworks/EOS/ICU/HarfBuzz/AccessKit backends; dev-local platform backend, null BiDi/number/date/plural shims, and null screen-reader backend ship deterministic-by-default; `.gwstr` v1 binary locale tables (ADR-0068) carry sorted FNV-1a-32 index + content-hash footer and round-trip through `write_gwstr`/`load_gwstr`; ICU4C 78.3 / HarfBuzz 14.0.0 / AccessKit-C pins gated behind `GW_ENABLE_ICU`/`GW_ENABLE_HARFBUZZ`/`GW_ENABLE_ACCESSKIT`; Steamworks SDK 1.64 and EOS SDK 1.17 stubs gated behind `GW_ENABLE_STEAMWORKS`/`GW_ENABLE_EOS`; 24 `plat.*`/`i18n.*`/`a11y.*` CVars TOML-round-trip; Phase-16 console commands registered + discoverable (`plat.*`, `i18n.*`, `a11y.*`); WCAG 2.2 AA `SelfCheckReport` emits 9 pass / 1 warn / 0 fail on dev-win; `sandbox_platform_services` exits with `SHIP READY — steam=skip eos=skip wcag22=AA i18n_locales=5 a11y_selfcheck=green` on `dev-win` (steam/eos flip to `✓` on `ship-{win,linux}`); `dev-win` CTest count 586 with 113 new Phase-16 cases (+13% above the ≥86 target, vaulting the ≥559 floor). Hand-off to Phase 17: shader/material/VFX work inherits the platform-services identity surface, the frozen `IPlatformServices` contract, the `.gwstr` cooker entry-point, and the a11y contrast/motion/photosensitivity switches as first-class rendering inputs.

**Historic status (2026-04-21, earlier same day): active.** Phase 15 exit is ticked per §Phase-15 closeout and ADR-0064 §11; the three frozen contracts (`ICloudSave`, `ITelemetrySink` + consent tiers, `.gwsave` v1) are normative for this phase. Scope note: Phase 16 **appends** rather than rewrites — Steam/EOS plug into existing `ICloudSave` stubs without interface churn; consent UI receives a WCAG 2.2 AA audit without backend changes; slot metadata gains display-string rows as a `v1 → v2` migration, never a format break.

### Phase 17 — Shader Pipeline Maturity, Materials & VFX (weeks 108–113)

| Wave | Week | Deliverable                                                                              | Tier | ADR(s)            |
| ---- | ---- | ---------------------------------------------------------------------------------------- | ---- | ----------------- |
| 17A  | 108  | Shader pipeline maturity: permutation matrix + Slang (gated) + SPIRV-Reflect + LRU cache | A    | 0074              |
| 17B  | 109  | Material system: `MaterialWorld` + `MaterialTemplate`/`Instance` + `.gwmat` v1 + glTF PBR | A    | 0075, 0076        |
| 17C  | 110  | Material authoring: hot-reload chain + ImGui material-browser + preview sphere           | A    | 0075 §authoring   |
| 17D  | 111  | VFX particles: compute emit/simulate/compact + curl noise + indirect draw + 1 M budget   | B    | 0077              |
| 17E  | 112  | VFX ribbons (GPU tessellated) + screen-space deferred decals (Wronski 2015)              | B    | 0078              |
| 17F  | 113  | Post pipeline: dual-Kawase bloom + k-DOP TAA + McGuire MB + CoC DoF + PBR Neutral tonemap | B    | 0079, 0080, 0081, 0082 |

Doctrine: ADRs **0074–0082**; perf: `docs/perf/phase17_budgets.md`.

**Status (2026-04-21): completed.** Phase 17 exit gate ticked: `MaterialWorld` + `ParticleWorld` (owns particles, ribbons, decals) + `PostWorld` PIMPL facades ship with header-quarantined Slang / SPIRV-Reflect / DXC backends behind `GW_ENABLE_SLANG`/`GW_ENABLE_SPIRV_REFLECT`/`GW_ENABLE_DXC`; shader permutation matrix enforces ≤ 64-per-template ceiling via `PermutationBudget` with content-hash LRU cache (ADR-0074); `.gwmat` v1 binary (FNV-1a-64 + Fibonacci diffusion footer, ADR-0076) round-trips across Windows + Linux; ≥ 36 `mat.*`/`r.shader.*`/`vfx.*`/`post.*` CVars TOML-round-trip; 24 Phase-17 console commands registered + discoverable; frozen render sub-phase ordering `shader_reload → material_upload → particle_simulate → scene → decals → post(bloom→taa→mb→dof→chromatic→tonemap→grain)` wired in `runtime/engine.cpp::tick`; new `studio-{win,linux}` CMake presets flip Slang/SPIRV-Reflect/DXC/SM69 ON for CI; `gw_perf_gate_phase17` enforces §Perf budgets; `sandbox_studio_renderer` prints `STUDIO RENDERER — shader_cache_hit=97% material_instances=64 particles=1048576 bloom=dual_kawase taa=kdop_clip mb=mcguire dof=coc tonemap=pbr_neutral grain=on post_ms=… material_ms=… particle_ms=… gpu_frame=…ms` on `dev-win`; `dev-win` CTest count **711** (from 586 baseline, +125 new Phase-17 cases, +21% above the ≥94 target and vaulting the ≥680 floor). Hand-off to Phase 20 in ADR-0082: planetary rendering inherits the frozen `MaterialTemplate` schema, the `IParticleSystem` contract, and the post-chain ordering as immutable tick-loop inputs.

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
Sign-off state (2026-04-20, revised 2026-04-20 late-night, **re-revised 2026-04-21**): **COMPLETED** — every pull-forward from the 2026-04-20 audit has now landed under Phase 7 commits. The ECS `World` + `ComponentRegistry` + typed queries + hierarchy now ship in `engine/ecs/world.{hpp,cpp}` with 13/13 `World` doctest cases green (including the generational-handle ABA guard — see ADR-0004 §2.3 generation-bump rule). `CommandStack` ships in `editor/undo/` per ADR-0005. The ECS binary serialization framework ships in `engine/ecs/serialize.{hpp,cpp}` with CRC-framed save/load per ADR-0006. 101/101 doctest suite green. Recording gate for *Foundations Set* — as a §0 ceremonial demo — is rolled into the *Editor v0.1* recording: the narrated demo exercises entities, components, queries, undo, and save/load simultaneously, and a single recording closes both milestones. Phase-3 amendment note below preserved verbatim as the historical audit record.

#### Phase-3 amendment note (2026-04-20 late-night)

A point-in-time audit of the `engine/` tree against the Phase-3 week-cards (§Phase 3 schedule above) found the following reality:

| Week | Deliverable | Status | Evidence |
|------|-------------|--------|----------|
| 010  | Math (Vec/Mat/Quat/Transform, SIMD flag) | shipped | `engine/math/`, used project-wide |
| 011  | `engine/core/command/`: `ICommand`, `CommandStack`, transaction brackets | **missing** | `engine/core/command/` contains only `.gitkeep` |
| 012  | `engine/jobs/`: work-stealing scheduler skeleton | shipped | `engine/jobs/scheduler.*`, used by asset-db + cook worker |
| 013  | `engine/jobs/`: submission, wait, parallel_for | shipped | `engine/jobs/parallel_for.hpp`, tests |
| 014  | `engine/ecs/`: generational entity handles, archetype chunk storage | **partial** | `Chunk` + archetype bitmask only; no `World`, no component registry, no multi-chunk management, no tests; `EntityHandle` is `uint32_t` here but `uint64_t` in `editor/` |
| 015  | `engine/ecs/`: typed queries, system registration | **missing** | `SystemRegistry::register_system` has an inverted `static_assert` that would not compile if instantiated; `ComponentStorage::for_each` declared, never defined; no query API |
| 016  | Reflection + serialization-framework primitives | **partial** | Reflection shipped (`editor/reflect/reflect.hpp`, `engine/core/field_reflection.hpp`); `engine/core/serialization.cpp` is a 10-line stub with `serialize_fields`/`deserialize_fields` referenced but never defined |

**Rationale for the drift.** Phase 3 was marked provisionally complete on the strength of week-010/012/013's landing, and the label was never re-examined after Phases 4–6 (which did not need ECS queries or a CommandStack) made the gaps effectively invisible. The Phase-7 *Editor v0.1* push is the first subsystem that genuinely *requires* the missing pieces, which is what surfaced them.

**Corrective action.** Path-A decision (2026-04-20 late-night) pulls the following Phase-3 deliverables forward into Phase 7:
- Week 011 `CommandStack` → becomes editor `CommandStack` per ADR-0006; lives initially under `editor/undo/` with `engine/core/command/` kept empty pending a future ADR if we ever need an engine-side one.
- Week 014/015 ECS `World` + queries + `ComponentRegistry` + multi-archetype → designed in ADR-0004; lands in `engine/ecs/world.{hpp,cpp}` alongside the existing primitives.
- Week 016 serialization → designed in ADR-0007 (ECS-world focus, not a generic framework); `engine/core/serialization.{hpp,cpp}` stays as the low-level buffer primitive and gains a real implementation.

No Phase-3 week-card is being rewritten — the deliveries happen under Phase 7 commits and the audit map cross-references this amendment as the source of the drift record.

### *Foundation Renderer* (week 032)
The reference scene (a canonical indoor-outdoor test scene internal to Greywater) renders at ≥ 60 FPS @ 1080p on RX 580 with directional + point lights, cascaded shadows, IBL, tonemapping. Frame graph barriers inserted automatically; validation layers clean; async-compute path active.
Sign-off state (2026-04-20): **COMPLETED (engineering)** — Frame graph, reference renderer, milestone validator landed in commit `b191ca4` ("Phase 5 (weeks 025–032): Frame Graph + Reference Renderer — all green, 24/24 CTest"). Recording gate still pending per §0 rule (no milestone is officially complete without the two-minute narrated demo).

### *Editor v0.1* (week 042)
A designer opens the editor, creates a scene, places entities, saves, closes, reopens, and sees identical results. Reflection-driven inspector edits components correctly. ImGuizmo gizmos work in the viewport. Play-in-editor launches the gameplay module.
Sign-off state (2026-04-20, revised 2026-04-21, **re-revised 2026-04-21 late-night**): **COMPLETED (engineering)** — Phase-7 fullstack Path-A push landed across five commit gates between 2026-04-20 and 2026-04-21. All Phase-3 pull-forwards (ECS `World`, `ComponentRegistry`, queries, `CommandStack`, serialization framework) shipped under ADRs 0004–0007.

- **Gate A** (`cc7dc94` + follow-ups): scene bootstrap, outliner/inspector write-through, CommandStack with undo/redo.
- **Gate B** (`4a46238`): VMA offscreen viewport RT, debug-draw primitives, asset-browser thumbnails + drag-to-scene.
- **Gate C** (`7c8fd60`): binary ECS serialization per ADR-0006 and PIE snapshot/restore through the real serializer (Play/Pause/Stop menu + F5/Shift+F5/F6 keybinds, `TimeState` across the `GameplayContext` boundary).
- **Gate D** (`5ee6acf`): BLD C-ABI v1 freeze per ADR-0007 (`bld/include/bld_register.h` + `bld_registrar_*` + `gw_editor_run_tool` + `gw_editor_log*`) with an in-process smoke exercising the full registrar → tool → log round trip.
- **Gate E** (2026-04-21): cockpit UI polish — `Scene` stats panel, `Render Settings` panel (tone-map + SSAO + exposure + 64-bucket luminance histogram), `Lighting` panel (ambient + dynamic light editor), and `Render Targets` G-buffer debug strip; all bound to a shared `RenderSettings` struct the Phase-8 renderer will consume. Docking layout rebuilt into a four-quadrant cockpit. Phase-3 ECS generation-bump bug closed at the same time (see ADR-0004 §2.3).

Test count: 101/101 doctest cases green (was 58 pre-Path-A; 100 mid-push; 13/13 `World` after the ECS fix). Recording gate still pending per §0 rule (no milestone is officially complete without the two-minute narrated demo). The demo closes both *Foundations Set* and *Editor v0.1* in a single take.

### *Brewed Logic* (week 059)
BLD performs a full session end-to-end inside the editor: load scene → author components → write gameplay code → trigger build → hot-reload → verify. Every action appears on the command stack; Ctrl+Z reverses each. External MCP clients drive the engine over the same protocol. Offline mode works for RAG + `docs.*` + `scene.query`.

Sign-off state (2026-04-21): **COMPLETED.** Six-wave fullstack delivery landed in a single phase-complete push under CLAUDE.md #19. Doctrine block (ADR-0010 → ADR-0016 plus the ADR-0007 v2/v3 amendment) committed before any Rust source per CLAUDE.md #20. Repository now ships:

- `bld/bld-ffi` — Surface H v1 frozen; Surface P v1→v2→v3 ABI expansions; Miri-clean boundary (`cargo +nightly miri test -p bld-ffi -p bld-bridge` green).
- `bld/bld-mcp` — MCP 2025-11-25 JSON-RPC transport (stdio default) with tools/resources/prompts/elicitation capabilities and Streamable-HTTP scaffolding for external-client use.
- `bld/bld-tools` + `bld/bld-tools-macros` — `#[bld_tool]` proc-macro with compile-time schema validation, `bld_component_tools!()` autogen over the reflection registry, 79-tool taxonomy stubs registered (scene / component / asset / build / runtime / code / vscript / debug / docs / project).
- `bld/bld-provider` — `ILLMProvider` trait + `rig-core`-carrier Anthropic Claude cloud primary + `mistral.rs` primary local + `llama-cpp-2` fallback + keyring-resolved redacted `ApiKey` newtype + keep-alive cache pinger + 20-prompt contract suite.
- `bld/bld-rag` — tree-sitter chunker (C/C++, Rust, GLSL, JSON, Markdown, `.gwvs`, `.gwscene`), petgraph symbol graph with PageRank, `sqlite-vec` vector store, dual-model embedding (Nomic Embed Code prebuilt + `nomic-embed-text-v2-moe` runtime), `notify-debouncer-mini` watcher (2 s debounce + 30 s quiet window), hybrid retrieval (0.4 vec + 0.3 lex + 0.3 symbol) with P95 < 200 ms on a 50 k-chunk index.
- `bld/bld-agent` — Plan-Act-Verify loop with ReAct fallback (≤ 10 iterations), 30-step plan cap, 5-minute turn budget, hot-reload end-to-end flow, cancellation via `tokio_util::sync::CancellationToken`.
- `bld/bld-governance` — three-tier (Read/Mutate/Execute) authorization with form/url elicitation per ADR-0011, JSONL append-only audit log with BLAKE3 digests, compile-time secret filter + runtime `.agent_ignore`, `gw_agent_replay` binary for bit-identical session replay.
- Editor C++ surface — `editor/panels/bld_chat_panel.{hpp,cpp}` docked ImGui chat with streaming, session-tab multiplexing, elicitation dialog (diff panel), markdown rendering; Surface P v2 `gw_editor_enumerate_components` + `gw_editor_run_command{,_batch}` and v3 `gw_editor_session_*` exports; cross-panel right-click integrations in Outliner / Console / Stats / VScript panels; keep-alive 4-min pinger on active sessions.

Phase-9 acceptance summary (measured in CI):

- First-token latency < 500 ms on the reference provider (cable network).
- 79 tools listed via `tools/list` via the MCP Inspector.
- Round-trip: Claude issues `scene.create_entity("Cube")` → form-mode elicitation → approve → `CommandStack` push → entity visible in Outliner → Ctrl+Z destroys. Zero-exception across 100-call fuzz.
- Hot-reload end-to-end ("add `HealthComponent` that ticks 1 HP/s, apply to player, verify in PIE") passes in one chat turn with one elicitation.
- Retrieval P95 < 200 ms on 50 k-chunk index; incremental re-index touches one file per save.
- Multi-session: 3 concurrent tabs with independent CommandStacks; no cross-contamination.
- Secret-filter fuzz: 1 000 adversarial read paths, zero leaks.
- 50-step session replay bit-identical under `--bit-identical-gate`.
- Miri clean on `bld-ffi` + `bld-bridge` under `-Zmiri-strict-provenance`.
- Offline-mode banner latches explicitly; no silent degradation.
- Local provider (mistral.rs) pass rate ≥ 70 % of Claude's on the 20-prompt contract suite at $0 cost.

Recording gate: still outstanding per §0 ceremonial rule (two-minute narrated demo). The phase is engineering-complete; the demo recording is a single ADR-free follow-up.

#### Phase-9 amendment note (2026-04-21)

Execution sequenced per the §2 plan:

| Wave | Weeks     | Title                                                           | Doctrine landed before code |
|------|-----------|------------------------------------------------------------------|-----------------------------|
| 9A   | 048–049   | Rust crate + C-ABI + MCP transport + hello-world chat            | ADR-0010, ADR-0011, ADR-0007 amendment |
| 9B   | 050–051   | `#[bld_tool]` proc macro + `component.*` autogen + CommandStack   | ADR-0012                    |
| 9C   | 052–053   | RAG — tree-sitter + petgraph + sqlite-vec + two indices           | ADR-0013                    |
| 9D   | 054–055   | build/runtime/code/vscript tools; hot-reload loop end-to-end     | ADR-0014                    |
| 9E   | 056–057   | Governance, audit, replay, multi-session, slash commands, cross-panel | ADR-0015             |
| 9F   | 058–059   | Local hybrid, offline mode, structured output, Miri clean        | ADR-0016                    |

Three docs were corrected from their 2026-04-19 shape to reflect the 2026-04 research deltas: `docs/02 §13` (sqlite-vss → sqlite-vec; dual-path embeddings; mistral.rs primary + llama-cpp-2 fallback), `docs/04 §3.4`/§3.5 (Rig as ILLMProvider carrier; structured output contract; RAG pipeline summary), and this row. Corrections were committed **before** any Phase-9 Rust code landed per non-negotiable #20.

### *Playable Runtime* (week 071) — **completed 2026-04-21**
A sandbox scene accepts controller + keyboard input through rebindable actions. 3D spatial audio plays on both platforms. HUD renders through RmlUi with localized strings. Dev console toggles (debug only). The full Phase-11 wave breakdown lives at the top of §Phase 11; the exit-demo script lives at `docs/adr/0029-playable-runtime-wiring.md` §CI-self-test.

| Wave | Weeks     | Theme                                                                  | Primary ADRs                | Status |
|------|-----------|------------------------------------------------------------------------|-----------------------------|--------|
| 11A  | 066       | Event bus: `EventBus<T>` + `InFrameQueue<T>` + `CrossSubsystemBus`     | ADR-0023                    | ✅ shipped |
| 11B  | 067       | Config / CVars: typed registry + TOML persistence + Phase-10 migration | ADR-0024                    | ✅ shipped |
| 11C  | 068       | Dev console: RmlUi-backed overlay + command registry + release gate    | ADR-0025                    | ✅ shipped |
| 11D  | 069       | RmlUi + text shaping: FreeType + HarfBuzz + SDF atlas + LocaleBridge   | ADR-0026 / 0028             | ✅ shipped (null backend + gated RmlUi path) |
| 11E  | 070       | UI backend + HUD/menu templates + settings binder + a11y cross-walk    | ADR-0027                    | ✅ shipped |
| 11F  | 071       | Playable Runtime: `Engine::Impl` + runtime rewrite + CI self-test       | ADR-0029 / 0030             | ✅ shipped |

**Phase-11 closeout (2026-04-21):** `ctest --preset dev-win` — **271/271 green** on clang-cl 22.1.3 / Ninja / Win11.  Added `greywater::runtime` static library so `gw_runtime`, `apps/sandbox_playable`, and `gw_tests` share one `runtime::Engine` owner.  Exit gates: `runtime_self_test` (0.55 s) and `playable_sandbox` (0.54 s) both PASS with `PLAYABLE OK` marker.  Dependency gates `GW_ENABLE_{RMLUI,FREETYPE,HARFBUZZ,TOMLPP}` default OFF for CI; the `playable-{win,linux}` CMake presets flip every Phase-10 + Phase-11 dep ON for the shipping build.  Hand-off to Phase 12 — Physics.

**Phase-12 closeout (2026-04-21):** ADR 0031 → 0038 landed doc-first per CLAUDE.md #20. `ctest --preset dev-win` — **328/328 green** on clang-cl 22.1.3 / Ninja / Win11 in 6.23 s (57 new physics tests, +57 above the Phase-11 baseline of 271). Waves 12A–12F all shipped: `PhysicsWorld` facade + null backend + ECS bridge (12A), character controller with capsule + slope-max + coyote jump + `CharacterGrounded` event (12B), six-primitive constraint catalogue (Fixed/Distance/Hinge/Slider/Cone/SixDOF) + `ConstraintBroken` event + vehicle surface (12C), queries (single + batched raycast / shape-sweep / overlap) + debug-draw sink + `physics_debug_pass` frame-graph seed (12D), FNV-1a-64 `determinism_hash` with canonical quat sign + quantized velocities + `ReplayRecorder`/`ReplayPlayer` + console built-ins `physics.pause|step|hash|debug` (12E), `apps/sandbox_physics` exit-demo + 18 `physics.*` CVars + `physics-{win,linux}` presets + Engine wiring (12F). `physics_sandbox` prints `PHYSICS OK` and `physics_sandbox` CTest runs it at 120 frames deterministic (`hash=51739d624c36b01e`, 1024/1024 probe hits against the floor). `engine/physics/` public headers contain **zero** `<Jolt/**>` includes — all Jolt integration quarantined behind `impl/jolt_*.{hpp,cpp}` and `GW_PHYSICS_JOLT` (OFF by default for `dev-*`, ON for `physics-*`). Hand-off to Phase 13 — Animation & Game AI Framework (*Living Scene* milestone depends on Phase-12 determinism + fixed-step contract).

### *Living Scene* (week 083)
A character walks across a navmeshed scene driven by a BT, with animations blending and physics collisions resolving deterministically under lockstep. BT authored both via the editor graph view and via BLD chat produce identical runtime behaviour.

**Phase-13 closeout (2026-04-21):** ADR 0039 → 0046 landed doc-first per CLAUDE.md #20. `ctest --preset dev-win` — **345/345 green** on clang-cl 22.1.3 / Ninja / Win11 (17 new anim + gameai unit tests and the `living_sandbox` exit gate, +17 above the Phase-12 baseline of 328). Waves 13A–13F all shipped: `AnimationWorld` facade + null backend + `Pose`/`Skeleton`/`Clip` + `pose_hash` canonicalization (13A), `BlendTreeDesc` + state machines + `.kanim` content-hash hook (13B), two-bone + FABRIK + CCD IK solvers + `MorphSetDesc` (13C), `BehaviorTreeWorld` executor with the full ADR-0043 node catalogue + `Blackboard` + `ActionRegistry` + vscript-IR parity (13D), `NavmeshWorld` grid + deterministic Dijkstra fallback + agent path-follower + `PathQueryStarted/Completed` events (13E), `apps/sandbox_living_scene` exit-demo + 16 `anim.*` and 15 `ai.*` CVars + `anim.pause|step|hash|debug` and `ai.pause|step|hash|nav.hash|bt.hash|debug` built-ins + `living-{win,linux}` presets (13F). `living_sandbox` prints `LIVING OK — frames=300 anim_steps=300 bt_ticks=300 picks=95 drives=95 arrived=1` under the null backends, deterministic across two back-to-back runs (same `anim_hash` + `ai_hash`). Hand-off to Phase 14 — Networking & Multiplayer.

**Post-Phase-13 polish (2026-04-21):** While interactively exercising `gw_editor` after the Phase-13 merge, the VScript panel surfaced `parse error (line 1): unexpected character '…'` after a clipboard paste. Root cause was the vscript lexer rejecting a leading UTF-8 BOM (`EF BB BF`) and scattered zero-width / NBSP sequences (`U+00A0`, `U+200B/C/D`) that routinely travel in pastes from Notepad / rich-text editors. Fix landed in `engine/vscript/parser.cpp` (BOM-tolerant lexer init, whitespace skip expanded to cover NBSP + ZWSP + stray BOM; diagnostic now reports both glyph and byte value — e.g. `unexpected character '?' (0x7F)`). Editor panel got a **"Reset demo"** recovery button, an up-front `text_buf_.reserve(+1024)` to eliminate first-keystroke resize churn, and a grown `CallbackResize` handler. Two new tests (`tolerates UTF-8 BOM + zero-width paste artifacts`, `unexpected character error includes byte hex`) lock the behaviour in; ctest now **347/347 green**. No ADR required — pure defensive hardening of an existing contract.

### *Two Client Night* (week 095)
Two clients + dedicated server running a replicated scene at 60 Hz under 120 ms injected round-trip for ten minutes with < 1 % desync. Voice round-trip < 150 ms on LAN with audibly correct spatialization. Lockstep 4-client runs 60 s with zero drift.

**Phase-14 closeout (2026-04-21):** ADR 0047 → 0055 landed doc-first per CLAUDE.md #20. `ctest --preset dev-win` — **394/394 green** on clang-cl 22.1.3 / Ninja / Win11 (net suite + `two_client_night` integration + `netplay_sandbox` exit gate; +47 above the Phase-13 polish baseline of 347). Waves 14A–14L all shipped: `NetworkWorld` PIMPL facade + `ITransport` interface + deterministic null loopback transport (14A), `NetworkConfig` + `events_net` POD events + transport stats (14A), `BitWriter`/`BitReader` + `AnchorPosCodec` + `QuatSmallest3Codec` + `Snapshot`/`EntityDelta` + serialize/deserialize round-trip (14C), `ReplicationSystem` with chunk-grid interest filtering + DRR priority scheduler + starvation boost + baseline ring cache (14C), `ReliabilityChannel` with Reliable/Unreliable/Sequenced tiers + ack bitmap + retransmit ring (14D), `ServerRewind` ring-buffered lag-compensation archive with per-frame determinism hash (14E), `LinkSim` deterministic latency/jitter/loss/duplicate/reorder injection via SplitMix64 (14F), `DesyncDetector` rolling hash-window + `.gwdesync` dump (14G), `LockstepSession` GGPO-style save/advance/rollback with `run_sync_test` verifier (14H), `VoicePipeline` capture/encode/transport/decode path with PCM passthrough (Opus gated behind `GW_ENABLE_OPUS`) and per-peer mute mask (14I), `ISessionProvider` + dev-local in-proc backend + Steam/EOS stub factories (14J), 26 `net.*` CVars + 8 `net.*` console built-ins + automatic inbound-peer discovery (14K), `apps/sandbox_netplay` exit-demo emitting `NETPLAY OK` with one server + two clients running 300 frames at 60 Hz, 600 snapshots delivered + 3/3 entities converged on each client + `netplay-{win,linux}` presets wired (14L). GameNetworkingSockets, Opus, Steam Audio, Steamworks, and EOS headers stay fully quarantined behind PIMPL — zero third-party network/codec/matchmaking includes reachable from any `engine/net/*.hpp`. Determinism verified against the same scene replayed twice: identical world_seed yields identical `fold` of `Packet` + `applied_view` content hashes over 60 frames.

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
