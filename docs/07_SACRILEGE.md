# 07 — *Sacrilege* — Unified Program Specification GW-SAC-001

**⬛ TOP SECRET // COLD COFFEE LABS // GREYWATER-SAC-001 ⬛**

**One Engine. One Universe. One Purpose: Absolute Carnage.**

**Classification:** Proprietary Confidential · Patent Pending · Authorized Personnel Only  
**Precedence:** L4 — flagship product mandate; overrides L5–L7 on game-design decisions  
**Status:** Living Document — Active Development

| Field | Data |
|-------|------|
| Document Designation | GW-SAC-001-REV-A |
| Engine | Greywater Engine (GWE) |
| Framework | Blacklake Framework (BLF) |
| Languages / API | C++23 · Vulkan 1.3 / HLSL · Rust (BLD) |
| Date of Issue | 2026-04-21 |
| Supersedes | SAC-001-REV-B, BLF-CCL-001-REV-A |

*Unauthorized disclosure or reverse engineering is prohibited.*

---

## Abstract

**Greywater: Sacrilege** is a single unified system: a custom Greywater Engine, the Blacklake procedural framework it hosts, and the debut title *Sacrilege* that defines engine priorities.

- **Greywater Engine** — fiber jobs, archetype ECS, Vulkan frame graph, BLD (Rust AI copilot, MCP).
- **Blacklake** — deterministic pipeline from a 256-bit master seed: HEC (Hierarchical Entropy Cascade), SDR (Stratified Domain Resonance) noise, GPTM (Greywater Planetary Topology Model), TEBS (Tessellated Evolutionary Biology System), adapted for *Sacrilege*'s Nine Inverted Circles.
- ***Sacrilege*** — first-person action; **Martyrdom Engine** (Sin / Mantra, Blasphemies, Rapture & Ruin); speed-forward movement doctrine; crunchy low-poly horror art.

Supra-AAA bar: `01_CONSTITUTION_AND_PROGRAM.md §0.1`. Deep mathematical disclosure: `08_BLACKLAKE_AND_RIPPLE.md`. Implementation constraints: `01_CONSTITUTION_AND_PROGRAM.md`.

---

## Table of Contents

**Part I — Greywater Engine (foundation)**
1. Architectural philosophy (DOD)
2. Memory, jobs, ECS
3. Vulkan rendering — frame graph, Forward+, GPU decals / gore
4. BLD & Ripple — MCP, typed text IR, visual scripting

**Part II — Blacklake Framework (universe in the machine)**
5. Mandate for deterministic worlds
6. HEC · SDR · GPTM · TEBS (summary — full math in `08_BLACKLAKE_AND_RIPPLE.md`)
7. Planetary / arena-scale genesis
8. Life synthesis — TEBS repurposed for hellish biomes

**Part III — *Sacrilege* (the game)**
9. Martyrdom Engine — Sin, Mantra, Blasphemies, Rapture & Ruin
10. Movement — Speed Demon Doctrine
11. Arsenal — Profane vs Sacred
12. Damned Host — enemy roster
13. Nine Inverted Circles — levels
14. Boss — God Machine

**Part IV — Art, audio, UI**
15. Crunchy low-poly horror
16. Metal-forward dynamic score
17. Diegetic minimal HUD

**Part V — Integration & performance**
18. Sacrilege-specific engine extensions
19. Performance targets

**Part VI — Novel contributions (patent posture)**
20. Claim summaries

---

## Part I — Greywater Engine

### 1. Data-Oriented Design

All hot paths optimize for **cache locality**, batch iteration, and predictable memory access. ECS components are POD structs. Systems are pure functions over component spans. When a pattern improves OOP ergonomics at the cost of cache locality, we reject the pattern.

Cache-miss latency is the only budget that matters in the hot path.

### 2. Core Runtime

**Allocators:** linear (per frame, O(1) bump-pointer, reset each frame), pool (ECS components, render commands), block (assets, mesh data), virtual (large streaming buffers — arena heightmaps, planet terrain). No raw `malloc`/`new` in hot engine paths.

**Jobs:** Fiber-based work-stealing scheduler. All concurrent work — physics, AI, procedural generation, rendering prep — submits here. No `std::async`, no raw `std::thread`.

**ECS:** Archetype-based. Entities are generational handles (64-bit: index 32 + generation 32). Components are POD. Systems iterate contiguous chunk arrays. This is the architectural heart of handling hundreds of enemies, thousands of gibs, and dense particle systems simultaneously.

