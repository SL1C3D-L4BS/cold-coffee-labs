# 07 — *Sacrilege* — Complete Game Specification GW-SAC-001

**⬛ TOP SECRET // COLD COFFEE LABS // GREYWATER-SAC-001 ⬛**

**One Engine. One Game. Absolute Carnage.**

| Field | Data |
|-------|------|
| Document | GW-SAC-001-REV-B |
| Engine | Greywater Engine — C++23 / Vulkan |
| Game | *Sacrilege* — 3D first-person action |
| Logic | 100% C++23. No scripting language. No VM. No Lua. |
| Assets | 100% custom. No licensed packs. |
| Date | 2026-04-22 |
| Status | Living document — active development |

*Unauthorized disclosure prohibited.*

---

## The Pitch

*Sacrilege* is what happens when 1993 arena shooters are rebuilt from scratch in 2026 with a custom engine, a procedural generator that makes every run different, and a movement system that speedrunners will dedicate their lives to.

You descend through **Nine Inverted Circles of Hell**. Every circle is a different kind of hell, generated from a **Hell Seed** — sharable, deterministic, infinitely replayable. You fight with profane weapons that build Sin and sacred weapons that build Mantra. Let Sin hit 100 and **Rapture** takes you — six seconds of god-mode followed by twelve seconds of **Ruin** where you can barely walk. Manage that tension or die.

**The DNA:** Quake movement + Doom aggression + original resource system + infinite procedural levels + persistent gore that changes how the level plays.

---

## Table of Contents

**Part 0 — Cosmology & Cast (Story Bible entry point)**
0.1 The Primordial Schism
0.2 The Protagonist — Malakor / Father Niccolò
0.3 The Factions — CORE, LEGION, UNWRITTEN
0.4 The Locations — Rome, Vatican, Heaven, Hell, Silentium
0.5 The Three Acts

**Part I — The Game**
1. Core Loop
2. Martyrdom Engine — Sin, Mantra, Blasphemies, Rapture & Ruin (+ God Mode, Grace Meter)
3. Movement — Speed Demon Doctrine
4. Arsenal — Profane and Sacred Weapons
5. The Damned Host — Enemy Roster
6. Nine Inverted Circles — Level Design (with Location Skins)
7. The God Machine / Logos — Final Boss

**Part II — Systems (C++23 Implementation)**
8. ECS Architecture for Sacrilege
9. Player Controller Implementation
10. Enemy Behavior Trees
11. Weapon System
12. Gore & Dismemberment System
13. Diegetic HUD

**Part III — Art Direction & Narrative Reactivity**
14. Visual Style — Crunchy Low-Poly Horror
15. Sound Design — Metal & Mayhem
15b. Narrative Reactivity — Sin-Signature, Voice Director, Grace Finale

**Part IV — Performance & Technical Targets**
16. Frame Budget
17. Sacrilege-Specific Engine Extensions

**Part V — Novel Contributions (Patent Posture)**
18. Claims (1–8)

---

## Part 0 — Cosmology & Cast

> **Authority note.** The canonical narrative Bible lives in `docs/11_NARRATIVE_BIBLE.md`. This Part 0 is the shipping-relevant summary that the mechanics below assume. When lore and mechanics collide, `docs/11` wins on *lore*; this document (`docs/07`) wins on *mechanics and budgets*.

### 0.1 The Primordial Schism

Before creation there was **the Absolute** — pure undifferentiated unity. It fractured into three aspects:

- **The One** — the God of mercy, justice, and stewardship. Authored Heaven. Retired in silence.
- **The Other** — the adversary. Jealous shadow of the One. Authored Hell and the Factory that recycles damned souls into weapons.
- **The Witness** — the neutral observer. Authored the *Silentium*, a mirror dimension, and walks between. Player encounters the Witness in Act III.

The One's silence is the starting condition. Humanity prays to a God who does not answer. The Church (**CORE**) manufactures a false response for political control. The player's work is to cross the silence.

### 0.2 The Protagonist — Malakor / Father Niccolò

