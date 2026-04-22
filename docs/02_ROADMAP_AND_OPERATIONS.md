# 02 — Roadmap & Operations

**Status:** Operational — updated as phases complete  
**Horizon:** 162 weeks · ~37 months · 25 phases (24 build + LTS)  
**Start:** 2026-04-20 (Phase 1, week 001) · **Planned RC:** 2029-05-28 (Phase 24, week 162)

> **The phase is the contract. The week is the budget.**

This document governs the schedule and day-to-day workflow for Greywater Engine + *Sacrilege*. Phases 1–18 build the engine and general-purpose subsystems. Phases 19–23 build Blacklake + *Sacrilege*-facing systems. Phase 24 hardens and ships. Phase 25 sustains.

---

## Phase Overview

| Phase | Name | Weeks | Duration | Milestone | Status |
|-------|------|-------|----------|-----------|--------|
| 1 | Scaffolding & CI/CD | 001–004 | 4w | *First Light* | ✅ Complete |
| 2 | Platform & Core Utilities | 005–009 | 5w | — | ✅ Complete |
| 3 | Math, ECS, Jobs & Reflection | 010–016 | 7w | *Foundations Set* | ✅ Complete (pulled fwd through Ph7) |
| 4 | Renderer HAL & Vulkan Bootstrap | 017–024 | 8w | — | ✅ Complete |
| 5 | Frame Graph & Reference Renderer | 025–032 | 8w | *Foundation Renderer* | ✅ Complete |
| 6 | Asset Pipeline & Content Cook | 033–036 | 4w | — | ✅ Complete |
| 7 | Editor Foundation | 037–042 | 6w | *Editor v0.1* | ✅ Complete (demo gate pending) |
| 8 | Scene Serialization & Visual Scripting | 043–047 | 5w | — | ✅ Complete |
| 9 | BLD (Brewed Logic Directive, Rust) | 048–059 | 12w | *Brewed Logic* | ✅ Complete |
| 10 | Runtime I — Audio & Input | 060–065 | 6w | — | ✅ Complete |
| 11 | Runtime II — UI, Events, Config, Console | 066–071 | 6w | *Playable Runtime* | ✅ Complete (271/271 tests) |
| 12 | Physics | 072–077 | 6w | — | ✅ Complete |
| 13 | Animation & Game AI Framework | 078–083 | 6w | *Living Scene* | ✅ Complete (347/347 tests) |
| 14 | Networking & Multiplayer | 084–095 | 12w | *Two Client Night* | ✅ Complete (394/394 tests) |
| 15 | Persistence & Telemetry | 096–101 | 6w | — | ✅ Complete (473/473 tests) |
| 16 | Platform Services, i18n & a11y | 102–107 | 6w | *Ship Ready* | ✅ Complete (586/586 tests) |
| 17 | Shader Pipeline, Materials & VFX | 108–113 | 6w | *Studio Renderer* | ✅ Complete (711/711 tests) |
| 18 | Cinematics & Mods | 114–118 | 5w | *Studio Ready* | ✅ Complete (18-C: `mod_api` / `ModRegistry`, `example_mod`, `sandbox_studio`, BLD seq gate) |
| 19 | **Blacklake Core & Deterministic Genesis** | 119–126 | 8w | *Infinite Seed* | 🔜 Planned |
| 20 | **Arena Topology, GPTM & Sacrilege LOD** | 127–134 | 8w | *Nine Circles Ground* | 🔜 Planned |
| 21 | **Sacrilege Frame — Rendering, Atmosphere, Audio** | 135–142 | 8w | *Hell Frame* | 🔜 Planned |
| 22 | **Martyrdom Combat & Player Stack** | 143–148 | 6w | *Martyrdom Online* | 🔜 Planned |
| 23 | **Damned Host, Circles Pipeline & Boss** | 149–154 | 6w | *God Machine RC* | 🔜 Planned |
| 24 | Hardening & Release | 155–162 | 8w | *Release Candidate* | 🔜 Planned |
| 25 | LTS Sustenance | 163–∞ | Ongoing | Quarterly LTS cuts | 🔜 Ongoing |

---

## Week-by-Week Schedule (Active Phases)

### Phase 18 — Cinematics & Mods (weeks 114–118)

| Week | Deliverable | Tier |
|------|-------------|------|
| 114 | `.gwseq` sequencer format + timeline ECS integration | A |
| 115 | Cinematic camera system — splines, cuts, blend modes | A |
| 116 | Mod sandbox: `gameplay_module` hot-reload as mod surface | B |
| 117 | Mod API documentation + BLD tool surface for sequencer | B |
| 118 | *Studio Ready* demo: cinematic plays + mod loads cleanly | A |

