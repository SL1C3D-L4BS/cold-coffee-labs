# 02 — Roadmap & Operations

**Status:** Operational — updated as phases complete  
**Horizon:** ~190 weeks · ~44 months · 29 phases (Sacrilege ships on the Phase 24 RC in week 162; Phases 26–29 are the franchise-platform tail)  
**Start:** 2026-04-20 · **Planned Sacrilege RC:** 2029-05-28 · **Planned Franchise Pre-Prod close:** 2030-01-??

> **The phase is the contract. The week is the budget. The milestone is the proof.**

---

## Phase Overview

| Phase | Name | Weeks | Duration | Milestone | Status |
|-------|------|-------|----------|-----------|--------|
| 1 | Scaffolding & CI/CD | 001–004 | 4w | *First Light* | ✅ Complete |
| 2 | Platform & Core Utilities | 005–009 | 5w | — | ✅ Complete |
| 3 | Math, ECS, Jobs & Reflection | 010–016 | 7w | *Foundations Set* | ✅ Complete |
| 4 | Renderer HAL & Vulkan Bootstrap | 017–024 | 8w | — | ✅ Complete |
| 5 | Frame Graph & Reference Renderer | 025–032 | 8w | *Foundation Renderer* | ✅ Complete |
| 6 | Asset Pipeline & Content Cook | 033–036 | 4w | — | ✅ Complete |
| 7 | Editor Foundation | 037–042 | 6w | *Editor v0.1* | ✅ Complete |
| 8 | Scene Serialization & Visual Scripting | 043–047 | 5w | — | ✅ Complete |
| 9 | BLD (Brewed Logic Directive, Rust) | 048–059 | 12w | *Brewed Logic* | ✅ Complete |
| 10 | Runtime I — Audio & Input | 060–065 | 6w | — | ✅ Complete |
| 11 | Runtime II — UI, Events, Config, Console | 066–071 | 6w | *Playable Runtime* | ✅ Complete (271/271) |
| 12 | Physics | 072–077 | 6w | — | ✅ Complete |
| 13 | Animation & Game AI Framework | 078–083 | 6w | *Living Scene* | ✅ Complete (347/347) |
| 14 | Networking & Multiplayer | 084–095 | 12w | *Two Client Night* | ✅ Complete (394/394) |
| 15 | Persistence & Telemetry | 096–101 | 6w | — | ✅ Complete (473/473) |
| 16 | Platform Services, i18n & a11y | 102–107 | 6w | *Ship Ready* | ✅ Complete (586/586) |
| 17 | Shader Pipeline, Materials & VFX | 108–113 | 6w | *Studio Renderer* | ✅ Complete (711/711) |
| 18 | Cinematics & Editor Toolchain | 114–118 | 5w | *Studio Ready* | ✅ Complete |
| 19 | **Blacklake Core: HEC, SDR, Chunk Streaming** | 119–126 | 8w | *Infinite Seed* | ✅ Complete (26 universe_tests) |
| 20 | **GPTM, Nine Circles Geometry, Floating Origin** | 127–134 | 8w | *Nine Circles Ground* | ✅ Complete |
| 21 | **Hell Frame & Narrative Skin** | 135–142 | 8w | *Hell Frame* | ✅ Complete |
| 22 | **Martyrdom & God Mode** | 143–148 | 6w | *Martyrdom Online* | ✅ Complete |
| 23 | **Damned Host, Encounters, God Machine + Logos** | 149–154 | 6w | *God Machine RC* | ✅ Complete |
| 24 | Hardening & Release (**Sacrilege ships**) | 155–162 | 8w | *Release Candidate* | 🔜 Planned |
| 25 | LTS Sustenance | 163–∞ | Ongoing | Quarterly LTS | 🔜 Ongoing |
| 26 | **Runtime AI Stack** (ai_runtime, hybrid Director, symbolic music) | parallel 135–162 | 8w | *Living Director* | 🔜 Planned |
| 27 | **Franchise Services Hardening** (seven IP-agnostic services) | parallel 143–162 | 6w | *Content Engine* | 🔜 Planned |
| 28 | **Cook-time AI Pipeline** (VAE weapons, WFC+RL, neural material, symbolic music training) | parallel 149–162 | 6w | *Forge Online* | 🔜 Planned |
| 29 | **Franchise Pre-Production** (Apostasy / Silentium / The Witness design + prototype slots) | 163–190 | ~28w | *Second IP Ready* | 🔜 Planned |

