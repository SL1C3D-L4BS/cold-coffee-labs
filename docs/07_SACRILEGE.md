# 07 — Sacrilege (flagship)



---

## Merged from `games/README.md`

# Games

Greywater Engine is the **platform**. Titles consume it through public APIs only — the engine never `#include`s gameplay.

## Flagship: *Sacrilege*

All shipping game vision lives in **`sacrilege/`** (start with `README.md` and `12_SACRILEGE_PROGRAM_SPEC_GW-SAC-001_REV-A.md`).

```
docs/games/
├── README.md          (this file)
└── sacrilege/         flagship title docs
```

## Rule

**Engine code does not reference game docs.** Game docs reference engine docs. See `docs/01_CONSTITUTION_AND_PROGRAM.md` §7.

---

*Sacrilege first. Everything else is future scope.*


---

## Merged from `docs/07_SACRILEGE.mdREADME.md`

# *Sacrilege*

**Studio:** Cold Coffee Labs  
**Engine:** Greywater Engine  
**Role:** Debut title — what the engine ships first.

---

## Canonical spec

| Document | Purpose |
| -------- | ------- |
| `12_SACRILEGE_PROGRAM_SPEC_GW-SAC-001_REV-A.md` | **GW-SAC-001** — engine + Blacklake + game program specification |

Add `13_*`, `14_*`, … as design deepens (combat tuning, circles, roster data, audio bible).

## Related

- **Blacklake (procedural):** `docs/08_BLACKLAKE_AND_RIPPLE_STUDIO_SPECS.md`  
- **Engine + Ripple (studio spec):** `docs/08_BLACKLAKE_AND_RIPPLE_STUDIO_SPECS.md`  
- **Doc map:** `docs/README.md`  

## Engine boundary

Gameplay in `gameplay_module` only. Engine never imports game headers — `docs/01_CONSTITUTION_AND_PROGRAM.md` §7.

---

*Cold Coffee Labs → Greywater Engine → Sacrilege.*


---

## Merged from `docs/07_SACRILEGE.md`

# GREYWATER: SACRILEGE — unified program specification

**⬛ TOP SECRET // COLD COFFEE LABS // GREYWATER-SAC-001 ⬛**

**GREYWATER: SACRILEGE**  
*Unified Architecture, Framework & Game Specification*

**One Engine. One Universe. One Purpose: Absolute Carnage.**

**CLASSIFICATION:** PROPRIETARY CONFIDENTIAL · **PATENT PENDING** · AUTHORIZED PERSONNEL ONLY

---

| Field | Data |
| ----- | ---- |
| Document designation | GW-SAC-001-REV-A |
| Classification | Cold Coffee Labs — Internal Counsel |
| Engine | Greywater Engine (GWE) |
| Framework | Blacklake Framework (BLF) — hosted in-engine |
| Game | *Sacrilege* |
| Languages / API | C++23 · Vulkan 1.3 / GLSL 4.60 target · Rust (BLD) |
| Date of issue | 2026-04-21 |
| Status | **LIVING DOCUMENT** — active development |
| Doc system | Canonical flagship spec under `docs/07_SACRILEGE.md` per `docs/README.md` |

*Unauthorized disclosure or reverse engineering of systems described herein is prohibited.*

**⬛ END COVER BLOCK ⬛**

---

## Abstract

**Greywater: Sacrilege** is a **single unified system**: a custom **Greywater Engine** (C++23 / Vulkan), the **Blacklake** procedural framework it hosts, and the debut title ***Sacrilege*** that defines engine priorities.

- **Greywater Engine** — fiber jobs, archetype ECS, Vulkan frame graph, **BLD** (Rust AI copilot, MCP).  
- **Blacklake** — deterministic pipeline from a **256-bit master seed**: **HEC** (Hierarchical Entropy Cascade), **SDR** (Stratified Domain Resonance) noise, **GPTM** (Greywater Planetary Topology Model), **TEBS** (Tessellated Evolutionary Biology System), adapted for *Sacrilege*’s **Nine Inverted Circles**.  
- ***Sacrilege*** — first-person action; **Martyrdom Engine** (Sin / Mantra, **Blasphemies**, **Rapture & Ruin**); speed-forward movement doctrine; crunchy low-poly horror art.