### Phase 19 — Blacklake Core & Deterministic Genesis (weeks 119–126)

| Week | Deliverable | Tier |
|------|-------------|------|
| 119 | 256-bit universe seed + HEC protocol implementation | A |
| 120 | SDR Noise primitive — Gabor wavelet superposition | A |
| 121 | Hierarchical galaxy → system → planet seed derivation | A |
| 122 | Chunk streaming foundation: 32 m chunks, LRU, time-sliced | A |
| 123 | Biome classification: temperature + humidity + soil parameters | A |
| 124 | Resource distribution: geological-rule placement from seed | A |
| 125 | Determinism validation suite: hash cross-platform | A |
| 126 | *Infinite Seed* milestone: arena gen from Hell Seed, deterministic | A |

### Phase 20 — Arena Topology, GPTM & Sacrilege LOD (weeks 127–134)

| Week | Deliverable | Tier |
|------|-------------|------|
| 127 | GPTM quad-sphere topology model implementation | A |
| 128 | Heightmap + sparse voxel octree hybrid | A |
| 129 | LOD handoff shader: seamless transition at clipmap boundary | A |
| 130 | Nine Circles base geography generation from SDR | A |
| 131 | Floating origin system: recentering at configurable threshold | A |
| 132 | Vegetation instancing: GPU indirect draw, 4 LOD levels | A |
| 133 | Circle-specific SDR parameter tables (Limbo → Treachery) | A |
| 134 | *Nine Circles Ground* milestone: all 9 circles generate, LOD correct | A |

### Phase 21 — Sacrilege Frame (weeks 135–142)

| Week | Deliverable | Tier |
|------|-------------|------|
| 135 | Atmospheric scattering: physically-based raymarched single-pass | A |
| 136 | Horror post stack: chromatic aberration, film grain, screen shake | A |
| 137 | GPU-driven blood decal system: stamp + drip compute shaders | A |
| 138 | Dynamic BPM-sync audio layer crossfade (Combo Meter hook) | A |
| 139 | Sin/Mantra visual feedback: red tendril shader, choir mix | A |
| 140 | Diegetic HUD: blood vial health, runic Sin meter, Mantra counter | A |
| 141 | Performance gate: RX 580 @ 1080p / 144 FPS with full horror post | A |
| 142 | *Hell Frame* milestone: playable frame with full Sacrilege aesthetic | A |

### Phase 22 — Martyrdom Combat & Player Stack (weeks 143–148)

| Week | Deliverable | Tier |
|------|-------------|------|
| 143 | ECS components: SinComponent, MantraComponent, RaptureState, RuinState, StatModifier | A |
| 144 | Martyrdom systems: Sin accrual, Blasphemy expenditure, Rapture auto-trigger at 100 | A |
| 145 | Ruin aftermath system: 12s debuff, stat modifier application | A |
| 146 | Player controller: custom swept-AABB; bunny hop, slide, wall kick | A |
| 147 | Grapple hook + blood surf mechanics | A |
| 148 | *Martyrdom Online* milestone: 2-player Martyrdom loop with netcode | A |

### Phase 23 — Damned Host, Circles Pipeline & Boss (weeks 149–154)

| Week | Deliverable | Tier |
|------|-------------|------|
| 149 | Enemy ECS archetypes: Cherubim, Deacon, Leviathan, Warden (behavior trees) | A |
| 150 | Enemy ECS archetypes: Martyr, Hell Knight, Painweaver, Abyssal | A |
| 151 | Persistent dismemberment: limb entities, simplified collision, nav interaction | A |
| 152 | Full weapon roster: all 6 weapons, alt-fires, Sin/Mantra coupling | A |
| 153 | God Machine boss: 4-phase fight, arena collapse, Martyrdom economy | A |
| 154 | *God Machine RC* milestone: full Circle 9 (Treachery) playable end-to-end | A |

### Phase 24 — Hardening & Release (weeks 155–162)

| Week | Deliverable | Tier |
|------|-------------|------|
| 155–156 | Full regression suite: 711+ tests green on all presets | A |
| 157 | WCAG 2.2 AA audit pass; all a11y toggles functional | A |
| 158 | Platform certification builds: Windows installer + Linux AppImage/DEB | A |
| 159 | BLD ships with ~60 tools; RAG index shipped as asset | A |
| 160 | Sponza at 144 FPS @ 1440p; multiplayer sandbox 4-client OK | A |
| 161 | Engineering handbook, onboarding guide, LTS runbook indexable by BLD | A |
| 162 | *Release Candidate* — 10-minute board demo, no intervention | A |