---

## Active Phase: Phase 24 — Hardening & Release

*Phase 23 (*God Machine RC*) is complete — see §Completed Phase: Phase 23 below. Prior pivot work: phases 20–22 on `pivot/phase-21-22-execution`.*

## Completed Phase: Phase 20 — GPTM, Nine Circles Geometry, Floating Origin

### Week-by-Week

| Week | Deliverable | Tier |
|------|-------------|------|
| 127 | `GptmVertex` / `GptmTile` types; `GptmMeshBuilder` Lod0 implementation | A |
| 128 | `GptmMeshBuilder` full LOD chain (Lod1–Lod4); SVO cave skeleton | A |
| 129 | LOD selector — screen-space error metric; LOD handoff shader | A |
| 130 | Circle SDR profile integration: all 9 `CircleProfile` constexprs driving terrain | A |
| 131 | `FloatingOrigin` — recentre at 2048m; `OriginShiftEvent` broadcast; all subscribers | A |
| 132 | Vegetation instancing — GPU indirect draw, 4 LOD levels, 256 instances/chunk | A |
| 133 | `GptmTileRenderer` — 5 draw calls total for all terrain; `terrain.hlsl` | A |
| 134 | *Nine Circles Ground* exit gate: `sandbox_nine_circles` — all 9 Circles generate, LOD correct, floating origin fires | A |

### Phase 20 Acceptance Criteria

- [x] All 9 `CircleProfile` constants produce visually distinct terrain (distinct mean/variance in height distribution)
- [x] LOD selector returns Lod0 at < 32m, Lod4 at > 2km — verified by test
- [x] `FloatingOrigin` fires when camera moves > 2048m; all subscribers receive `OriginShiftPayload`
- [x] Vegetation instances > 0 for all non-void chunks in all 9 Circles
- [x] `sandbox_nine_circles` prints: `NINE CIRCLES GROUND — circles=9 chunks=81 lod=OK veg=OK origin=OK`
- [x] CI green on dev-win + dev-linux (see `tests/unit/render/frame_graph_test.cpp` — P20-FRAMEGRAPH-ALIASING path moved to lazy execute-time aliasing, compile() now pure CPU)
- [x] No Vulkan validation errors in any phase of the exit gate

---

## Completed Phase: Phase 21 — Hell Frame & Narrative Skin

*The phase that makes Greywater Engine look and sound like Sacrilege — and gives the Nine Circles their Story-Bible Location Skins.*

### Week-by-Week

| Week | Deliverable | Tier |
|------|-------------|------|
| 135 | `horror_post.hlsl` — chromatic aberration, film grain, screen shake in single pass | A |
| 136 | Sin tendril vignette shader; Ruin desaturation; Rapture whiteout (post-process CVars) | A |
| 137 | `BloodDecalSystem` — compute stamp + drip; 512-entry ring buffer; `DecalComponent` | A |
| 138 | Circle-specific fog parameters + **Location Skins** (Vatican Necropolis, Colosseum of Echoes, Sewer of Grace, Hell's Factory, Rome Above, Palazzo of the Unspoken, Open Arena, Heaven's Back Door, Silentium) wired as `CircleProfile::location_skin` data | A |
| 139 | `MartyrdomAudioController` — Sin choir, Rapture thunder, Ruin drone, BPM layer crossfade; `engine/narrative/dialogue_graph` line-trigger events on the horror post stack | A |
| 140 | Diegetic HUD — blood vial, runic Sin ring, Mantra counter in RmlUi; editor **Circle Editor** + **Encounter Editor** panels land for Location Skin authoring | A |
| 141 | Performance gate: RX 580 @ 1080p / 144 FPS with full horror post stack + narrative-line trigger overhead | A |
| 142 | *Hell Frame* exit gate: `sandbox_hell_frame` — Violence Circle (**Open Arena** skin), full post, decals, HUD, three Malakor / Niccolò voice lines triggered by Sin thresholds | A |