**Father Niccolò Bianchi** is a Vatican exorcist who commits the ritual forbidden by the One: he fuses his soul with **Malakor**, a fallen angel of the *Unwritten* (angels who defected from the One's silence). The fusion is the gameplay. The player is both.

- **Malakor** is the combat voice. Aggressive, hungry, amoral. Speaks during Rapture, Blasphemy casts, kills, and *God Mode*.
- **Niccolò** is the moral voice. Measured, regretful, devotional. Speaks during exploration, post-combat downtime, and the Grace finale.

Voice selection is runtime, driven by the Sin-signature (§15b) and current combat state. Never both at once. The duality is diegetic — the player never sees Niccolò's face; they see their own hands, their own weapons, and hear the argument in their head.

### 0.3 The Factions

| Faction | Role | Play pattern |
|---------|------|--------------|
| **The CORE** (Church of the Recycled Eden) | Institutional antagonist; manufactures the false God response; staffed by fanatical clerics and cyber-cherubim | Sacred-weapon encounters; the most politically charged kills; Mantra-rich |
| **The LEGION** | Hell's extraction infrastructure; recyclers of damned souls into weapons, monsters, and the Factory Floor itself | Profane-weapon encounters; Sin-rich; bodies are literally raw material |
| **The UNWRITTEN** | Fallen angels who defected from the One's silence; Malakor's people; mostly allies (rare, silent NPCs you rescue, never fight) | Grace-eligible; harming one is a Grace penalty |

### 0.4 The Locations

Shipping locations skin the Nine Circles (§6) without changing their mechanical identity:

| Location | Skin of | Notes |
|----------|---------|-------|
| **Rome (streets)** | Circle I — Limbo | Act I opening; tutorial vertical movement across collapsed basilicas |
| **Vatican Necropolis** | Circle II — Lust | Layered catacombs; frozen shear |
| **Colosseum of Echoes** | Circle VII — Violence | Gladiatorial pit for the Act II crescendo |
| **Palazzo of the Unspoken** | Circle VI — Heresy | Non-Euclidean CORE administrative heart |
| **Sewer of Grace** | Circle VIII — Fraud | False corridors; the Unwritten are hiding here |
| **Heaven's Back Door** | Circle V — Wrath | The One's abandoned throne room |
| **Hell's Factory Floor** | Circle IV — Greed | The LEGION's recycler |
| **The Silentium** | Circle IX — Treachery (profile variant) | Mirror dimension; zero-gravity void; **not an open world** — a special procedural profile layered on Circle IX per `docs/01` §2.6 |

The lore-to-mechanics mapping is therefore **total**: every Circle has a narrative skin, every skin has a deterministic profile, every profile is Blacklake-generated from the Hell Seed.

### 0.5 The Three Acts

| Act | Title | Structural beat | Gameplay focus |
|-----|-------|-----------------|----------------|
| **I** | *The Rite of Unspeaking* | Niccolò performs the forbidden fusion; the player learns Sin/Mantra/movement across Rome and the Vatican | Circles I–III; learn Blasphemies; first **God Mode** trigger at end |
| **II** | *The Blasphemy Circuit* | The player carves through the CORE and into the LEGION; encounters the Witness | Circles IV–VII; full Blasphemy economy; Sin-signature begins driving dialogue divergence |
| **III** | *The Unmaking* | The player confronts the Logos (silent God); chooses annihilation or the **Grace Meter** finale | Circles VIII–IX + Silentium; Logos fight (§7); `forgive` Blasphemy unlocks |

---

## Part I — The Game

### 1. Core Loop

```
Enter Circle → Navigate procedural geometry → Kill enemies → Build Sin/Mantra
→ Spend Blasphemies strategically → Survive Rapture/Ruin cycles
→ Reach the Circle's exit → Descend to next Circle → Reach Circle IX → Kill the God Machine
```

The player has no base. No crafting. No map. They have momentum, weapons, and the Martyrdom meter. Every decision is instantaneous. Every room is potentially lethal.

**Speedrun viability is a first-class design constraint.** The deterministic Hell Seed means seeds are sharable. A seed is effectively a level code. Speedruns on a given seed are directly comparable. Categories: Any%, All Circles, Pacifist (no kills), Max Sin.

### 2. Martyrdom Engine

The Martyrdom Engine is the mechanical heart of *Sacrilege*. It is *Sacrilege*'s original contribution to the genre — nothing else does this.

#### 2.1 Sin

**Sin** accrues from **Profane** weapon damage and from certain zone effects in specific Circles. Sin ranges from 0 to 100.

- As Sin climbs, the screen fills with hellish red tendrils. A discordant choir rises in the audio mix.
- At Sin ≥ 70, tendrils begin moving and distorting the view edges.
- At **Sin = 100**, the player **automatically enters Rapture**. There is no opt-out. There is no button. This is intentional and load-bearing — hoarding Sin is not a safe strategy.

#### 2.2 Mantra

**Mantra** accrues from **Sacred** weapon kills, precision parries, and Glory Kills. Mantra fuels Sacred weapon alt-fires and can be spent on one Absolution charge (passive Sin drain, unlockable late-game).

Mantra is the counterweight. Skilled play — precise, measured, skillful — builds Mantra. Aggressive, reckless play builds Sin. The best players manage both.

#### 2.3 Blasphemies — Spending Sin

The player spends accumulated Sin on Blasphemies — temporary, powerful ability activations:

| Blasphemy | Sin Cost | Duration | Effect |
|-----------|----------|----------|--------|
| **Stigmata** | 25 | 5s | All damage taken is reflected back at attacker. Stack with Rapture's 3× damage for maximum reflect damage. |
| **Crown of Thorns** | 40 | 8s | A ring of ethereal thorns orbits the player; any enemy entering melee range is shredded instantly. |
| **Wrath of God** | 60 | Instant | Targeted pillar of unholy light at cursor; massive AOE damage. Skill-cap: requires aim. |
| **False Idol** | 30 | 10s | A stationary decoy spawns at current position; all nearby enemies retarget it; explodes on death or timer. |
| **Unholy Communion** | 50 | 12s | Lifesteal active; 50% of all damage taken converts to a temporary overshield. |

Timing Blasphemy use is the intermediate skill layer. Spending 60 Sin on Wrath of God at Sin=65 is safe. Letting Sin climb to 90 to maximize a Stigmata reflect window is a high-risk, high-reward play that experienced players will master.

#### 2.4 Rapture & Ruin

| State | Duration | Effect |
|-------|----------|--------|
| **Rapture** | 6 seconds | 3× movement speed, 3× damage, perfect accuracy, piercing rounds. Screen floods white. |
| **Ruin** | 12 seconds | No health regen, 50% movement speed, 75% damage reduction. Screen desaturates. Audio drops to a low drone. |

**Ruin lasts exactly twice as long as Rapture. This ratio is non-negotiable.** The punishment must be severe enough that Rapture is dreaded as much as it is desired. Players who reach endgame fluency will learn to spend Sin to ~85 and then burn the last 15 via Blasphemies to prevent Rapture from triggering. That mastery is the intended ceiling.

#### 2.5 C++ ECS Representation

All Martyrdom state is plain POD structs in the ECS. No virtual functions. No heap per tick.

```cpp
// gameplay/martyrdom/martyrdom_components.hpp
// All components are POD — zero heap per tick, rollback-safe

struct SinComponent {
    float value       = 0.f;
    float max         = 100.f;
    float accrual_rate = 5.f;   // base units per profane hit; scaled by weapon multiplier
};

struct MantraComponent {
    float value       = 0.f;
    float max         = 100.f;
};

struct RaptureState {
    float time_remaining = 0.f;
    bool  active         = false;
};

struct RuinState {
    float time_remaining = 0.f;
    bool  active         = false;
};

// Composed from all active buffs/debuffs each frame by StatCompositionSystem
struct ResolvedStats {
    float damage_mult   = 1.f;
    float speed_mult    = 1.f;
    float accuracy_mult = 1.f;
    float lifesteal     = 0.f;
};

struct BlasphemyInstance {
    BlasphemyType type;
    float         time_remaining;
};

struct ActiveBlasphemies {
    static constexpr int MAX = 3;
    BlasphemyInstance entries[MAX];
    int               count = 0;
};
```

Systems:
- `SinAccrualSystem` — increments sin_value on ProfaneHitEvent; clamps to max
- `MantraAccrualSystem` — increments mantra_value on SacredKillEvent, ParryEvent, GloryKillEvent
- `RaptureCheckSystem` — if sin_value >= max and not ruin_active: trigger Rapture, zero sin, set time_remaining=6.0f
- `RaptureTickSystem` — decrement rapture time; on expiry: begin Ruin
- `RuinTickSystem` — decrement ruin time; on expiry: clear debuffs
- `BlasphemySystem` — consume Sin, create BlasphemyInstance, validate cost gate
- `StatCompositionSystem` — recompute ResolvedStats from all active states each frame

All systems are pure functions over component spans. Deterministic at fixed dt. Rollback-safe.

#### 2.6 God Mode — the Blasphemy State (narrative skin over Rapture)

**God Mode** is the narrative name the player and dialogue use for the state that the Martyrdom Engine already implements as *Rapture*. It is not a new mechanical state — the existing `RaptureState` component is the authority. God Mode layers the following **presentation and scripting hooks** on top:

- **Malakor speaker lock.** While `RaptureState.active == true`, the narrative voice director (`engine/narrative/voice_director.hpp`) locks to Malakor regardless of Sin-signature. Niccolò cannot speak during God Mode.
- **Reality-warp hooks (opt-in, Tier A).** The render frame graph reads a `GodModePresentation` component and applies a per-pass distortion (chromatic aberration pulse, colour grade shift, post stack intensity +1 tier). Pure visual — zero replay-hash impact.
- **Grace Meter coupling.** Each Act I God Mode trigger increments a `GraceCost` counter. The player *can* reach the Grace finale without ever triggering God Mode (Max Sin category is precisely that run), but Grace unlock prices scale with God Mode count.
- **Act I first-trigger scripting.** The first time Rapture auto-triggers in Act I, a deterministic dialogue hook fires (Niccolò crying, Malakor laughing). This is scripted per seed — not randomized — so speedrun splits reproduce.

**Rule:** God Mode never changes the Rapture/Ruin 6s/12s ratio (§2.4). It never adds damage, speed, or invulnerability beyond what Rapture already provides. The narrative layer is a pure skin over the mechanical spine. See `docs/01` §0.1 for the governing principle.

#### 2.7 Grace Meter — the forgiveness ending

**Grace** is a third Martyrdom-adjacent resource introduced in Act III. It is a Blasphemy-payment currency that enables exactly one terminal Blasphemy: **`forgive`**.

**Accrual rules (deterministic, ECS-resident):**

| Source | Grace | Notes |
|--------|-------|-------|
| Choosing not to kill an Unwritten NPC | +15 | Per-encounter, deduplicated by entity id |
| Completing a Circle without entering Rapture | +25 | Tracked by `RaptureTriggerCounter` |
| Glory-killing only hostile (CORE/LEGION) targets for a full Circle | +10 | Sin-signature driven |
| Parrying the Logos' first phase without damage | +50 | Act III §7 Phase 1 hook |
| Using a Blasphemy to save an NPC life | +5 | Per-entity-id, one-shot |

**Expenditure:**
- `forgive` Blasphemy (Sin cost: 100, Grace cost: 100) is available **only** during the Logos fight Phase 4 window (see §7). It ends the game in the Grace path and triggers the Witness epilogue.
- The player cannot spend Grace on anything else. It is not a combat resource.

**ECS representation:**

```cpp
// gameplay/martyrdom/grace_meter.hpp
struct GraceComponent {
    float value = 0.f;
    float max   = 100.f;
};

struct GraceTransactionEvent {
    u32   actor_id;
    float delta;
    u16   source;    // enum: UnwrittenSaved, CircleNoRapture, ParryLogos, ...
    u64   seed;      // per-transaction seed for determinism
};
```

All Grace accrual is seed-stamped so replays reproduce identical Grace curves. The `forgive` Blasphemy is the only Blasphemy whose cost gate includes a Grace check.

---

### 3. Movement — Speed Demon Doctrine

Movement in *Sacrilege* is a **weapon**. Players who master movement are effectively invincible; players who stand still die within seconds. The movement system is designed for deep mastery — easy to understand, hard to execute perfectly, rewarding to improve.

The player controller is a **custom swept-AABB implementation** in `gameplay/player/`. It does not use Jolt's character controller. It runs at fixed timestep. Same inputs → same trajectory across all machines → speedrun-verifiable, rollback-safe.

#### Movement Techniques

| Technique | Input | Mechanic | Skill Ceiling |
|-----------|-------|----------|---------------|
| **Bunny Hop** | Jump on landing | Preserves horizontal velocity on jump; air-strafe to redirect and accelerate | High — optimal strafing requires frame-perfect timing |
| **Slide** | Crouch while sprinting | Preserves all momentum; lowers hitbox; allows passing under obstacles | Medium — directional input during slide steers |
| **Wall Kick** | Jump while wall-adjacent | Applies `wall_normal × 4 + up × 5` impulse; resets on ground contact; chains once | Medium — reading wall normals in fast movement |
| **Grapple** (tap) | Grapple button | Ray-cast; pulls player toward hit point at 15 m/s; range 20 m | High — chaining grapple with air strafe |
| **Grapple** (hold on small enemy) | Grapple hold | Pulls enemy to player; triggers Glory Kill on impact; builds Mantra | Medium — target acquisition at speed |
| **Blood Surf** | Crouch on slope/decal | On slope > 15° or blood decal cell: friction = 0.02; velocity preserved | Medium — reading terrain; routing through blood |

**CVars for tuning (all movement values):**
```
mv.bhop_multiplier        1.10     // horizontal vel boost on jump
mv.slide_friction         0.15     // friction during slide
mv.wall_kick_horizontal   4.0      // m/s horizontal impulse
mv.wall_kick_vertical     5.0      // m/s vertical impulse
mv.grapple_speed          15.0     // m/s pull speed
mv.grapple_range          20.0     // metres
mv.surf_friction          0.02     // friction on blood surf
mv.air_control            0.75     // fraction of ground control in air
```

---

### 4. Arsenal — Instruments of Blasphemy

**No weapon has a reload.** All weapons have alt-fires. Weapons are divided into **Profane** (builds Sin) and **Sacred** (burns/builds Mantra). Loadout strategy — which weapons to carry into a given Circle — is the pre-run strategy layer.

| Weapon | Class | Primary | Alt-Fire | Ammo Economy |
|--------|-------|---------|---------|--------------|
| **Halo-Scythe** | Sacred / melee | 3-hit combo; builds Mantra on kill | Mantra Blast (25 Mantra): disintegration wave, no cooldown | Infinite primary; Mantra-gated alt |
| **Dogma** (×2) | Profane / SMG | Dual SMGs; +8 Sin per hit | Suppressing Fire: max fire rate 3s, 40s cooldown | Ammo pickups; alt is cooldown-gated |
| **Heretic's Lament** | Sacred / railgun | Piercing shot; high damage, 1.2s cycle | Overcharge (10 Mantra/shot): 4× damage; kills restore 15 HP | Low ammo; alt is Mantra-gated |
| **Confessor** | Profane / shotgun | Bouncing shrapnel; +12 Sin per kill | Censer Bomb: toxic smoke; +5 Sin per second per enemy inside | Ammo pickups |
| **Schism** | Profane / sawblade | Launches spinning blade; ricochets off walls | Recall: reverses blade; +bonus Sin for return hits | One blade active at a time |
| **Cathedral** | Sacred / stake launcher | Fires explosive stakes; pins enemies to surfaces | Sanctuary (75 Mantra): protective dome for 5s; blocks projectiles | Ammo pickups; alt Mantra-gated |

**Weapon design principles:**
- Profane weapons are aggressive, fast, loud — they reward reckless play with Sin accumulation
- Sacred weapons are measured, precise, punishing — they reward skillful play with Mantra
- Every weapon alt-fire has a real cost (Sin, Mantra, or cooldown) — no free power
- Weapon feel is exaggerated — recoil, sound, screen-shake, impact effects all dial to 11

---

### 5. The Damned Host — Enemy Roster

Every enemy is designed to challenge a specific element of the Martyrdom loop and movement system. None are bullet sponges. All are fast. All punish standing still.

| Enemy | Archetype | Signature | Sin on Kill | Hard Counter |
|-------|-----------|-----------|-------------|--------------|
| **Cherubim** | Strafer | Hovers; twin machine guns; swarms via Blacklake flocking algorithm | 5 | AOE Blasphemy or Rapture mass-clear |
| **Deacon** | Teleporter | Warps behind player when out of LoS; parry window → 20 Mantra | 15 | Situational awareness; parry timing |
| **Leviathan** | Charger/Environmental | Burrows through arena walls following L-system path; can be baited into self-stun | 20 | Bait into walls; exploit 2s stun window |
| **Warden** | Summoner/Tank | Shielded until all summoned Cherubim die; drops large Sin | 50 | Kill adds first; chain Blasphemies on expose |
| **Martyr** | Charger | Explodes on death; explosion radius builds +10 Sin; chain explosions possible | 10 | Grapple → Glory Kill to bypass death explosion |
| **Hell Knight** | Berserker | Melee swipes drain Mantra on hit; immune to knockback | 25 | Maintain range; Sacred weapons only |
| **Painweaver** | Grenadier | Web projectiles slow by 70% and pause Sin generation for 3s | 30 | Immediate priority target; movement is survival |
| **Abyssal** | Glass Cannon | Fires 3 homing projectiles; each one shot down awards 5 Mantra | 15 | Intercept shots → Mantra economy tool |

**Enemy behavior is data-driven behavior trees.** Tree structure is in `.gwbt` files authored in the editor. Node implementations (attack, patrol, flank, flee, sync-with-allies) are **registered C++ functions** in `gameplay/enemies/`. Designers wire the trees; programmers write the nodes.

---

### 6. Nine Inverted Circles — Level Design

Each Circle is a procedurally generated 3D arena generated by the Blacklake Framework from the session's Hell Seed. Designers set the SDR parameters and encounter templates; Blacklake generates the actual geometry. No two runs with different seeds produce the same level.

**Design intent per Circle:**

| Circle | Location Skin | Blacklake Profile | Environment | Signature Encounter | New Mechanic |
|--------|---------------|------------------|-------------|---------------------|--------------|
| **I — Limbo (Inverted)** | Rome (streets) | Vertical displacement, floating islands | Collapsed basilicas over void, perpetual twilight | Cherubim swarm introduction | Wall Kick, Slide |
| **II — Lust (Frozen)** | Vatican Necropolis | Directional shear, ice formations | Layered catacombs, slippery floors, low visibility | Deacon pack — parry training | Blood Surf |
| **III — Gluttony (Gut)** | Sewer of Grace (entry) | Sinuous organic forms, flesh geometry | Flesh tunnels, acid pools, pulsating walls | Leviathan introduction | Cathedral staking |
| **IV — Greed (Forge)** | Hell's Factory Floor | Rectilinear industrial, crushers | LEGION recycler foundry, molten metal, conveyor geometry | Warden + Cherubim combo | Grapple Hook |
| **V — Wrath (Battlefield)** | Heaven's Back Door | Cratered, disrupted terrain | The One's abandoned throne room (shattered) | Martyr chain management | Dogma dual-wield |
| **VI — Heresy (Library)** | Palazzo of the Unspoken | Non-Euclidean angular displacement, impossible geometry | CORE administrative heart; shifting walls, mirror traps | Hell Knight + Painweaver combo | Parry |
| **VII — Violence (Arena)** | Colosseum of Echoes | Clean amphitheatre topology, tiered seating | Gladiatorial pit, spectator enemies, terrain traps | Full enemy roster encounter | Sacrilege Mode (naked run) |
| **VIII — Fraud (Maze)** | Sewer of Grace (deep) | High-frequency false floors, mirror geometry | False walls, mirror rooms — Unwritten hide here | Abyssal fleet in maze | Heretic's Lament health-as-ammo |
| **IX — Treachery (Abyss)** | The Silentium (mirror profile) | Zero-gravity void debris field | Black void, frozen lake below, no atmosphere | Full gauntlet all enemy types | All mechanics final exam |

**Silentium note.** The Silentium is not an open world; it is a **special procedural profile** layered on Circle IX (Treachery), per `docs/01` §2.6. Same Blacklake pipeline, same Hell Seed, mirrored geometry + voided atmosphere + Witness encounters. The editor's `circle_editor` panel exposes the Silentium profile as a checkbox on Circle IX.

**Procedural generation rules:**
- Each Circle has a constexpr `CircleProfile` — SDR parameters, enemy density tables, resource density tables
- Blacklake generates arena geometry from those parameters and the Hell Seed
- Encounter placement is drawn from the profile's spawn tables, placed deterministically relative to chunk seed
- Designer-authored "set piece" encounters can be injected at specific narrative moments regardless of seed

---

### 7. The God Machine / Logos — Final Boss

A colossal clockwork angel, arena-filling, with four rotating faces. The final encounter tests mastery of every mechanic.

**Arena:** A cathedral that structurally collapses over the fight's duration. Safe ground shrinks as the fight progresses. Players who stall die to falling geometry.

| Phase | Active Face | Mechanic | Recommended Strategy |
|-------|-------------|----------|---------------------|
| **1 — Adoration** | Face of Mercy | Minion waves; arena intact; tutorial phase | Build Mantra; learn face vulnerability windows |
| **2 — Doubt** | Face of Judgment | First floor collapse; projectile barrage patterns | Maintain mobility; use False Idol for breathing room |
| **3 — Heresy** | Face of Wrath | Two faces simultaneously; homing artillery | Use Crown of Thorns; Rapture during double-face window if available |
| **4 — Revelation** | All four faces | Full arena collapse; forced Rapture/Ruin cycling; final window | Manage Sin to enter Rapture at optimal moment; finish during Rapture window |

**Design:** The God Machine is designed so that the Martyrdom Engine is not just useful — it is *required*. A player who reaches Phase 4 with no Blasphemy economy will die. The fight is a skill evaluation, not a DPS check.

#### 7b. Logos Alternate Phase 4 — the Grace Path

Phase 4 has **two entry conditions** that produce **two different endings**:

| Entry condition | Phase-4 behaviour | Ending |
|-----------------|-------------------|--------|
| `GraceComponent.value < 100` on entering Phase 4 | As written above — Revelation, full arena collapse, forced Rapture/Ruin cycling, boss dies to damage | **Annihilation ending** — Malakor wins, Niccolò is silent, the One remains silent |
| `GraceComponent.value == 100` on entering Phase 4 | Arena collapse halts; all minion spawns cease; the four faces become a single **Logos** face (silent, weeping); a 12-second window opens where `forgive` Blasphemy is spendable | **Grace ending** — player spends 100 Sin + 100 Grace to end the game; Witness epilogue unlocks |

The Grace path is **not** easier. It requires the player to have played largely restraint-focused runs across Acts I–II to accumulate 100 Grace (§2.7 accrual table), while still having earned enough Sin to be at 100 at the Phase-4 window. The Max-Sin speedrun category explicitly forbids the Grace path; the Grace category explicitly requires it.

**Implementation note:** Phase 4 entry is data-driven by `gameplay/boss/logos/phase4_selector.cpp`, which reads `GraceComponent` from the player ECS entity. The boss behaviour tree has two root branches; the selector picks one deterministically at Phase-4 entry and commits. No rollback across the selection.

---

## Part II — Systems (C++23 Implementation)

### 8. ECS Architecture for Sacrilege

All game entities — player, enemies, projectiles, gibs, blood decals, boss phases — are ECS entities. Components are POD. Systems are pure functions over component spans. This is non-negotiable.

**Core component types:**

```cpp
// Transform
struct TransformComponent { Vec3_f64 world_pos; Quat rotation; Vec3 scale; };

// Health & damage
struct HealthComponent    { float current; float max; };
struct DamageImmunityMask { u32 flags; };  // bitmask of DamageType enum

// Team / allegiance
struct TeamComponent      { TeamId team; };  // Player, Damned, Neutral

// Physics link
struct PhysicsBodyRef     { PhysicsBodyId body_id; };

// Rendering
struct MeshComponent      { AssetHandle mesh; AssetHandle material; };
struct GoreStateComponent { u8 missing_limbs_mask; u8 decal_count; };

// AI
struct BehaviorTreeRef    { AssetHandle bt_asset; BtInstanceId instance; };
struct PatrolPathRef      { AssetHandle path_asset; u32 current_node; };
struct AwarenessState     { EntityHandle target; float alert_level; };

// Loot
struct LootTableRef       { AssetHandle loot_table; };
struct SinDropComponent   { float sin_value; };  // awards sin to killer on death
```

### 9. Player Controller Implementation

The player controller is `gameplay/player/sacrilege_player_controller.hpp/.cpp`. It is a custom swept-AABB controller — not Jolt's character controller. Key properties:

- Fixed timestep: `1.0f / 144.0f` seconds per tick
- Pure function of `(PlayerInputFrame, PlayerPhysicsState) -> PlayerPhysicsState`
- No global state reads. Deterministic. Rollback-safe.
- Runs before physics sync, after input collection

**State machine:**

```
Grounded → (jump) → Airborne
Airborne → (land) → Grounded
Grounded → (crouch + sprint) → Sliding
Sliding → (land / speed decay) → Grounded
Airborne → (wall contact) → WallKicking
Any → (grapple fire) → Grappling
Grappling → (arrive / release) → Airborne
```

All transition conditions are plain `if` statements over float comparisons — no dynamic dispatch, no virtual calls in the movement tick.

### 10. Enemy Behavior Trees

Enemy behavior trees are stored as `.gwbt` binary assets (authored in the editor's BT panel). At runtime they are interpreted by `engine/ai/behavior_tree_world.hpp`.

Node implementations are C++ functions registered at startup:

```cpp
// gameplay/enemies/bt_nodes.cpp — registered at startup
BtRegistry::register_action("shoot_at_target", &BtNodes::shoot_at_target);
BtRegistry::register_action("patrol_to_node",  &BtNodes::patrol_to_node);
BtRegistry::register_action("flee_from_player",&BtNodes::flee_from_player);
BtRegistry::register_condition("can_see_player",&BtNodes::can_see_player);
BtRegistry::register_condition("health_below",  &BtNodes::health_below);
// ... all registered at startup, looked up by name hash at BT asset load time
```

The designer wires the tree in the editor. The programmer writes the node code. The editor bakes the tree to a `.gwbt` asset. The runtime looks up node implementations by name hash. **Zero interpreted code at runtime.**

### 11. Weapon System

Weapons are ECS entities with components:

```cpp
struct WeaponComponent {
    WeaponId     id;
    float        cooldown_remaining;
    u32          ammo_primary;
    float        mantra_cost;      // 0 if no mantra cost
    float        sin_per_hit;      // 0 for sacred weapons
    bool         alt_fire_active;
    float        alt_fire_cooldown;
};

struct HitscanComponent  { float range; float damage; u32 pellet_count; };
struct ProjectileSpawner { ProjectileType type; float speed; float damage; };
struct MeleeComponent    { float reach; float damage; float swing_angle_deg; };
```

Weapon firing is a system that reads `WeaponFireInputEvent` and resolves damage, projectile spawning, or melee traces. Hit detection is:
- **Hitscan:** immediate ray-cast via Jolt query; applies damage event to target
- **Projectile:** spawns a `ProjectileEntity` with velocity; `ProjectileMoveSystem` advances it; collision resolved via Jolt shape query
- **Melee:** capsule-cast forward from player position over the swing frame; applies damage to all hits in sweep

All weapon effects (muzzle flash, impact decal, blood spray) are spawned as transient ECS entities with a `LifetimeComponent { float remaining; }` that `LifetimeSystem` manages.

### 12. Gore & Dismemberment System

Persistent gore is a Sacrilege-differentiating feature and a **gameplay-affecting** system — not just visual polish.

**Severed limbs:**
- On enemy death (certain damage types: Halo-Scythe, explosive), `DismemberSystem` detaches limb entities
- `LimbEntity`: simplified collision mesh, Jolt rigid body, `NavObstacleTag` component
- `NavObstacleTag` causes the navmesh baker to treat the limb as an obstacle — enemies pathfind around them
- Limb entities persist for the session in the current Circle (not across Circle transitions)
- Max 64 active limb entities per Circle; oldest recycled on overflow

**Blood decals:**
- Spawned on any high-damage hit; placed on the surface normal of the hit point
- `BloodDecalComponent`: world position, normal, radius, variant (0–7), opacity
- GPU ring-buffer: 512 max active; oldest overwritten
- Decals contribute to `BloodSurfaceCell` — cells where blood surf is active
- Persistent within a Circle run; reset on Circle transition

**Limb-as-weapon:** Players can grapple-grab a severed limb and throw it as a weapon (deals 15 damage, +5 Sin, triggers Glory Kill on stagger). This is a hidden mechanic — discoverable, not tutorialized.

### 13. Diegetic HUD

No minimap. No objective markers. No floating damage numbers. No kill feed. The HUD is in-world whenever possible.

| Element | Presentation | Location |
|---------|-------------|---------|
| **Health** | Blood-filled vial, fill level = HP/max, colour shifts white→dark red | Bottom-left, wrist model |
| **Sin Meter** | Runic ring arc, fill = sin/100, glows blood red, pulses at >70 | Center-bottom near crosshair |
| **Mantra Counter** | Numeric, small, cool blue | Bottom-right below weapon name |
| **Rapture Flash** | Full-screen white overlay, fades over 0.5s | Post-process layer |
| **Ruin Desaturation** | Full-screen greyscale lerp, 0.5s in / 1.0s out | Post-process layer |
| **Crosshair** | Four-line cross; hidden during Ruin (ruin_weight > 0.5) | Center |

**No ammo counter.** Weapons have no reloads. Ammo counters imply reloads. Removing the counter removes the cognitive load. Players feel the ammo state through weapon behavior (fire rate drops, sound changes) before running dry.

---

## Part III — Art Direction

### 14. Visual Style — Crunchy Low-Poly Horror

*Sacrilege* uses a deliberate **crunchy low-poly** aesthetic. Not pixel art. Not voxel. Not PS1 smooth. Hard edges, deliberate polygon counts, high-contrast dynamic lighting.

**The seven properties:**

1. **Low polygon counts — deliberate.** Enemies and objects have visible facets. Silhouettes are angular. This is art direction, not a limitation.
2. **Hard-edge normals by default.** Smooth normals only where anatomically correct (organic forms like Gluttony's flesh walls).
3. **High-contrast dynamic lighting.** The scene is lit by a small number of intense, directional lights. Shadows are hard and deep.
4. **Restricted palette per Circle.** Each Circle has a 4–6 colour palette. The colours are extreme — blood red, bone white, bile yellow, void black.
5. **Texture resolution: small.** 256×256 for most surfaces. 512×512 for key enemy surfaces. No 4K textures. `GL_NEAREST` optional CVar for a harder pixel look.
6. **Post-processing is horror-forward.** Chromatic aberration, film grain, screen shake are always on (adjustable for accessibility). They are part of the visual language.
7. **Blood and gore are readable.** Blood is bright red, high contrast against all environment surfaces. Gore effects are immediately legible from gameplay distance.

**Colour palettes per Circle:**

| Circle | Primary | Accent | Danger | Void |
|--------|---------|--------|--------|------|
| Limbo | Obsidian `#0B1215` | Dim violet `#4A3060` | White `#E8DDC8` | Black |
| Lust | Ice blue `#A8C8D8` | Deep blue `#1A3A5C` | Frost white | Cold black |
| Gluttony | Bile yellow `#8B8B00` | Flesh pink `#D4847A` | Acid green | Dark brown |
| Greed | Forge orange `#D35400` | Iron grey `#4A4A4A` | Molten yellow | Soot black |
| Wrath | Battlefield grey `#5A5A4A` | Blood red `#8B0000` | Explosion white | Ash |
| Heresy | Impossible purple `#5A1A8A` | Mirror silver `#C0C0C0` | Signal blue `#5865F2` | Void |
| Violence | Arena sand `#C8A050` | Blood orange `#C04A00` | Combat white | Shadow black |
| Fraud | Mirror silver `#C0C0C0` | Deception grey `#808080` | False white | Mirror black |
| Treachery | Void black `#000000` | Star white `#F0F0F0` | Ice blue | Absolute black |

### 15. Sound Design — Metal & Mayhem

**Music system:**
- Layered metal soundtrack. Each Circle has a base track with 4 intensity layers.
- Layers activate based on **Combo Meter** (kills/second): drums → bass → guitar → full orchestra
- BPM is Circle-specific; the animation system syncs melee timing to BPM for satisfying hit-feel
- Martyrdom audio states: Sin choir rises 0→100; Rapture onset = divine thunder + tempo spike; Ruin = drone only

**SFX principles:**
- Weapon sounds are heavily compressed, loud, and physical — they hurt to listen to at max volume (intentional)
- Enemy death sounds: 12+ variants per enemy type; never the same sound twice in sequence
- The Apostate (player character) delivers impact one-liners on kill chains ("Repent." / "Too slow." / "Your god isn't listening.")
- Environmental audio responds to Circle — flesh tunnels squelch, ice caverns echo, forge pounds

Music layer mixing and BPM-sync layer selection are driven by the **symbolic adaptive music** transformer in `engine/ai_runtime/music_symbolic.hpp` — a ≤ 1M-parameter RoPE model running on ggml CPU with a 0.2 ms budget per frame. Stems are authored offline and pinned; the model selects which stem weights rise and fall frame-to-frame based on combat intensity. See `docs/06` §3.32.

---

### 15b. Narrative Reactivity — Sin-Signature, Voice Director, Grace Finale

Narrative reactivity is not dialogue randomization. It is a **deterministic function** of player play-style over a bounded recent window. The authoritative module is `engine/narrative/`.

#### 15b.1 Sin-Signature (rolling fingerprint)

A five-channel ECS component updated every gameplay tick:

| Channel | Bounds | Driven by |
|---------|--------|-----------|
| `god_mode_uptime_ratio` | `[0.0, 1.0]` | Rapture time ÷ Circle time |
| `precision_ratio` | `[0.0, 1.0]` | Headshot / parry kills ÷ total kills |
| `cruelty_ratio` | `[0.0, 1.0]` | Glory kills + dismemberment chain bonuses ÷ total kills |
| `hitless_streak` | `u16` kill count | Kills since last damage taken |
| `deaths_per_area_avg` | `[0.0, 10.0]` | Rolling avg over last 3 Circles |

The Sin-signature is part of the replicated ECS snapshot and is rollback-safe. It is the **input channel** to the voice director, the AI Director spawn table selector, and the Grace-unlock priority queue.

#### 15b.2 Voice Director

`engine/narrative/voice_director.hpp` picks Malakor vs Niccolò per line given:

1. Current combat state (`RaptureState.active`, `RuinState.active`, `CombatInCombat` flag).
2. Sin-signature snapshot.
3. Dialogue-graph line pool for the current Act + Circle + encounter.

**Rules (authoritative):**

- During God Mode (§2.6): Malakor-only.
- During Ruin: Niccolò-only (remorse lines).
- Otherwise: weighted roll from line pool biased by Sin-signature (high `cruelty_ratio` → Malakor-heavy; high `precision_ratio` with low `cruelty_ratio` → Niccolò-heavy).
- Weighted roll is seeded — same play-style produces same lines on replay.

Lines are authored in the editor's `dialogue_graph` panel and cooked to `.gwdlg`. The BLD `voice-line-generate` tool can *draft* lines offline; all production lines are HITL-approved through `bld-governance`.

#### 15b.3 Grace Finale Reactivity

The Logos Phase-4 selector (§7b) reads `GraceComponent.value`. The Witness epilogue cutscene's line choice reads Sin-signature at game-end time:

- High cruelty / low precision → Witness narrates "you forgave because you were tired, not because you were merciful."
- High precision / low cruelty → Witness narrates "you forgave because you understood the silence."
- Balanced → Witness narrates "you forgave. The word is enough."

These are three deterministic branches; no runtime LLM; all assets pinned.

---

## Part IV — Performance & Technical Targets

### 16. Frame Budget — RX 580 @ 1080p / 144 FPS

Total: 6.94 ms per frame.

| System | Budget | Notes |
|--------|--------|-------|
| Game logic (ECS, Martyrdom, AI ticks) | 1.0 ms | Parallelised via job system |
| Physics (player AABB + Jolt ragdolls) | 0.8 ms | Custom controller bypasses Jolt hot path |
| Render prep (culling, draw cmd fill) | 0.5 ms | GPU-driven; CPU cost is submission |
| GPU (geometry, decals, post) | 3.5 ms | Primary constraint; Forward+ |
| Audio (miniaudio + Steam Audio) | 0.4 ms | Async; budget is fence cost |
| Headroom | 0.74 ms | Never borrow |

Max active entities per Circle: 512 enemies + 128 projectiles + 64 limb entities + 512 blood decals + player. Total ~1300 active entities. ECS archetype iteration must complete within 1.0 ms budget.

### 17. Sacrilege-Specific Engine Extensions

| Subsystem | Extension |
|-----------|-----------|
| Physics | Custom swept-AABB player controller; Jolt for ragdolls/limbs only |
| Rendering | GPU blood decal compute (stamp + drip); horror post stack |
| ECS | Martyrdom component set; StatCompositionSystem |
| Audio | BPM-sync layer crossfade; Martyrdom audio state machine |
| Animation | Single-frame events for melee hit detection + dismemberment triggers |
| AI | BT node registration system; Blacklake-driven patrol path generation |

---

## Part V — Novel Contributions (Patent Posture)

**Claim 1 — HEC Seeding Protocol.** Deterministic hierarchical seed derivation using SHA3-256 with domain separation; random-access arena generation at any level of the hierarchy.

**Claim 2 — SDR Noise.** Superposition of phase-coherent Gabor wavelets over a Poisson-disc lattice; geologically coherent, anisotropic terrain generation.

**Claim 3 — GPTM.** Hybrid heightmap/sparse-voxel-octree for real-time arena-scale terrain with seamless LOD.

**Claim 4 — The Martyrdom Engine.** A dual-resource risk/reward system: Sin accrual via Profane weapon use; Mantra accrual via Sacred weapon skill; expenditure of Sin for temporary powers (Blasphemies); automatic triggering of an involuntary enhanced-power state (Rapture) followed by a mandatory vulnerability state (Ruin) upon Sin reaching maximum — with no player opt-out.

**Claim 5 — Persistent Gameplay-Affecting Dismemberment.** Severed limbs persist as physics entities with navigation obstacle classification; limbs affect enemy pathfinding and can be used by the player as thrown weapons.

**Claim 6 — Grace Meter Finale Gate.** A dual-counter system in which accumulation of a *restraint*-based currency (Grace) across the run **gates a non-combat ending branch** on the final boss, where the terminal ability is spending both the violence currency (Sin) and the restraint currency (Grace) simultaneously. The restraint currency is accumulated by deliberately *not* exercising the game's power mechanics (no-Rapture-Circle bonus, Unwritten-spared bonus), such that the ending requires *both* mastery and restraint — neither alone suffices.

**Claim 7 — Sin-Signature Narrative Reactivity.** A deterministic five-channel play-style fingerprint (God-Mode uptime, precision, cruelty, hitless streak, deaths per area) replicated through the ECS snapshot, used as a **seeded input** to a voice-director runtime that selects between two in-head speaker voices (combat persona vs. moral persona) without any runtime text generation. Same seed + same play pattern → same dialogue line on replay. Rollback-safe; replay-stable; speedrun-comparable.

**Claim 8 — Bounded Deterministic Runtime AI in a Rollback-Networked First-Person Shooter.** A runtime inference architecture (`engine/ai_runtime/`) in which (a) authoritative-state models are pure functions of `(ECS state, per-frame seed)` with fixed topology and fixed precision, (b) presentation-only models are explicitly tagged and excluded from the replay hash, and (c) all model weights are content-addressed and signature-verified at load. The combination permits on-device neural systems (hybrid AI Director, symbolic adaptive music, neural material evaluator) inside a lockstep rollback net architecture without breaking determinism.

---

*Cold Coffee Labs · 2026-04-22 · GW-SAC-001-REV-B*

**⬛ END OF DOCUMENT ⬛**
