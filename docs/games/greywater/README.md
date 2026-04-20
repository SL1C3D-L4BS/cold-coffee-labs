# Greywater — The Game

**Studio:** Cold Coffee Labs
**Engine:** Greywater_Engine
**Target hardware baseline:** AMD Radeon RX 580 8GB @ 1080p / 60 FPS
**Art direction:** Realistic low-poly

*"For I stand on the ridge of the universe and await nothing."*

---

## What Greywater is

**Greywater** is a persistent procedural universe simulation. Players explore, settle, build, raid, and survive across a procedurally-generated universe of star clusters, systems, and planets. Gameplay is seamless from surface exploration through atmospheric flight, orbital combat, and interstellar travel. Factions compete for resources. Ecosystems live and evolve. Bases are built and raided. The universe is hostile by design.

The game is the first title being built on Greywater_Engine at Cold Coffee Labs.

## Pillars

1. **Infinite without storage.** The universe exists only where the player's attention is. Everything else is reproducible from the seed.
2. **Seamless simulation continuity.** Ground → Atmosphere → Orbit → Space is one unbroken experience. No loading screens. No mode changes.
3. **Survival is the loop.** Gather → Craft → Build → Raid → Dominate → Survive. Every system supports conflict.
4. **Exploration is infinite.** Every planet is unique. Every system is discoverable. No two playthroughs overlap.
5. **Realistic low-poly aesthetic.** Reduced geometry, realistic lighting, atmospheric-scattering-driven mood. See `20_ART_DIRECTION.md`.
6. **Determinism is the contract.** Same seed, same coordinates, identical universe — across machines, across time.
7. **Multiplayer-first architecture.** Server-authoritative with rollback netcode for combat and deterministic lockstep for large-scale simulation. See `17_MULTIPLAYER.md`.

## Document hierarchy

| # | Document | Covers |
| --- | --- | --- |
| 12 | `12_GAME_ARCHITECTURE.md` | Universe hierarchy, coordinate model, deterministic rule systems |
| 13 | `13_PROCEDURAL_UNIVERSE.md` | Seed-to-world generation, noise composition, biomes, resources |
| 14 | `14_PLANETARY_SYSTEMS.md` | Spherical planets, quad-tree LOD, terrain, oceans, vegetation |
| 15 | `15_ATMOSPHERE_AND_SPACE.md` | Atmospheric scattering, clouds, orbital physics, re-entry |
| 16 | `16_SURVIVAL_AND_BUILDING.md` | Resources, crafting, base building, structural integrity, raiding |
| 17 | `17_MULTIPLAYER.md` | Rollback + lockstep hybrid, ECS replication, anti-cheat |
| 18 | `18_ECOSYSTEM_AI.md` | Wildlife, factions, LLM dialogue, pathfinding |
| 19 | `19_AUDIO_DESIGN.md` | Spatial audio, atmospheric filtering, re-entry audio |
| 20 | `20_ART_DIRECTION.md` | Realistic low-poly aesthetic, palettes, shading, composition |

## Engine/game boundary

The engine (`greywater_core`, `editor`, `bld`) provides capabilities. The game (`gameplay_module`, content, scenes) consumes them. The engine **never** `#include`s from `gameplay/`. Engine code that was designed specifically for Greywater's needs (universe generation, spherical planets, atmospheric scattering, etc.) lives in `engine/world/` and is accessed by the game via the public API. If Greywater's needs drive a change to that API, the change lands in the engine with a clean contract — never as a back door.

See `docs/00_SPECIFICATION_Claude_Opus_4.7.md` §7 for the formal boundary contract.

---

*Greywater is the game we are building. The engine exists because no existing engine can ship this vision within our constraints. The constraints are the design: RX 580 baseline, C++23, Vulkan 1.2, realistic low-poly, infinite-without-storage, determinism everywhere.*