---

## Completed Phase: Phase 22 — Martyrdom & God Mode

*The phase where Sacrilege becomes playable and the Sin 70–100 band escalates into Blasphemy State / God Mode.*

### Week-by-Week

| Week | Deliverable | Tier |
|------|-------------|------|
| 143 | Martyrdom ECS components: `SinComponent`, `MantraComponent`, `RaptureState`, `RuinState`, `ResolvedStats`; `gameplay/martyrdom/god_mode.hpp` + `blasphemy_state.hpp` additions | A |
| 144 | `SinAccrualSystem`, `MantraAccrualSystem`, `RaptureCheckSystem`, `RaptureTickSystem`, `RuinTickSystem`; `engine/narrative/sin_signature` rolling fingerprint | A |
| 145 | `BlasphemySystem` — all 5 canon Blasphemies + three Story-Bible unlockables (Salt Pillar, Inverted Gravity, Hymn Needles) gated to Acts II/III; `StatCompositionSystem` | A |
| 146 | Player controller: swept-AABB, bunny hop, slide, wall kick; **Malakor / Niccolò voice director** with mirror-step ability | A |
| 147 | Grapple hook, blood surf, Glory Kill trigger; **God Mode / Blasphemy State** reality-warp post hooks (Act II mirror-step, gravity invert) | A |
| 148 | *Martyrdom Online* exit gate: 2-player session, full Martyrdom loop, God Mode onset + exit parity, no desync over 5 min; editor **Dialogue Graph** + **Sin-Signature** + **Act / Phase State** panels land | A |

---

## Phase 20–22 Retrospectives

### Phase 20 retro — *Nine Circles Ground* (weeks 127–134)
- **Landed:** GPTM vertex / tile types, LOD0–LOD4 mesh builder, screen-space-error LOD selector, `FloatingOrigin` recentering at 2048 m, vegetation GPU indirect instancing, `sandbox_nine_circles` exit banner, all 9 `CircleProfile` constexprs driving terrain.
- **Shift:** `FrameGraph::compile()` initially aborted via `GW_UNIMPLEMENTED[P20-FRAMEGRAPH-ALIASING]` when transient resources were declared. Fix: deferred VMA-backed aliasing to a lazy `execute()`-time construction, keeping `compile()` a pure CPU operation and restoring the 788/788 smoke-test green bar. The ADR-0063 aliasing pass itself remains scheduled for Phase 23 parallel work (no behaviour change — memory is still correctly owned by the `ResourceRegistry`).
- **Risk cleared:** tests/unit/render/frame_graph_test.cpp is now part of the mandatory smoke set and will catch any regression on the aliasing pre-gate.

### Phase 21 retro — *Hell Frame* (weeks 135–142)
- **Landed:** `horror_post.hlsl` single-pass stack (chromatic aberration, grain, shake), sin tendril / ruin desaturation / rapture whiteout stages, `BloodDecalSystem` 512-entry ring buffer, the nine `CircleProfile` location skins (Vatican Necropolis → Silentium), `MartyrdomAudioController`, the diegetic RmlUi HUD model, and the Circle/Encounter editor panels. `gw_perf_gate_hell_frame` holds the RX 580 @ 1080 p / 144 FPS budget and `sandbox_hell_frame` prints the `HELL FRAME — circles=9 stages=horror+sin+ruin+rapture audio=OK decals=OK reality_warp=OK` banner.
- **Shift:** `DecalComponent::alpha` initially defaulted to 1.0 f, which made empty ring slots look live; flipped to 0.0 f with the active opacity set by the caller at `insert()`. The ring now correctly accounts for live vs. evicted entries and the blood-decal stats test passes.