---

## Kanban Operations

### Board Columns

```
Backlog → Ready → In Progress (WIP ≤ 2) → Review → Done
```

- **WIP limit of 2** is a hard rule, not a guideline. Never exceed it.
- A card stays In Progress until CI is green and review is passing.
- Blocked cards move to Blocked with a written reason.

### Card Format

```markdown
## [PHASE-WEEK-NN] Brief title

**Tier:** A / B / C
**Phase:** N
**Week:** NNN
**ADR:** NNNN (if architectural decision required)

### Context
One paragraph: what this builds on and why it exists.

### Acceptance criteria
- [ ] Criterion 1 (concrete, testable)
- [ ] Criterion 2
- [ ] CI green on dev-linux + dev-win
- [ ] Perf gate: [specific budget from 09_NETCODE_DETERMINISM_PERF.md]
```

### Rituals

| Ritual | Cadence | Duration | Rule |
|--------|---------|----------|------|
| Daily focus | Each working day | 90 min × 1–2 blocks | One card. No multitasking. |
| Commit | End of each focus block | 5 min | Small commits. Conventional Commits format. |
| Weekly retro | Sunday | 30 min | Read 7 daily logs. No coding during retro. |
| Phase gate review | End of each phase | 60 min | Demo the milestone. Update `docs/`. Write phase closeout note. |

### Anti-Patterns (These block the merge)

| Anti-pattern | Why it's blocked |
|--------------|-----------------|
| WIP > 2 cards | Splitting focus guarantees both fail |
| Skipping the cold-start checklist | You will code the wrong thing |
| "I'll write the tests later" | The phase gate requires CI green. Later never comes. |
| Direct commits to `main` | CI must gate every change. Branch + PR always. |
| ADR written after the code | Decisions made under implementation pressure are bad decisions. ADR first. |
| Sunday coding | Rest days are load-bearing. Burnout is the #1 project risk. |

---

## Milestone Definitions

| Milestone | Required deliverables | Demo |
|-----------|----------------------|------|
| *First Light* (Ph 1) | Hello-triangle sandbox + hello-BLD C-ABI smoke test | Recorded 2-min video |
| *Foundations Set* (Ph 3) | ECS World, CommandStack, Job scheduler, serialization — 101/101 tests | Entities + undo in editor |
| *Foundation Renderer* (Ph 5) | Frame graph + Forward+ reference; Sponza @ 60 FPS on RX 580 | Recorded scene |
| *Editor v0.1* (Ph 7) | Docking panels, scene hierarchy, vscript node graph, BLD chat panel | 5-min walkthrough |
| *Brewed Logic* (Ph 9) | BLD full 9A–9F waves; MCP tools; RAG; local inference | BLD authors vscript IR live |
| *Playable Runtime* (Ph 11) | RmlUi HUD, event system, console, config, audio — runtime ships | Sandbox plays audio + UI |
| *Living Scene* (Ph 13) | Animation blend trees + behavior trees + navmesh | 100 animated characters pathfind |
| *Two Client Night* (Ph 14) | GNS transport + ECS replication + 2-client integration | 2 clients sync 60 s |
| *Ship Ready* (Ph 16) | WCAG 2.2 AA, 5 locales, Steamworks + EOS integration | Localized menu + a11y audit |
| *Studio Renderer* (Ph 17) | Full VFX stack, PBR Neutral tone, bloom/TAA/DoF/CA/grain | Studio render of Sacrilege Circle |
| *Studio Ready* (Ph 18) | `.gwseq` playback, `sandbox_studio` exit gate, mod API + `example_mod` + BLD seq C-ABI | `sandbox_studio` prints STUDIO READY |
| *Infinite Seed* (Ph 19) | HEC + SDR + arena gen from Hell Seed — deterministic | Same seed = same Circle, two machines |
| *Nine Circles Ground* (Ph 20) | All 9 Circles generate, GPTM LOD correct | Circle flythrough at 144 FPS |
| *Hell Frame* (Ph 21) | Full Sacrilege aesthetic + 144 FPS on RX 580 | Gameplay video |
| *Martyrdom Online* (Ph 22) | 2-player combat, full Martyrdom loop, netcode | 2-player session, no desync |
| *God Machine RC* (Ph 23) | Full Circle 9 end-to-end, God Machine boss | Complete playthrough |
| *Release Candidate* (Ph 24) | All above + certification + docs | 10-min board demo, no intervention |

---

*The phase is the contract. The week is the budget. The milestone is the proof.*