This document is the **flagship program spec** for Cold Coffee Labs (**supra-AAA** bar: `docs/01_CONSTITUTION_AND_PROGRAM.md` §0.1). Deep mathematical disclosure: `docs/08_BLACKLAKE_AND_RIPPLE_STUDIO_SPECS.md`; engine IDE/Ripple depth: `docs/08_BLACKLAKE_AND_RIPPLE_STUDIO_SPECS.md`. Implementation binding constraints: `docs/01_CONSTITUTION_AND_PROGRAM.md`.

---

## Table of contents

**Part I — Greywater Engine (foundation)**  
1. Architectural philosophy (DOD)  
2. Memory, jobs, ECS  
3. Vulkan rendering (frame graph, forward+, GPU decals / gore)  
4. BLD & scripting (MCP; Ripple Script / RSL; text IR ground truth)

**Part II — Blacklake (universe in the machine)**  
5. Mandate for deterministic worlds  
6. HEC · SDR · GPTM · TEBS (summary — see `08_BLACKLAKE_AND_RIPPLE_STUDIO_SPECS.md`)  
7. Planetary / arena-scale genesis  
8. Life synthesis (TEBS repurposed for hellish biomes)

**Part III — *Sacrilege* (game)**  
9. Martyrdom Engine (Sin, Mantra, Blasphemies, Rapture & Ruin)  
10. Movement — Speed Demon Doctrine  
11. Arsenal (Profane vs Sacred)  
12. Damned Host (enemy roster)  
13. Nine Inverted Circles (levels)  
14. Boss — God Machine  

**Part IV — Art, audio, UI**  
15. Crunchy low-poly horror  
16. Metal-forward dynamic score  
17. Diegetic minimal HUD  

**Part V — Integration & performance**  
18. Sacrilege-specific engine extensions  
19. Performance targets (RX 580 through high-end)

**Part VI — Novel contributions (patent posture)**  
20. Claim summaries (HEC, SDR, GPTM, Martyrdom loop, persistent dismemberment)

---

## Part I — Greywater Engine

### 1. Data-oriented design

All hot paths optimize for **cache locality**, batch iteration, and predictable access. ECS components are POD; systems are pure functions over spans.

### 2. Core runtime

- **Allocators:** linear (per frame), pool (ECS), block (assets), virtual (large streaming buffers). No raw `malloc`/`new` in hot engine paths (per `docs/01_CONSTITUTION_AND_PROGRAM.md`).  
- **Jobs:** fiber-based work stealing; procedural generation, physics, AI, rendering prep submit here.  
- **ECS:** archetype model; SIMD-friendly; high entity counts for crowds, gibs, projectiles.

### 3. Rendering

- **Vulkan** with declarative **frame graph** (barriers, transient memory, async compute).  
- **Forward+** clustered — transparency-friendly for blood, smoke, fire.  
- **GPU-driven decals** — blood stamp, drip; persistent gore readable for gameplay where spec’d.

### 4. BLD & Ripple

- **BLD** — Rust, narrow C-ABI, MCP tools over public engine API; undo stack = commit log.  
- **Ripple Script (RSL)** — typed bytecode VM; visual graph is a **projection** of text IR (same non-negotiable as `vscript` / IR in repo). Enemy and boss logic **targets** RSL; reconcile naming with shipped `vscript` via ADRs.

---

## Part II — Blacklake Framework

Blacklake turns **Σ₀ ∈ ℤ/(2²⁵⁶ℤ)** into deterministic **U = {G, S, P, B, Ω}** (galaxies, systems, planets, biospheres, state). For *Sacrilege*, the **same machinery** generates **circle-scale** arenas and traversal — not only full planets.

| Construct | Role |
| --------- | ---- |
| **HEC** | Hierarchical seed derivation (e.g. SHA3-512 family); domain separation; Hell Seed → layout / encounters / secrets. |
| **SDR** | Phase-coherent Gabor-style noise; coherent terrain / flesh / ice / heresy geometry. |
| **GPTM** | Hybrid heightmap / sparse-volume style topology; LOD handoff for huge arenas. |
| **TEBS** | L-systems + evolutionary flavor; repurposed for **biomechanical** hell life, burrows, swarms. |

**Authoritative math:** `docs/08_BLACKLAKE_AND_RIPPLE_STUDIO_SPECS.md`.

---

## Part III — *Sacrilege* — gameplay

### 5. Martyrdom Engine

**Sin** (0–100): accrues from **Profane** weapons and certain kills / zone effects. Visual + audio pressure intensify with Sin.