### Phase 22 retro — *Martyrdom Online* (weeks 143–148)
- **Landed:** the full `gameplay/martyrdom/` surface (`SinComponent`, `MantraComponent`, `RaptureState` with trigger counter, `RuinState`, `ResolvedStats`, `ActiveBlasphemies` FIFO 3-slot, `BlasphemySystem` + 3 unlockable Story-Bible Blasphemies, `StatCompositionSystem`), swept-AABB player controller with bunny-hop / slide / wall-kick, Mirror-Step ability with a Malakor hard-override line in `VoiceDirector`, grapple hook with reel-in + small-enemy auto Glory Kill, blood surf (slope > 15° / blood decal), `evaluate_glory_kill` per-class HP thresholds, and the `sandbox_martyrdom_online` 2-peer determinism sandbox. Editor **Dialogue Graph**, **Sin-Signature**, and **Act / Phase State** panels landed as authoring data models (ImGui-guarded, unit-testable without a display context).
- **Shift:** the determinism test initially reported 10 divergences. Root cause was that `sizeof()`-based struct hashing walks **padding bytes**, which are indeterminate after aggregate assignment per `[basic.types.general]`. Fix: both `martyrdom_determinism_test` and `sandbox_martyrdom_online` now hash explicit fields (with enum-to-uint cast for `BlasphemyType`/`GrappleMode` to avoid a second implicit-padding bug). Determinism is now bit-stable 2-peer × 360 ticks × `final_state_hash=ad06af9be69adb62`.
- **New invariant:** any future rollback-snapshot hash must hash individual fields, not `sizeof(Struct)`. Codified in `ADR-0119` (existing) — followups tracked in `docs/10_APPENDIX_ADRS_AND_REFERENCES.md`.

---

## Completed Phase: Phase 23 — Damned Host, Encounters, God Machine + Logos

*The phase that makes Sacrilege a game, not a tech demo — and adds the Grace finale (Logos alternate Phase 4).* 

### Week-by-Week

| Week | Deliverable | Tier |
|------|-------------|------|
| 149 | Enemy archetypes: Cherubim, Deacon, Leviathan, Warden — ECS + BT wiring | A |
| 150 | Enemy archetypes: Martyr, Hell Knight, Painweaver, Abyssal — ECS + BT wiring | A |
| 151 | Gore system: dismemberment, limb entities as nav obstacles, blood decal placement on hit; **hybrid AI Director** (rule state machine + bounded RL params) wired from Phase 26 | A |
| 152 | Full weapon roster: all 6 weapons, hitscan/projectile/melee, alt-fires, Sin/Mantra coupling | A |
| 153 | God Machine boss: 4 phases, arena collapse, Martyrdom economy gating; **Logos alternate Phase 4** — Silent God speaks with player's own voice from Sin-signature data; **Grace meter** unlocks in Act III | A |
| 154 | *God Machine RC* exit gate: Circle IX (**The Silentium / Throne of the Logos** skin) end-to-end playable; God Machine defeatable by damage **and** Logos defeatable by Grace (`forgive` Blasphemy); editor **AI Director Sandbox** + **Editor Copilot** panels land | A |

### Phase 23 retrospective

- **Landed:** `gameplay/enemies/` (eight archetypes + shared BT), `gameplay/combat/gore_system` with `gore_service` delegation, `encounter_director_bridge` → `director::evaluate`, `gameplay/weapons/` six-weapon runtime, `gameplay/boss/god_machine` + Logos Phase 4 / Grace hooks, `apps/sandbox_god_machine_rc` exit banner + CTest gate, Sacrilege **AI Director Sandbox** and **Editor Copilot** panels (ImGui).
- **Shift:** multi-queue `FrameGraph` scheduling had a `GW_UNIMPLEMENTED` abort in `submit.cpp` when any pass was scheduled; Phase 23 closes this with a **structured `CompilationFailed`** error (`P20-FRAMEGRAPH-CMDBUF`) and unit tests — use single-queue `execute()` until per-pass command recording ships under ADR-0063.
- **Preflight:** `pre-eng-ai-runtime-dispatch` — `director_service`, `audio_weave`, and `material_forge` already include and call `engine/ai_runtime`; runtime AI budget scaffold **`gw_perf_gate_director`** (`tests/perf/director_budgets_perf_test.cpp`, CTest `director_budgets_perf`) passes under default (non-strict) CI settings.

---

## Phase 24 — Hardening & Release

