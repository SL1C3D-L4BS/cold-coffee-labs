# 02 — Roadmap & Operations

**Status:** Operational — updated as phases complete  
**Horizon:** 162 weeks · ~37 months · 25 phases  
**Start:** 2026-04-20 · **Planned RC:** 2029-05-28

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
| 20 | **GPTM, Nine Circles Geometry, Floating Origin** | 127–134 | 8w | *Nine Circles Ground* | 🔜 Active |
| 21 | **Hell Frame: Horror Post, Decals, HUD, Audio** | 135–142 | 8w | *Hell Frame* | 🔜 Planned |
| 22 | **Martyrdom Combat & Player Stack** | 143–148 | 6w | *Martyrdom Online* | 🔜 Planned |
| 23 | **Damned Host, Encounters, God Machine Boss** | 149–154 | 6w | *God Machine RC* | 🔜 Planned |
| 24 | Hardening & Release | 155–162 | 8w | *Release Candidate* | 🔜 Planned |
| 25 | LTS Sustenance | 163–∞ | Ongoing | Quarterly LTS | 🔜 Ongoing |

---

## Active Phase: Phase 20 — GPTM, Nine Circles Geometry, Floating Origin

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

- [ ] All 9 `CircleProfile` constants produce visually distinct terrain (distinct mean/variance in height distribution)
- [ ] LOD selector returns Lod0 at < 32m, Lod4 at > 2km — verified by test
- [ ] `FloatingOrigin` fires when camera moves > 2048m; all subscribers receive `OriginShiftPayload`
- [ ] Vegetation instances > 0 for all non-void chunks in all 9 Circles
- [ ] `sandbox_nine_circles` prints: `NINE CIRCLES GROUND — circles=9 chunks=81 lod=OK veg=OK origin=OK`
- [ ] CI green on dev-win + dev-linux
- [ ] No Vulkan validation errors in any phase of the exit gate

---

## Upcoming Phase: Phase 21 — Hell Frame

*The phase that makes Greywater Engine look and sound like Sacrilege.*

### Week-by-Week

| Week | Deliverable | Tier |
|------|-------------|------|
| 135 | `horror_post.hlsl` — chromatic aberration, film grain, screen shake in single pass | A |
| 136 | Sin tendril vignette shader; Ruin desaturation; Rapture whiteout (post-process CVars) | A |
| 137 | `BloodDecalSystem` — compute stamp + drip; 512-entry ring buffer; `DecalComponent` | A |
| 138 | Circle-specific fog parameters — per-biome ambient, fog colour, visibility range | A |
| 139 | `MartyrdomAudioController` — Sin choir, Rapture thunder, Ruin drone, BPM layer crossfade | A |
| 140 | Diegetic HUD — blood vial, runic Sin ring, Mantra counter in RmlUi | A |
| 141 | Performance gate: RX 580 @ 1080p / 144 FPS with full horror post stack | A |
| 142 | *Hell Frame* exit gate: `sandbox_hell_frame` — Violence Circle, full post, decals, HUD | A |

---

## Upcoming Phase: Phase 22 — Martyrdom Combat & Player Stack

*The phase where Sacrilege becomes playable.*

### Week-by-Week

| Week | Deliverable | Tier |
|------|-------------|------|
| 143 | Martyrdom ECS components: `SinComponent`, `MantraComponent`, `RaptureState`, `RuinState`, `ResolvedStats` | A |
| 144 | `SinAccrualSystem`, `MantraAccrualSystem`, `RaptureCheckSystem`, `RaptureTickSystem`, `RuinTickSystem` | A |
| 145 | `BlasphemySystem` — all 5 Blasphemies; `StatCompositionSystem` | A |
| 146 | Player controller: swept-AABB, bunny hop, slide, wall kick | A |
| 147 | Grapple hook, blood surf, Glory Kill trigger | A |
| 148 | *Martyrdom Online* exit gate: 2-player session, full Martyrdom loop, no desync over 5 min | A |

---

## Upcoming Phase: Phase 23 — Damned Host, Encounters, God Machine

*The phase that makes Sacrilege a game, not a tech demo.*

### Week-by-Week

| Week | Deliverable | Tier |
|------|-------------|------|
| 149 | Enemy archetypes: Cherubim, Deacon, Leviathan, Warden — ECS + BT wiring | A |
| 150 | Enemy archetypes: Martyr, Hell Knight, Painweaver, Abyssal — ECS + BT wiring | A |
| 151 | Gore system: dismemberment, limb entities as nav obstacles, blood decal placement on hit | A |
| 152 | Full weapon roster: all 6 weapons, hitscan/projectile/melee, alt-fires, Sin/Mantra coupling | A |
| 153 | God Machine boss: 4 phases, arena collapse, Martyrdom economy gating | A |
| 154 | *God Machine RC* exit gate: Circle IX (Treachery) end-to-end playable; God Machine defeatable | A |

---

## Phase 24 — Hardening & Release

| Week | Deliverable | Tier |
|------|-------------|------|
| 155–156 | Full regression: 711+ tests green all presets; zero sanitizer warnings | A |
| 157 | WCAG 2.2 AA audit pass; post-effect accessibility toggles all functional | A |
| 158 | Platform builds: Windows installer + Linux AppImage/DEB, signed | A |
| 159 | BLD ships ~60 tools; RAG index shipped as asset; all editor tools functional | A |
| 160 | 4-client co-op session stable 15+ min; voice chat works | B |
| 161 | Engineering handbook, LTS runbook, onboarding guide — all indexable by BLD | A |
| 162 | *Release Candidate* — all 9 Circles completable; God Machine defeatable; 10-min demo | A |

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