**Blasphemies** — spend Sin for timed powers (examples):

| Name | Sin | Duration | Effect (summary) |
| ---- | --- | -------- | ---------------- |
| Stigmata | 25 | 5s | Reflect damage to attackers |
| Crown of Thorns | 40 | 8s | Orbiting thorns shred on contact |
| Wrath of God | 60 | instant | Large pillar blast |
| False Idol | 30 | 10s | Decoy + taunt → explodes |
| Unholy Communion | 50 | 12s | Lifesteal; part of damage taken → temp shield |

**Rapture & Ruin**

- At **Sin = 100**, auto-enter **Rapture** (~6s): massive move speed, damage, accuracy, piercing; blinding divine read.  
- Then **Ruin** (~12s): no regen, slowed, heavily reduced damage; greyed, vulnerable read.

**Mantra** — parallel resource for **Sacred** weapons and abilities (build via Sacred kills / parries / skillful play per tuning).

### 6. Movement — Speed Demon Doctrine

Bunny hop / air control, slide, wall kick (chain rules), grapple (tap pull / hold glory-kill pull), **blood surf** on slopes / pools. Swept-AABB / engine controller extensions as in `00` / future ADRs.

### 7. Arsenal (sketch)

| Weapon | Class | Notes |
| ------ | ----- | ----- |
| Halo-Scythe | Sacred / melee | Mantra spend on alt |
| Dogma | Profane / SMG | Sin builder |
| Heretic's Lament | Sacred / rail | Mantra overcharge |
| Confessor | Profane / shotgun | Sin builder |
| Schism | Profane / sawblade | Recall path |
| Cathedral | Sacred / stakes + dome | Large Mantra spend |

### 8. Damned Host (examples)

Cherubim, Deacon, Leviathan, Warden, Martyr, Hell Knight, Painweaver, Abyssal — each tuned vs Martyrdom loop (Sin on kill, Mantra opportunities, counter-moves).

### 9. Nine Inverted Circles

Procedural **base geography** per circle + designer-authored encounters. Examples: Limbo (inverted), Lust (frozen), Gluttony (gut), Greed (forge), Wrath (battlefield), Heresy (library), Violence (arena), Fraud (maze), Treachery (abyss / zero-G gauntlet).

### 10. Final boss — God Machine

Multi-phase; Sin / Mantra economy feeds survival; arena collapse choreography.

---

## Part IV — Art, audio, UI

- **Art:** crunchy low-poly horror; small textures; optional nearest filtering; high contrast; heavy post (CA, grain, shake — accessibility toggles).  
- **Audio:** layered metal score driven by combat intensity / combo; VO for Apostate; dense enemy death reads.  
- **UI:** diegetic health / Sin / Mantra; **no** minimap / objective markers (design intent).

---

## Part V — Engine extensions & targets

| Subsystem | *Sacrilege* focus |
| --------- | ------------------ |
| Physics | Fast player controller; gib entities |
| Rendering | Forward+, decal compute, horror post |
| ECS | Buff/debuff / Martyrdom state |
| Audio | Layer crossfade / BPM sync |
| Animation | Frame-accurate melee / dismember events |

**FPS targets (intent — gate per phase budgets):**

| Hardware | Resolution | Avg FPS | 1% low |
| -------- | ---------- | ------- | ------ |
| High-end | 4K | 240+ | 144 |
| Mid | 1440p | 200+ | 120 |
| Baseline RX 580 | 1080p | 144+ | 60 |
| Steam Deck | ~800p | 60+ | 45 |

Reconcile with `docs/09_NETCODE_DETERMINISM_PERF.md` (merged Phase 12–17 performance budget sections) and `docs/01_CONSTITUTION_AND_PROGRAM.md` RX 580 contract.

---

## Part VI — Novel contributions (summary)

Patent posture covers among others: **HEC** seeding; **SDR** noise; **GPTM** representation; **Martyrdom** dual-resource / Rapture-Ruin loop; **persistent dismemberment** affecting navigation. Legal maintains full claim language; this section is **not** legal advice.

---

## Conclusion

**Cold Coffee Labs** presents **Greywater Engine**. Greywater Engine presents ***Sacrilege***. Blacklake supplies the hells-cape. BLD supplies the authoring velocity. **Now we build.**

---

***Cold Coffee Labs · April 21, 2026 · GW-SAC-001-REV-A***

**⬛ END OF DOCUMENT ⬛**