| Week | Deliverable | Tier |
|------|-------------|------|
| 155–156 | Full regression: 711+ tests green all presets; zero sanitizer warnings; fuzz (tests/fuzz/) + determinism (nightly `determinism.yml`) + Steam Deck probe all green | A |
| 157 | WCAG 2.2 AA audit pass (gameplay + editor); post-effect accessibility toggles all functional; photosensitivity pre-warning | A |
| 158 | Platform builds: Windows installer + Linux AppImage/DEB + Steam Deck (steamrt-sniper) — Ed25519-signed cooked asset verification enabled in Release | A |
| 159 | BLD ships ~60 tools; RAG index shipped as asset; all editor tools functional (incl. concept-to-material, encounter-suggest, exploit-detect, scene-heatmap, voice-line-generate) | A |
| 160 | 4-client co-op session stable 15+ min; voice chat works; gameplay_replay capture (< 1 MB per run) round-trips deterministically | B |
| 161 | Engineering handbook, LTS runbook, onboarding guide, Mod SDK docs — all indexable by BLD | A |
| 162 | *Release Candidate* — all 9 Circles completable; God Machine defeatable; Logos defeatable by Grace; 10-min demo; **Sacrilege ships** | A |

### Phase 24 engineering checklist (weeks 155–162)

Use this as the **repo-local execution surface** for the week table above (CI + focused tests). Full program work (installers, 4-client soak, handbook) stays in Linear/cards; these items are the merge gates we can verify in-tree.

| Weeks | Repo / CI signal |
|------|-------------------|
| 155–156 | `.github/workflows/ci.yml` matrix (Debug + Release on Ubuntu + Windows); `determinism.yml`, `fuzz.yml`, `sanitizers.yml`, `steam_deck.yml`, `agentshield.yml` green on schedule; `ctest --preset dev-win` / `dev-linux` full monolith. |
| 157 | WCAG / a11y acceptance in editor + gameplay (manual QA); code: `photosensitivity_warn_ack_` path in editor (Phase 24 week 157). |
| 158 | Release `NDEBUG`: `cmake --preset release-win` / `release-linux`, `ctest` on Release build; `engine/assets/cook_trust` + `tools/cook/content_signing` alignment per ADR-0096. |
| 159 | BLD tool surface + RAG asset (tracked separately in `bld/`). |
| 160 | `gameplay_replay` + director tests; net stack soak (manual / perf job). |
| 161 | Docs index + LTS runbook (`docs/` + onboarding — card-driven). |
| 162 | RC exit: `sandbox_god_machine_rc` (or successor RC gate), Steam Deck probe, 10-min demo script. |

### After Phase 24 RC (Phases 25–29 entry)

**Phase 25 (LTS):** quarterly sustenance on the shipped Phase 24 baseline — security/crash hotfixes, toolchain bumps, and `docs/02` Phase 25 row (ongoing).

**Parallel franchise platform (26–29):** start from the week tables in this doc §Phase 26–29 — **26** Runtime AI (`engine/ai_runtime`, director perf gates), **27** franchise services contract tests, **28** cook-time ML pipelines, **29** second-IP prototypes. None of these block Sacrilege RC; they consume dedicated WIP slots per Kanban rules above.

---

## Phase 26 — Runtime AI Stack  *(parallel with 21–24, 8w)*

*Living Director — on-device ML under the `01` §2.2.2 carve-out. Hybrid rule state machine + bounded RL params; symbolic-transformer music; optional neural material eval.*

| Week (par. offset) | Deliverable | Tier |
|-|-------------|------|
| W+0 | `engine/ai_runtime/inference_runtime.{hpp,cpp}` — ggml CPU wrapper (opt-in Vulkan behind `GW_AI_VULKAN=OFF`); `model_registry.hpp` with BLAKE3 + Ed25519 verify | A |
| W+1 | `director_policy.hpp` — L4D-style state machine (Build-Up / Sustained Peak / Peak Fade / Relax); bounded RL parameter clamps; reset-on-rewind for rollback safety | A |
| W+2 | `music_symbolic.hpp` — ≤1M param RoPE transformer picking BPM-sync layer crossfade over cooked stems; presentation-only | A |
| W+3 | `material_eval.hpp` — ggml Vulkan compute shader eval of cooked neural material weights; non-authoritative | A |
| W+4 | `ai_cvars.hpp` + perf gates: `gw_perf_gate_ai_runtime` enforces ≤ 0.74 ms total budget on RX 580 | A |
| W+5 | `apps/sandbox_director/` — standalone headless-CI + interactive ImGui modes; `gw_perf_gate_director` ≤ 0.1 ms | A |
| W+6 | Determinism validator extension: same seed + same checkpoint + same synthetic input = bitwise-identical state hash Linux ↔ Windows | A |
| W+7 | *Living Director* milestone: sandbox_director prints `DIRECTOR SANDBOX OK`, determinism.yml passes, editor AI Director Sandbox panel reads the same library | A |

