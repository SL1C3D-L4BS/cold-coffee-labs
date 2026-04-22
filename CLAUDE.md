# CLAUDE.md — Greywater Engine × Cold Coffee Labs

**Cold-start primer for Claude Code (and any AI assistant) on this repository. Read before every session.**

---

## The Project in One Sentence

**Cold Coffee Labs** builds **Greywater Engine** (C++23 / Vulkan) to ship ***Sacrilege*** — a fast, gory, 3D first-person action game that is the spiritual heir of Quake's movement, Doom's aggression, and Ultrakill's speed, rebuilt from scratch with infinite procedural hellscapes and a unique risk/reward resource system.

---

## The Stack at a Glance

| Domain | Language | Rule |
|--------|----------|------|
| Engine, editor, gameplay, all *Sacrilege* logic | **C++23** | No exceptions. No scripting VM. No Lua. No Python in binaries or runtime (offline `tools/` only: `docs/01_CONSTITUTION_AND_PROGRAM.md` §2.2.1). |
| BLD AI copilot only | **Rust 2024** | Lives in `bld/`. Nowhere else. |
| Shaders | **HLSL** → DXC → SPIR-V | GLSL for ports only. |
| Compiler | **Clang/LLVM 18+** | `clang-cl` on Windows, `clang` on Linux. MSVC and GCC not supported. |
| Build | **CMake 3.28+** + Ninja + Corrosion (Rust) | Presets for `dev-win`, `dev-linux`, `studio-*`. |
| Editor UI | **Dear ImGui** + docking + viewports | No custom UI framework. |
| In-game UI | **RmlUi** | Not ImGui. |
| Hardware floor | **AMD Radeon RX 580 8GB** | 1080p @ 144 FPS for *Sacrilege*. |

---

## What We're Actually Building

***Sacrilege*** is a **3D first-person action game** designed for speedrunners and adrenaline junkies.

- **Movement is a weapon.** Bunny hop, air strafe, slide, wall kick, grapple, blood surf. All tuned for frame-perfect mastery.
- **Nine Inverted Circles of Hell.** Each procedurally generated from a **Hell Seed**. Same seed = same level, always. Shareable, speedrun-verifiable.
- **The Martyrdom Engine.** Sin accrues from aggressive Profane play; Mantra accrues from precise Sacred play. Let Sin hit 100 and **Rapture** triggers (6s god-mode) followed by **Ruin** (12s debuff). Manage the meter or die.
- **144 FPS on an RX 580.** No exceptions. This is a fast game. It must run fast.
- **All custom assets.** No licensed packs. Crunchy low-poly horror aesthetic.

**What *Sacrilege* is NOT:** Not open world. Not survival. Not base building. Not a planet simulator. It is nine arena-scale hells connected by a descent narrative.

---

## The Editor — A 3D Sketchpad for Violence

The **Greywater Editor** is a native C++ application built into the engine runtime. Think GoldSrc's Hammer or QuakeEd, but real-time and purpose-built for fast FPS combat design.

- **What you see is what ships.** Real-time 3D viewport using the same Vulkan renderer as the game.
- **Play Mode (F5).** Drop into the game instantly from your current editor camera position.
- **Circle Editor.** Tweak procedural generation parameters (SDR noise, biome blending) in real time.
- **Encounter Editor.** Drag enemies into the arena, paint patrol paths, wire behavior trees visually.
- **No scripting IDE.** Gameplay logic is compiled C++23. The editor places entities and sets parameters; it does not run scripts.

---

## Non-Negotiables (The Law)

### Hardware & Performance
1. **RX 580 @ 144 FPS** is the Tier A baseline. Features that break this are Tier G (gated).
2. **Vulkan 1.2 baseline** with opportunistic 1.3 features via `RHICapabilities`.

### Language & Build
3. **Clang only** for C++. MSVC and GCC are not supported.
4. **Rust only for BLD** (`bld/` directory). Nowhere else.