### 3. Rendering

**Vulkan** with declarative **frame graph** — automatic barrier insertion, transient memory aliasing, async compute scheduling. Every frame is a composed DAG of render passes with dependencies declared explicitly.

**Forward+ Clustered** — transparency-friendly; essential for *Sacrilege*'s dense blood, smoke, and fire effects.

**GPU-driven blood decal system** — compute shader stamps decals onto world geometry. Second compute pass simulates drip and surface accumulation. Both run without CPU intervention. Persistent for the session lifetime via ring-buffer pool managed by `DecalComponent`.

### 4. BLD & Ripple

**BLD** — Rust, narrow C-ABI, MCP 2025-11-25, tool surface over public engine API. Editor undo stack = BLD commit log. Every BLD action is reversible.

**Ripple Script (RSL)** — the engine's typed text IR for visual scripting. The node graph is a **projection** of the IR, not the ground truth. Enemy behavior trees and boss phase logic target RSL. Reconcile naming with shipped `vscript` via ADRs.

---

## Part II — Blacklake Framework

Blacklake turns **Σ₀ ∈ ℤ/(2²⁵⁶ℤ)** into a deterministic universe **U = {G, S, P, B, Ω}** (galaxies, systems, planets, biospheres, state). For *Sacrilege*, the same machinery generates **circle-scale arenas** — not full planets, but using the same seeding, noise, and topology stack.

Full mathematical specification: `08_BLACKLAKE_AND_RIPPLE.md`.

| Construct | Role in *Sacrilege* |
|-----------|---------------------|
| **HEC** | SHA3-512 hierarchical seed derivation; Hell Seed → layout / encounters / secrets. Players share seeds to race identical level instances. |
| **SDR** | Phase-coherent Gabor-wavelet noise; generates organic flesh tunnels (Gluttony), ice formations (Lust), non-Euclidean architecture (Heresy). |
| **GPTM** | Hybrid heightmap / sparse-voxel-octree; seamless LOD for massive arena volumes. |
| **TEBS** | L-systems + evolutionary algorithm; repurposed for biomechanical hell life — Leviathan burrows, Cherubim swarms, pulsating organic walls. |

---

## Part III — *Sacrilege* — Gameplay

### 5. Martyrdom Engine

The Martyrdom Engine is *Sacrilege*'s defining mechanic. It forces the player to constantly balance accumulation of power with the risk of self-destruction. The involuntary Rapture trigger is the critical design choice — it removes the "safe" option of hoarding Sin forever.

#### 5.1 Sin

**Sin** (0–100): accrues from **Profane** weapons and certain zone effects. Visual feedback intensifies with Sin — hellish red tendrils invade the player's vision, a discordant choir rises. At **Sin = 100**, the player **automatically** enters Rapture. There is no button press. There is no opt-out.

#### 5.2 Blasphemies — Spending Sin

| Name | Sin Cost | Duration | Effect |
|------|----------|----------|--------|
| **Stigmata** | 25 | 5s | All damage taken is reflected back at the attacker. Pairs devastating with Rapture's 3× damage multiplier. |
| **Crown of Thorns** | 40 | 8s | A ring of ethereal thorns orbits the player, shredding any enemy that enters melee range. |
| **Wrath of God** | 60 | Instant | A targeted pillar of unholy light incinerates everything in its radius. High skill-cap — targets cursor position. |
| **False Idol** | 30 | 10s | A stationary player decoy taunts enemies, then explodes on timer or death. |
| **Unholy Communion** | 50 | 12s | Lifesteal active. 50% of damage taken converts to a temporary overshield. |

#### 5.3 Rapture & Ruin — The High-Stakes Core

| State | Duration | Effects |
|-------|----------|---------|
| **Rapture** | 6 seconds | 3× movement speed · 3× damage · perfect accuracy · piercing rounds · screen floods with blinding divine light |
| **Ruin** (aftermath) | 12 seconds | No health regeneration · 50% movement speed · 75% reduced damage · chilling silence · grey-muted visuals |

**Ruin lasts twice as long as Rapture. This asymmetry is non-negotiable.** The penalty must be severe enough that Rapture feels earned, not free.

#### 5.4 Mantra — The Parallel Resource

**Mantra** accrues from Sacred weapon kills, parries, and skillful play. It fuels Sacred weapon alt-fires. It is the counterweight to Sin — careful, precise play versus aggressive, high-pressure play. Players who balance both resources optimally achieve peak Martyrdom efficiency.