---

## Phase 27 — Franchise Services Hardening  *(parallel with 22–24, 6w)*

*Content Engine — declare, contract-test, and ship the seven IP-agnostic services. Sacrilege consumes all seven; no second IP required yet.*

| Week (par. offset) | Deliverable | Tier |
|-|-------------|------|
| W+0 | `engine/services/material_forge/` INTERFACE schema + impl shim wrapping `engine/render/material/` | A |
| W+1 | `engine/services/level_architect/` wrapping `engine/world/` + cook-time WFC/RL hooks | A |
| W+2 | `engine/services/combat_simulator/` wrapping `engine/ai/` + enemy DNA schema; `engine/services/gore/` schema + impl | A |
| W+3 | `engine/services/audio_weave/` wrapping `engine/audio/` + symbolic-music runtime; `engine/services/director/` wrapping Phase 26 Director | A |
| W+4 | `engine/services/editor_copilot/` wrapping BLD tool surface + HITL approval | A |
| W+5 | *Content Engine* milestone: each service has a `schema/` header, an impl target, a doctest, and an integration test that a dummy "Apostasy" pre-prod prototype can link to without touching Sacrilege | A |

---

## Phase 28 — Cook-time AI Pipeline  *(parallel with 23–24, 6w)*

*Forge Online — offline ML bakeries under §2.2.1 (Python). No shipped runtime impact beyond pinned cooked weights.*

| Week (par. offset) | Deliverable | Tier |
|-|-------------|------|
| W+0 | `tools/cook/ai/vae_weapons/` — VAE generates weapon permutations; balance filter emits cooked `.gwweapon` | A |
| W+1 | `tools/cook/ai/wfc_rl_scorer/` — scores WFC layouts with trained RL agent; emits baked parameter sets consumed by Blacklake | A |
| W+2 | `tools/cook/ai/music_symbolic_train/` — trains the symbolic transformer used by `audio_weave` | A |
| W+3 | `tools/cook/ai/neural_material/` — concept/prompt → baked `.gwmat` nodes | A |
| W+4 | `tools/cook/ai/director_train/` — produces pinned Director policy checkpoints (ADR-0097) | A |
| W+5 | *Forge Online* milestone: all five pipelines reproduce deterministic outputs on CI; pinned weights live under `assets/ai/` with signatures | A |

---

## Phase 29 — Franchise Pre-Production  *(163–190, non-blocking)*

*Second IP Ready — design docs, prototype slots, and platform-fit validation for the Sacrilege family. Never competes with flagship resources.*

| Weeks | Deliverable | Tier |
|-------|-------------|------|
| 163–170 | ***Sacrilege 2: Apostasy*** one-page vision in `docs/11_NARRATIVE_BIBLE.md` + 2-week prototype slot in `apps/sandbox_apostasy_prototype/` | B |
| 171–178 | ***Sacrilege: Silentium*** (survival-horror in the mirror dimension) vision + prototype slot | B |
| 179–186 | ***Sacrilege: The Witness*** (prequel) vision + prototype slot | B |
| 187–190 | *Second IP Ready* milestone — one prototype completes a 10-min loop using the seven services unchanged; zero patches to Sacrilege runtime | B |

---

## Kanban Operations

### Board

```
Backlog → Ready → In Progress (WIP ≤ 2) → Review → Done
```

WIP limit of 2 is a **hard rule**. Never exceed it. Exceeding it splits focus and guarantees both cards regress.

### Card Format