### Code Discipline
5. No raw `new` / `delete` in engine hot paths. Use `std::unique_ptr`, `gw::Ref<T>`, or arena allocators.
6. No two-phase public `init()` / `shutdown()`. RAII only.
7. No C-style casts. Use `static_cast`, `reinterpret_cast` with `// SAFETY:` comments.
8. No `std::string_view` passed to C APIs expecting null-termination.
9. No STL containers in hot paths. Use `engine/core/` containers.
10. No `std::async` or raw `std::thread`. All concurrency through `engine/jobs/`.
11. OS headers only inside `engine/platform/`.

### UI & Game Logic
12. **ImGui is editor-only.** **RmlUi** is for in-game HUD and menus.
13. **All gameplay logic is C++23.** There is no scripting language. No VM. No Lua. No Python in shipped code paths. No custom language. (Offline Python in `tools/`: §2.2.1 in `docs/01_CONSTITUTION_AND_PROGRAM.md`.)
14. Visual scripting uses a **typed text IR** as ground truth. The node graph is a projection.

### World & Simulation
15. **Arena-scale only.** No planetary simulation, no orbital mechanics, no galaxy generation. *Sacrilege* is nine arenas.
16. **Deterministic procedural generation.** Same Hell Seed + same Circle index = identical level, every machine, every time.
17. **World-space positions are `f64`.** `float` only for local-to-chunk math and GPU attributes after floating-origin subtraction.
18. **Floating origin** fires when the camera travels far within a large arena.

### Secrets & Safety
19. **No secrets in the repo.** API keys live in OS keychains, never in source.
20. **Documentation never lags directive.** Update `docs/` before code when the vision changes.

---

## Session Protocol (Do This Every Time)

1. `git log --oneline -10`
2. Re-read this file (`CLAUDE.md`).
3. Check `docs/02_ROADMAP_AND_OPERATIONS.md` for the current active phase and week.
4. Read the relevant spec for the active phase:
   - **Phase 20–23:** `docs/07_SACRILEGE.md` + `docs/08_BLACKLAKE.md`
   - **Rendering:** `docs/05_RESEARCH_BUILD_AND_STANDARDS.md`
   - **Architecture:** `docs/06_ARCHITECTURE.md`

---

## Where to Start (First Time on the Project)

1. `docs/README.md` — The map of all docs.
2. `docs/01_CONSTITUTION_AND_PROGRAM.md` — What is allowed and what is forbidden.
3. `docs/07_SACRILEGE.md` — The game we are building.
4. `docs/03_PHILOSOPHY_AND_ENGINEERING.md` — Why we make the choices we make.
5. `docs/06_ARCHITECTURE.md` — How the engine hangs together.

---

## Code Style at a Glance

| Element | Convention | Example |
|---------|------------|---------|
| Types | `PascalCase` | `RaptureState`, `GptmMeshBuilder` |
| Functions/methods | `snake_case` | `apply_sin_accrual()`, `build_lod_mesh()` |
| Member variables | trailing `_` | `device_`, `sin_value_` |
| Namespaces | `snake_case` | `gw::render`, `gw::sacrilege` |
| Macros | `GW_UPPER_SNAKE` | `GW_ASSERT`, `GW_LOG` |
| Enums | `enum class` | `BlasphemyType::Stigmata` |
| Files | `.hpp` / `.cpp` | `#pragma once` in all headers |

---

## The Vibe

> *"For I stand on the ridge of the universe and await nothing."*

This is not a corporate project. This is a fast, brutal, beautiful game built on a custom engine by a small team that values **engineering sovereignty** and **player feel** over feature bloat. Every line of code should serve the goal of making *Sacrilege* feel incredible to play at 144 FPS.

**When in doubt, ask:** *Does this make the game faster, more readable, or more fun?* If not, cut it.

---

*Brewed with BLD. Shipped by Cold Coffee Labs.*