#### 5.5 ECS Representation

The Martyrdom Engine maps cleanly onto the archetype ECS as pure POD components:

```cpp
// All components are plain data — no virtual functions, no inheritance
struct SinComponent    { float value; float max; float accrual_rate; };
struct MantraComponent { float value; float max; };
struct RaptureState    { float time_remaining; bool active; };
struct RuinState       { float time_remaining; bool active; };
struct StatModifier    { float damage_mult; float speed_mult;
                         float accuracy_mult; float lifesteal; };
struct BlasphemyCooldown { BlasphemyType type; float remaining; };
```

Systems: `SinAccrualSystem` (Profane weapon hits → Sin delta), `BlasphemySystem` (input → Sin expenditure), `RaptureSystem` (Sin == max → trigger Rapture, reset Sin), `RuinSystem` (Rapture expiry → apply Ruin debuffs), `StatModifierSystem` (compose all active modifiers into final stats per entity).

All systems are **pure functions over component spans** — deterministic, rollback-safe, unit-testable in isolation.

---

### 6. Movement — Speed Demon Doctrine

Movement is a weapon. The player controller uses a **custom swept-AABB implementation** bypassing Jolt Physics for Quake-like precision and determinism. Same inputs → same trajectory across all clients. Required for rollback netcode.

| Technique | Description | Mechanical Role |
|-----------|-------------|-----------------|
| **Bunny Hopping** | Strafe-jump to maintain and build speed; air strafing preserves momentum | Core traversal; skill expression floor |
| **Slide** | Crouch while sprinting preserves all momentum; lowers profile | Evasion; entering tight spaces at speed |
| **Wall Kick** | Jump off any wall surface for height and direction; chains once | Verticality; escaping encirclement |
| **Grapple Hook** | Tap: pull self to surface or enemy. Hold: pull smaller enemy → Glory Kill | Aggression; cross-arena repositioning |
| **Blood Surf** | Crouch on slopes or blood pools; no momentum loss | Environmental exploit; Ruin escape tool |

---

### 7. Arsenal — Instruments of Blasphemy

All weapons: **no reloads**. All weapons: **alt-fires**. Weapons divide into Profane (builds Sin on damage) and Sacred (consumes or generates Mantra).

| Weapon | Class | Primary Fire | Alt-Fire | Resource |
|--------|-------|-------------|---------|---------|
| **Halo-Scythe** | Sacred / melee | Combo; builds Mantra on kill | Mantra Blast (25 Mantra): disintegration wave | Mantra |
| **Dogma** (×2) | Profane / SMG dual | Dual SMGs; builds Sin on hit | Suppressing Fire: 3s burst, 40s cooldown | Sin gen |
| **Heretic's Lament** | Sacred / railgun | Piercing shot; high damage, slow rate | Overcharge (10 Mantra/shot): kills restore health | Mantra |
| **Confessor** | Profane / shotgun | Shrapnel bounce | Censer Bomb: toxic smoke cloud builds Sin per enemy inside | Sin gen |
| **Schism** | Profane / sawblade | Launches spinning blade; ricochets | Recall: blade reverses; bonus Sin on return hits | Sin gen |
| **Cathedral** | Sacred / stakes | Explosive stakes pin enemies to surfaces | Sanctuary (75 Mantra): protective dome for 5s | Mantra |

---

### 8. Damned Host — Enemy Roster

Every enemy is designed to challenge specific elements of the Martyrdom loop and movement system.

| Enemy | Archetype | Signature Mechanic | Sin on Kill | Counter |
|-------|-----------|-------------------|-------------|---------|
| **Cherubim** | Strafer | Hover + twin machine guns; swarms via TEBS flocking | 5 | AOE Blasphemies; Rapture for mass clear |
| **Deacon** | Teleporter | Warps behind player when unseen; parry window → Mantra | 15 | Situational awareness; parry timing |
| **Leviathan** | Charger/Environmental | Burrows through walls via L-system paths; can be baited into self-stun | 20 | Bait into walls; exploit stun window |
| **Warden** | Summoner/Tank | Shielded until all summoned Cherubim die | 50 | Kill adds first; chain Blasphemies |
| **Martyr** | Charger | Explodes on death; explosion builds Sin | 10 | Grapple → Glory Kill to skip explosion |
| **Hell Knight** | Berserker | Melee swipes drain Mantra from player | 25 | Maintain range; Sacred weapons only |
| **Painweaver** | Grenadier | Web projectiles slow player + pause Sin generation | 30 | Immediate priority target; mobility |
| **Abyssal** | Glass Cannon | Homing projectiles shootable for Mantra | 15 | Intercept shots → Mantra economy |