```markdown
## [P20-W127] GptmVertex types and Lod0 mesh builder

**Tier:** A  **Phase:** 20  **Week:** 127  **ADR:** 0087 (if new decision)

### Context
One paragraph explaining what this builds on and why it matters now.

### Acceptance criteria
- [ ] `sizeof(GptmVertex) == 32` (static_assert)
- [ ] Lod0 mesh for a test chunk has correct vertex count
- [ ] All vertex normals are unit vectors (within 1e-5)
- [ ] Build time < 3ms for Lod0 on dev hardware
- [ ] CI green dev-win + dev-linux
```

### Rituals

| Ritual | Cadence | Duration | Rule |
|--------|---------|----------|------|
| Daily focus | Each working day | 90 min × 1–2 blocks | One card. No multitasking. |
| Commit | End of each block | 5 min | Conventional Commits format. CI stays green. |
| Weekly retro | Sunday | 30 min | Read 7 daily logs. **No coding during retro.** |
| Phase gate | End of each phase | 60 min | Run the exit gate binary. Update `docs/`. Write closeout note. |

### Anti-Patterns (merge-blocking)

| Anti-pattern | Rule |
|---|---|
| WIP > 2 | Splits focus → both cards fail |
| Skipping cold-start checklist | You will code the wrong thing |
| "Tests later" | Phase gate requires CI green. Later never comes. |
| Direct commit to `main` | Branch + PR always. CI gates every change. |
| ADR written after code | Decisions under implementation pressure are bad decisions. ADR first. |
| Sunday coding | Rest days are load-bearing. Burnout is the #1 project risk. |

---

## Milestone Definitions

| Milestone | Required | Exit Proof |
|-----------|----------|------------|
| *First Light* (Ph 1) | Hello-triangle + BLD C-ABI smoke test | 2-min recorded video |
| *Foundations Set* (Ph 3) | ECS World, CommandStack, Jobs — 101/101 tests | Entities + undo in editor |
| *Foundation Renderer* (Ph 5) | Frame graph + Forward+; Sponza @ ≥ 60 FPS RX 580 (engine reference gate — not the *Sacrilege* 144 FPS ship bar; see `05` §1.4) | Recorded scene |
| *Editor v0.1* (Ph 7) | 3D viewport, asset browser, BLD panel, play mode | 5-min walkthrough |
| *Brewed Logic* (Ph 9) | BLD MCP tools; RAG; local inference; editor wired | BLD places enemy in editor live |
| *Playable Runtime* (Ph 11) | Audio, HUD, console, config — runtime ships | Sandbox plays audio + UI |
| *Living Scene* (Ph 13) | Animation blend trees + BTs + navmesh | 100 animated enemies pathfind |
| *Two Client Night* (Ph 14) | GNS transport + ECS replication + 2-client test | 2 clients sync 60s |
| *Ship Ready* (Ph 16) | WCAG 2.2 AA, 5 locales, Steamworks/EOS integration | Localized menu + audit |
| *Studio Renderer* (Ph 17) | Full VFX, PBR Neutral, bloom/TAA/DoF/CA/grain | Studio render of Sacrilege Circle |
| *Studio Ready* (Ph 18) | `.gwseq` playback, mod API, BLD seq tools, `sandbox_studio` | `sandbox_studio` prints STUDIO READY |
| *Infinite Seed* (Ph 19) | HEC + SDR + chunk streaming — deterministic | Same seed = same terrain, 2 machines |
| *Nine Circles Ground* (Ph 20) | All 9 Circles generate; GPTM LOD correct; floating origin | `sandbox_nine_circles` exit gate |
| *Hell Frame* (Ph 21) | Horror post, blood decals, diegetic HUD, 144 FPS RX 580 | `sandbox_hell_frame` + recorded gameplay |
| *Martyrdom Online* (Ph 22) | Full Martyrdom loop; player controller; 2-player no-desync | 2-player session video |
| *God Machine RC* (Ph 23) | All 9 Circles completable; God Machine defeatable | Full playthrough video |
| *Release Candidate* (Ph 24) | All above + cert builds + docs | 10-min board demo, no intervention |

---

*The phase is the contract. The week is the budget. The milestone is the proof.*