---

### 9. Nine Inverted Circles — Levels

Blacklake generates the **base geography** for nine distinct, non-linear levels from a Hell Seed. Designers hand-author combat encounters, secrets, and mechanic introductions on top of the procedural foundation. No two playthroughs have identical enemy placement or secret locations.

| Circle | Environment | SDR Profile | Mechanic Introduced |
|--------|-------------|-------------|---------------------|
| **I — Limbo (Inverted)** | Floating islands, perpetual twilight | Low-freq vertical displacement | Wall Kick + Slide |
| **II — Lust (Frozen)** | Ice caverns, slippery surfaces | High-freq directional shear | Blood Surf |
| **III — Gluttony (Gut)** | Organic flesh tunnels, hazardous fluids | Sinuous low-poly organic forms | Cathedral staking / pinning |
| **IV — Greed (Forge)** | Industrial foundry, crushers, molten metal | Rectilinear industrial noise | Grapple Hook |
| **V — Wrath (Battlefield)** | Perpetual warzone, trenches, explosions | Cratered-terrain disruption | Dual-Wielding (Dogma) |
| **VI — Heresy (Library)** | Twisted impossible architecture, shifting walls | Non-Euclidean angular displacement | Parry |
| **VII — Violence (Arena)** | Gladiatorial pits, spectator masses | Clean amphitheatre topology | Sacrilege Mode |
| **VIII — Fraud (Maze)** | Deceptive geometry, false walls, mirrors | High-freq noise with false floors | Heretic's Lament health-as-ammo |
| **IX — Treachery (Abyss)** | Zero-gravity void over frozen lake | Sparse asteroid field distribution | Full mechanic gauntlet |

---

### 10. The God Machine — Final Boss

A colossal clockwork angel with four rotating faces. Arena: a collapsing cathedral — structural sections fall over time, reducing safe ground. Players must manage Sin and Mantra simultaneously across four phases.

| Phase | Face Active | Gimmick | Sin Reward |
|-------|-------------|---------|------------|
| **1 — Adoration** | Face of Mercy | Minion swarms; arena intact | 5–15 per kill |
| **2 — Doubt** | Face of Judgment | First floor collapse; projectile walls | 10–20 per kill |
| **3 — Heresy** | Face of Wrath | Two faces active simultaneously; homing artillery | 20–30 per kill |
| **4 — Revelation** | All four faces | Full arena collapse; Rapture + Ruin cycling; final window | 50 on final hit |

---

## Part IV — Art, Audio, UI

### 11. Visual Style — Crunchy Low-Poly Horror

| Property | *Sacrilege* |
|----------|-------------|
| Polygon density | PS1-era hard edges; geometry is the horror |
| Shading | High-contrast dynamic lighting; flat shading default |
| Texture resolution | 128×128 or 256×256; `GL_NEAREST` filtering optional |
| Colour palette | Reds, blacks, whites, sickly greens; high contrast throughout |
| Post-processing | Chromatic aberration, film grain, screen shake 0–200% (adjustable) |
| Mood driver | Lighting contrast + particle density + audio sync |

### 12. Sound Design — Metal & Mayhem

- **Dynamic music:** Layered metal soundtrack driven by the Combo Meter. Builds from isolated drums to full orchestra as the player chains kills. BPM-synchronized with the animation system for precise hit-feel.
- **Martyrdom audio:** Choir rises with Sin. Distorts at Rapture threshold. Drops to near-silence in Ruin.
- **Voice acting:** The Apostate delivers impact one-liners on kill chains.
- **Enemy SFX:** 12+ unique death screams per enemy type, heavily compressed for physical impact.

### 13. User Interface — Diegetic & Minimal

No minimap. No objective markers. Navigation is environmental — the Nine Circles are designed to funnel flow without UI assistance.

| Element | Presentation | Notes |
|---------|-------------|-------|
| Health | Blood-filled vial (wrist-mount or weapon-body) | Diegetic; panic-readable at a glance |
| Sin Meter | Glowing runic icon near crosshair; tendrils at 70+ | Cannot be hidden; always peripheral |
| Mantra | Minimal numeric counter below weapon name | Non-intrusive |
| Ammo | None — no reloads in *Sacrilege* | Zero UI surface |
| Enemy health | HP bar on lock-on / boss encounters only | Hidden on standard enemies |
| Minimap | Not present | Intentional — spatial memory |
| Objectives | Not present | Environmental design handles flow |

---

## Part V — Engine Extensions & Performance

### 14. Sacrilege-Specific Engine Extensions

| Subsystem | *Sacrilege* Extension |
|-----------|----------------------|
| Physics | Custom swept-AABB player controller bypassing Jolt for Quake-like determinism |
| Rendering | Forward+ + GPU-driven decal system (blood stamp, drip compute shaders) |
| ECS | `StatModifierSystem` for Martyrdom buff/debuff stacking |
| Audio | BPM-synchronized dynamic music mixer with real-time layer crossfading |
| Animation | Single-frame events for precise melee hit detection and dismemberment triggers |

### 15. Performance Targets

**RX 580 targets 144 FPS for *Sacrilege*.** This is justified by lower world complexity than a full Blacklake planetary simulation — bounded arena volumes, no seamless orbit transition, scene complexity dominated by character and particle count rather than terrain draw calls.

| Hardware | Resolution | Avg FPS | 1% Low |
|----------|-----------|---------|--------|
| RX 580 8 GB (Baseline) | 1080p | 144+ | 60 |
| RX 6700 / RTX 3060 | 1440p | 200+ | 120 |
| RX 7900 XT / RTX 4080 | 4K | 240+ | 144 |
| Steam Deck | 800p | 60+ | 45 |

**Frame budget at 1080p / 144 FPS on RX 580 (6.94 ms total):**

| System | Budget | Notes |
|--------|--------|-------|
| Game logic (ECS, Martyrdom, AI) | 1.0 ms | Pure CPU; parallelised via job system |
| Physics (swept-AABB controller + Jolt ragdolls) | 0.8 ms | Custom controller bypasses Jolt hot path |
| Render prep (culling, batch building, draw cmd fill) | 0.5 ms | GPU-driven; CPU cost is barrier submission |
| GPU frame (geometry, decals, post-process) | 3.5 ms | Primary constraint; Forward+ forward rendering |
| Audio (miniaudio + Steam Audio HRTF) | 0.4 ms | Async thread; budget is fence cost |
| Headroom / OS / driver | 0.74 ms | Never borrow; absorbs spikes |

Reconcile with `09_NETCODE_DETERMINISM_PERF.md` for physics and animation phase budgets.

---

## Part VI — Novel Contributions (Patent Posture)

*These claims are for internal record. Formal patent applications require independent legal counsel and prior-art searches before filing.*

**Claim 1 — Hierarchical Entropy Cascade (HEC) Seeding Protocol.**  
A method for deterministic, hierarchical seed derivation using SHA3-512 with domain separation, providing provable collision resistance and random-access universe generation at any scale.

**Claim 2 — Stratified Domain Resonance (SDR) Noise.**  
A method for generating continuous scalar fields via a superposition of phase-coherent Gabor wavelets over a Poisson-disc lattice, producing geologically coherent, anisotropic terrain features superseding classical Perlin/Simplex noise.

**Claim 3 — Greywater Planetary Topology Model (GPTM).**  
A hybrid quad-sphere/sparse-voxel-octree system for representing and rendering planetary-scale geometry in real-time using a Vulkan-managed clipmap and handoff shader.

**Claim 4 — The Martyrdom Engine.**  
A dual-resource risk/reward gameplay system: accrual of Sin via Profane weapon actions; accrual of Mantra via Sacred weapon actions; expenditure of Sin for temporary power states (Blasphemies); automatic triggering of an enhanced-power state (Rapture) followed by a mandatory vulnerability state (Ruin) upon Sin reaching maximum — without player opt-out.

**Claim 5 — Persistent Gameplay-Affecting Dismemberment.**  
A system wherein detached limbs persist as physics-enabled entities with simplified collision and can be interacted with by AI navigation agents and the player character to affect pathfinding, provide line-of-sight cover, and trigger environmental chain reactions.

---

## Conclusion

**Cold Coffee Labs** presents **Greywater Engine**. Greywater Engine presents ***Sacrilege***. Blacklake supplies the hells-cape. BLD supplies the authoring velocity.

The engine is ready. The framework is designed. The design is locked.

**Now we build.**

---

***Cold Coffee Labs · 2026-04-21 · GW-SAC-001-REV-A***

**⬛ END OF DOCUMENT ⬛**
